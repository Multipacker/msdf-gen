// TODO: Allow for pruning small contours. This would hopefully increase the
// quality of the final MSDF, although it won't be as accurate any more.

// NOTE(simon): New algorithm for resolving contours overlap:
//
// 1. Convert to simple contours
//    Contours should not self-intersect after this is done, This can use the
//    current solution.
// 2. Compute local winding numbers
//    Every contour gets assigned a winding number describing in which order
//    the vertices are specified. This can use the current solution.
// 3. Do joins between different winding numbers (boolean xor)
// 4. Join contours.
//    Every pair of contours is checked, and any new contours produced by this
//    step has to be compared against all other ones. If the two contours have
//    different winding numbers, we are done. This is because this would
//    produce two contours that are infinitely close to each other, which is
//    not well defined. Otherwise, the only possible operation is a boolean
//    join between the two shapes.
//
//    Find the leftmost on curve point of both contours. This must be part of
//    the outermost contour that these inputs form. If both contours have a
//    vertex in the exakt same spot, look at direction of the derivative of
//    both contours. The one that changes inwards the slowest is outside. If
//    they are identical, move to the next point and repeat the check. If you
//    get all the way back to the starting point, the two contours perfectly
//    overlap each other and you can discard one of them.
//
//    Now that we have a point that is outside, pick of segments from the
//    contour, until you hit an intersection. At this point, switch which
//    contour you are grabbing segments from and which one you are checking
//    against. Once there are no more segments to grab, you have finished the
//    outer contour. This will _always_ be pushed to the result and should have
//    the same winding number as the inputs. This process is repeated until
//    there are no more segments, but now we only the contours if their winding
//    number is the opposite of the inputs (maybe if they sum to zero?), these
//    would cut holes into the shape. Anything else would just double up the
//    area in one place. (Maybe we always add contours and then do the contour
//    correction pass that we currently do?).

global MSDF_LogState msdf_log_state;

internal MSDF_LogNode *msdf_log_create_node(Void) {
    MSDF_LogNode *result = arena_push_struct(msdf_log_state.arena, MSDF_LogNode);

    if (msdf_log_state.parent_stack) {
        result->parent = msdf_log_state.parent_stack->node;
        sll_queue_push(result->parent->first, result->parent->last, result);
    }

    return result;
}

internal MSDF_LogNode *msdf_log_create_node_from_string(Str8 string) {
    MSDF_LogNode *result = msdf_log_create_node();
    result->string = str8_copy(msdf_log_state.arena, string);
    return result;
}

internal MSDF_LogNode *msdf_log_create_node_from_string_format(CStr format, ...) {
    MSDF_LogNode *result = msdf_log_create_node();

    va_list arguments;
    va_start(arguments, format);
    result->string = str8_format_list(msdf_log_state.arena, format, arguments);
    va_end(arguments);

    return result;
}

internal MSDF_LogNode *msdf_log_create_node_from_string_format_list(CStr format, va_list arguments) {
    MSDF_LogNode *result = msdf_log_create_node();

    result->string = str8_format_list(msdf_log_state.arena, format, arguments);

    return result;
}

internal Void msdf_log_node_set_string(MSDF_LogNode *node, Str8 string) {
    node->string = str8_copy(msdf_log_state.arena, string);
}

internal Void msdf_log_node_set_string_format(MSDF_LogNode *node, CStr format, ...) {
    va_list arguments;
    va_start(arguments, format);
    node->string = str8_format_list(msdf_log_state.arena, format, arguments);
    va_end(arguments);
}

internal Void msdf_log_push_parent(MSDF_LogNode *node) {
    MSDF_LogNodeStack *node_node = msdf_log_state.stack_freelist;
    if (node_node) {
        sll_stack_pop(msdf_log_state.stack_freelist);
        memory_zero_struct(node_node);
    } else {
        node_node = arena_push_struct(msdf_log_state.scratch_arena, MSDF_LogNodeStack);
    }

    node_node->node = node;
    sll_stack_push(msdf_log_state.parent_stack, node_node);
}

internal Void msdf_log_pop_parent(Void) {
    MSDF_LogNodeStack *node_node = msdf_log_state.parent_stack;
    sll_stack_pop(msdf_log_state.parent_stack);
    sll_stack_push(msdf_log_state.stack_freelist, node_node);
}

internal MSDF_LogNodeIterator msdf_log_iterator_depth_first_pre_order(MSDF_LogNode *node, MSDF_LogNode *root) {
    MSDF_LogNodeIterator iterator = { 0 };

    if (node->first) {
        iterator.next       = node->first;
        iterator.push_count = 1;
    } else {
        for (MSDF_LogNode *parent = node; parent && parent != root; parent = parent->parent, iterator.pop_count) {
            if (parent->next) {
                iterator.next = parent->next;
                break;
            }
        }
    }

    return iterator;
}

internal MSDF_LogNode *msdf_log_push_parent_string(Str8 string) {
    MSDF_LogNode *result = msdf_log_create_node_from_string(string);
    msdf_log_push_parent(result);
    return result;
}

internal MSDF_LogNode *msdf_log_push_parent_string_format(CStr format, ...) {
    va_list arguments;
    va_start(arguments, format);
    MSDF_LogNode *result = msdf_log_create_node_from_string_format_list(format, arguments);
    va_end(arguments);

    msdf_log_push_parent(result);

    return result;
}

internal MSDF_LogNode *msdf_log_create_point(V2F32 p0, V4F32 color) {
    MSDF_LogNode *result = msdf_log_create_node_from_string(str8_literal("Point"));
    result->flags |= MSDF_LogNodeFlag_DrawPoint;
    result->color  = color;
    result->p0     = p0;
    return result;
}

internal MSDF_LogNode *msdf_log_create_line(V2F32 p0, V2F32 p1, V4F32 color) {
    MSDF_LogNode *result = msdf_log_create_node_from_string(str8_literal("Line"));
    result->flags |= MSDF_LogNodeFlag_DrawLine;
    result->color  = color;
    result->p0     = p0;
    result->p1     = p1;
    return result;
}

internal MSDF_LogNode *msdf_log_create_bezier(V2F32 p0, V2F32 p1, V2F32 p2, V4F32 color) {
    MSDF_LogNode *result = msdf_log_create_node_from_string(str8_literal("Bezier"));
    result->flags |= MSDF_LogNodeFlag_DrawBezier;
    result->color  = color;
    result->p0     = p0;
    result->p1     = p1;
    result->p2     = p2;
    return result;
}

internal MSDF_LogNode *msdf_log_create_segment(MSDF_Segment *segment, V4F32 color) {
    MSDF_LogNode *result = 0;
    switch (segment->kind) {
        case MSDF_Segment_Null: {
        } break;
        case MSDF_Segment_Line: {
            result = msdf_log_create_line(segment->p0, segment->p1, color);
        } break;
        case MSDF_Segment_QuadraticBezier: {
            result = msdf_log_create_bezier(segment->p0, segment->p1, segment->p2, color);
        } break;
        case MSDF_Segment_COUNT: {
        } break;
    }
    return result;
}

internal MSDF_LogNode *msdf_log_create_contour(MSDF_Contour *contour, V4F32 color) {
    MSDF_LogNode *contour_group = msdf_log_push_parent_string(str8_literal("Contour"));
    for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
        msdf_log_create_segment(segment, color);
    }
    msdf_log_pop_parent();
    return contour_group;
}

internal MSDF_LogNode *msdf_log_create_glyph(MSDF_Glyph *glyph, V4F32 color) {
    MSDF_LogNode *glyph_group = msdf_log_push_parent_string(str8_literal("Glyph"));
    for (MSDF_Contour *contour = glyph->first_contour; contour; contour = contour->next) {
        msdf_log_create_contour(contour, color);
    }
    msdf_log_pop_parent();
    return glyph_group;
}



internal Void msdf_quadratic_bezier_split(MSDF_Segment segment, F32 t, MSDF_Segment *result_a, MSDF_Segment *result_b) {
    // De Casteljau's algorithm.
    V2F32 a = v2f32_add(segment.p0, v2f32_scale(v2f32_subtract(segment.p1, segment.p0), t));
    V2F32 b = v2f32_add(segment.p1, v2f32_scale(v2f32_subtract(segment.p2, segment.p1), t));
    V2F32 c = v2f32_add(a, v2f32_scale(v2f32_subtract(b, a), t));

    result_a->kind  = MSDF_Segment_QuadraticBezier;
    result_a->p0    = segment.p0;
    result_a->p1    = a;
    result_a->p2    = c;
    result_a->flags = segment.flags;

    result_b->kind  = MSDF_Segment_QuadraticBezier;
    result_b->p0    = c;
    result_b->p1    = b;
    result_b->p2    = segment.p2;
    result_b->flags = segment.flags;
}

internal Void msdf_line_split(MSDF_Segment segment, F32 t, MSDF_Segment *result_a, MSDF_Segment *result_b) {
    V2F32 point = v2f32_add(segment.p0, v2f32_scale(v2f32_subtract(segment.p1, segment.p0), t));

    result_a->kind  = MSDF_Segment_Line;
    result_a->p0    = segment.p0;
    result_a->p1    = point;
    result_a->flags = segment.flags;

    result_b->kind  = MSDF_Segment_Line;
    result_b->p0    = point;
    result_b->p1    = segment.p1;
    result_b->flags = segment.flags;
}

internal Void msdf_segment_split(MSDF_Segment segment, F32 t, MSDF_Segment *result_a, MSDF_Segment *result_b) {
    if (segment.kind == MSDF_Segment_Line) {
        msdf_line_split(segment, t, result_a, result_b);
    } else if (segment.kind == MSDF_Segment_QuadraticBezier) {
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

internal U32 msdf_quadratic_bezier_intersect_recurse(MSDF_Segment a, MSDF_Segment b, U32 iteration_count, F32 *result_ats, F32 *result_bts, U32 slots_left) {
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

                if (0.0f <= at && at < 1.0f && 0.0f <= bt && bt < 1.0f && slots_left) {
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
                U32 new_solutions = msdf_quadratic_bezier_intersect_recurse(segments[i / 2], segments[2 + i % 2], iteration_count - 1, &result_ats[count], &result_bts[count], slots_left - count);
                for (U32 j = 0; j < new_solutions; ++j) {
                    result_ats[count + j] = (F32) (i / 2) * 0.5f + result_ats[count + j] * 0.5f;
                    result_bts[count + j] = (F32) (i % 2) * 0.5f + result_bts[count + j] * 0.5f;
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
    U32 ar = (U32) f32_max(0.0f, f32_log2(f32_sqrt(f32_sqrt(alx * alx + aly * aly) / (8.0f * error))));
    U32 br = (U32) f32_max(0.0f, f32_log2(f32_sqrt(f32_sqrt(blx * blx + bly * bly) / (8.0f * error))));
    U32 r  = u32_max(ar, br);

    U32 count = msdf_quadratic_bezier_intersect_recurse(a, b, r, result_ats, result_bts, 4);

    return count;
}

internal U32 msdf_line_quadratic_bezier_intersect(MSDF_Segment line, MSDF_Segment bezier, F32 *result_line_ts, F32 *result_bezier_ts) {
    // NOTE(simon): Extract points.
    V2F32 l0 = line.p0;
    V2F32 l1 = line.p1;
    V2F32 q0 = bezier.p0;
    V2F32 q1 = bezier.p1;
    V2F32 q2 = bezier.p2;

    // NOTE(simon): Compute coefficients for normal forms.
    // q = qa * qt^2 + qb * qt + qc
    // l = la * qt + lb
    V2F32 la = v2f32_subtract(l1, l0);
    V2F32 lb = l0;
    V2F32 qa = v2f32_add(v2f32_subtract(q0, v2f32_scale(q1, 2.0f)), q2);
    V2F32 qb = v2f32_scale(v2f32_subtract(q1, q0), 2.0f);
    V2F32 qc = q0;

    // NOTE(simon): Compute coefficients for quadratic formula.
    // a * qt^2 + b * qt + c = 0
    F32 a = la.x * qa.y - la.y * qa.x;
    F32 b = la.x * qb.y - la.y * qb.x;
    F32 c = (la.x * qc.y - la.y * qc.x) - (la.x * lb.y - la.y * lb.x);

    // NOTE(simon): Solve quadratic.
    F32 qt0 = f32_infinity();
    F32 qt1 = f32_infinity();
    if (f32_abs(a) >= 0.000001f) {
        // NOTE(simon): Use the quadratic formula.
        F32 discriminant = b * b - 4.0f * a * c;
        if (f32_abs(discriminant) > 0.000001f) {
            // NOTE(simon): Real roots <=> intersections.
            F32 sqrt_discriminant = f32_sqrt(discriminant);

            qt0 = (-b - sqrt_discriminant) / (2.0f * a);
            qt1 = (-b + sqrt_discriminant) / (2.0f * a);
        } else {
            // NOTE(simon): Complex roots <=> no intersection.
        }
    } else if (f32_abs(b) >= 0.000001f) {
        // NOTE(simon): The bezier is a line. Solve it as such.
        qt0 = qt1 = -c / b;
    } else {
        // NOTE(simon): The bezier is a point => no intersection.
    }

    // NOTE(simon): Compute lt0 and lt1 from qt0 and qt1.
    F32 lt0 = f32_infinity();
    F32 lt1 = f32_infinity();
    if (f32_abs(la.x) > 0.000001f) {
        lt0 = (qa.x * qt0 * qt0 + qb.x * qt0 + qc.x - lb.x) / la.x;
        lt1 = (qa.x * qt1 * qt1 + qb.x * qt1 + qc.x - lb.x) / la.x;
    } else if (f32_abs(la.y) > 0.000001f) {
        lt0 = (qa.y * qt0 * qt0 + qb.y * qt0 + qc.y - lb.y) / la.y;
        lt1 = (qa.y * qt1 * qt1 + qb.y * qt1 + qc.y - lb.y) / la.y;
    } else {
        // NOTE(simon): The line is just a point => no intersection.
    }

    // NOTE(simon): Push valid solutions to output.
    U32 intersection_count = 0;
    if (0.0f <= lt0 && lt0 < 1.0f && 0.0f <= qt0 && qt0 < 1.0f) {
        result_line_ts[intersection_count] = lt0;
        result_bezier_ts[intersection_count] = qt0;
        ++intersection_count;
    }
    if (0.0f <= lt1 && lt1 < 1.0f && 0.0f <= qt1 && qt1 < 1.0f) {
        result_line_ts[intersection_count] = lt1;
        result_bezier_ts[intersection_count] = qt1;
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

internal V2F32 msdf_point_from_segment_t(MSDF_Segment *segment, F32 t) {
    V2F32 result = { 0 };

    switch (segment->kind) {
        case MSDF_Segment_Null: {
        } break;
        case MSDF_Segment_Line: {
            result = v2f32_add(segment->p0, v2f32_scale(v2f32_subtract(segment->p1, segment->p0), t));
        } break;
        case MSDF_Segment_QuadraticBezier: {
            V2F32 a = v2f32_add(segment->p0, v2f32_scale(v2f32_subtract(segment->p1, segment->p0), t));
            V2F32 b = v2f32_add(segment->p1, v2f32_scale(v2f32_subtract(segment->p2, segment->p1), t));
            result  = v2f32_add(a,           v2f32_scale(v2f32_subtract(b,           a),           t));
        } break;
        case MSDF_Segment_COUNT: {
        } break;
    }

    return result;
}

internal U32 msdf_segment_intersect(MSDF_Segment a, MSDF_Segment b, F32 *result_ats, F32 *result_bts) {
    Str8 log_name = { 0 };

    U32 intersection_count = 0;
    if (a.kind == MSDF_Segment_Line && b.kind == MSDF_Segment_Line) {
        intersection_count = msdf_line_intersect(a, b, result_ats, result_bts);
        log_name = str8_literal("line-line intersection");
    } else if (a.kind == MSDF_Segment_Line && b.kind == MSDF_Segment_QuadraticBezier) {
        intersection_count = msdf_line_quadratic_bezier_intersect(a, b, result_ats, result_bts);
        log_name = str8_literal("line-bezier intersection");
    } else if (a.kind == MSDF_Segment_QuadraticBezier && b.kind == MSDF_Segment_Line) {
        intersection_count = msdf_line_quadratic_bezier_intersect(b, a, result_bts, result_ats);
        log_name = str8_literal("line-bezier intersection");
    } else if (a.kind == MSDF_Segment_QuadraticBezier && b.kind == MSDF_Segment_QuadraticBezier) {
        intersection_count = msdf_quadratic_bezier_intersect(a, b, result_ats, result_bts);
        log_name = str8_literal("bezier-bezier intersection");
    }

    if (intersection_count) {
        MSDF_LogNode *group = msdf_log_create_node_from_string(log_name);
        msdf_log_push_parent(group);

        msdf_log_create_segment(&a, v4f32(1.0f, 0.0f, 0.0f, 1.0f));
        msdf_log_create_segment(&b, v4f32(0.0f, 1.0f, 0.0f, 1.0f));

        for (U32 i = 0; i < intersection_count; ++i) {
            MSDF_LogNode *intersection = msdf_log_create_node_from_string_format("at: %f, bt: %f", result_ats[i], result_bts[i]);
            msdf_log_push_parent(intersection);
            msdf_log_create_point(msdf_point_from_segment_t(&a, result_ats[i]), v4f32(0.5f, 0.0f, 0.0f, 1.0f));
            msdf_log_create_point(msdf_point_from_segment_t(&b, result_bts[i]), v4f32(0.0f, 0.5f, 0.0f, 1.0f));
            msdf_log_pop_parent();
        }

        msdf_log_pop_parent();
    }

    return intersection_count;
}

// Shoelace formula for calculating signed area, and thus winding number
// https://en.wikipedia.org/wiki/Shoelace_formula
internal S32 msdf_contour_calculate_own_winding_number(MSDF_Contour *contour) {
    F32 double_signed_area = 0.0f;
    V2F32 previous = (contour->last_segment->kind == MSDF_Segment_Line ? contour->last_segment->p1 : contour->last_segment->p2);
    for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
        if (segment->kind == MSDF_Segment_Line) {
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

    S32 winding = (S32) f32_sign(double_signed_area);

    return winding;
}

internal S32 msdf_contour_calculate_global_winding_number(Arena *arena, MSDF_Glyph *glyph, MSDF_Contour *contour) {
    // NOTE(simon): Calculcate global winding number.
    S32 global_winding = contour->local_winding;

    V2F32 test_point = contour->first_segment->p0;

    // https://en.wikipedia.org/wiki/Point_in_polygon
    for (MSDF_Contour *other_contour = glyph->first_contour; other_contour; other_contour = other_contour->next) {
        if (contour != other_contour) {
            U32 intersection_count = 0;

            for (MSDF_Segment *segment = other_contour->first_segment; segment; segment = segment->next) {
                if (segment->kind == MSDF_Segment_Line) {
                    // Ray line intersection
                    // p0 + u * (p1 - p0) = test_point + v * (1, 0)  u in [0, 1), v in [0, inf)
                    //   p0.x + u * (p1.x - p0.x) = test_point.x + v
                    //   p0.y + u * (p1.y - p0.y) = test_point.y
                    if (
                        (test_point.x <= segment->p0.x || test_point.x <= segment->p1.x) && (
                            (segment->p0.y <= test_point.y && test_point.y < segment->p1.y) ||
                            (segment->p1.y <= test_point.y && test_point.y < segment->p0.y)
                        )
                    ) {
                        F32 u = (test_point.y - segment->p0.y) / (segment->p1.y - segment->p0.y);
                        F32 v = segment->p0.x + u * (segment->p1.x - segment->p0.x) - test_point.x;

                        if (0.0f <= u && u <= 1.0f && 0.0f <= v) {
                            MSDF_LogNode *intersection = msdf_log_create_node_from_string(str8_literal("Line intersection"));
                            msdf_log_push_parent(intersection);
                            msdf_log_create_contour(contour, v4f32(0, 1, 0, 1));
                            msdf_log_create_contour(other_contour, v4f32(1, 0, 0, 1));
                            msdf_log_create_line(test_point, v2f32_add(test_point, v2f32(100.0f, 0.0f)), v4f32(0, 0, 0, 1));
                            msdf_log_create_point(test_point, v4f32(0, 0, 0, 1));
                            msdf_log_create_point(v2f32_add(test_point, v2f32(v, 0.0f)), v4f32(0, 0, 0, 1));
                            msdf_log_create_segment(segment, v4f32(0, 0, 1, 1));
                            msdf_log_create_point(segment->p0, v4f32(1, 0, 0, 1));
                            msdf_log_create_point(segment->p1, v4f32(0, 1, 0, 1));
                            msdf_log_pop_parent();
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
                    if (f32_abs(discriminant) < 0.000001f) {
                        F32 u = -by / (2.0f * ay);

                        F32 v = u * u * ax + u * bx + cx;

                        if (0.0f <= u && u < 1.0f && 0.0f <= v) {
                            MSDF_LogNode *intersection = msdf_log_create_node_from_string_format("Bezier intersection (double root)");
                            msdf_log_push_parent(intersection);
                            msdf_log_create_contour(contour, v4f32(0, 1, 0, 1));
                            msdf_log_create_contour(other_contour, v4f32(1, 0, 0, 1));
                            msdf_log_create_line(test_point, v2f32_add(test_point, v2f32(100.0f, 0.0f)), v4f32(0, 0, 0, 1));
                            msdf_log_create_point(test_point, v4f32(0, 0, 0, 1));
                            msdf_log_create_point(v2f32_add(test_point, v2f32(v, 0.0f)), v4f32(0, 0, 0, 1));
                            msdf_log_create_segment(segment, v4f32(0, 0, 1, 1));
                            msdf_log_pop_parent();
                            ++intersection_count;
                        }
                    } else if (0 < discriminant) {
                        F32 u0 = (-by - f32_sqrt(discriminant)) / (2.0f * ay);
                        F32 u1 = (-by + f32_sqrt(discriminant)) / (2.0f * ay);

                        F32 v0 = u0 * u0 * ax + u0 * bx + cx;
                        F32 v1 = u1 * u1 * ax + u1 * bx + cx;

                        if (0.0f <= u0 && u0 < 1.0f && 0.0f <= v0) {
                            MSDF_LogNode *intersection = msdf_log_create_node_from_string(str8_literal("Bezier intersection (negative root)"));
                            msdf_log_push_parent(intersection);
                            msdf_log_create_contour(contour, v4f32(0, 1, 0, 1));
                            msdf_log_create_contour(other_contour, v4f32(1, 0, 0, 1));
                            msdf_log_create_line(test_point, v2f32_add(test_point, v2f32(100.0f, 0.0f)), v4f32(0, 0, 0, 1));
                            msdf_log_create_point(test_point, v4f32(0, 0, 0, 1));
                            msdf_log_create_point(v2f32_add(test_point, v2f32(v0, 0.0f)), v4f32(0, 0, 0, 1));
                            msdf_log_create_segment(segment, v4f32(0, 0, 1, 1));
                            msdf_log_pop_parent();
                            ++intersection_count;
                        }

                        if (0.0f <= u1 && u1 < 1.0f && 0.0f <= v1) {
                            MSDF_LogNode *intersection = msdf_log_create_node_from_string(str8_literal("Bezier intersection (positive root)"));
                            msdf_log_push_parent(intersection);
                            msdf_log_create_contour(contour, v4f32(0, 1, 0, 1));
                            msdf_log_create_contour(other_contour, v4f32(1, 0, 0, 1));
                            msdf_log_create_line(test_point, v2f32_add(test_point, v2f32(100.0f, 0.0f)), v4f32(0, 0, 0, 1));
                            msdf_log_create_point(test_point, v4f32(0, 0, 0, 1));
                            msdf_log_create_point(v2f32_add(test_point, v2f32(v1, 0.0f)), v4f32(0, 0, 0, 1));
                            msdf_log_create_segment(segment, v4f32(0, 0, 1, 1));
                            msdf_log_pop_parent();
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
        case MSDF_Segment_Null:            a_dir = v2f32(0.0f, 0.0f);                           break;
        case MSDF_Segment_Line:            a_dir = v2f32_normalize(v2f32_subtract(a.p1, a.p0)); break;
        case MSDF_Segment_QuadraticBezier: a_dir = v2f32_normalize(v2f32_subtract(a.p2, a.p1)); break;
        case MSDF_Segment_COUNT:           a_dir = v2f32(0.0f, 0.0f);                           break;
    }
    switch (b.kind) {
        case MSDF_Segment_Null:            b_dir = v2f32(0.0f, 0.0f);                           break;
        case MSDF_Segment_Line:            b_dir = v2f32_normalize(v2f32_subtract(b.p1, b.p0)); break;
        case MSDF_Segment_QuadraticBezier: b_dir = v2f32_normalize(v2f32_subtract(b.p1, b.p0)); break;
        case MSDF_Segment_COUNT:           b_dir = v2f32(0.0f, 0.0f);                           break;
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

    MSDF_Distance result = { 0 };
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
    V2F32 perpendicular = v2f32_scale(min_vector_distance, 1.0f / distance);

    MSDF_Distance result = { 0 };
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

internal F32 msdf_quadratic_bezier_signed_pseudo_distance(V2F32 point, MSDF_Segment bezier, F32 unclamped_t) {
    V2F32 p  = v2f32_subtract(point, bezier.p0);
    V2F32 p1 = v2f32_subtract(bezier.p1, bezier.p0);
    V2F32 p2 = v2f32_add(v2f32_add(bezier.p2, v2f32_scale(bezier.p1, -2)), bezier.p0);

    V2F32 derivative = { 0 };
    V2F32 distance   = { 0 };

    if (unclamped_t < 0.0f) {
        derivative = v2f32_subtract(bezier.p1, bezier.p0);
        F32 t = v2f32_dot(v2f32_subtract(point, bezier.p0), derivative) / v2f32_length_squared(derivative);
        distance = v2f32_subtract(v2f32_add(bezier.p0, v2f32_scale(derivative, t)), point);
    } else if (unclamped_t > 1.0f) {
        derivative = v2f32_subtract(bezier.p2, bezier.p1);
        F32 t = v2f32_dot(v2f32_subtract(point, bezier.p1), derivative) / v2f32_length_squared(derivative);
        distance = v2f32_subtract(v2f32_add(bezier.p1, v2f32_scale(derivative, t)), point);
    } else {
        distance   = v2f32_subtract(v2f32_add(v2f32_add(v2f32_scale(p2, unclamped_t * unclamped_t), v2f32_scale(p1, 2.0f * unclamped_t)), bezier.p0), point);
        derivative = v2f32_add(v2f32_scale(p2, 2.0f * unclamped_t), v2f32_scale(p1, 2.0f));
    }

    F32 sign = f32_sign(v2f32_cross(derivative, distance));
    return sign * v2f32_length(distance);
}

// TODO(simon): We will need to rethink how we handle overlapping contours.
// This approach doesn't handle endpoints on contours correctly in all cases.
internal Void msdf_resolve_contour_overlap(Arena *arena, MSDF_Glyph *glyph) {
    msdf_log_push_parent_string(str8_literal("resolve contour overlap"));
    for (MSDF_Contour *a_contour = glyph->first_contour; a_contour; a_contour = a_contour->next) {
        for (MSDF_Contour *b_contour = glyph->first_contour; b_contour; b_contour = b_contour->next) {
            if (a_contour == b_contour) {
                continue;
            }

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
                    U32 local_intersection_count = msdf_segment_intersect(*a_segment, *b_segment, ats, bts);
                    F32 intersection_epsilon = 0.0001f; // TODO: Move this out and figure out an appropiate value for it.
                    for (U32 i = 0; i < local_intersection_count; ++i) {
                        if (ats[i] < min_at && intersection_epsilon < ats[i] && ats[i] < 1.0f - intersection_epsilon) {
                            min_at = ats[i];
                            min_bt = bts[i];
                            min_b  = b_segment;
                        }
                    }
                }

                if (min_at <= 1.0f) {
                    MSDF_Segment *b_segment = min_b;

                    MSDF_Segment *a_new = arena_push_struct(arena, MSDF_Segment);
                    MSDF_Segment *b_new = arena_push_struct(arena, MSDF_Segment);
                    dll_insert_after(a_contour->first_segment, a_contour->last_segment, a_segment, a_new);
                    dll_insert_after(b_contour->first_segment, b_contour->last_segment, b_segment, b_new);
                    msdf_segment_split(*a_segment, min_at, a_segment, a_new);
                    msdf_segment_split(*b_segment, min_bt, b_segment, b_new);
                    a_intersections[intersection_count] = a_new;
                    b_intersections[intersection_count] = b_new;

                    MSDF_LogNode *intersection = msdf_log_create_node_from_string(str8_literal("Intersection"));
                    msdf_log_push_parent(intersection);
                    msdf_log_create_contour(a_contour, v4f32(1.0f, 0.0f, 0.0f, 1.0f));
                    msdf_log_create_contour(b_contour, v4f32(0.0f, 1.0f, 0.0f, 1.0f));
                    msdf_log_create_point(a_new->p0, v4f32(0.5f, 0.0f, 0.0f, 1.0f));
                    msdf_log_create_point(b_new->p0, v4f32(0.0f, 0.5f, 0.0f, 1.0f));
                    msdf_log_pop_parent();

                    // The corner with the narrowest angle is moved "inwards".
                    // It needs to move at least the distance between the two
                    // corners. This can be 0, so we also add a small amount to
                    // ensure that the corners do not overlapp.
                    // TODO: Why is the "small amount" 0.005f? What should it be?
                    V2F32 *a0_corner    = (a_segment->kind == MSDF_Segment_Line ? &a_segment->p1 : &a_segment->p2);
                    V2F32  a0_direction = v2f32_normalize(v2f32_subtract((a_segment->kind == MSDF_Segment_Line ? a_segment->p0 : a_segment->p1), *a0_corner));
                    V2F32 *a1_corner    = &a_new->p0;
                    V2F32  a1_direction = v2f32_normalize(v2f32_subtract(a_new->p1, *a1_corner));
                    V2F32 *b0_corner    = (b_segment->kind == MSDF_Segment_Line ? &b_segment->p1 : &b_segment->p2);
                    V2F32  b0_direction = v2f32_normalize(v2f32_subtract((b_segment->kind == MSDF_Segment_Line ? b_segment->p0 : b_segment->p1), *b0_corner));
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
                        // NOTE(simon): Link together the segment into a circle
                        b_contour->first_segment->previous = b_contour->last_segment;
                        b_contour->last_segment->next      = b_contour->first_segment;
                        // NOTE(simon): Split the circle so that the section
                        // between b_intersections[0] and b_intersections[1]
                        // becomes the end. This is a half open range, so
                        // b_intersections[1] should be first, and the segement
                        // before it last.
                        b_contour->first_segment           = b_intersections[1];
                        b_contour->last_segment            = b_intersections[1]->previous;
                        b_contour->first_segment->previous = 0;
                        b_contour->last_segment->next      = 0;

                        // NOTE(simon): Swap the middle parts.
                        // NOTE(simon): This is the atomic set of operations that we want to happen
                        MSDF_Segment *b_insert_first = b_intersections[0]->previous;
                        MSDF_Segment *b_swap_start   = b_intersections[0];
                        MSDF_Segment *b_swap_end     = b_contour->last_segment;
                        // NOTE(simon): b_insert_last is just b_contour->last_segment

                        MSDF_Segment *a_insert_first = a_intersections[0]->previous;
                        MSDF_Segment *a_swap_first = a_intersections[0];
                        MSDF_Segment *a_swap_last  = a_intersections[1]->previous;
                        MSDF_Segment *a_insert_last = a_intersections[1];

                        a_insert_first->next = b_swap_start;
                        b_swap_start->previous = a_insert_first;

                        a_insert_last->previous = b_swap_end;
                        b_swap_end->next = a_insert_last;

                        b_insert_first->next = a_swap_first;
                        a_swap_first->previous = b_insert_first;

                        b_contour->last_segment = a_swap_last;
                        a_swap_last->next = 0;

                        MSDF_LogNode *swap_node = msdf_log_create_node_from_string(str8_literal("Swap"));
                        msdf_log_push_parent(swap_node);
                        msdf_log_create_contour(a_contour, v4f32(1, 0, 0, 1));
                        msdf_log_create_contour(b_contour, v4f32(0, 0, 1, 1));
                        msdf_log_pop_parent();

                        intersection_count = 0;
                    }
                }
            }
        }
    }
    msdf_log_pop_parent();
}

internal Void msdf_convert_to_simple_polygons(Arena *arena, MSDF_Glyph *glyph) {
    msdf_log_push_parent_string(str8_literal("resolve self intersection"));
    for (MSDF_Contour *contour = glyph->first_contour; contour; contour = contour->next) {
        for (MSDF_Segment *a_segment = contour->first_segment; a_segment; a_segment = a_segment->next) {
            for (MSDF_Segment *b_segment = a_segment->next; b_segment; b_segment = b_segment->next) {
                F32 ats[4] = { 0 };
                F32 bts[4] = { 0 };
                U32 intersection_count = msdf_segment_intersect(*a_segment, *b_segment, ats, bts);

                F32 min_at = f32_infinity();
                F32 min_bt = f32_infinity();
                for (U32 i = 0; i < intersection_count; ++i) {
                    F32 intersection_epsilon = 0.001f;
                    if (ats[i] < min_at && intersection_epsilon < ats[i] && ats[i] < 1.0f - intersection_epsilon) {
                        min_at = ats[i];
                        min_bt = bts[i];
                    }
                }

                if (min_at <= 1.0f) {
                    MSDF_Contour *new_contour = arena_push_struct(arena, MSDF_Contour);
                    dll_push_back(glyph->first_contour, glyph->last_contour, new_contour);

                    MSDF_LogNode *intersection = msdf_log_create_node_from_string(str8_literal("self intersection"));
                    msdf_log_push_parent(intersection);
                    msdf_log_create_segment(a_segment, v4f32(1, 0, 0, 1));
                    msdf_log_create_segment(b_segment, v4f32(0, 1, 0, 1));
                    msdf_log_pop_parent();

                    MSDF_Segment *a_new = arena_push_struct(arena, MSDF_Segment);
                    msdf_segment_split(*a_segment, min_at, a_segment, a_new);
                    dll_insert_after(contour->first_segment, contour->last_segment, a_segment, a_new);
                    msdf_log_create_point(a_new->p0, v4f32(0, 0, 0, 1));

                    MSDF_Segment *b_new = arena_push_struct(arena, MSDF_Segment);
                    msdf_segment_split(*b_segment, min_bt, b_new, b_segment);
                    dll_insert_before(contour->first_segment, contour->last_segment, b_segment, b_new);
                    msdf_log_create_point(b_new->p0, v4f32(0, 0, 0, 1));

                    new_contour->first_segment = a_new;
                    new_contour->last_segment  = b_new;
                    a_new->previous = 0;
                    b_new->next     = 0;

                    a_segment->next     = b_segment;
                    b_segment->previous = a_segment;

                    // Ensure that the contour is connected properly.
                    V2F32 *a0_corner = &a_new->p0;
                    V2F32 *a1_corner = (b_new->kind == MSDF_Segment_Line ? &b_new->p1 : &b_new->p2);
                    V2F32 a_corner = v2f32_scale(v2f32_add(*a0_corner, *a1_corner), 0.5f);
                    *a0_corner = a_corner;
                    *a1_corner = a_corner;

                    V2F32 *b0_corner = &b_segment->p0;
                    V2F32 *b1_corner = (a_segment->kind == MSDF_Segment_Line ? &a_segment->p1 : &a_segment->p2);
                    V2F32 b_corner = v2f32_scale(v2f32_add(*b0_corner, *b1_corner), 0.5f);
                    *b0_corner = b_corner;
                    *b1_corner = b_corner;
                }
            }
        }
    }
    msdf_log_pop_parent();
}

internal Void msdf_correct_contour_orientation(Arena *arena, MSDF_Glyph *glyph) {
    msdf_log_push_parent_string(str8_literal("correct contour orientation"));
    msdf_log_parent_string(str8_literal("local windig")) {
        for (MSDF_Contour *contour = glyph->first_contour; contour; contour = contour->next) {
            contour->local_winding = msdf_contour_calculate_own_winding_number(contour);
            V4F32 color = contour->local_winding == 1 ? v4f32(0, 1, 0, 1) : v4f32(1, 0, 0, 1);
            msdf_log_create_contour(contour, color);
        }
    }

    for (MSDF_Contour *contour = glyph->first_contour; contour; contour = contour->next) {
        msdf_log_push_parent_string(str8_literal("contour correction"));
        S32 global_winding = msdf_contour_calculate_global_winding_number(arena, glyph, contour);

        // NOTE(simon): Determine if each contour should be kept and if we need to flip it.
        if (-1 <= global_winding && global_winding <= 1) {
            contour->flags |= MSDF_ContourFlag_Keep;
        }

        if ((global_winding == 0 && contour->local_winding == 1) || (global_winding != 0 && contour->local_winding == -1)) {
            contour->flags |= MSDF_ContourFlag_Flip;
        }

        V4F32 color = v4f32(1, 0, 0, 1);
        if ((contour->flags & (MSDF_ContourFlag_Flip | MSDF_ContourFlag_Keep)) == (MSDF_ContourFlag_Flip | MSDF_ContourFlag_Keep)) {
            color = v4f32(1, 1, 0, 1);
        } else if (contour->flags & MSDF_ContourFlag_Keep) {
            color = v4f32(0, 1, 0, 1);
        }
        msdf_log_create_contour(contour, color);

        msdf_log_pop_parent();
    }

    msdf_log_parent_string(str8_literal("orientation correction")) {
        for (MSDF_Contour *contour = glyph->first_contour; contour; contour = contour->next) {
            V4F32 color = v4f32(1, 0, 0, 1);
            if ((contour->flags & (MSDF_ContourFlag_Flip | MSDF_ContourFlag_Keep)) == (MSDF_ContourFlag_Flip | MSDF_ContourFlag_Keep)) {
                color = v4f32(1, 1, 0, 1);
            } else if (contour->flags & MSDF_ContourFlag_Keep) {
                color = v4f32(0, 1, 0, 1);
            }
            msdf_log_create_contour(contour, color);
        }
    }

    // Rebuild the list of contours while applying the accumulated change set.
    MSDF_Contour *first = 0;
    MSDF_Contour *last  = 0;
    msdf_log_parent_string(str8_literal("final winding")) {
        for (MSDF_Contour *contour = glyph->first_contour, *next; contour; contour = next) {
            next = contour->next;

            if (contour->flags & MSDF_ContourFlag_Flip) {
                // Flip all links and points for all segments.
                for (MSDF_Segment *segment = contour->first_segment, *next_segment = 0; segment; segment = next_segment) {
                    next_segment = segment->next;

                    swap(segment->next, segment->previous, MSDF_Segment *);
                    if (segment->kind == MSDF_Segment_Line) {
                        swap(segment->p0, segment->p1, V2F32);
                    } else if (segment->kind == MSDF_Segment_QuadraticBezier) {
                        swap(segment->p0, segment->p2, V2F32);
                    }
                }

                swap(contour->first_segment, contour->last_segment, MSDF_Segment *);
            }

            if (contour->flags & MSDF_ContourFlag_Keep) {
                dll_push_back(first, last, contour);
                V4F32 color = ((contour->local_winding == 1) ^ ((contour->flags & MSDF_ContourFlag_Flip) == MSDF_ContourFlag_Flip)) ? v4f32(0, 1, 0, 1) : v4f32(1, 0, 0, 1);
                msdf_log_create_contour(contour, color);
            }
        }
    }

    glyph->first_contour = first;
    glyph->last_contour  = last;
    msdf_log_pop_parent();
}

internal Void msdf_color_edges(MSDF_Glyph glyph) {
    msdf_log_push_parent_string(str8_literal("color edges"));
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
                current->flags  |= MSDF_SegmentFlag_Start;
                previous->flags |= MSDF_SegmentFlag_End;
            }
        }

        if (corner_count == 0) {
            msdf_log_parent_string(str8_literal("no corners")) {
                msdf_log_create_contour(contour, v4f32(1, 0, 0, 1));
                for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
                    segment->flags = MSDF_SegmentFlag_Red | MSDF_SegmentFlag_Green | MSDF_SegmentFlag_Blue;
                }
            }
        } else if (corner_count == 1) {
            msdf_log_parent_string(str8_literal("one corners")) {
                msdf_log_create_contour(contour, v4f32(1, 0, 0, 1));
                for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
                    if (segment->flags & MSDF_SegmentFlag_Start) {
                        msdf_log_create_point(segment->p0, v4f32(0, 1, 0, 1));
                    }

                    segment->flags = MSDF_SegmentFlag_Red | MSDF_SegmentFlag_Blue;
                }

                // TODO(simon): More carefully handle how we split edges in this case.
                // NOTE(simon): We need to split the contour into two edges in order to preserve the corner.
                last_corner_start->flags = MSDF_SegmentFlag_Red | MSDF_SegmentFlag_Green;
            }
        } else {
            msdf_log_parent_string(str8_literal("multiple corners")) {
                msdf_log_create_contour(contour, v4f32(1, 0, 0, 1));
                MSDF_SegmentFlags current_color = MSDF_SegmentFlag_Red | MSDF_SegmentFlag_Blue;

                for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
                    segment->flags |= current_color;

                    if (segment->flags & MSDF_SegmentFlag_Start) {
                        msdf_log_create_point(segment->p0, v4f32(0, 1, 0, 1));
                    }

                    if (segment->flags & MSDF_SegmentFlag_End) {
                        if (current_color == (MSDF_SegmentFlag_Red | MSDF_SegmentFlag_Green)) {
                            current_color = MSDF_SegmentFlag_Green | MSDF_SegmentFlag_Blue;
                        } else {
                            current_color = MSDF_SegmentFlag_Red | MSDF_SegmentFlag_Green;
                        }
                    }
                }

                // NOTE(simon): The first edge might cross the start and end of the
                // list and would now have two different colors, correct it!
                if (!(contour->first_segment->flags & MSDF_SegmentFlag_Start)) {
                    for (
                        MSDF_Segment *segment = contour->last_segment;
                        segment && !(segment->flags & MSDF_SegmentFlag_End);
                        segment = segment->previous
                    ) {
                        segment->flags = MSDF_SegmentFlag_Red | MSDF_SegmentFlag_Blue;
                    }
                }
            }
        }
    }

    msdf_log_parent_string(str8_literal("final coloring")) {
        for (MSDF_Contour *contour = glyph.first_contour; contour; contour = contour->next) {
            MSDF_LogNode *contour_group = msdf_log_push_parent_string(str8_literal("Contour"));
            for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
                V4F32 color = v4f32(
                    (F32) (segment->flags & MSDF_SegmentFlag_Red),
                    (F32) (segment->flags & MSDF_SegmentFlag_Green),
                    (F32) (segment->flags & MSDF_SegmentFlag_Blue),
                    1
                );
                msdf_log_create_segment(segment, color);
            }
            msdf_log_pop_parent();
        }
    }
    msdf_log_pop_parent();
}

internal MSDF_RasterResult msdf_generate_from_glyph_index(Arena *arena, TTF_Font *font, U32 glyph_index, U32 render_size) {
    prof_function_begin();
    MSDF_RasterResult result = { 0 };
    result.glyph_index = glyph_index;

    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    // NOTE(simon): Setup logging state.
    memory_zero_struct(&msdf_log_state);
    msdf_log_state.arena         = arena;
    msdf_log_state.scratch_arena = scratch.arena;
    result.logs = msdf_log_push_parent_string(str8_literal("Logs"));

    MSDF_Glyph glyph = ttf_expand_contours_to_msdf(scratch.arena, font, glyph_index);
    TTF_HmtxMetrics metrics = ttf_get_metrics(font, glyph_index);

    result.min = v2f32_scale(glyph.min, 1.0f / (F32) font->funits_per_em);
    result.max = v2f32_scale(glyph.max, 1.0f / (F32) font->funits_per_em);
    result.advance_width     = (F32) metrics.advance_width / (F32) font->funits_per_em;
    result.left_side_bearing = (F32) metrics.left_side_bearing / (F32) font->funits_per_em;

    MSDF_LogNode *whole_glyph = msdf_log_create_glyph(&glyph, v4f32(1, 0, 0, 1));
    msdf_log_node_set_string(whole_glyph, str8_literal("Whole glyph"));

    // NOTE(simon): Simplify outline
    {
        prof_zone_begin(prof_simplify, "Simplify outline");
        msdf_resolve_contour_overlap(scratch.arena, &glyph);
        msdf_convert_to_simple_polygons(scratch.arena, &glyph);
        msdf_correct_contour_orientation(arena, &glyph);
        msdf_color_edges(glyph);
        prof_zone_end(prof_simplify);
    }

    // NOTE(simon): We no longer need the segments to be organized in curves or
    // have any order amongst themselves. Separate them by kind to ease
    // processing.
    MSDF_SegmentList lines        = { 0 };
    MSDF_SegmentList quad_beziers = { 0 };
    for (MSDF_Contour *contour = glyph.first_contour; contour; contour = contour->next) {
        for (MSDF_Segment *segment = contour->first_segment, *next; segment; segment = next) {
            next = segment->next;

            switch (segment->kind) {
                case MSDF_Segment_Null: {
                } break;
                case MSDF_Segment_Line: {
                    dll_push_back(lines.first, lines.last, segment);
                } break;
                case MSDF_Segment_QuadraticBezier: {
                    dll_push_back(quad_beziers.first, quad_beziers.last, segment);
                } break;
                case MSDF_Segment_COUNT: {
                } break;
            }
        }
    }

    // Scale the contours to the range [0--1] and generate bounding circles
    // for the them.
    U32 padding = 1;

    F32 x_scale = (F32) (render_size - 2 * padding) / (F32) (glyph.max.x - glyph.min.x);
    F32 y_scale = (F32) (render_size - 2 * padding) / (F32) (glyph.max.y - glyph.min.y);

    for (MSDF_Segment *line = lines.first; line; line = line->next) {
        line->p0 = v2f32(
            ((line->p0.x  - (F32) glyph.min.x) * x_scale + (F32) padding) / (F32) render_size,
            (((F32) glyph.max.y - line->p0.y)  * y_scale + (F32) padding) / (F32) render_size
        );
        line->p1 = v2f32(
            ((line->p1.x  - (F32) glyph.min.x) * x_scale + (F32) padding) / (F32) render_size,
            (((F32) glyph.max.y - line->p1.y)  * y_scale + (F32) padding) / (F32) render_size
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
            ((bezier->p0.x - (F32) glyph.min.x)  * x_scale + (F32) padding) / (F32) render_size,
            (((F32) glyph.max.y  - bezier->p0.y) * y_scale + (F32) padding) / (F32) render_size
        );
        bezier->p1 = v2f32(
            ((bezier->p1.x - (F32) glyph.min.x)  * x_scale + (F32) padding) / (F32) render_size,
            (((F32) glyph.max.y  - bezier->p1.y) * y_scale + (F32) padding) / (F32) render_size
        );
        bezier->p2 = v2f32(
            ((bezier->p2.x - (F32) glyph.min.x)  * x_scale + (F32) padding) / (F32) render_size,
            (((F32) glyph.max.y  - bezier->p2.y) * y_scale + (F32) padding) / (F32) render_size
        );

        V2F32 min = v2f32_min(v2f32_min(bezier->p0, bezier->p1), bezier->p2);
        V2F32 max = v2f32_max(v2f32_max(bezier->p0, bezier->p1), bezier->p2);
        V2F32 center = v2f32_scale(v2f32_add(max, min), 0.5f);
        F32 radius = 0.5f * v2f32_length(v2f32_subtract(max, min));
        bezier->circle_center = center;
        bezier->circle_radius = radius;
    }

    // NOTE(simon): Generate
    {
        prof_zone_begin(prof_generate, "generate");
        F32 distance_range = 2.0f / (F32) render_size;
        U32 pixel_index = 0;
        result.data = arena_push_array_no_zero(arena, U8, 4 * render_size * render_size);
        for (U32 y = 0; y < render_size; ++y) {
            for (U32 x = 0; x < render_size; ++x) {
                MSDF_Segment nil_segment     = { 0 };
                MSDF_Distance red_distance   = { .distance = f32_infinity(), .orthogonality = 0.0f };
                MSDF_Segment *red_segment    = &nil_segment;
                MSDF_Distance green_distance = { .distance = f32_infinity(), .orthogonality = 0.0f };
                MSDF_Segment *green_segment  = &nil_segment;
                MSDF_Distance blue_distance  = { .distance = f32_infinity(), .orthogonality = 0.0f };
                MSDF_Segment *blue_segment   = &nil_segment;

                V2F32 point = v2f32(((F32) x + 0.5f) / (F32) render_size, ((F32) y + 0.5f) / (F32) render_size);
                for (MSDF_Segment *line = lines.first; line; line = line->next) {
                    F32 min_distance = v2f32_length_squared(v2f32_subtract(line->circle_center, point));

                    F32 red   = red_distance.distance   + line->circle_radius;
                    F32 green = green_distance.distance + line->circle_radius;
                    F32 blue  = blue_distance.distance  + line->circle_radius;
                    if (red * red >= min_distance || green * green >= min_distance || blue * blue >= min_distance) {
                        MSDF_Distance distance = msdf_line_distance_orthogonality(point, *line);

                        if ((line->flags & MSDF_SegmentFlag_Red) && msdf_distance_is_closer(distance, red_distance)) {
                            red_distance = distance;
                            red_segment  = line;
                        }
                        if ((line->flags & MSDF_SegmentFlag_Green) && msdf_distance_is_closer(distance, green_distance)) {
                            green_distance = distance;
                            green_segment  = line;
                        }
                        if ((line->flags & MSDF_SegmentFlag_Blue) && msdf_distance_is_closer(distance, blue_distance)) {
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

                        if ((bezier->flags & MSDF_SegmentFlag_Red) && msdf_distance_is_closer(distance, red_distance)) {
                            red_distance = distance;
                            red_segment  = bezier;
                        }
                        if ((bezier->flags & MSDF_SegmentFlag_Green) && msdf_distance_is_closer(distance, green_distance)) {
                            green_distance = distance;
                            green_segment  = bezier;
                        }
                        if ((bezier->flags & MSDF_SegmentFlag_Blue) && msdf_distance_is_closer(distance, blue_distance)) {
                            blue_distance = distance;
                            blue_segment  = bezier;
                        }
                    }
                }

                if (red_segment->kind == MSDF_Segment_Line) {
                    red_distance.distance = msdf_line_signed_pseudo_distance(point, *red_segment);
                } else if (red_segment->kind == MSDF_Segment_QuadraticBezier) {
                    red_distance.distance = msdf_quadratic_bezier_signed_pseudo_distance(point, *red_segment, red_distance.unclamped_t);
                }
                if (green_segment->kind == MSDF_Segment_Line) {
                    green_distance.distance = msdf_line_signed_pseudo_distance(point, *green_segment);
                } else if (green_segment->kind == MSDF_Segment_QuadraticBezier) {
                    green_distance.distance = msdf_quadratic_bezier_signed_pseudo_distance(point, *green_segment, green_distance.unclamped_t);
                }
                if (blue_segment->kind == MSDF_Segment_Line) {
                    blue_distance.distance = msdf_line_signed_pseudo_distance(point, *blue_segment);
                } else if (blue_segment->kind == MSDF_Segment_QuadraticBezier) {
                    blue_distance.distance = msdf_quadratic_bezier_signed_pseudo_distance(point, *blue_segment, blue_distance.unclamped_t);
                }

                U8 red   = (U8) s32_min(s32_max(0, f32_round_to_s32((red_distance.distance   / distance_range + 0.5f) * 255.0f)), 255);
                U8 green = (U8) s32_min(s32_max(0, f32_round_to_s32((green_distance.distance / distance_range + 0.5f) * 255.0f)), 255);
                U8 blue  = (U8) s32_min(s32_max(0, f32_round_to_s32((blue_distance.distance  / distance_range + 0.5f) * 255.0f)), 255);

                result.data[pixel_index++] = red;
                result.data[pixel_index++] = green;
                result.data[pixel_index++] = blue;
                result.data[pixel_index++] = 0;
            }
        }
        prof_zone_end(prof_generate);
    }

    msdf_log_pop_parent();
    arena_end_temporary(scratch);

    prof_function_end();
    return result;
}

internal MSDF_RasterResult msdf_generate_from_codepoint(Arena *arena, TTF_Font *font, U32 codepoint, U32 render_size) {
    U32 glyph_index = ttf_glyph_index_from_font_codepoint(font, codepoint);
    MSDF_RasterResult result = msdf_generate_from_glyph_index(arena, font, glyph_index, render_size);
    return result;
}
