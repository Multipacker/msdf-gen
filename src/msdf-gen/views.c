TabKind tab_kind_from_string(Str8 string) {
    TabKind result = Tab_Null;

    for (TabKind kind = 0; kind < Tab_COUNT; ++kind) {
        if (str8_equal(string, tab_specifications[kind].name)) {
            result = kind;
            break;
        }
    }

    return result;
}

TabSpecification *tab_specification_from_string(Str8 string) {
    TabKind tab_kind = tab_kind_from_string(string);
    TabSpecification *result = &tab_specifications[tab_kind];
    return result;
}

internal UI_BOX_DRAW_FUNCTION(draw_ui_msdf) {
    U32 *codepoint = (U32 *) data;
    MSDFCache_Glyph *glyph = msdf_cache_get_glyph(global_state->ttf_font, *codepoint);

    V2F32 box_size = v2f32_subtract(box->calculated_rectangle.max, box->calculated_rectangle.min);
    V2F32 glyph_size = r2f32_size(glyph->rectangle_pt);
    F32 scale_to_fit = 0.8f * f32_min(box_size.x / glyph_size.x, box_size.y / glyph_size.y);

    M3F32 center_glyph = m3f32_translation(v2f32_negate(r2f32_center(glyph->rectangle_pt)));
    M3F32 scale        = m3f32_scale(v2f32(scale_to_fit, scale_to_fit));
    M3F32 center_box   = m3f32_translation(r2f32_center(box->calculated_rectangle));

    M3F32 transform = m3f32_multiply_m3f32(center_box, m3f32_multiply_m3f32(scale, center_glyph));

    // NOTE(simon): We do the transform on the CPU in order to avoid generating
    // one batch per draw operation. The box isn't rotated so this is fine.
    V2F32 min_pt = m3f32_multiply_v2f32(transform, glyph->rectangle_pt.min);
    V2F32 max_pt = m3f32_multiply_v2f32(transform, glyph->rectangle_pt.max);

    draw_texture(
        r2f32(min_pt.x, min_pt.y, max_pt.x, max_pt.y),
        glyph->uv,
        glyph->texture,
        box->palette.text,
        0.0f, 0.0f, 0.0f,
        Render_ShapeFlag_MSDF
    );
}

internal S64 index_from_map_codepoint(TTF_CodepointMap map, U32 codepoint) {
    S64 result = 0;

    for (U32 range_index = 0, codepoint_index = 0; range_index < map.range_count; ++range_index) {
        TTF_CodepointRange range = map.ranges[range_index];

        if (range.first_codepoint <= codepoint && codepoint < range.first_codepoint + range.size) {
            result = codepoint_index + (codepoint - range.first_codepoint);
            break;
        }

        codepoint_index += range.size;
    }

    return result;
}

internal U32 codepoint_from_map_index(TTF_CodepointMap map, S64 index) {
    U32 result = 0;

    for (U32 range_index = 0; range_index < map.range_count; ++range_index) {
        TTF_CodepointRange range = map.ranges[range_index];

        if (index < range.size) {
            result = range.first_codepoint + (U32) index;
            break;
        }

        index -= range.size;
    }

    return result;
}

// TODO(simon): Implement better handling of resizing.
PANEL_BUILD_FUNCTION(view_glyph_list) {
    prof_function_begin();

    typedef struct {
        UI_ScrollPosition position;
        U32 previous_codepoint;
    } ViewState;

    ViewState *state = tab_get_state(tab, sizeof(ViewState));

    // NOTE(simon): Get codepoint ranges.
    TTF_CodepointMap codepoint_map = { 0 };
    if (global_state->all_of_unicode) {
        TTF_CodepointRange *codepoint_range = arena_push_struct(ui_frame_arena(), TTF_CodepointRange);
        codepoint_range->size = 0x110000;

        codepoint_map.ranges = codepoint_range;
        codepoint_map.range_count = 1;
        codepoint_map.codepoint_count = codepoint_range->size;
    } else {
        codepoint_map = global_state->ttf_font->codepoint_map;
    }

    // NOTE(simon): Build
    V2F32 panel_size     = r2f32_size(panel_rectangle);
    F32 scrollbar_width  = (F32) ui_font_size_top();
    F32 container_width  = panel_size.x - scrollbar_width;
    F32 container_height = panel_size.y;

    // NOTE(simon): Compute codepoint size.
    // TODO(simon): At the moment we use a dumb 1:2 aspect ratio, maybe there
    // is something smarter to do here.
    F32 preferred_width    = 3.0f * (F32) ui_font_size_top();
    S64 codepoints_per_row = s64_max(1, (S64) f32_floor(container_width / preferred_width));
    F32 width  = container_width / (F32) codepoints_per_row;
    F32 height = width * 2.0f;

    // NOTE(simon): Properties of the data begin viewed.
    S64 first_row    = 0;
    S64 last_row     = ((S64) codepoint_map.codepoint_count + codepoints_per_row - 1) / codepoints_per_row;
    S64 visible_rows = (S64) f32_ceil(container_height / height);

    // NOTE(simon): Properties of the current view.
    S64 top_row    = state->position.index + (S64) f32_floor(state->position.offset);
    S64 bottom_row = s64_min(top_row + (state->position.offset != 0.0f) + visible_rows, last_row);

    // NOTE(simon): Scroll region
    ui_width_next(ui_size_pixels(panel_size.x, 1.0f));
    ui_height_next(ui_size_pixels(panel_size.y, 1.0f));
    ui_layout_axis_next(Axis2_X);
    UI_Box *region = ui_create_box_from_string(UI_BoxFlag_OverflowY | UI_BoxFlag_Scrollable, str8_literal("##region"));
    ui_parent(region) {
        // NOTE(simon): Scroll container
        ui_width_next(ui_size_pixels(container_width, 1.0f));
        ui_height_next(ui_size_pixels(container_height, 1.0f));
        ui_layout_axis_next(Axis2_Y);
        UI_Box *container = ui_create_box_from_string(0, str8_literal("##glyphs"));
        container->view_offset.y = height * (f32_mod(state->position.offset, 1.0f) + (state->position.offset < 0.0f));

        ui_palette(palette_from_code(PaletteCode_Button))
        ui_focus(UI_Focus_Active)
        ui_parent(container) {
            for (S64 row = top_row, index = top_row * codepoints_per_row; row < bottom_row; ++row) {
                ui_width_next(ui_size_pixels(container_width, 1.0f));
                ui_height(ui_size_pixels(height, 1.0f))
                ui_row() {
                    ui_width(ui_size_pixels(width, 1.0f))
                    ui_draw_function(draw_ui_msdf)
                    ui_hover_cursor(Gfx_Cursor_Hand)
                    for (S64 column = 0; column < codepoints_per_row && index < codepoint_map.codepoint_count; ++column, ++index) {
                        U32 *codepoint = arena_push_struct_no_zero(ui_frame_arena(), U32);
                        *codepoint = codepoint_from_map_index(codepoint_map, index);

                        ui_draw_data_next(codepoint);
                        ui_focus_hot_next(*codepoint == top_context()->codepoint ? UI_Focus_Active : UI_Focus_Inactive);
                        ui_focus_active_next(*codepoint == top_context()->codepoint ? UI_Focus_Active : UI_Focus_Inactive);

                        UI_Box *box = ui_create_box_from_string_format(
                            UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
                            UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable,
                            "##codepoint_%u", *codepoint
                        );

                        UI_Input input = ui_input_from_box(box);
                        if (input.flags & UI_InputFlag_Clicked) {
                            push_command(Command_SelectCodepoint, .codepoint = *codepoint);
                        }
                    }
                }
            }

            if (ui_is_focus_active()) {
                S64 index = index_from_map_codepoint(codepoint_map, top_context()->codepoint);

                for (UI_Event *event = 0; ui_next_event(&event);) {
                    if (event->kind != UI_EventKind_Navigation) {
                        continue;
                    }

                    S64 codepoint_delta = 0;
                    switch (event->unit) {
                        case UI_EventDeltaUnit_Null: {
                        } break;
                        case UI_EventDeltaUnit_Character: {
                            codepoint_delta += event->delta.x;
                            codepoint_delta += event->delta.y * codepoints_per_row;
                        } break;
                        case UI_EventDeltaUnit_Word: {
                        } break;
                        case UI_EventDeltaUnit_Line: {
                            if (event->delta.x == -1) {
                                codepoint_delta += -index % codepoints_per_row;
                            } else if (event->delta.x == 1) {
                                codepoint_delta += codepoints_per_row - 1 - index % codepoints_per_row;
                            }
                        } break;
                        case UI_EventDeltaUnit_Page: {
                            S64 codepoints_per_page = visible_rows * codepoints_per_row;
                            codepoint_delta += event->delta.y * codepoints_per_page;
                        } break;
                        case UI_EventDeltaUnit_Whole: {
                            if (event->delta.x == -1) {
                                codepoint_delta += -index;
                            } else if (event->delta.x == 1) {
                                codepoint_delta += (S64) codepoint_map.codepoint_count - 1 - index;
                            }
                        } break;
                        case UI_EventDeltaUnit_COUNT: {
                        } break;
                    }

                    index = s64_min(s64_max(0, index + codepoint_delta), (S64) codepoint_map.codepoint_count - 1);
                    ui_consume_event(event);
                }

                U32 new_codepoint = codepoint_from_map_index(codepoint_map, index);

                if (new_codepoint != top_context()->codepoint) {
                    push_command(Command_SelectCodepoint, .codepoint = new_codepoint);
                }
            }
        }

        ui_palette(palette_from_code(PaletteCode_Button))
        ui_width(ui_size_pixels(scrollbar_width, 1.0f))
        ui_height(ui_size_pixels(panel_size.y, 1.0f)) {
            state->position = ui_scroll_bar(state->position, first_row, last_row, visible_rows);
        }
    }

    // NOTE(simon): Scrolling.
    UI_Input region_input = ui_input_from_box(region);
    S64 scroll_delta = (S64) f32_round(region_input.scroll.y);
    state->position.index  -= scroll_delta;
    state->position.offset += (F32) scroll_delta;

    // NOTE(simon): Recenter if the new codepoint is out of view.
    if (top_context()->codepoint != state->previous_codepoint) {
        state->previous_codepoint = top_context()->codepoint;

        S64 active_index = index_from_map_codepoint(codepoint_map, top_context()->codepoint);
        S64 active_row = active_index / codepoints_per_row;
        if (!(top_row <= active_row && active_row < bottom_row)) {
            S64 target_row = active_row - visible_rows / 2;
            S64 delta = target_row - state->position.index;
            state->position.index  += delta;
            state->position.offset -= (F32) delta;
        }
    }

    // NOTE(simon): Clamp scrolling.
    if (state->position.index < 0) {
        state->position.offset += (F32) state->position.index;
        state->position.index = 0;
    } else if (last_row <= state->position.index) {
        state->position.offset -= (F32) (s64_max(0, last_row - 1) - state->position.index);
        state->position.index = s64_max(0, last_row - 1);
    }

    // NOTE(simon): Animation
    state->position.offset += -state->position.offset * ui_animation_slow_rate();
    if (f32_abs(state->position.offset) < 0.001f) {
        state->position.offset = 0.0f;
    } else {
        request_frame();
    }
    prof_function_end();
}

PANEL_BUILD_FUNCTION(view_glyph) {
    prof_function_begin();
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    typedef struct ViewState ViewState;
    struct ViewState {
        B32 render_outline;
        B32 render_points;
        B32 render_raw;
        B32 render_nearest;
        B32 render_logs;
        F32 target_zoom;
        F32 zoom;
        V2F32 offset;
        U32 codepoint;
        U64 log_index;
        B32 *is_group_visible;
        U64 hovered_group;
        F32 hovered_t;
        F32 hovered_target_t;
    };

    ViewState *state = (ViewState *) tab_get_state(tab, sizeof(ViewState));
    if (state->zoom == 0.0f) {
        state->zoom = 1.0f;
        state->target_zoom = 1.0f;
    }

    // NOTE(simon): Reset panning information when a new codepoint is selected.
    if (global_state->selected_codepoint != state->codepoint) {
        state->zoom = 1.0f;
        state->target_zoom = 1.0f;
        state->offset = v2f32(0.0f, 0.0f);
        state->log_index = 0;
        state->is_group_visible = 0;
        state->codepoint = global_state->selected_codepoint;
    }

    ui_width(ui_size_parent_percent(1.0f, 1.0f))
    ui_height(ui_size_parent_percent(1.0f, 1.0f))
    ui_column() {
        ui_palette(palette_from_code(PaletteCode_Button)) {
            ui_width_next(ui_size_parent_percent(1.0f, 0.0f));
            ui_height_next(ui_size_parent_percent(1.0f, 0.0f));
            UI_Box *box = ui_create_box_from_string(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clip | UI_BoxFlag_Scrollable | UI_BoxFlag_Clickable, str8_literal("glyph_viewer"));
            UI_Input input = ui_input_from_box(box);

            if (input.flags & UI_InputFlag_LeftDragging) {
                typedef struct PanState PanState;
                struct PanState {
                    V2F32 mouse;
                    V2F32 offset;
                };
                if (input.flags & UI_InputFlag_LeftPressed) {
                    PanState pan_state = { 0 };
                    pan_state.mouse = ui_mouse();
                    pan_state.offset = state->offset;
                    ui_set_drag_data(&pan_state);
                }

                // TODO(simon): Make this zoom aware so that you can both pan
                // and zoom at the same time and both of them track correctly.
                PanState pan_state = *ui_get_drag_data(PanState);
                state->offset = v2f32_add(pan_state.offset, v2f32_subtract(ui_mouse(), pan_state.mouse));
            }

            // NOTE(simon): Zoom in/out around the mouse
            {
                F32 old_zoom = state->zoom;

                // NOTE(simon): Animate
                state->target_zoom *= f32_pow(0.8f, input.scroll.y);
                state->zoom += (state->target_zoom - state->zoom) * ui_animation_slow_rate();
                if (f32_abs(1.0f - state->zoom / state->target_zoom) < 0.001f) {
                    state->zoom = state->target_zoom;
                } else {
                    request_frame();
                }

                V2F32 relative_mouse = v2f32_subtract(ui_mouse(), r2f32_center(box->calculated_rectangle));
                state->offset = v2f32_subtract(relative_mouse, v2f32_scale(v2f32_subtract(relative_mouse, state->offset), old_zoom / state->zoom));
            }

            MSDFCache_Glyph *msdf_glyph = msdf_cache_get_glyph(global_state->ttf_font, global_state->selected_codepoint);
            MSDF_LogEntry *log_entry = msdf_glyph->log.first;
            for (U64 i = 0; i < state->log_index; ++i) {
                log_entry = log_entry->next;
            }
            // NOTE(simon): Group visibility state
            if (log_entry) {
                B32 *is_group_visible = arena_push_array_no_zero(frame_arena(), B32, log_entry->group_count);
                if (!state->is_group_visible) {
                    for (U64 i = 0; i < log_entry->group_count; ++i) {
                        is_group_visible[i] = true;
                    }
                } else {
                    memory_copy(is_group_visible, state->is_group_visible, log_entry->group_count * sizeof(*is_group_visible));
                }
                state->is_group_visible = is_group_visible;
            }

            Draw_List *draw_list = draw_list_create();
            draw_list_scope(draw_list) {
                F32 padding = 2.0f * (F32) ui_font_size_top();
                F32 point_size = 0.4f * (F32) ui_font_size_top();

                U32 glyph_index = ttf_glyph_index_from_font_codepoint(global_state->ttf_font, global_state->selected_codepoint);
                MSDF_Glyph glyph = ttf_expand_contours_to_msdf(scratch.arena, global_state->ttf_font, glyph_index);

                V2F32 box_size = r2f32_size(box->calculated_rectangle);
                R2F32 glyph_rectangle = r2f32((F32) glyph.min.x, (F32) glyph.min.y, (F32) glyph.max.x, (F32) glyph.max.y);
                V2F32 glyph_size = r2f32_size(glyph_rectangle);

                M3F32 center_glyph = m3f32_translation(v2f32_negate(r2f32_center(glyph_rectangle)));
                F32 scale = f32_max(0.0f, f32_min((box_size.x - 2.0f * padding) / glyph_size.x, (box_size.y - 2.0f * padding) / glyph_size.y)) / state->zoom;
                M3F32 scale_to_box = m3f32_scale(v2f32(scale, -scale));
                M3F32 center_box = m3f32_translation(v2f32_add(v2f32_scale(r2f32_size(box->calculated_rectangle), 0.5f), state->offset));

                M3F32 transform = m3f32_multiply_m3f32(center_box, m3f32_multiply_m3f32(scale_to_box, center_glyph));

                draw_transform(transform) {
                    V4F32 tint = box->palette.text;
                    Render_ShapeFlags flags = Render_ShapeFlag_MSDF;
                    Render_Filtering filtering = state->render_nearest ? Render_Filtering_Nearest : Render_Filtering_Linear;
                    if (state->render_raw) {
                        tint = v4f32(1.0f, 1.0f, 1.0f, 1.0f);
                        flags = Render_ShapeFlag_Texture;
                    }

                    V2F32 uv_size = r2f32_size(msdf_glyph->uv);

                    draw_filtering(filtering) {
                        draw_texture(
                            r2f32(
                                glyph_rectangle.min.x - glyph_size.width  * 0.5f / uv_size.width,
                                glyph_rectangle.min.y - glyph_size.height * 0.5f / uv_size.height,
                                glyph_rectangle.max.x + glyph_size.width  * 0.5f / uv_size.width,
                                glyph_rectangle.max.y + glyph_size.height * 0.5f / uv_size.height
                            ),
                            // NOTE(simon): Need flip vertically because outlines use the
                            // same coordinates system as TTF-files, which is flipped
                            // vertically.
                            r2f32(
                                msdf_glyph->uv.min.x,
                                msdf_glyph->uv.max.y,
                                msdf_glyph->uv.max.x,
                                msdf_glyph->uv.min.y
                            ),
                            msdf_glyph->texture,
                            tint,
                            0.0f, 0.0f, 0.0f,
                            flags
                        );
                    }

                    if (state->render_outline) {
                        for (MSDF_Contour *contour = glyph.first_contour; contour; contour = contour->next) {
                            for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
                                switch (segment->kind) {
                                    case MSDF_Segment_Null: {
                                    } break;
                                    case MSDF_Segment_Line: {
                                        draw_line(segment->p0, segment->p1, color_from_theme(ThemeColor_Outline), 2.0f / scale, 0.0f, 1.0f / scale);
                                    } break;
                                    case MSDF_Segment_QuadraticBezier: {
                                        draw_bezier(segment->p0, segment->p1, segment->p2, color_from_theme(ThemeColor_Outline), 2.0f / scale, 0.0f, 1.0f / scale);
                                    } break;
                                    case MSDF_Segment_COUNT: {
                                    } break;
                                }
                            }
                        }
                    }

                    if (state->render_points) {
                        for (MSDF_Contour *contour = glyph.first_contour; contour; contour = contour->next) {
                            for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
                                switch (segment->kind) {
                                    case MSDF_Segment_Null: {
                                    } break;
                                    case MSDF_Segment_Line: {
                                        draw_circle(segment->p0, point_size / scale, color_from_theme(ThemeColor_OnCurve), 0.0f, 1.0f / scale);
                                        draw_circle(segment->p1, point_size / scale, color_from_theme(ThemeColor_OnCurve), 0.0f, 1.0f / scale);
                                    } break;
                                    case MSDF_Segment_QuadraticBezier: {
                                        draw_circle(segment->p0, point_size / scale, color_from_theme(ThemeColor_OnCurve), 0.0f, 1.0f / scale);
                                        draw_circle(segment->p1, point_size / scale, color_from_theme(ThemeColor_OffCurve), 0.0f, 1.0f / scale);
                                        draw_circle(segment->p2, point_size / scale, color_from_theme(ThemeColor_OnCurve), 0.0f, 1.0f / scale);
                                    } break;
                                    case MSDF_Segment_COUNT: {
                                    } break;
                                }
                            }
                        }
                    }

                    if (state->render_logs && log_entry) {
                        U64 group_index = 0;
                        for (MSDF_LogGroup *group = log_entry->first_group; group; group = group->next, ++group_index) {
                            if (!state->is_group_visible[group_index]) {
                                continue;
                            }
                            for (MSDF_LogGeometry *geometry = group->first_geometry; geometry; geometry = geometry->next) {
                                V4F32 color = geometry->color;

                                if (1 + group_index == state->hovered_group) {
                                    V4F32 target = color_from_theme(ThemeColor_Hover);
                                    color.r = f32_lerp(color.r, target.r, state->hovered_t);
                                    color.g = f32_lerp(color.g, target.g, state->hovered_t);
                                    color.b = f32_lerp(color.b, target.b, state->hovered_t);
                                    color.a = f32_lerp(color.a, target.a, state->hovered_t);
                                }

                                switch (geometry->kind) {
                                    case MSDF_LogKind_Point: {
                                        draw_circle(geometry->p0, point_size / scale, color, 0.0f, 1.0f / scale);
                                    } break;
                                    case MSDF_LogKind_Line: {
                                        draw_line(geometry->p0, geometry->p1, color, 2.0f / scale, 0.0f, 1.0f / scale);
                                    } break;
                                    case MSDF_LogKind_Bezier: {
                                        draw_bezier(geometry->p0, geometry->p1, geometry->p2, color, 2.0f / scale, 0.0f, 1.0f / scale);
                                    } break;
                                }
                            }
                        }

                        if (state->hovered_group) {
                            state->hovered_t += (state->hovered_target_t - state->hovered_t) * ui_animation_super_slow_rate();
                            if (f32_abs(state->hovered_target_t - state->hovered_t) < 0.01f) {
                                state->hovered_target_t = 1.0f - state-> hovered_target_t;
                            }
                            request_frame();
                        } else {
                            state->hovered_t = 0.0f;
                            state->hovered_target_t = 0.0f;
                        }
                    }
                }
            }
            ui_box_set_draw_list(box, draw_list);

            ui_width_next(ui_size_fill());
            ui_height_next(ui_size_children_sum(1.0f));
            ui_row()
            ui_width(ui_size_children_sum(1.0f))
            ui_height(ui_size_children_sum(1.0f)) {
                ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                ui_column() {
                    ui_width(ui_size_text_content(0.0f, 1.0f))
                    ui_height(ui_size_text_content(0.0f, 1.0f))
                    ui_palette(palette_from_code(PaletteCode_Button)) {
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                        ui_checkbox_b32(&state->render_outline, str8_literal("Draw outlines"));
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                        ui_checkbox_b32(&state->render_points, str8_literal("Draw points"));
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                        ui_checkbox_b32(&state->render_raw, str8_literal("Draw raw"));
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                        ui_checkbox_b32(&state->render_nearest, str8_literal("Draw nearest"));
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                        ui_checkbox_b32(&state->render_logs, str8_literal("Draw logs"));
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                        ui_label_format("Selected glyph: U+%.6X", global_state->selected_codepoint);
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    }
                }
                if (state->render_logs && log_entry) {
                    ui_spacer_sized(ui_size_ems(1.0f, 1.0f));
                    ui_column() {
                        ui_width(ui_size_children_sum(1.0f))
                        ui_height(ui_size_children_sum(1.0f)) {
                            U64 next_log_index = state->log_index;
                            U64 next_hovered_group = 0;

                            ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                            ui_row()
                            ui_width(ui_size_text_content(10.0f, 1.0f))
                            ui_height(ui_size_text_content(0.0f, 1.0f))
                            ui_palette(palette_from_code(PaletteCode_Button)) {
                                UI_Input previous_input = ui_button(str8_literal("Previous"));
                                ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                                UI_Input next_input = ui_button(str8_literal("Next"));
                                ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                                ui_label_format("%lu: %.*s", state->log_index, str8_expand(log_entry->description));
                                if (next_input.flags & UI_InputFlag_LeftClicked) {
                                    next_log_index = (state->log_index + 1) % msdf_glyph->log.count;
                                }
                                if (previous_input.flags & UI_InputFlag_LeftClicked) {
                                    next_log_index = (state->log_index + msdf_glyph->log.count - 1) % msdf_glyph->log.count;
                                }
                            }

                            ui_row() {
                                U32 group_index = 0;
                                ui_width(ui_size_text_content(0.0f, 1.0f))
                                ui_height(ui_size_text_content(0.0f, 1.0f))
                                for (MSDF_LogGroup *group = log_entry->first_group; group; group = group->next, ++group_index) {
                                    if (group_index % 5 == 0) {
                                        if (group_index != 0) {
                                            ui_column_end();
                                            ui_spacer_sized(ui_size_ems(1.0f, 1.0f));
                                        }
                                        ui_width_next(ui_size_children_sum(1.0f));
                                        ui_height_next(ui_size_children_sum(1.0f));
                                        ui_column_begin();
                                    }
                                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                                    UI_Input check_input = ui_checkbox_b32_format(&state->is_group_visible[group_index], "%lu", group_index);
                                    if (check_input.flags & UI_InputFlag_Hovering) {
                                        next_hovered_group = 1 + group_index;
                                    }
                                }
                                if (group_index != 0) {
                                    ui_column_end();
                                }
                            }
                            ui_spacer_sized(ui_size_ems(0.5f, 1.0f));

                            if (next_log_index != state->log_index) {
                                state->is_group_visible = 0;
                                state->log_index = next_log_index;
                            }
                            state->hovered_group = next_hovered_group;
                        }
                    }
                }
            }
        }
    }

    arena_end_temporary(scratch);
    prof_function_end();
}

PANEL_BUILD_FUNCTION(view_stats) {
    prof_function_begin();
    ui_width(ui_size_fill())
    ui_height(ui_size_fill())
    ui_row() {
        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
        ui_column() {
            ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
            ui_width(ui_size_text_content(0.0f, 1.0f))
            ui_height(ui_size_text_content(0.0f, 1.0f)) {
                Render_Stats stats = render_get_stats();
                ui_label_format("Batches: %u", stats.batch_count);
                ui_label_format("Shapes: %u", stats.shape_count);
                ui_label_format("Bytes uploaded: %u", stats.bytes_uploaded_to_gpu);
            }
        }
    }
    prof_function_end();
}

PANEL_BUILD_FUNCTION(view_theme) {
    prof_function_begin();

    typedef struct ViewState ViewState;
    struct ViewState {
        ThemeColor context_color;
    };

    ViewState *state = (ViewState *) tab_get_state(tab, sizeof(ViewState));

    UI_Key context_key = ui_key_from_string(ui_active_seed_key(), str8_literal("##picker"));
    ui_context_menu(context_key) {
        ui_extra_box_flags_next(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawDropShadow);
        ui_width(ui_size_children_sum(1.0f))
        ui_height(ui_size_children_sum(1.0f))
        ui_corner_radius(5.0f)
        ui_column()
        ui_padding(ui_size_ems(0.5f, 1.0f)) {
            ui_row()
            ui_padding(ui_size_ems(0.5f, 1.0f))
            ui_height(ui_size_ems(12.0f, 1.0f)) {
                V4F32 color = color_from_theme(state->context_color); 
                B32 changed = ui_color_picker(&color, ui_size_ems(12.0f, 1.0f), ui_size_ems(2.0f, 1.0f), ui_size_ems(1.0f, 1.0f));

                if (changed) {
                    global_state->theme.colors[state->context_color] = color;
                    global_state->target_theme.colors[state->context_color] = color;
                }
            }
        }
    }

    ui_width(ui_size_fill())
    ui_height(ui_size_fill())
    ui_row() {
        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
        ui_column() {
            ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
            ui_width_next(ui_size_text_content(0.0f, 1.0f));
            ui_height_next(ui_size_text_content(0.0f, 1.0f));
            ui_label_format("Active theme: %.*s", str8_expand(global_themes[global_state->theme_index].name));
            ui_width(ui_size_children_sum(1.0))
            ui_height(ui_size_children_sum(1.0))
            ui_row() {
                ui_column() {
                    ui_width(ui_size_text_content(0.0f, 1.0f))
                    ui_height(ui_size_ems(1.0f, 1.0f))
                    for (ThemeColor color = 0; color < ThemeColor_COUNT; ++color) {
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                        ui_label(theme_color_names[color]);
                    }
                }
                ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                ui_corner_radius(5.0f)
                ui_column() {
                    ui_width(ui_size_ems(10.0f, 1.0f))
                    ui_height(ui_size_ems(1.0f, 1.0f))
                    for (ThemeColor color = 0; color < ThemeColor_COUNT; ++color) {
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                        UI_Palette display = ui_palette_top();
                        display.background = color_from_theme(color);
                        ui_palette_next(display);
                        ui_hover_cursor_next(Gfx_Cursor_Hand);
                        UI_Box *box = ui_create_box_from_string_format(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive | UI_BoxFlag_Clickable, "###color_%u", color);
                        UI_Input input = ui_input_from_box(box);
                        if (input.flags & UI_InputFlag_LeftClicked) {
                            state->context_color = color;
                            ui_context_menu_open(context_key, box->key, v2f32(0, 0));
                        }
                    }
                }
            }

        }
    }

    prof_function_end();
}

PANEL_BUILD_FUNCTION(view_test) {
    prof_function_begin();
    ui_center() {
        ui_width_next(ui_size_parent_percent(1.0f, 1.0f));
        ui_height_next(ui_size_children_sum(1.0f));
        ui_row()
        ui_center() {
            ui_width(ui_size_ems(20.0f, 1.0f))
            ui_height(ui_size_ems(1.5f, 1.0f))
            ui_corner_radius(5.0f) {
                ui_palette_next(palette_from_code(PaletteCode_Button));

                // NOTE(simon): Setup
                Str8 text = str8_literal("Sample text, this is some really long example text. Like really long");
                local U64 cursor = 0;
                local U64 mark = 0;
                local U8 buffer[1024];
                U64 buffer_capacity = array_count(buffer);
                local U64 buffer_size = 0;
                local B32 is_initialized = false;
                if (!is_initialized) {
                    buffer_size = u64_min(text.size, buffer_capacity);
                    memory_copy(buffer, text.data, buffer_size);
                    is_initialized = true;
                }

                ui_line_edit(buffer, &buffer_size, buffer_capacity, &cursor, &mark, ui_key_from_string(global_ui_null_key, str8_literal("line_edit")));

                ui_spacer_sized(ui_size_ems(1.0f, 1.0f));

                local F32 slider = 5.0f;
                ui_palette_next(palette_from_code(PaletteCode_Button));
                ui_slider(0.0f, &slider, 10.0f, ui_key_from_string(ui_active_seed_key(), str8_literal("slider")));
            }
        }
    }
    prof_function_end();
}
