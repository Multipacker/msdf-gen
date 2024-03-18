// TODO: Allow for pruning small contours. This would hoepfully increase the
// quality of the final MSDF, although it won't be as accurate any more.

internal B32 msdf_distance_is_closer(MSDF_Distance a, MSDF_Distance b) {
    if (f32_abs(a.distance - b.distance) < F32_EPSILON) {
        return a.orthogonality > b.orthogonality;
    } else {
        return a.distance < b.distance;
    }
}

internal B32 msdf_is_corner(MSDF_Segment a, MSDF_Segment b, F32 threshold) {
    V2F32 a_dir = v2f32(0.0f, 0.0f);
    V2F32 b_dir = v2f32(0.0f, 0.0f);
    switch (a.kind) {
        case MSDF_SEGMENT_LINE:             a_dir = v2f32_normalize(v2f32_subtract(a.p1, a.p0)); break;
        case MSDF_SEGMENT_QUADRATIC_BEZIER: a_dir = v2f32_normalize(v2f32_subtract(a.p2, a.p1)); break;
        case MSDF_SEGMENT_KIND_COUNT:       a_dir = v2f32(0.0f, 0.0f);                           break;
    }
    switch (b.kind) {
        case MSDF_SEGMENT_LINE:             b_dir = v2f32_normalize(v2f32_subtract(b.p1, b.p0)); break;
        case MSDF_SEGMENT_QUADRATIC_BEZIER: b_dir = v2f32_normalize(v2f32_subtract(b.p1, b.p0)); break;
        case MSDF_SEGMENT_KIND_COUNT:       b_dir = v2f32(0.0f, 0.0f);                           break;
    }

    B32 are_parallel       = f32_abs(v2f32_cross(a_dir, b_dir)) <= threshold;
    B32 are_same_direction = v2f32_dot(a_dir, b_dir) > 0.0f;

    return !are_parallel || !are_same_direction;
}

internal MSDF_Distance msdf_line_distance_orthogonality(V2F32 point, MSDF_Segment line) {
    V2F32 length = v2f32_subtract(line.p1, line.p0);
    F32 t = v2f32_dot(v2f32_subtract(point, line.p0), length) / v2f32_length_squared(length);
    t = f32_min(f32_max(0.0f, t), 1.0f);
    V2F32 vector_distance = v2f32_subtract(point, v2f32_add(line.p0, v2f32_scale(length, t)));

    F32 distance = v2f32_length(vector_distance);

    MSDF_Distance result;
    result.distance      = distance;
    result.orthogonality = f32_abs(v2f32_cross(v2f32_normalize(length), v2f32_scale(vector_distance, 1.0f / distance)));
    result.unclamped_t   = t;

    return result;
}

internal MSDF_Distance msdf_quadratic_bezier_distance_orthogonality(V2F32 point, MSDF_Segment bezier) {
    V2F32 p  = v2f32_subtract(point, bezier.p0);
    V2F32 p1 = v2f32_subtract(bezier.p1, bezier.p0);
    V2F32 p2 = v2f32_add(v2f32_add(bezier.p2, v2f32_scale(bezier.p1, -2)), bezier.p0);

    F32 a = v2f32_length_squared(p2);
    F32 b = 3.0f * v2f32_dot(p1, p2);
    F32 c = 2.0f * v2f32_length_squared(p1) - v2f32_dot(p2, p);
    F32 d = -v2f32_dot(p1, p);

    // NOTE: We always need to check both end points of the curve, thus we
    // always have at least 2 "solutions" and start filling in the remaining
    // solutions at index 2.
    F32 ts[5] = { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    U32 solution_count = 2 + f32_solve_cubic(a, b, c, d, &ts[2]);

    F32 min_distance = f32_infinity();
    F32 min_t = 0.0f;
    F32 unclamped_t = 0.0f;
    V2F32 min_vector_distance = v2f32(0.0f, 0.0f);
    for (U32 i = 0; i < solution_count; ++i) {
        F32 t = f32_min(f32_max(0.0f, ts[i]), 1.0f);

        V2F32 vector_distance = v2f32_subtract(point, v2f32_add(v2f32_add(v2f32_scale(p2, t * t), v2f32_scale(p1, 2.0f * t)), bezier.p0));
        F32 distance = v2f32_length_squared(vector_distance);

        if (distance < min_distance) {
            min_distance = distance;
            min_t = t;
            unclamped_t = ts[i];
            min_vector_distance = vector_distance;
        }
    }

    F32 distance = f32_sqrt(min_distance);
    V2F32 direction = v2f32_normalize(v2f32_add(v2f32_scale(p2, min_t), p1));
    V2F32 perpendicular = v2f32_scale(min_vector_distance, 1.0 / distance);

    MSDF_Distance result;
    result.distance      = distance;
    result.orthogonality = f32_abs(v2f32_cross(direction, perpendicular));
    result.unclamped_t   = unclamped_t;
    return result;
}

internal F32 msdf_line_signed_pseudo_distance(V2F32 point, MSDF_Segment line) {
    V2F32 length = v2f32_subtract(line.p1, line.p0);
    F32 t = v2f32_dot(v2f32_subtract(point, line.p0), length) / v2f32_length_squared(length);
    V2F32 distance = v2f32_subtract(v2f32_add(line.p0, v2f32_scale(length, t)), point);

    F32 sign = f32_sign(v2f32_cross(length, distance));
    return sign * v2f32_length(distance);
}

internal F32 msdf_quadratic_bezier_signed_pseudo_distance(V2F32 point, MSDF_Segment bezier, F32 clamped_t) {
    V2F32 p  = v2f32_subtract(point, bezier.p0);
    V2F32 p1 = v2f32_subtract(bezier.p1, bezier.p0);
    V2F32 p2 = v2f32_add(v2f32_add(bezier.p2, v2f32_scale(bezier.p1, -2)), bezier.p0);

    V2F32 derivative;
    V2F32 distance;

    if (clamped_t < 0.0f) {
        derivative = v2f32_subtract(bezier.p1, bezier.p0);
        F32 t = v2f32_dot(v2f32_subtract(point, bezier.p0), derivative) / v2f32_length_squared(derivative);
        distance = v2f32_subtract(v2f32_add(bezier.p0, v2f32_scale(derivative, t)), point);
    } else if (clamped_t > 1.0f) {
        derivative = v2f32_subtract(bezier.p2, bezier.p1);
        F32 t = v2f32_dot(v2f32_subtract(point, bezier.p1), derivative) / v2f32_length_squared(derivative);
        distance = v2f32_subtract(v2f32_add(bezier.p1, v2f32_scale(derivative, t)), point);
    } else {
        distance   = v2f32_subtract(v2f32_add(v2f32_add(v2f32_scale(p2, clamped_t * clamped_t), v2f32_scale(p1, 2.0f * clamped_t)), bezier.p0), point);
        derivative = v2f32_add(v2f32_scale(p2, 2.0f * clamped_t), v2f32_scale(p1, 2.0f));
    }

    F32 sign = f32_sign(v2f32_cross(derivative, distance));
    return sign * v2f32_length(distance);
}

internal Void msdf_quadratic_bezier_split(MSDF_Segment segment, F32 t, MSDF_Segment *result_a, MSDF_Segment *result_b) {
    V2F32 new_points[6] = { 0 };
    quadratic_bezier_split(segment.p0, segment.p1, segment.p2, t, new_points);

    result_a->kind  = MSDF_SEGMENT_QUADRATIC_BEZIER;
    result_a->p0    = new_points[0];
    result_a->p1    = new_points[1];
    result_a->p2    = new_points[2];
    result_a->color = segment.color;

    result_b->kind  = MSDF_SEGMENT_QUADRATIC_BEZIER;
    result_b->p0    = new_points[3];
    result_b->p1    = new_points[4];
    result_b->p2    = new_points[5];
    result_b->color = segment.color;
}

internal Void msdf_line_split(MSDF_Segment segment, F32 t, MSDF_Segment *result_a, MSDF_Segment *result_b) {
    V2F32 point = v2f32_add(segment.p0, v2f32_scale(v2f32_subtract(segment.p1, segment.p0), t));

    result_a->kind  = MSDF_SEGMENT_LINE;
    result_a->p0    = segment.p0;
    result_a->p1    = point;
    result_a->color = segment.color;

    result_b->kind  = MSDF_SEGMENT_LINE;
    result_b->p0    = point;
    result_b->p1    = segment.p1;
    result_b->color = segment.color;
}

internal Void msdf_segment_split(MSDF_Segment segment, F32 t, MSDF_Segment *result_a, MSDF_Segment *result_b) {
    if (segment.kind == MSDF_SEGMENT_LINE) {
        msdf_line_split(segment, t, result_a, result_b);
    } else if (segment.kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
        msdf_quadratic_bezier_split(segment, t, result_a, result_b);
    }
}

internal U32 msdf_quadratic_bezier_intersect_recurse(MSDF_Segment a, MSDF_Segment b, U32 iteration_count, F32 *result_ats, F32 *result_bts) {
    V2F32 a_min = v2f32_min(v2f32_min(a.p0, a.p1), a.p2);
    V2F32 a_max = v2f32_max(v2f32_max(a.p0, a.p1), a.p2);
    V2F32 b_min = v2f32_min(v2f32_min(b.p0, b.p1), b.p2);
    V2F32 b_max = v2f32_max(v2f32_max(b.p0, b.p1), b.p2);

    if (a_min.x < b_max.x && a_max.x > b_min.x && a_min.y < b_max.y && a_max.y > b_min.y) {
        if (iteration_count == 0) {
            // Assume that the curves equivalent to lines at this point.
            F32 denominator = (a.p0.x - a.p2.x) * (b.p0.y - b.p2.y) - (a.p0.y - a.p2.y) * (b.p0.x - b.p2.x);
            if (f32_abs(denominator) > F32_EPSILON) {
                F32 at = ((a.p0.x - b.p0.x) * (b.p0.y - b.p2.y) - (a.p0.y - b.p0.y) * (b.p0.x - b.p2.x)) / denominator;
                F32 bt = ((a.p0.x - b.p0.x) * (a.p0.y - a.p2.y) - (a.p0.y - b.p0.y) * (a.p0.x - a.p2.x)) / denominator;

                if (0.0f <= at && at < 1.0f && 0.0f <= bt && bt < 1.0f) {
                    result_ats[0] = at;
                    result_bts[0] = bt;
                    return 1;
                } else {
                    return 0;
                }
            } else {
                return 0;
            }
        } else {
            // May intersect, split into two and try again.
            MSDF_Segment segments[4] = { 0 };
            msdf_quadratic_bezier_split(a, 0.5f, &segments[0], &segments[1]);
            msdf_quadratic_bezier_split(b, 0.5f, &segments[2], &segments[3]);

            U32 count = 0;
            for (U32 i = 0; i < 4; ++i) {
                U32 new_solutions = msdf_quadratic_bezier_intersect_recurse(segments[i / 2], segments[2 + i % 2], iteration_count - 1, &result_ats[count], &result_bts[count]);
                for (U32 j = 0; j < new_solutions; ++j) {
                    result_ats[count + j] = (i / 2) * 0.5f + result_ats[count + j] * 0.5f;
                    result_bts[count + j] = (i % 2) * 0.5f + result_bts[count + j] * 0.5f;
                }
                count += new_solutions;
            }
            return count;
        }
    } else {
        // Don't intersect
        return 0;
    }
}

internal U32 msdf_quadratic_bezier_intersect(MSDF_Segment a, MSDF_Segment b, F32 *result_ats, F32 *result_bts) {
    F32 error = 0.00001f;
    F32 alx = 2.0f * f32_abs(a.p2.x - 2.0f * a.p1.x + a.p0.x);
    F32 aly = 2.0f * f32_abs(a.p2.y - 2.0f * a.p1.y + a.p0.y);
    F32 blx = 2.0f * f32_abs(b.p2.x - 2.0f * b.p1.x + b.p0.x);
    F32 bly = 2.0f * f32_abs(b.p2.y - 2.0f * b.p1.y + b.p0.y);
    U32 ar = f32_max(0.0f, f32_log2(f32_sqrt(f32_sqrt(alx * alx + aly * aly) / (8.0f * error))));
    U32 br = f32_max(0.0f, f32_log2(f32_sqrt(f32_sqrt(blx * blx + bly * bly) / (8.0f * error))));
    U32 r = u32_max(ar, br);

    U32 count = msdf_quadratic_bezier_intersect_recurse(a, b, r, result_ats, result_bts);

    return count;
}

/*
 * Solving for the intersection of a quadratci bezier and a line segment it
 * equivalent to solving the following equation.
 *
 *   p0 * (1 - t) + p1 * t = q0 * (1 - u)^2 + q1 * 2 * u * (1 - u) + q2 * u^2 { 0 <= t <= 1, 0 <= u <= 1 }
 *
 * Where:
 *   p0 and p1 are the points defining the line.
 *   q0, q1 and q2 are the points defining the quadratci bezier curve.
 *   t specifies how far along the line the solution is.
 *   u specifies how far along the quadratic bezier the solution is.
 *
 * Solving this gives the folowing quadratic.
 *               __________
 *       -b +- \/ b^2 - 4ac
 *   u = ------------------
 *              2a
 *
 * Where:
 *   a = (p1 - p0) x (q0 - 2 * q1 + q2)
 *   b = (p1 - p0) x (2 * q1 - 2 * q0)
 *   c = (p1 - p0) x (q0 - p0)
 */
internal U32 msdf_line_quadratic_bezier_intersect(MSDF_Segment line, MSDF_Segment bezier, F32 *result_ats, F32 *result_bts) {
    V2F32 line_direction = v2f32_subtract(line.p1, line.p0);

    F32 a = v2f32_cross(line_direction, v2f32_add(v2f32_add(bezier.p0, v2f32_scale(bezier.p1, -2.0f)), bezier.p2));
    F32 b = v2f32_cross(line_direction, v2f32_scale(v2f32_subtract(bezier.p1, bezier.p0), 2.0f));
    F32 c = v2f32_cross(line_direction, v2f32_subtract(bezier.p0, line.p0));

    F32 u0 = 0.0f;
    F32 u1 = 0.0f;

    if (f32_abs(a) >= 0.01f) {
        F32 discriminant = b * b - 4.0f * a * c;

        if (discriminant <= 0.0f) {
            // Complex roots or a double root to the quadratic; the segments do not intersect.
            return 0;
        }

        F32 sqrt_discriminant = f32_sqrt(discriminant);

        u0 = (-b - sqrt_discriminant) / (2.0f * a);
        u1 = (-b + sqrt_discriminant) / (2.0f * a);
    } else {
        u0 = u1 = -c / b;
    }

    F32 t0 = 0.0f;
    F32 t1 = 0.0f;

    if (f32_abs(line.p1.x - line.p0.x) >= F32_EPSILON) {
        t0 = (u0 * u0 * (bezier.p0.x - 2.0f * bezier.p1.x + bezier.p2.x) + 2.0f * u0 * (bezier.p1.x - bezier.p0.x) + bezier.p0.x - line.p0.x) / (line.p1.x - line.p0.x);
        t1 = (u1 * u1 * (bezier.p0.x - 2.0f * bezier.p1.x + bezier.p2.x) + 2.0f * u1 * (bezier.p1.x - bezier.p0.x) + bezier.p0.x - line.p0.x) / (line.p1.x - line.p0.x);
    } else {
        t0 = (u0 * u0 * (bezier.p0.y - 2.0f * bezier.p1.y + bezier.p2.y) + 2.0f * u0 * (bezier.p1.y - bezier.p0.y) + bezier.p0.y - line.p0.y) / (line.p1.y - line.p0.y);
        t1 = (u1 * u1 * (bezier.p0.y - 2.0f * bezier.p1.y + bezier.p2.y) + 2.0f * u1 * (bezier.p1.y - bezier.p0.y) + bezier.p0.y - line.p0.y) / (line.p1.y - line.p0.y);
    }

    U32 intersection_count = 0;

    if (0.0f <= t0 && t0 < 1.0f && 0.0f <= u0 && u0 < 1.0f) {
        result_ats[intersection_count] = t0;
        result_bts[intersection_count] = u0;
        ++intersection_count;
    }

    if (0.0f <= t1 && t1 < 1.0f && 0.0f <= u1 && u1 < 1.0f) {
        result_ats[intersection_count] = t1;
        result_bts[intersection_count] = u1;
        ++intersection_count;
    }

    return intersection_count;
}

internal U32 msdf_line_intersect(MSDF_Segment a, MSDF_Segment b, F32 *result_ats, F32 *result_bts) {
    U32 intersection_count = 0;

    F32 denominator = (a.p0.x - a.p1.x) * (b.p0.y - b.p1.y) - (a.p0.y - a.p1.y) * (b.p0.x - b.p1.x);
    if (f32_abs(denominator) > F32_EPSILON) {
        F32 u = ((a.p0.x - b.p0.x) * (b.p0.y - b.p1.y) - (a.p0.y - b.p0.y) * (b.p0.x - b.p1.x)) / denominator;
        F32 v = ((a.p0.x - b.p0.x) * (a.p0.y - a.p1.y) - (a.p0.y - b.p0.y) * (a.p0.x - a.p1.x)) / denominator;

        if (0.0f <= u && u < 1.0f && 0.0f <= v && v < 1.0f) {
            result_ats[intersection_count] = u;
            result_bts[intersection_count] = v;
            ++intersection_count;
        }
    }

    return intersection_count;
}

internal U32 msdf_segment_intersect(MSDF_Segment a, MSDF_Segment b, F32 *result_ats, F32 *result_bts) {
    U32 intersection_count = 0;
    if (a.kind == MSDF_SEGMENT_LINE && b.kind == MSDF_SEGMENT_LINE) {
        intersection_count = msdf_line_intersect(a, b, result_ats, result_bts);
    } else if (a.kind == MSDF_SEGMENT_LINE && b.kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
        intersection_count = msdf_line_quadratic_bezier_intersect(a, b, result_ats, result_bts);
    } else if (a.kind == MSDF_SEGMENT_QUADRATIC_BEZIER && b.kind == MSDF_SEGMENT_LINE) {
        intersection_count = msdf_line_quadratic_bezier_intersect(b, a, result_bts, result_ats);
    } else if (a.kind == MSDF_SEGMENT_QUADRATIC_BEZIER && b.kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
        intersection_count = msdf_quadratic_bezier_intersect(a, b, result_ats, result_bts);
    }
    return intersection_count;
}

internal V2F32 msdf_point_along_segment(MSDF_Segment segment, F32 t) {
    V2F32 result = { 0 };

    if (segment.kind == MSDF_SEGMENT_LINE) {
        result = v2f32_add(segment.p0, v2f32_scale(v2f32_subtract(segment.p1, segment.p0), t));
    } else if (segment.kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
        V2F32 a = v2f32_add(segment.p0, v2f32_scale(v2f32_subtract(segment.p1, segment.p0), t));
        V2F32 b = v2f32_add(segment.p1, v2f32_scale(v2f32_subtract(segment.p2, segment.p1), t));
        result = v2f32_add(a, v2f32_scale(v2f32_subtract(b, a), t));
    }

    return result;
}

// https://en.wikipedia.org/wiki/Curve_orientation
internal S32 msdf_contour_calculate_own_winding_number(MSDF_Segment *segments, U32 segment_count) {
    S32 winding = 0;
    V2F32 top_left = v2f32(f32_infinity(), f32_infinity());

    MSDF_Segment *last_segment = &segments[segment_count - 1];
    V2F32 previous = (last_segment->kind == MSDF_SEGMENT_LINE ? last_segment->p0 : last_segment->p1);
    V2F32 current  = (last_segment->kind == MSDF_SEGMENT_LINE ? last_segment->p1 : last_segment->p2);
    for (U32 i = 0; i < segment_count; ++i) {
        MSDF_Segment *segment = &segments[i];

        V2F32 next;

        next = segment->p1;
        if (current.x < top_left.x || (current.x <= top_left.x && current.y < top_left.y)) {
            F32 potential_winding = v2f32_cross(v2f32_subtract(current, previous), v2f32_subtract(next, previous));
            if (f32_abs(potential_winding) > 0.00001f) {
                winding = f32_sign(potential_winding);
                top_left = current;
            }
        }
        previous = current;
        current = next;

        if (segment->kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
            next = segment->p2;
            if (current.x < top_left.x || (current.x <= top_left.x && current.y < top_left.y)) {
                F32 potential_winding = v2f32_cross(v2f32_subtract(current, previous), v2f32_subtract(next, previous));
                if (f32_abs(potential_winding) > 0.00001f) {
                    winding = f32_sign(potential_winding);
                    top_left = current;
                }
            }
            previous = current;
            current = next;
        }
    }

    return winding;
}

internal S32 msdf_contour_calculate_winding_number(MSDF_Segment *segments, U32 segment_count, V2F32 point) {
    S32 winding_number = 0;

    for (U32 i = 0; i < segment_count; ++i) {
        if (segments[i].kind == MSDF_SEGMENT_LINE) {
            V2F32 p0 = v2f32_subtract(segments[i].p0, point);
            V2F32 p1 = v2f32_subtract(segments[i].p1, point);
            F32 discriminant = v2f32_cross(p0, p1);
            winding_number += (p0.y < 0.0f && 0.0f < p1.y && discriminant >= 0.0f);
            winding_number -= (p1.y < 0.0f && 0.0f < p0.y && discriminant <= 0.0f);
        } else if (segments[i].kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
            V2F32 p0 = v2f32_subtract(segments[i].p0, point);
            V2F32 p1 = v2f32_subtract(segments[i].p1, point);
            V2F32 p2 = v2f32_subtract(segments[i].p2, point);

            F32 a = p0.y - 2.0f * p1.y + p2.y;
            F32 b = 2.0f * (p1.y - p0.y);
            F32 c = p0.y;

            F32 t0 = 0.0f;
            F32 t1 = 0.0f;
            // TODO: Choose an appropriate epsilon.
            if (f32_abs(a) < 0.001f) {
                t0 = t1 = -c / b;
            } else {
                F32 sqrt = f32_sqrt(b * b - 4 * a * c);
                t0 = (-b - sqrt) / (2.0f * a);
                t1 = (-b + sqrt) / (2.0f * a);
            }

            F32 x0 = p0.x * (1.0f - t0) * (1.0f - t0) + p1.x * 2.0f * t0 * (1.0f - t0) + p2.x * t0 * t0;
            F32 x1 = p0.x * (1.0f - t1) * (1.0f - t1) + p1.x * 2.0f * t1 * (1.0f - t1) + p2.x * t1 * t1;

            U32 state = (p0.y > 0.0f ? 4 : 0) | (p1.y > 0.0f ? 2 : 0) | (p2.y > 0.0f ? 1 : 0);
            U8 t0_should = 0x74;
            U8 t1_should = 0x2E;

            winding_number -= (0.0f <= t0 && t0 < 1.0f && ((t0_should >> state) & 1) && x0 >= 0.0f);
            winding_number += (0.0f <= t1 && t1 < 1.0f && ((t1_should >> state) & 1) && x1 >= 0.0f);
        }
    }

    return winding_number;
}

internal Void msdf_resolve_contour_overlap(MSDF_State *state) {
    for (U32 i = 0; i < state->contour_count; ++i) {
        for (U32 j = i + 1; j < state->contour_count; ++j) {
            MSDF_Segment *a_segments = state->contour_segments[i];
            MSDF_Segment *b_segments = state->contour_segments[j];

            // Find 2 consecutive intersections along one of the contours.
            // Split the contours at the intersections.
            // The parts "between" the intersections, switch which contour they belong to.
            // Repeat until there are no more intersections.
            U32 intersection_count = 0;
            U32 intersection_ais[2];
            U32 intersection_bis[2];
            for (U32 a_index = 0; a_index < state->contour_segment_counts[i]; ++a_index) {
                F32 min_at      = f32_infinity();
                F32 min_bt      = f32_infinity();
                U32 min_b_index = 0;
                for (U32 b_index = 0; b_index < state->contour_segment_counts[j]; ++b_index) {
                    F32 ats[4] = { 0 };
                    F32 bts[4] = { 0 };
                    U32 intersection_count = msdf_segment_intersect(a_segments[a_index], b_segments[b_index], ats, bts);
                    F32 intersection_epsilon = 0.0001f; // TODO: Move this out and figure out an appropiate value for it.
                    for (U32 i = 0; i < intersection_count; ++i) {
                        if (ats[i] < min_at && intersection_epsilon < ats[i] && ats[i] < 1.0f - intersection_epsilon) {
                            min_at      = ats[i];
                            min_bt      = bts[i];
                            min_b_index = b_index;
                        }
                    }
                }

                if (min_at <= 1.0f) {
                    U32 b_index = min_b_index;

                    intersection_ais[intersection_count] = a_index + 1;
                    intersection_bis[intersection_count] = b_index + 1;

                    memory_move(&a_segments[a_index + 2], &a_segments[a_index + 1], (state->contour_segment_counts[i] - (a_index + 1)) * sizeof(*a_segments));
                    memory_move(&b_segments[b_index + 2], &b_segments[b_index + 1], (state->contour_segment_counts[j] - (b_index + 1)) * sizeof(*b_segments));
                    msdf_segment_split(a_segments[a_index], min_at, &a_segments[a_index], &a_segments[a_index + 1]);
                    msdf_segment_split(b_segments[b_index], min_bt, &b_segments[b_index], &b_segments[b_index + 1]);

                    // The corner with the narrowest angle is moved "inwards".
                    // It needs to move at least the distance between the two
                    // corners. This can be 0, so we also add a small amount to
                    // ensure that the corners do not overlapp.
                    // TODO: Why is the "small amount" 0.005f? What should it be?
                    V2F32 *a0_corner    = (a_segments[a_index].kind == MSDF_SEGMENT_LINE ? &a_segments[a_index].p1 : &a_segments[a_index].p2);
                    V2F32  a0_direction = v2f32_normalize(v2f32_subtract((a_segments[a_index].kind == MSDF_SEGMENT_LINE ? a_segments[a_index].p0 : a_segments[a_index].p1), *a0_corner));
                    V2F32 *a1_corner    = &a_segments[a_index + 1].p0;
                    V2F32  a1_direction = v2f32_normalize(v2f32_subtract(a_segments[a_index + 1].p1, *a1_corner));
                    V2F32 *b0_corner    = (b_segments[b_index].kind == MSDF_SEGMENT_LINE ? &b_segments[b_index].p1 : &b_segments[b_index].p2);
                    V2F32  b0_direction = v2f32_normalize(v2f32_subtract((b_segments[b_index].kind == MSDF_SEGMENT_LINE ? b_segments[b_index].p0 : b_segments[b_index].p1), *b0_corner));
                    V2F32 *b1_corner    = &b_segments[b_index + 1].p0;
                    V2F32  b1_direction = v2f32_normalize(v2f32_subtract(b_segments[b_index + 1].p1, *b1_corner));
                    F32    move_amount  = 0.005f + v2f32_length(v2f32_subtract(v2f32_add(*a0_corner, *b1_corner), v2f32_add(*a1_corner, *b0_corner))) * 0.5f;

                    if (v2f32_dot(a0_direction, b1_direction) < v2f32_dot(a1_direction, b0_direction)) {
                        V2F32 move = v2f32_scale(v2f32_normalize(v2f32_add(a0_direction, b1_direction)), move_amount);
                        V2F32 new_corner = v2f32_add(v2f32_scale(v2f32_add(*a0_corner, *b1_corner), 0.5f), move);
                        *a0_corner = new_corner;
                        *b1_corner = new_corner;

                        V2F32 new_other_corner = v2f32_scale(v2f32_add(*a1_corner, *b0_corner), 0.5f);
                        *a1_corner = new_other_corner;
                        *b0_corner = new_other_corner;
                    } else {
                        V2F32 move = v2f32_scale(v2f32_normalize(v2f32_add(a1_direction, b0_direction)), move_amount);
                        V2F32 new_corner = v2f32_add(v2f32_scale(v2f32_add(*a1_corner, *b0_corner), 0.5f), move);
                        *a1_corner = new_corner;
                        *b0_corner = new_corner;

                        V2F32 new_other_corner = v2f32_scale(v2f32_add(*a0_corner, *b1_corner), 0.5f);
                        *a0_corner = new_other_corner;
                        *b1_corner = new_other_corner;
                    }

                    ++state->contour_segment_counts[i];
                    ++state->contour_segment_counts[j];
                    ++intersection_count;

                    if (intersection_count == 2) {
                        // If we split a segment that appears earlier in
                        // `b_segments` than the first one we split, we need to
                        // move the split index for the first split.
                        if (intersection_bis[1] <= intersection_bis[0]) {
                            ++intersection_bis[0];
                        }

                        // The list of segments for b must be treated as a circular
                        // buffer as the a segment might first intersect with a
                        // segment further into b and then with an earlier one. The
                        // interval crosses the end of the list
                        while (intersection_bis[0] > intersection_bis[1]) {
                            b_segments[state->contour_segment_counts[j]] = b_segments[0];
                            memory_move(b_segments, &b_segments[1], state->contour_segment_counts[j] * sizeof(*b_segments));

                            intersection_bis[0] = (intersection_bis[0] == 0 ? state->contour_segment_counts[j] : intersection_bis[0]) - 1;
                            intersection_bis[1] = (intersection_bis[1] == 0 ? state->contour_segment_counts[j] : intersection_bis[1]) - 1;
                        }

                        U32 a_start_count  = intersection_ais[0];
                        U32 b_start_count  = intersection_bis[0];
                        U32 a_middle_count = intersection_ais[1] - intersection_ais[0];
                        U32 b_middle_count = intersection_bis[1] - intersection_bis[0];
                        U32 a_end_count    = state->contour_segment_counts[i] - intersection_ais[1];
                        U32 b_end_count    = state->contour_segment_counts[j] - intersection_bis[1];

                        // First swap
                        memory_copy(state->temporary_buffer,          &a_segments[intersection_ais[1]], a_end_count * sizeof(*a_segments));
                        memory_copy(&a_segments[intersection_ais[1]], &b_segments[intersection_bis[1]], b_end_count * sizeof(*b_segments));
                        memory_copy(&b_segments[intersection_bis[1]], state->temporary_buffer,          a_end_count * sizeof(*a_segments));

                        // Second swap
                        memory_copy(state->temporary_buffer,          &a_segments[intersection_ais[0]], (a_middle_count + b_end_count) * sizeof(*a_segments));
                        memory_copy(&a_segments[intersection_ais[0]], &b_segments[intersection_bis[0]], (b_middle_count + a_end_count) * sizeof(*b_segments));
                        memory_copy(&b_segments[intersection_bis[0]], state->temporary_buffer,          (a_middle_count + b_end_count) * sizeof(*a_segments));

                        state->contour_segment_counts[i] = a_start_count + b_middle_count + a_end_count;
                        state->contour_segment_counts[j] = b_start_count + a_middle_count + b_end_count;

                        intersection_count = 0;
                    }
                }
            }
        }
    }
}

internal Void msdf_convert_to_simple_polygons(MSDF_State *state) {
    for (U32 contour_index = 0; contour_index < state->contour_count && state->contour_count < state->max_contour_count; ++contour_index) {
        MSDF_Segment *segments = state->contour_segments[contour_index];
        U32 segment_count = state->contour_segment_counts[contour_index];

        for (U32 a_index = 0; a_index < segment_count && state->contour_count < state->max_contour_count; ++a_index) {
            for (U32 b_index = a_index + 1; b_index < segment_count && state->contour_count < state->max_contour_count; ++b_index) {
                F32 ats[4] = { 0 };
                F32 bts[4] = { 0 };
                U32 intersection_count = msdf_segment_intersect(segments[a_index], segments[b_index], ats, bts);

                F32 min_at = f32_infinity();
                F32 min_bt = f32_infinity();
                for (U32 i = 0; i < intersection_count; ++i) {
                    F32 intersection_epsilon = 0.0001f;
                    if (ats[i] < min_at && intersection_epsilon < ats[i] && ats[i] < 1.0f - intersection_epsilon) {
                        min_at = ats[i];
                        min_bt = bts[i];
                    }
                }

                if (min_at <= 1.0f) {
                    memory_move(&segments[b_index + 2], &segments[b_index + 1], (segment_count - (b_index + 1)) * sizeof(*segments));
                    msdf_segment_split(segments[b_index], min_bt, &segments[b_index], &segments[b_index + 1]);
                    ++segment_count;

                    memory_move(&segments[a_index + 2], &segments[a_index + 1], (segment_count - (a_index + 1)) * sizeof(*segments));
                    msdf_segment_split(segments[a_index], min_at, &segments[a_index], &segments[a_index + 1]);
                    ++segment_count;

                    // Adjust b_index so that both it and a_index point to the start of the original segments.
                    ++b_index;

                    U32 new_segment_count = b_index - a_index;
                    state->contour_segment_counts[state->contour_count] = new_segment_count;
                    memory_copy(state->contour_segments[state->contour_count], &segments[a_index + 1], new_segment_count * sizeof(*segments));
                    ++state->contour_count;

                    memory_move(&segments[a_index + 1], &segments[b_index + 1], (segment_count - (b_index + 1)) * sizeof(*segments));
                    segment_count -= new_segment_count;

                    // Ensure that the contour is connected properly.
                    V2F32 *a0_corner = &segments[a_index + 1].p0;
                    V2F32 *a1_corner = (segments[a_index].kind == MSDF_SEGMENT_LINE ? &segments[a_index].p1 : &segments[a_index].p2);
                    V2F32 a_corner = v2f32_scale(v2f32_add(*a0_corner, *a1_corner), 0.5f);
                    *a0_corner = a_corner;
                    *a1_corner = a_corner;

                    MSDF_Segment *b_segments = state->contour_segments[state->contour_count - 1];
                    V2F32 *b0_corner = &b_segments[0].p0;
                    V2F32 *b1_corner = (b_segments[new_segment_count - 1].kind == MSDF_SEGMENT_LINE ? &b_segments[new_segment_count - 1].p1 : &b_segments[new_segment_count - 1].p2);
                    V2F32 b_corner = v2f32_scale(v2f32_add(*b0_corner, *b1_corner), 0.5f);
                    *b0_corner = b_corner;
                    *b1_corner = b_corner;
                }
            }
        }

        state->contour_segment_counts[contour_index] = segment_count;
    }
}

internal Void msdf_correct_contour_orientation(MSDF_State *state) {
    for (U32 i = 0; i < state->contour_count; ++i) {
        U32 segment_count = state->contour_segment_counts[i];
        MSDF_Segment *segments = state->contour_segments[i];

        segments[0].color &= ~(MSDF_FLAG_REMOVE | MSDF_FLAG_FLIP);

        V2F32 test_point = segments[0].p0;
        test_point.y += 0.0001f;

        S32 local_winding = msdf_contour_calculate_own_winding_number(segments, segment_count);
        S32 global_winding = local_winding;
        for (U32 j = 0; j < state->contour_count; ++j) {
            if (i != j) {
                global_winding += msdf_contour_calculate_winding_number(state->contour_segments[j], state->contour_segment_counts[j], test_point);
            }
        }

        if (global_winding < -1 || global_winding > 1) {
            segments[0].color |= MSDF_FLAG_REMOVE;
        } else if ((global_winding == 0 && local_winding == 1) || (global_winding != 0 && local_winding == -1)) {
            segments[0].color |= MSDF_FLAG_FLIP;
        }
    }

    for (U32 i = 0; i < state->contour_count; ++i) {
        U32 segment_count = state->contour_segment_counts[i];
        MSDF_Segment *segments = state->contour_segments[i];

        if (segments[0].color & MSDF_FLAG_FLIP) {
            U32 start = 0;
            U32 end = segment_count - 1;

            while (start < end) {
                if (segments[start].kind == MSDF_SEGMENT_LINE) {
                    V2F32 temp = segments[start].p0;
                    segments[start].p0 = segments[start].p1;
                    segments[start].p1 = temp;
                } else if (segments[start].kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
                    V2F32 temp = segments[start].p0;
                    segments[start].p0 = segments[start].p2;
                    segments[start].p2 = temp;
                }

                if (segments[end].kind == MSDF_SEGMENT_LINE) {
                    V2F32 temp = segments[end].p0;
                    segments[end].p0 = segments[end].p1;
                    segments[end].p1 = temp;
                } else if (segments[end].kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
                    V2F32 temp = segments[end].p0;
                    segments[end].p0 = segments[end].p2;
                    segments[end].p2 = temp;
                }

                MSDF_Segment temp = segments[start];
                segments[start] = segments[end];
                segments[end] = temp;
                ++start;
                --end;
            }

            // If there is an odd number of segments, the middle one wont be flipped yet.
            if (start == end) {
                if (segments[start].kind == MSDF_SEGMENT_LINE) {
                    V2F32 temp = segments[start].p0;
                    segments[start].p0 = segments[start].p1;
                    segments[start].p1 = temp;
                } else if (segments[start].kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
                    V2F32 temp = segments[start].p0;
                    segments[start].p0 = segments[start].p2;
                    segments[start].p2 = temp;
                }
            }
        } else if (segments[0].color & MSDF_FLAG_REMOVE) {
            memory_copy(segments, state->contour_segments[state->contour_count - 1], state->contour_segment_counts[state->contour_count - 1] * sizeof(**state->contour_segments));
            state->contour_segment_counts[i] = state->contour_segment_counts[state->contour_count - 1];
            --state->contour_count;
            --i;
        }
    }
}

internal MSDF_State msdf_state_initialize(Arena *arena, U32 max_contour_count, U32 max_segment_count) {
    MSDF_State state = { 0 };

    state.max_segment_count = 5 * max_segment_count + 2; // NOTE: Should be max points per contour, not for the entier glyph.
    // NOTE: To be correct and acutally handle all cases,
    // `max_contour_count` would have to be follow:
    //
    // A segment can at maximum make for intersections with another
    // segment. Every intersection means one new contour. Every new segment
    // to a contour can thus intersect with all previous segments 4 times
    // each. This forms a multiple of an arithmetic sum as follows:
    //
    //   4 * (0 + 1 + 2 + 3 + 4 + 5 + ... + (n - 1))
    //
    // This is can be written in closed form as
    //
    //       (n - 1) + (n - 1)^2
    //   4 * ------------------- = 2 * ((n - 1) + (n - 1)^2) = 2 * (n^2 - n)
    //               2
    //
    // Where n is the segment count of the original contour. Setting this
    // to the maximum number of segments for a contour and multiplying by
    // the maximum number of contours will yield enough space to handle any
    // glyph in the file.
    //
    // But this is not viable as it can sometimes generate counts up in the 10s
    // of thousands causing us to run out of memory, and all practical
    // applications use significantly fewer contours. Most glyphs don't
    // generate a single new contour, so twice the maximum number of contours
    // in any glyph should be enough for everyone. Hopefully
    state.max_contour_count = 2 * max_contour_count;
    state.contour_segment_counts = arena_push_array(arena, U32,            state.max_contour_count);
    state.temporary_buffer       = arena_push_array(arena, MSDF_Segment,   state.max_segment_count);
    state.contour_segments       = arena_push_array(arena, MSDF_Segment *, state.max_contour_count);
    state.all_segments           = arena_push_array(arena, MSDF_Segment,   state.max_contour_count * state.max_segment_count);
    state.circle_centers         = arena_push_array(arena, V2F32,          state.max_contour_count * state.max_segment_count);
    state.circle_radie           = arena_push_array(arena, F32,            state.max_contour_count * state.max_segment_count);
    for (U32 i = 0; i < state.max_contour_count; ++i) {
        state.contour_segments[i] = &state.all_segments[i * state.max_segment_count];
    }

    return state;
}

internal Void msdf_color_edges(MSDF_State *state) {
    F32 corner_threshold = f32_sin(0.1f);

    for (U32 contour_index = 0; contour_index < state->contour_count; ++contour_index) {
        MSDF_Segment *segments      = state->contour_segments[contour_index];
        U32           segment_count = state->contour_segment_counts[contour_index];

        // Find the first corner
        U32 corner_count = 0;
        U32 last_corner_start_index = 0;
        for (U32 i = 0, pre_i = segment_count - 1; i < segment_count; pre_i = i++) {
            if (msdf_is_corner(segments[pre_i], segments[i], corner_threshold)) {
                last_corner_start_index = i;
                ++corner_count;
            }
        }

        if (corner_count == 0) {
            for (U32 i = 0; i < segment_count; ++i) {
                segments[i].color = MSDF_COLOR_RED | MSDF_COLOR_GREEN | MSDF_COLOR_BLUE;
            }
        } if (corner_count == 1) {
            // TODO: More carefully handle how we split edges in this case.

            MSDF_ColorFlags current_color = MSDF_COLOR_RED | MSDF_COLOR_BLUE;
            for (U32 i = 0; i < segment_count; ++i) {
                U32 this_segment = (last_corner_start_index + i + 0) % segment_count;
                U32 next_segment = (last_corner_start_index + i + 1) % segment_count;

                segments[this_segment].color = current_color;
                if (msdf_is_corner(segments[this_segment], segments[next_segment], corner_threshold)) {
                    if (current_color == (MSDF_COLOR_RED | MSDF_COLOR_GREEN)) {
                        current_color = MSDF_COLOR_GREEN | MSDF_COLOR_BLUE;
                    } else {
                        current_color = MSDF_COLOR_RED | MSDF_COLOR_GREEN;
                    }
                }
            }

            // We need to split the contour into two edges in order to preserve the corner.
            segments[last_corner_start_index].color = MSDF_COLOR_RED | MSDF_COLOR_GREEN;
        } else {
            MSDF_ColorFlags current_color = MSDF_COLOR_RED | MSDF_COLOR_BLUE;
            for (U32 i = 0; i < segment_count; ++i) {
                U32 this_segment = (last_corner_start_index + i + 0) % segment_count;
                U32 next_segment = (last_corner_start_index + i + 1) % segment_count;

                segments[this_segment].color = current_color;
                if (msdf_is_corner(segments[this_segment], segments[next_segment], corner_threshold)) {
                    if (current_color == (MSDF_COLOR_RED | MSDF_COLOR_GREEN)) {
                        current_color = MSDF_COLOR_GREEN | MSDF_COLOR_BLUE;
                    } else {
                        current_color = MSDF_COLOR_RED | MSDF_COLOR_GREEN;
                    }
                }
            }
        }
    }
}

internal Void msdf_generate(MSDF_State *state, U8 *buffer, U32 stride, U32 x, U32 y, U32 width, U32 height) {
    if (!state->contour_count) {
        U32 pixel_index = (x + y * stride) * 4;
        for (U32 y = 0; y < height; ++y) {
            U32 row = pixel_index;
            for (U32 x = 0; x < width; ++x) {
                buffer[row++] = 0;
                buffer[row++] = 0;
                buffer[row++] = 0;
                buffer[row++] = 0;
            }
            pixel_index += stride * 4;
        }
        return;
    }

    msdf_resolve_contour_overlap(state);
    msdf_convert_to_simple_polygons(state);
    msdf_correct_contour_orientation(state);
    msdf_color_edges(state);

    MSDF_Segment *segments = state->all_segments;
    U32 segment_count = state->contour_segment_counts[0];
    for (U32 i = 1; i < state->contour_count; ++i) {
        memory_move(
            &segments[segment_count],
            state->contour_segments[i],
            state->contour_segment_counts[i] * sizeof(**state->contour_segments)
        );
        segment_count += state->contour_segment_counts[i];
    }

    // Sort segments based on which type they are.
    U32 line_count = segment_count;
    U32 bezier_count = 0;
    for (U32 i = 0, j = segment_count - 1; i < j; ++i) {
        while (segments[i].kind != MSDF_SEGMENT_LINE && i < j) {
            MSDF_Segment temp = segments[i];
            segments[i] = segments[j];
            segments[j] = temp;
            --j;

            --line_count;
            ++bezier_count;
        }
    }

    if (segments[line_count - 1].kind != MSDF_SEGMENT_LINE) {
        --line_count;
        ++bezier_count;
    }

    // Scale the contours to the range [0--1].
    U32 padding = 1;

    F32 x_scale = (F32) (width - 2 * padding) / (F32) (state->x_max - state->x_min);
    F32 y_scale = (F32) (height - 2 * padding) / (F32) (state->y_max - state->y_min);

    for (U32 i = 0; i < line_count; ++i) {
        segments[i].p0 = v2f32(
            ((segments[i].p0.x - state->x_min) * x_scale + (F32) padding) / (F32) width,
            ((state->y_max - segments[i].p0.y) * y_scale + (F32) padding) / (F32) height
        );
        segments[i].p1 = v2f32(
            ((segments[i].p1.x - state->x_min) * x_scale + (F32) padding) / (F32) width,
            ((state->y_max - segments[i].p1.y) * y_scale + (F32) padding) / (F32) height
        );
    }
    for (U32 i = line_count; i < line_count + bezier_count; ++i) {
        segments[i].p0 = v2f32(
            ((segments[i].p0.x - state->x_min) * x_scale + (F32) padding) / (F32) width,
            ((state->y_max - segments[i].p0.y) * y_scale + (F32) padding) / (F32) height
        );
        segments[i].p1 = v2f32(
            ((segments[i].p1.x - state->x_min) * x_scale + (F32) padding) / (F32) width,
            ((state->y_max - segments[i].p1.y) * y_scale + (F32) padding) / (F32) height
        );
        segments[i].p2 = v2f32(
            ((segments[i].p2.x - state->x_min) * x_scale + (F32) padding) / (F32) width,
            ((state->y_max - segments[i].p2.y) * y_scale + (F32) padding) / (F32) height
        );
    }

    // Generating bounding circles for the curves.
    for (U32 i = 0; i < line_count; ++i) {
        MSDF_Segment line = segments[i];
        V2F32 min = v2f32_min(line.p0, line.p1);
        V2F32 max = v2f32_max(line.p0, line.p1);
        V2F32 center = v2f32_scale(v2f32_add(max, min), 0.5f);
        F32 radius = 0.5f * v2f32_length(v2f32_subtract(max, min));
        state->circle_centers[i] = center;
        state->circle_radie[i]   = radius;
    }
    for (U32 i = line_count; i < line_count + bezier_count; ++i) {
        MSDF_Segment bezier = segments[i];
        V2F32 min = v2f32_min(v2f32_min(bezier.p0, bezier.p1), bezier.p2);
        V2F32 max = v2f32_max(v2f32_max(bezier.p0, bezier.p1), bezier.p2);
        V2F32 center = v2f32_scale(v2f32_add(max, min), 0.5f);
        F32 radius = 0.5f * v2f32_length(v2f32_subtract(max, min));
        state->circle_centers[i] = center;
        state->circle_radie[i]   = radius;
    }

    F32 distance_range = 2.0f / f32_min(width, height);
    U32 pixel_index = (x + y * stride) * 4;
    for (U32 y = 0; y < height; ++y) {
        U32 row = pixel_index;
        for (U32 x = 0; x < width; ++x) {
            MSDF_Distance red_distance   = { .distance = f32_infinity(), .orthogonality = 0.0f };
            S32 red_segment              = 0;
            MSDF_Distance green_distance = { .distance = f32_infinity(), .orthogonality = 0.0f };
            S32 green_segment            = 0;
            MSDF_Distance blue_distance  = { .distance = f32_infinity(), .orthogonality = 0.0f };
            S32 blue_segment             = 0;

            V2F32 point = v2f32((x + 0.5f) / (F32) width, (y + 0.5f) / (F32) height);
            for (U32 i = 0; i < line_count; ++i) {
                F32 min_distance = v2f32_length_squared(v2f32_subtract(state->circle_centers[i], point));

                F32 red   = red_distance.distance   + state->circle_radie[i];
                F32 green = green_distance.distance + state->circle_radie[i];
                F32 blue  = blue_distance.distance  + state->circle_radie[i];
                if (red * red >= min_distance || green * green >= min_distance || blue * blue >= min_distance) {
                    MSDF_Segment line = segments[i];
                    MSDF_Distance distance = msdf_line_distance_orthogonality(point, line);

                    if ((line.color & MSDF_COLOR_RED) && msdf_distance_is_closer(distance, red_distance)) {
                        red_distance = distance;
                        red_segment  = i;
                    }
                    if ((line.color & MSDF_COLOR_GREEN) && msdf_distance_is_closer(distance, green_distance)) {
                        green_distance = distance;
                        green_segment  = i;
                    }
                    if ((line.color & MSDF_COLOR_BLUE) && msdf_distance_is_closer(distance, blue_distance)) {
                        blue_distance = distance;
                        blue_segment  = i;
                    }
                }
            }

            for (U32 i = line_count; i < line_count + bezier_count; ++i) {
                F32 min_distance = v2f32_length_squared(v2f32_subtract(state->circle_centers[i], point));

                F32 red   = red_distance.distance   + state->circle_radie[i];
                F32 green = green_distance.distance + state->circle_radie[i];
                F32 blue  = blue_distance.distance  + state->circle_radie[i];
                if (red * red >= min_distance || green * green >= min_distance || blue * blue >= min_distance) {
                    MSDF_Segment bezier = segments[i];
                    MSDF_Distance distance = msdf_quadratic_bezier_distance_orthogonality(point, bezier);

                    if ((bezier.color & MSDF_COLOR_RED) && msdf_distance_is_closer(distance, red_distance)) {
                        red_distance = distance;
                        red_segment  = i;
                    }
                    if ((bezier.color & MSDF_COLOR_GREEN) && msdf_distance_is_closer(distance, green_distance)) {
                        green_distance = distance;
                        green_segment  = i;
                    }
                    if ((bezier.color & MSDF_COLOR_BLUE) && msdf_distance_is_closer(distance, blue_distance)) {
                        blue_distance = distance;
                        blue_segment  = i;
                    }
                }
            }

            if (segments[red_segment].kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
                red_distance.distance = msdf_quadratic_bezier_signed_pseudo_distance(point, segments[red_segment], red_distance.unclamped_t);
            } else {
                red_distance.distance = msdf_line_signed_pseudo_distance(point, segments[red_segment]);
            }
            if (segments[green_segment].kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
                green_distance.distance = msdf_quadratic_bezier_signed_pseudo_distance(point, segments[green_segment], green_distance.unclamped_t);
            } else {
                green_distance.distance = msdf_line_signed_pseudo_distance(point, segments[green_segment]);
            }
            if (segments[blue_segment].kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
                blue_distance.distance = msdf_quadratic_bezier_signed_pseudo_distance(point, segments[blue_segment], blue_distance.unclamped_t);
            } else {
                blue_distance.distance = msdf_line_signed_pseudo_distance(point, segments[blue_segment]);
            }

            S32 red   = s32_min(s32_max(0, f32_round_to_s32((red_distance.distance   / distance_range + 0.5f) * 255.0f)), 255);
            S32 green = s32_min(s32_max(0, f32_round_to_s32((green_distance.distance / distance_range + 0.5f) * 255.0f)), 255);
            S32 blue  = s32_min(s32_max(0, f32_round_to_s32((blue_distance.distance  / distance_range + 0.5f) * 255.0f)), 255);

            buffer[row++] = red;
            buffer[row++] = green;
            buffer[row++] = blue;
            buffer[row++] = 0;
        }
        pixel_index += stride * 4;
    }
}
