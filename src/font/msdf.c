// TODO: Allow for pruning small contours. This would hopefully increase the
// quality of the final MSDF, although it won't be as accurate any more.

internal Void msdf_quadratic_bezier_split(MSDF_Segment segment, F32 t, MSDF_Segment *result_a, MSDF_Segment *result_b) {
    V2F32 new_points[6] = { 0 };
    quadratic_bezier_split(segment.p0, segment.p1, segment.p2, t, new_points);

    result_a->kind  = MSDF_SEGMENT_QUADRATIC_BEZIER;
    result_a->p0    = new_points[0];
    result_a->p1    = new_points[1];
    result_a->p2    = new_points[2];
    result_a->flags = segment.flags;

    result_b->kind  = MSDF_SEGMENT_QUADRATIC_BEZIER;
    result_b->p0    = new_points[3];
    result_b->p1    = new_points[4];
    result_b->p2    = new_points[5];
    result_b->flags = segment.flags;
}

internal Void msdf_line_split(MSDF_Segment segment, F32 t, MSDF_Segment *result_a, MSDF_Segment *result_b) {
    V2F32 point = v2f32_add(segment.p0, v2f32_scale(v2f32_subtract(segment.p1, segment.p0), t));

    result_a->kind  = MSDF_SEGMENT_LINE;
    result_a->p0    = segment.p0;
    result_a->p1    = point;
    result_a->flags = segment.flags;

    result_b->kind  = MSDF_SEGMENT_LINE;
    result_b->p0    = point;
    result_b->p1    = segment.p1;
    result_b->flags = segment.flags;
}

internal Void msdf_segment_split(MSDF_Segment segment, F32 t, MSDF_Segment *result_a, MSDF_Segment *result_b) {
    if (segment.kind == MSDF_SEGMENT_LINE) {
        msdf_line_split(segment, t, result_a, result_b);
    } else if (segment.kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
        msdf_quadratic_bezier_split(segment, t, result_a, result_b);
    }
}

internal B32 msdf_distance_is_closer(MSDF_Distance a, MSDF_Distance b) {
    if (f32_abs(a.distance - b.distance) < F32_EPSILON) {
        return a.orthogonality > b.orthogonality;
    } else {
        return a.distance < b.distance;
    }
}

internal U32 msdf_quadratic_bezier_intersect_recurse(MSDF_Segment a, MSDF_Segment b, U32 iteration_count, F32 *result_ats, F32 *result_bts) {
    V2F32 a_min = v2f32_min(v2f32_min(a.p0, a.p1), a.p2);
    V2F32 a_max = v2f32_max(v2f32_max(a.p0, a.p1), a.p2);
    V2F32 b_min = v2f32_min(v2f32_min(b.p0, b.p1), b.p2);
    V2F32 b_max = v2f32_max(v2f32_max(b.p0, b.p1), b.p2);

    if (a_min.x < b_max.x && a_max.x > b_min.x && a_min.y < b_max.y && a_max.y > b_min.y) {
        if (iteration_count == 0) {
            // Assume that the curves are equivalent to lines at this point.
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

// Shoelace formula for calculating signed area, and thus winding number
// https://en.wikipedia.org/wiki/Shoelace_formula
internal S32 msdf_contour_calculate_own_winding_number(MSDF_Contour *contour) {
    F32 double_signed_area = 0.0f;
    V2F32 previous = (contour->last_segment->kind == MSDF_SEGMENT_LINE ? contour->last_segment->p1 : contour->last_segment->p2);
    for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
        if (segment->kind == MSDF_SEGMENT_LINE) {
            double_signed_area += previous.x    * segment->p0.y - segment->p0.x * previous.y;
            double_signed_area += segment->p0.x * segment->p1.y - segment->p1.x * segment->p0.y;
            previous = segment->p1;
        } else {
            // TODO(simon): We might want to use the vertex of the curve instead of P1.
            double_signed_area += previous.x    * segment->p0.y - segment->p0.x * previous.y;
            double_signed_area += segment->p0.x * segment->p1.y - segment->p1.x * segment->p0.y;
            double_signed_area += segment->p1.x * segment->p2.y - segment->p2.x * segment->p1.y;
            previous = segment->p2;
        }
    }

    S32 winding = f32_sign(double_signed_area);

    return winding;
}

internal S32 msdf_contour_calculate_global_winding_number(MSDF_Glyph *glyph, MSDF_Contour *contour) {
    // NOTE(simon): Calculcate global winding number.
    S32 global_winding = contour->local_winding;

    V2F32 test_point = contour->first_segment->p0;

    // https://en.wikipedia.org/wiki/Point_in_polygon
    for (MSDF_Contour *other_contour = glyph->first_contour; other_contour; other_contour = other_contour->next) {
        if (contour != other_contour) {
            U32 intersection_count = 0;

            for (MSDF_Segment *segment = other_contour->first_segment; segment; segment = segment->next) {
                if (segment->kind == MSDF_SEGMENT_LINE) {
                    // Ray line intersection
                    // p0 + u * (p1 - p0) = test_point + v * (1, 0)  u in [0, 1), v in [0, inf)
                    //   p0.x + u * (p1.x - p0.x) = test_point.x + v
                    //   p0.y + u * (p1.y - p0.y) = test_point.y
                    if ((segment->p0.y < test_point.y && test_point.y <= segment->p1.y) || (segment->p1.y < test_point.y && test_point.y <= segment->p0.y)) {
                        F32 u = (test_point.y - segment->p0.y) / (segment->p1.y - segment->p0.y);
                        F32 v = segment->p0.x + u * (segment->p1.x - segment->p0.x) - test_point.x;

                        if (0.0f <= u && u < 1.0f && 0.0f <= v) {
                            ++intersection_count;
                        }
                    }
                } else {
                    // TODO(simon): Properly handle the case where we intersect at a corner, see above.

                    // Ray quadratic bezier intersection
                    // u^2 * (p0 - 2 * p1 + p2) + u * 2 * (p1 - p0) + p0 = test_point + v * (1, 0)  u in [0, 1), v in [0, inf)
                    //   u^2 * (p0.x - 2 * p1.x + p2.x) + u * 2 * (p1.x - p0.x) + p0.x = test_point.x + v
                    //   u^2 * (p0.y - 2 * p1.y + p2.y) + u * 2 * (p1.y - p0.y) + p0.y = test_point.y

                    F32 ax = segment->p0.x - 2.0f * segment->p1.x + segment->p2.x;
                    F32 bx = 2.0f * (segment->p1.x - segment->p0.x);
                    F32 cx = segment->p0.x - test_point.x;

                    F32 ay = segment->p0.y - 2.0f * segment->p1.y + segment->p2.y;
                    F32 by = 2.0f * (segment->p1.y - segment->p0.y);
                    F32 cy = segment->p0.y - test_point.y;

                    F32 discriminant = by * by - 4.0f * ay * cy;
                    if (-0.0001f <= discriminant) {
                        F32 u0 = (-by - f32_sqrt(f32_max(0.0f, discriminant))) / (2.0f * ay);
                        F32 u1 = (-by + f32_sqrt(f32_max(0.0f, discriminant))) / (2.0f * ay);

                        F32 v0 = u0 * u0 * ax + u0 * bx + cx;
                        F32 v1 = u1 * u1 * ax + u1 * bx + cx;

                        if (0.0f <= u0 && u0 < 1.0f && 0.0f <= v0) {
                            ++intersection_count;
                        }

                        if (0.0f <= u1 && u1 < 1.0f && 0.0f <= v1) {
                            ++intersection_count;
                        }
                    }
                }
            }

            if (intersection_count % 2 != 0) {
                global_winding += other_contour->local_winding;
            }
        }
    }

    return global_winding;
}

internal B32 msdf_is_corner(MSDF_Segment a, MSDF_Segment b, F32 threshold) {
    V2F32 a_dir = v2f32(0.0f, 0.0f);
    V2F32 b_dir = v2f32(0.0f, 0.0f);
    switch (a.kind) {
        case MSDF_SEGMENT_NULL:             assert(!"Not reached!");                             break;
        case MSDF_SEGMENT_LINE:             a_dir = v2f32_normalize(v2f32_subtract(a.p1, a.p0)); break;
        case MSDF_SEGMENT_QUADRATIC_BEZIER: a_dir = v2f32_normalize(v2f32_subtract(a.p2, a.p1)); break;
        case MSDF_SEGMENT_KIND_COUNT:       a_dir = v2f32(0.0f, 0.0f);                           break;
    }
    switch (b.kind) {
        case MSDF_SEGMENT_NULL:             assert(!"Not reached!");                             break;
        case MSDF_SEGMENT_LINE:             b_dir = v2f32_normalize(v2f32_subtract(b.p1, b.p0)); break;
        case MSDF_SEGMENT_QUADRATIC_BEZIER: b_dir = v2f32_normalize(v2f32_subtract(b.p1, b.p0)); break;
        case MSDF_SEGMENT_KIND_COUNT:       b_dir = v2f32(0.0f, 0.0f);                           break;
    }

    B32 are_parallel       = f32_abs(v2f32_cross(a_dir, b_dir)) <= threshold;
    B32 are_same_direction = v2f32_dot(a_dir, b_dir) > 0.0f;
    B32 is_same_edge       = are_parallel && are_same_direction;
    B32 is_corner          = !is_same_edge;

    return is_corner;
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

internal Void msdf_resolve_contour_overlap(Arena *arena, MSDF_Glyph *glyph) {
    for (MSDF_Contour *a_contour = glyph->first_contour; a_contour; a_contour = a_contour->next) {
        for (MSDF_Contour *b_contour = a_contour->next; b_contour; b_contour = b_contour->next) {
            // Find 2 consecutive intersections along one of the contours.
            // Split the contours at the intersections. The parts "between" the
            // intersections switch which contour they belong to. Repeat until
            // there are no more intersections.
            U32 intersection_count = 0;
            MSDF_Segment *a_intersections[2];
            MSDF_Segment *b_intersections[2];
            for (MSDF_Segment *a_segment = a_contour->first_segment; a_segment; a_segment = a_segment->next) {
                F32 min_at          = f32_infinity();
                F32 min_bt          = f32_infinity();
                MSDF_Segment *min_b = 0;
                for (MSDF_Segment *b_segment = b_contour->first_segment; b_segment; b_segment = b_segment->next) {
                    F32 ats[4] = { 0 };
                    F32 bts[4] = { 0 };
                    U32 intersection_count = msdf_segment_intersect(*a_segment, *b_segment, ats, bts);
                    F32 intersection_epsilon = 0.0001f; // TODO: Move this out and figure out an appropiate value for it.
                    for (U32 i = 0; i < intersection_count; ++i) {
                        if (ats[i] < min_at && intersection_epsilon < ats[i] && ats[i] < 1.0f - intersection_epsilon) {
                            min_at = ats[i];
                            min_bt = bts[i];
                            min_b  = b_segment;
                        }
                    }
                }

                if (min_at <= 1.0f) {
                    MSDF_Segment *b_segment = min_b;

                    MSDF_Segment *a_new = arena_push_struct_zero(arena, MSDF_Segment);
                    MSDF_Segment *b_new = arena_push_struct_zero(arena, MSDF_Segment);
                    dll_insert_after(a_contour->first_segment, a_contour->last_segment, a_segment, a_new);
                    dll_insert_after(b_contour->first_segment, b_contour->last_segment, b_segment, b_new);
                    msdf_segment_split(*a_segment, min_at, a_segment, a_new);
                    msdf_segment_split(*b_segment, min_bt, b_segment, b_new);
                    a_intersections[intersection_count] = a_new;
                    b_intersections[intersection_count] = b_new;

                    // The corner with the narrowest angle is moved "inwards".
                    // It needs to move at least the distance between the two
                    // corners. This can be 0, so we also add a small amount to
                    // ensure that the corners do not overlapp.
                    // TODO: Why is the "small amount" 0.005f? What should it be?
                    V2F32 *a0_corner    = (a_segment->kind == MSDF_SEGMENT_LINE ? &a_segment->p1 : &a_segment->p2);
                    V2F32  a0_direction = v2f32_normalize(v2f32_subtract((a_segment->kind == MSDF_SEGMENT_LINE ? a_segment->p0 : a_segment->p1), *a0_corner));
                    V2F32 *a1_corner    = &a_new->p0;
                    V2F32  a1_direction = v2f32_normalize(v2f32_subtract(a_new->p1, *a1_corner));
                    V2F32 *b0_corner    = (b_segment->kind == MSDF_SEGMENT_LINE ? &b_segment->p1 : &b_segment->p2);
                    V2F32  b0_direction = v2f32_normalize(v2f32_subtract((b_segment->kind == MSDF_SEGMENT_LINE ? b_segment->p0 : b_segment->p1), *b0_corner));
                    V2F32 *b1_corner    = &b_new->p0;
                    V2F32  b1_direction = v2f32_normalize(v2f32_subtract(b_new->p1, *b1_corner));
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

                    ++intersection_count;

                    if (intersection_count == 2) {
                        // NOTE(simon): Swap so that we have b_intersections[0] ... b_intersections[1] ... last
                        b_contour->first_segment->previous = b_contour->last_segment;
                        b_contour->last_segment->next      = b_contour->first_segment;
                        b_contour->first_segment           = b_intersections[0];
                        b_contour->last_segment            = b_intersections[0]->previous;
                        b_contour->first_segment->previous = 0;
                        b_contour->last_segment->next      = 0;

                        // NOTE(simon): Swap the middle parts.
                        swap(a_intersections[0]->previous->next, b_contour->first_segment,           MSDF_Segment *);
                        swap(a_intersections[0]->previous,       b_intersections[0]->previous,       MSDF_Segment *);
                        swap(a_intersections[1]->previous->next, b_intersections[1]->previous->next, MSDF_Segment *);
                        swap(a_intersections[1]->previous,       b_intersections[1]->previous,       MSDF_Segment *);

                        intersection_count = 0;
                    }
                }
            }
        }
    }
}

internal Void msdf_convert_to_simple_polygons(Arena *arena, MSDF_Glyph *glyph) {
    for (MSDF_Contour *contour = glyph->first_contour; contour; contour = contour->next) {
        for (MSDF_Segment *a_segment = contour->first_segment; a_segment; a_segment = a_segment->next) {
            for (MSDF_Segment *b_segment = a_segment->next; b_segment; b_segment = b_segment->next) {
                F32 ats[4] = { 0 };
                F32 bts[4] = { 0 };
                U32 intersection_count = msdf_segment_intersect(*a_segment, *b_segment, ats, bts);

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
                    MSDF_Contour *new_contour = arena_push_struct_zero(arena, MSDF_Contour);
                    dll_push_back(glyph->first_contour, glyph->last_contour, new_contour);

                    MSDF_Segment *a_new = arena_push_struct_zero(arena, MSDF_Segment);
                    msdf_segment_split(*a_segment, min_at, a_segment, a_new);
                    dll_insert_after(contour->first_segment, contour->last_segment, a_segment, a_new);

                    MSDF_Segment *b_new = arena_push_struct_zero(arena, MSDF_Segment);
                    msdf_segment_split(*b_segment, min_bt, b_new, b_segment);
                    dll_insert_before(contour->first_segment, contour->last_segment, b_segment, b_new);

                    new_contour->first_segment = a_new;
                    new_contour->last_segment  = b_new;
                    a_new->previous = 0;
                    b_new->next     = 0;

                    a_segment->next     = b_segment;
                    b_segment->previous = a_segment;

                    // Ensure that the contour is connected properly.
                    V2F32 *a0_corner = &a_new->p0;
                    V2F32 *a1_corner = (b_new->kind == MSDF_SEGMENT_LINE ? &b_new->p1 : &b_new->p2);
                    V2F32 a_corner = v2f32_scale(v2f32_add(*a0_corner, *a1_corner), 0.5f);
                    *a0_corner = a_corner;
                    *a1_corner = a_corner;

                    V2F32 *b0_corner = &b_segment->p0;
                    V2F32 *b1_corner = (a_segment->kind == MSDF_SEGMENT_LINE ? &a_segment->p1 : &a_segment->p2);
                    V2F32 b_corner = v2f32_scale(v2f32_add(*b0_corner, *b1_corner), 0.5f);
                    *b0_corner = b_corner;
                    *b1_corner = b_corner;
                }
            }
        }
    }
}

internal Void msdf_correct_contour_orientation(MSDF_Glyph *glyph) {
    for (MSDF_Contour *contour = glyph->first_contour; contour; contour = contour->next) {
        contour->local_winding = msdf_contour_calculate_own_winding_number(contour);
    }

    for (MSDF_Contour *contour = glyph->first_contour; contour; contour = contour->next) {
        S32 global_winding = msdf_contour_calculate_global_winding_number(glyph, contour);

        // NOTE(simon): Determine if each contour should be kept and if we need to flip it.
        if (-1 <= global_winding && global_winding <= 1) {
            contour->flags |= MSDF_ContourFlags_Keep;
        }

        if ((global_winding == 0 && contour->local_winding == 1) || (global_winding != 0 && contour->local_winding == -1)) {
            contour->flags |= MSDF_ContourFlags_Flip;
        }
    }

    // Rebuild the list of contours while applying the accumulated change set.
    MSDF_Contour *first = 0;
    MSDF_Contour *last  = 0;
    for (MSDF_Contour *contour = glyph->first_contour, *next; contour; contour = next) {
        next = contour->next;

        if (contour->flags & MSDF_ContourFlags_Flip) {
            MSDF_Segment *first_segment = 0;
            MSDF_Segment *last_segment  = 0;

            for (MSDF_Segment *segment = contour->last_segment, *previous; segment; segment = previous) {
                previous = segment->previous;

                if (segment->kind == MSDF_SEGMENT_LINE) {
                    V2F32 temp = segment->p0;
                    segment->p0 = segment->p1;
                    segment->p1 = temp;
                } else if (segment->kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
                    V2F32 temp = segment->p0;
                    segment->p0 = segment->p2;
                    segment->p2 = temp;
                }

                dll_push_back(first_segment, last_segment, segment);
            }

            contour->first_segment = first_segment;
            contour->last_segment  = last_segment;
        }

        if (contour->flags & MSDF_ContourFlags_Keep) {
            dll_push_back(first, last, contour);
        }
    }

    glyph->first_contour = first;
    glyph->last_contour  = last;
}

internal Void msdf_color_edges(MSDF_Glyph glyph) {
    F32 corner_threshold = f32_sin(0.1f);

    for (MSDF_Contour *contour = glyph.first_contour; contour; contour = contour->next) {
        U32 corner_count = 0;
        MSDF_Segment *last_corner_start = 0;
        for (
            MSDF_Segment *previous = contour->last_segment, *current = contour->first_segment;
            current;
            previous = current, current = current->next
        ) {
            current->flags = 0;

            if (msdf_is_corner(*previous, *current, corner_threshold)) {
                last_corner_start = current;
                ++corner_count;
                current->flags  |= MSDF_EDGE_START;
                previous->flags |= MSDF_EDGE_END;
            }
        }

        if (corner_count == 0) {
            for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
                segment->flags = MSDF_COLOR_RED | MSDF_COLOR_GREEN | MSDF_COLOR_BLUE;
            }
        } if (corner_count == 1) {
            for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
                segment->flags = MSDF_COLOR_RED | MSDF_COLOR_BLUE;
            }

            // TODO(simon): More carefully handle how we split edges in this case.
            // NOTE(simon): We need to split the contour into two edges in order to preserve the corner.
            last_corner_start->flags = MSDF_COLOR_RED | MSDF_COLOR_GREEN;
        } else {
            MSDF_ColorFlags current_color = MSDF_COLOR_RED | MSDF_COLOR_BLUE;

            for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
                segment->flags |= current_color;

                if (segment->flags & MSDF_EDGE_END) {
                    if (current_color == (MSDF_COLOR_RED | MSDF_COLOR_GREEN)) {
                        current_color = MSDF_COLOR_GREEN | MSDF_COLOR_BLUE;
                    } else {
                        current_color = MSDF_COLOR_RED | MSDF_COLOR_GREEN;
                    }
                }
            }

            // NOTE(simon): The first edge might cross the start and end of the
            // list and would now have two different colors, correct it!
            if (!(contour->first_segment->flags & MSDF_EDGE_START)) {
                for (
                    MSDF_Segment *segment = contour->last_segment;
                    segment && !(segment->flags & MSDF_EDGE_END);
                    segment = segment->previous
                ) {
                    segment->flags |= MSDF_COLOR_RED | MSDF_COLOR_BLUE;
                }
            }
        }
    }
}

internal U8 *msdf_generate(Arena *arena, MSDF_Glyph glyph, U32 render_size) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    msdf_resolve_contour_overlap(scratch.arena, &glyph);
    msdf_convert_to_simple_polygons(scratch.arena, &glyph);
    msdf_correct_contour_orientation(&glyph);
    msdf_color_edges(glyph);

    // NOTE(simon): We no longer need the segments to be organized in curves or
    // have any order amongst themselves. Separate them by kind to ease
    // processing.
    MSDF_SegmentList lines        = { 0 };
    MSDF_SegmentList quad_beziers = { 0 };
    for (MSDF_Contour *contour = glyph.first_contour; contour; contour = contour->next) {
        for (MSDF_Segment *segment = contour->first_segment, *next; segment; segment = next) {
            next = segment->next;

            switch (segment->kind) {
                case MSDF_SEGMENT_NULL: {
                    assert(!"Not reached");
                } break;
                case MSDF_SEGMENT_LINE: {
                    dll_push_back(lines.first, lines.last, segment);
                } break;
                case MSDF_SEGMENT_QUADRATIC_BEZIER: {
                    dll_push_back(quad_beziers.first, quad_beziers.last, segment);
                } break;
                case MSDF_SEGMENT_KIND_COUNT: {
                    assert(!"Not reached");
                } break;
            }
        }
    }

    // Scale the contours to the range [0--1] and generate bounding circles
    // for the them.
    U32 padding = 1;

    F32 x_scale = (F32) (render_size - 2 * padding) / (F32) (glyph.x_max - glyph.x_min);
    F32 y_scale = (F32) (render_size - 2 * padding) / (F32) (glyph.y_max - glyph.y_min);

    for (MSDF_Segment *line = lines.first; line; line = line->next) {
        line->p0 = v2f32(
            ((line->p0.x  - glyph.x_min) * x_scale + (F32) padding) / (F32) render_size,
            ((glyph.y_max - line->p0.y)  * y_scale + (F32) padding) / (F32) render_size
        );
        line->p1 = v2f32(
            ((line->p1.x  - glyph.x_min) * x_scale + (F32) padding) / (F32) render_size,
            ((glyph.y_max - line->p1.y)  * y_scale + (F32) padding) / (F32) render_size
        );

        V2F32 min = v2f32_min(line->p0, line->p1);
        V2F32 max = v2f32_max(line->p0, line->p1);
        V2F32 center = v2f32_scale(v2f32_add(max, min), 0.5f);
        F32 radius = 0.5f * v2f32_length(v2f32_subtract(max, min));
        line->circle_center = center;
        line->circle_radius = radius;
    }
    for (MSDF_Segment *bezier = quad_beziers.first; bezier; bezier = bezier->next) {
        bezier->p0 = v2f32(
            ((bezier->p0.x - glyph.x_min)  * x_scale + (F32) padding) / (F32) render_size,
            ((glyph.y_max  - bezier->p0.y) * y_scale + (F32) padding) / (F32) render_size
        );
        bezier->p1 = v2f32(
            ((bezier->p1.x - glyph.x_min)  * x_scale + (F32) padding) / (F32) render_size,
            ((glyph.y_max  - bezier->p1.y) * y_scale + (F32) padding) / (F32) render_size
        );
        bezier->p2 = v2f32(
            ((bezier->p2.x - glyph.x_min)  * x_scale + (F32) padding) / (F32) render_size,
            ((glyph.y_max  - bezier->p2.y) * y_scale + (F32) padding) / (F32) render_size
        );

        V2F32 min = v2f32_min(v2f32_min(bezier->p0, bezier->p1), bezier->p2);
        V2F32 max = v2f32_max(v2f32_max(bezier->p0, bezier->p1), bezier->p2);
        V2F32 center = v2f32_scale(v2f32_add(max, min), 0.5f);
        F32 radius = 0.5f * v2f32_length(v2f32_subtract(max, min));
        bezier->circle_center = center;
        bezier->circle_radius = radius;
    }

    F32 distance_range = 2.0f / render_size;
    U32 pixel_index = 0;
    U8 *buffer = arena_push_array(arena, U8, 4 * render_size * render_size);
    for (U32 y = 0; y < render_size; ++y) {
        for (U32 x = 0; x < render_size; ++x) {
            MSDF_Segment nil_segment     = { 0 };
            MSDF_Distance red_distance   = { .distance = f32_infinity(), .orthogonality = 0.0f };
            MSDF_Segment *red_segment    = &nil_segment;
            MSDF_Distance green_distance = { .distance = f32_infinity(), .orthogonality = 0.0f };
            MSDF_Segment *green_segment  = &nil_segment;
            MSDF_Distance blue_distance  = { .distance = f32_infinity(), .orthogonality = 0.0f };
            MSDF_Segment *blue_segment   = &nil_segment;

            V2F32 point = v2f32((x + 0.5f) / (F32) render_size, (y + 0.5f) / (F32) render_size);
            for (MSDF_Segment *line = lines.first; line; line = line->next) {
                F32 min_distance = v2f32_length_squared(v2f32_subtract(line->circle_center, point));

                F32 red   = red_distance.distance   + line->circle_radius;
                F32 green = green_distance.distance + line->circle_radius;
                F32 blue  = blue_distance.distance  + line->circle_radius;
                if (red * red >= min_distance || green * green >= min_distance || blue * blue >= min_distance) {
                    MSDF_Distance distance = msdf_line_distance_orthogonality(point, *line);

                    if ((line->flags & MSDF_COLOR_RED) && msdf_distance_is_closer(distance, red_distance)) {
                        red_distance = distance;
                        red_segment  = line;
                    }
                    if ((line->flags & MSDF_COLOR_GREEN) && msdf_distance_is_closer(distance, green_distance)) {
                        green_distance = distance;
                        green_segment  = line;
                    }
                    if ((line->flags & MSDF_COLOR_BLUE) && msdf_distance_is_closer(distance, blue_distance)) {
                        blue_distance = distance;
                        blue_segment  = line;
                    }
                }
            }

            for (MSDF_Segment *bezier = quad_beziers.first; bezier; bezier = bezier->next) {
                F32 min_distance = v2f32_length_squared(v2f32_subtract(bezier->circle_center, point));

                F32 red   = red_distance.distance   + bezier->circle_radius;
                F32 green = green_distance.distance + bezier->circle_radius;
                F32 blue  = blue_distance.distance  + bezier->circle_radius;
                if (red * red >= min_distance || green * green >= min_distance || blue * blue >= min_distance) {
                    MSDF_Distance distance = msdf_quadratic_bezier_distance_orthogonality(point, *bezier);

                    if ((bezier->flags & MSDF_COLOR_RED) && msdf_distance_is_closer(distance, red_distance)) {
                        red_distance = distance;
                        red_segment  = bezier;
                    }
                    if ((bezier->flags & MSDF_COLOR_GREEN) && msdf_distance_is_closer(distance, green_distance)) {
                        green_distance = distance;
                        green_segment  = bezier;
                    }
                    if ((bezier->flags & MSDF_COLOR_BLUE) && msdf_distance_is_closer(distance, blue_distance)) {
                        blue_distance = distance;
                        blue_segment  = bezier;
                    }
                }
            }

            if (red_segment->kind == MSDF_SEGMENT_LINE) {
                red_distance.distance = msdf_line_signed_pseudo_distance(point, *red_segment);
            } else if (red_segment->kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
                red_distance.distance = msdf_quadratic_bezier_signed_pseudo_distance(point, *red_segment, red_distance.unclamped_t);
            }
            if (green_segment->kind == MSDF_SEGMENT_LINE) {
                green_distance.distance = msdf_line_signed_pseudo_distance(point, *green_segment);
            } else if (green_segment->kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
                green_distance.distance = msdf_quadratic_bezier_signed_pseudo_distance(point, *green_segment, green_distance.unclamped_t);
            }
            if (blue_segment->kind == MSDF_SEGMENT_LINE) {
                blue_distance.distance = msdf_line_signed_pseudo_distance(point, *blue_segment);
            } else if (blue_segment->kind == MSDF_SEGMENT_QUADRATIC_BEZIER) {
                blue_distance.distance = msdf_quadratic_bezier_signed_pseudo_distance(point, *blue_segment, blue_distance.unclamped_t);
            }

            S32 red   = s32_min(s32_max(0, f32_round_to_s32((red_distance.distance   / distance_range + 0.5f) * 255.0f)), 255);
            S32 green = s32_min(s32_max(0, f32_round_to_s32((green_distance.distance / distance_range + 0.5f) * 255.0f)), 255);
            S32 blue  = s32_min(s32_max(0, f32_round_to_s32((blue_distance.distance  / distance_range + 0.5f) * 255.0f)), 255);

            buffer[pixel_index++] = red;
            buffer[pixel_index++] = green;
            buffer[pixel_index++] = blue;
            buffer[pixel_index++] = 0;
        }
    }

    arena_end_temporary(scratch);

    return buffer;
}
