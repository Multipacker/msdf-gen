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

PANEL_BUILD_FUNCTION(view_null) {
}

// TODO(simon): Implement better handling of resizing.
PANEL_BUILD_FUNCTION(view_glyph_list) {
    prof_function_begin();

    typedef struct {
        UI_ScrollPosition position;
        S64 index;
    } ViewState;

    B32 is_new_tab = tab->view_state == 0;
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

    if (is_new_tab) {
        state->index = index_from_map_codepoint(codepoint_map, top_context()->codepoint);
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

    S64 new_index = s64_min(s64_max(0, state->index), (S64) codepoint_map.codepoint_count - 1);

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
                        ui_focus_hot_next(index == state->index ? UI_Focus_Active : UI_Focus_Inactive);
                        ui_focus_active_next(index == state->index ? UI_Focus_Active : UI_Focus_Inactive);

                        UI_Box *box = ui_create_box_from_string_format(
                            UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
                            UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable,
                            "##codepoint_%u", *codepoint
                        );

                        UI_Input input = ui_input_from_box(box);
                        if (input.flags & UI_InputFlag_Clicked) {
                            new_index = index;
                            push_command(Command_SelectCodepoint, .codepoint = *codepoint);
                        } else if (input.flags & UI_InputFlag_RightClicked) {
                            new_index = index;
                            push_command(Command_OpenTab, .tab_specification = str8_literal("GlyphView"), .codepoint = *codepoint);
                        }
                    }
                }
            }

            if (ui_is_focus_active()) {
                for (UI_Event *event = 0; ui_next_event(&event);) {
                    if (event->kind == UI_EventKind_Navigation) {
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
                                    codepoint_delta += -new_index % codepoints_per_row;
                                } else if (event->delta.x == 1) {
                                    codepoint_delta += codepoints_per_row - 1 - new_index % codepoints_per_row;
                                }
                            } break;
                            case UI_EventDeltaUnit_Page: {
                                S64 codepoints_per_page = visible_rows * codepoints_per_row;
                                codepoint_delta += event->delta.y * codepoints_per_page;
                            } break;
                            case UI_EventDeltaUnit_Whole: {
                                if (event->delta.x == -1) {
                                    codepoint_delta += -new_index;
                                } else if (event->delta.x == 1) {
                                    codepoint_delta += (S64) codepoint_map.codepoint_count - 1 - new_index;
                                }
                            } break;
                            case UI_EventDeltaUnit_COUNT: {
                            } break;
                        }

                        new_index = s64_min(s64_max(0, new_index + codepoint_delta), (S64) codepoint_map.codepoint_count - 1);
                        ui_consume_event(event);
                    } else if (event->kind == UI_EventKind_Accept) {
                        U32 new_codepoint = codepoint_from_map_index(codepoint_map, new_index);
                        push_command(Command_SelectCodepoint, .codepoint = new_codepoint);
                    }
                }
            }
        }

        ui_palette(palette_from_code(PaletteCode_Button))
        ui_focus(UI_Focus_None)
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
    if (new_index != state->index) {
        state->index = new_index;

        S64 active_row = state->index / codepoints_per_row;
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

typedef enum {
    MeasureFlag_Vertical   = 1 << 0,
    MeasureFlag_Horizontal = 1 << 1,
    MeasureFlag_Inward     = 1 << 2,
} MeasureFlags;

typedef struct DrawMeasure DrawMeasure;
struct DrawMeasure {
    MeasureFlags flags;
};

UI_BOX_DRAW_FUNCTION(draw_measure) {
    DrawMeasure *draw_data = (DrawMeasure *) data;

    R2F32 rectangle = box->calculated_rectangle;
    V2F32 center    = r2f32_center(rectangle);
    V2F32 half_size = v2f32_scale(r2f32_size(rectangle), 0.5f);
    V4F32 color     = box->palette.text;

    V2F32 point_offset = draw_data->flags & MeasureFlag_Inward ? v2f32_negate(half_size) : half_size;

    if (draw_data->flags & MeasureFlag_Vertical) {
        V2F32 min = v2f32(center.x, center.y - half_size.height);
        V2F32 max = v2f32(center.x, center.y + half_size.height);
        draw_line(min, max, color, 2, 0, 1.0f);

        draw_line(max, v2f32(max.x - point_offset.width, max.y - point_offset.width), color, 2, 0, 1.0f);
        draw_line(max, v2f32(max.x + point_offset.width, max.y - point_offset.width), color, 2, 0, 1.0f);

        draw_line(min, v2f32(min.x - point_offset.width, min.y + point_offset.width), color, 2, 0, 1.0f);
        draw_line(min, v2f32(min.x + point_offset.width, min.y + point_offset.width), color, 2, 0, 1.0f);
    }

    if (draw_data->flags & MeasureFlag_Horizontal) {
        V2F32 min = v2f32(center.x - half_size.width, center.y);
        V2F32 max = v2f32(center.x + half_size.width, center.y);
        draw_line(min, max, color, 2, 0, 1.0f);

        draw_line(min, v2f32(min.x + point_offset.height, min.y - point_offset.height), color, 2, 0, 1.0f);
        draw_line(min, v2f32(min.x + point_offset.height, min.y + point_offset.height), color, 2, 0, 1.0f);

        draw_line(max, v2f32(max.x - point_offset.height, max.y - point_offset.height), color, 2, 0, 1.0f);
        draw_line(max, v2f32(max.x - point_offset.height, max.y + point_offset.height), color, 2, 0, 1.0f);
    }
}

internal Void point_widget(V2F32 *location, M3F32 transform, F32 point_size, V4F32 color) {
    // NOTE(simon): Build palette.
    UI_Palette palette = ui_palette_top();
    palette.background = color;
    ui_palette_next(palette);

    ui_corner_radius_next(point_size);

    // NOTE(simon): Build location and size.
    V2F32 transformed_location = v2f32_subtract(m3f32_multiply_v2f32(transform, *location), v2f32(point_size, point_size));
    ui_fixed_position_next(transformed_location);
    ui_width_next(ui_size_pixels(2.0f * point_size, 1.0f));
    ui_height_next(ui_size_pixels(2.0f * point_size, 1.0f));

    UI_Box *point_box = ui_create_box_from_string_format(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive | UI_BoxFlag_Clickable, "##point_%p", location);

    // NOTE(simon): Input
    UI_Input point_input = ui_input_from_box(point_box);
    if (point_input.flags & UI_InputFlag_Hovering) {
        ui_tooltip(point_box->key) {
            ui_extra_box_flags_next(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawDropShadow);

            ui_palette(palette_from_code(PaletteCode_Button))
            ui_width(ui_size_children_sum(1.0f))
            ui_height(ui_size_children_sum(1.0f))
            ui_column()
            ui_width(ui_size_text_content(5.0f, 1.0f))
            ui_height(ui_size_text_content(5.0f, 1.0f)) {
                ui_label_format("X: %f", location->x);
                ui_label_format("Y: %f", location->y);
            }
        }
    }
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
        B32 render_metrics;
        F32 target_zoom;
        F32 zoom;
        V2F32 offset;
        F32 point_size;
        F32 line_width;
    };

    B32 is_new_tab = tab->view_state == 0;
    ViewState *state = (ViewState *) tab_get_state(tab, sizeof(ViewState));

    if (is_new_tab) {
        state->zoom = 1.0f;
        state->target_zoom = 1.0f;
        state->point_size = 0.5f;
        state->line_width = 0.3f;
    }

    F32 point_size = state->point_size * (F32) ui_font_size_top();
    F32 line_width = state->line_width * (F32) ui_font_size_top();

    ui_extra_box_flags_next(UI_BoxFlag_DefaultNavigation);
    ui_focus_next(UI_Focus_Active);
    ui_width(ui_size_parent_percent(1.0f, 1.0f))
    ui_height(ui_size_parent_percent(1.0f, 1.0f))
    ui_column_string(str8_literal("##panel"))
    ui_palette(palette_from_code(PaletteCode_Button)) {
        ui_width_next(ui_size_parent_percent(1.0f, 0.0f));
        ui_height_next(ui_size_parent_percent(1.0f, 0.0f));
        UI_Box *box = ui_create_box_from_string(
            UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clip |
            UI_BoxFlag_Scrollable | UI_BoxFlag_Clickable |
            UI_BoxFlag_DefaultNavigationSkip,
            str8_literal("##glyph_viewer")
        );
        ui_parent_push(box);

        MSDFCache_Glyph *msdf_glyph = msdf_cache_get_glyph(global_state->ttf_font, top_context()->codepoint);

        F32 padding = 2.0f * (F32) ui_font_size_top();

        U32 glyph_index = ttf_glyph_index_from_font_codepoint(global_state->ttf_font, top_context()->codepoint);
        MSDF_Glyph glyph = ttf_expand_contours_to_msdf(scratch.arena, global_state->ttf_font, glyph_index);

        V2F32 box_size = r2f32_size(box->calculated_rectangle);
        R2F32 glyph_rectangle = r2f32((F32) glyph.min.x, (F32) glyph.min.y, (F32) glyph.max.x, (F32) glyph.max.y);
        V2F32 glyph_size = r2f32_size(glyph_rectangle);

        M3F32 center_glyph = m3f32_translation(v2f32_negate(r2f32_center(glyph_rectangle)));
        F32 scale = f32_max(0.0f, f32_min((box_size.x - 2.0f * padding) / glyph_size.x, (box_size.y - 2.0f * padding) / glyph_size.y)) / state->zoom;
        M3F32 scale_to_box = m3f32_scale(v2f32(scale, -scale));
        M3F32 center_box = m3f32_translation(v2f32_add(v2f32_scale(r2f32_size(box->calculated_rectangle), 0.5f), state->offset));

        M3F32 transform = m3f32_multiply_m3f32(center_box, m3f32_multiply_m3f32(scale_to_box, center_glyph));

        if (state->render_metrics) {
            V2F32 origin  = m3f32_multiply_v2f32(transform, v2f32(0.0f, 0.0f));
            V2F32 advance = m3f32_multiply_v2f32(transform, v2f32(msdf_glyph->advance_pt, 0.0f));

            typedef enum {
                Metric_Point,
                Metric_Horizontal,
                Metric_Vertical,
            } MetricKind;
            typedef struct Metric Metric;
            struct Metric {
                MetricKind kind;
                V2F32      location;
                F32        size;
                Str8       key;
                Str8       name;
            };
            Metric metrics[] = {
                { .kind = Metric_Point,      .location = origin,                                          .key = str8_literal("##origin_point"),  .name = str8_literal("Origin"),        },
                { .kind = Metric_Point,      .location = advance,                                         .key = str8_literal("##advance_point"), .name = str8_literal("Advance point"), },
                { .kind = Metric_Horizontal, .location = origin,  .size = msdf_glyph->advance_pt * scale, .key = str8_literal("##advance_width"), .name = str8_literal("Advance width"), },
                { .kind = Metric_Vertical,   .location = origin,  .size = 1.0f * scale,                   .key = str8_literal("##em"),            .name = str8_literal("em size"),       },
            };

            UI_Palette metrics_palette = { 0 };
            metrics_palette.background = color_from_theme(ThemeColor_Outline);
            metrics_palette.text       = color_from_theme(ThemeColor_Text);

            F32 arrow_half_width = ui_size_ems(1.0f, 0.0f).value;

            ui_palette(metrics_palette)
            for (U32 i = 0; i < array_count(metrics); ++i) {
                B32 hovered = false;
                UI_Key tooltip_key = global_ui_null_key;

                switch (metrics[i].kind) {
                    case Metric_Point: {
                        ui_corner_radius_next(point_size);
                        ui_fixed_x_next(metrics[i].location.x - point_size);
                        ui_fixed_y_next(metrics[i].location.y - point_size);
                        ui_width_next(ui_size_pixels(2.0f * point_size, 1.0f));
                        ui_height_next(ui_size_pixels(2.0f * point_size, 1.0f));
                        UI_Box *metric_box = ui_create_box_from_string(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clickable | UI_BoxFlag_DefaultNavigationSkip, metrics[i].key);

                        hovered |= ui_input_from_box(metric_box).flags & UI_InputFlag_Hovering;
                        tooltip_key = metric_box->key;
                    } break;
                    case Metric_Horizontal: {
                        DrawMeasure *draw_data = arena_push_struct(ui_frame_arena(), DrawMeasure);
                        draw_data->flags = MeasureFlag_Horizontal;
                        ui_draw_function_next(draw_measure);
                        ui_draw_data_next(draw_data);
                        ui_fixed_x_next(metrics[i].location.x);
                        ui_fixed_y_next(metrics[i].location.y - arrow_half_width);
                        ui_width_next(ui_size_pixels(metrics[i].size, 1.0f));
                        ui_height_next(ui_size_pixels(2.0f * arrow_half_width, 1.0f));
                        UI_Box *arrow_box = ui_create_box_from_string_format(UI_BoxFlag_Clickable | UI_BoxFlag_DefaultNavigationSkip, "%.*sarrow", str8_expand(metrics[i].key));
                        hovered |= ui_input_from_box(arrow_box).flags & UI_InputFlag_Hovering;

                        ui_fixed_x_next(metrics[i].location.x + metrics[i].size - line_width);
                        ui_fixed_y_next(0);
                        ui_width_next(ui_size_pixels(2.0f * line_width, 1.0f));
                        ui_height_next(ui_size_fill());
                        UI_Box *metric_box = ui_create_box_from_string(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clickable | UI_BoxFlag_DefaultNavigationSkip, metrics[i].key);
                        hovered |= ui_input_from_box(metric_box).flags & UI_InputFlag_Hovering;

                        tooltip_key = arrow_box->key;
                    } break;
                    case Metric_Vertical: {
                        DrawMeasure *draw_data = arena_push_struct(ui_frame_arena(), DrawMeasure);
                        draw_data->flags = MeasureFlag_Vertical;
                        ui_draw_function_next(draw_measure);
                        ui_draw_data_next(draw_data);
                        ui_fixed_x_next(metrics[i].location.x - arrow_half_width);
                        ui_fixed_y_next(metrics[i].location.y - metrics[i].size);
                        ui_width_next(ui_size_pixels(2.0f * arrow_half_width, 1.0f));
                        ui_height_next(ui_size_pixels(metrics[i].size, 1.0f));
                        UI_Box *arrow_box = ui_create_box_from_string_format(UI_BoxFlag_Clickable | UI_BoxFlag_DefaultNavigationSkip, "%.*sarrow", str8_expand(metrics[i].key));
                        hovered |= ui_input_from_box(arrow_box).flags & UI_InputFlag_Hovering;

                        ui_fixed_x_next(0);
                        ui_fixed_y_next(metrics[i].location.y - metrics[i].size - line_width);
                        ui_width_next(ui_size_fill());
                        ui_height_next(ui_size_pixels(2.0f * line_width, 1.0f));
                        UI_Box *metric_box = ui_create_box_from_string(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clickable | UI_BoxFlag_DefaultNavigationSkip, metrics[i].key);
                        hovered |= ui_input_from_box(metric_box).flags & UI_InputFlag_Hovering;

                        tooltip_key = arrow_box->key;
                    } break;
                }

                if (hovered) {
                    ui_tooltip(tooltip_key) {
                        ui_width_next(ui_size_text_content(0.0f, 1.0f));
                        ui_height_next(ui_size_text_content(0.0f, 1.0f));
                        ui_label(metrics[i].name);
                    }
                }
            }
        }

        Draw_List *draw_list = draw_list_create();
        draw_list_scope(draw_list) {
            V4F32 tint = box->palette.text;
            Render_ShapeFlags flags = Render_ShapeFlag_MSDF;
            Render_Filtering filtering = state->render_nearest ? Render_Filtering_Nearest : Render_Filtering_Linear;
            if (state->render_raw) {
                tint = v4f32(1.0f, 1.0f, 1.0f, 1.0f);
                flags = Render_ShapeFlag_Texture;
            }

            V2F32 uv_size = r2f32_size(msdf_glyph->uv);

            draw_filtering(filtering) {
                V2F32 min_corner = m3f32_multiply_v2f32(transform, v2f32(glyph_rectangle.min.x - glyph_size.width  * 0.5f / uv_size.width, glyph_rectangle.min.y - glyph_size.height * 0.5f / uv_size.height));
                V2F32 max_corner = m3f32_multiply_v2f32(transform, v2f32(glyph_rectangle.max.x + glyph_size.width  * 0.5f / uv_size.width, glyph_rectangle.max.y + glyph_size.height * 0.5f / uv_size.height));
                draw_texture(
                    r2f32(min_corner.x, min_corner.y, max_corner.x, max_corner.y),
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
                                draw_line(m3f32_multiply_v2f32(transform, segment->p0), m3f32_multiply_v2f32(transform, segment->p1), color_from_theme(ThemeColor_Outline), line_width, 0.0f, 1.0f);
                            } break;
                            case MSDF_Segment_QuadraticBezier: {
                                draw_bezier(m3f32_multiply_v2f32(transform, segment->p0), m3f32_multiply_v2f32(transform, segment->p1), m3f32_multiply_v2f32(transform, segment->p2), color_from_theme(ThemeColor_Outline), line_width, 0.0f, 1.0f);
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
                                point_widget(&segment->p0, transform, point_size, color_from_theme(ThemeColor_OnCurve));
                                point_widget(&segment->p1, transform, point_size, color_from_theme(ThemeColor_OnCurve));
                            } break;
                            case MSDF_Segment_QuadraticBezier: {
                                point_widget(&segment->p0, transform, point_size, color_from_theme(ThemeColor_OnCurve));
                                point_widget(&segment->p1, transform, point_size, color_from_theme(ThemeColor_OffCurve));
                                point_widget(&segment->p2, transform, point_size, color_from_theme(ThemeColor_OnCurve));
                            } break;
                            case MSDF_Segment_COUNT: {
                            } break;
                        }
                    }
                }
            }
        }
        ui_box_set_draw_list(box, draw_list);

        // NOTE(simon): Pop box
        ui_parent_pop();

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
                    ui_checkbox_b32(&state->render_metrics, str8_literal("Draw metrics"));
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    ui_width_next(ui_size_children_sum(1.0f));
                    ui_height_next(ui_size_children_sum(1.0f));
                    ui_row() {
                        ui_width_next(ui_size_ems(5.0f, 1.0f));
                        ui_slider(0.1f, &state->point_size, 1.0f, ui_key_from_string(ui_active_seed_key(), str8_literal("point_size")));
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                        ui_label(str8_literal("Point size"));
                    }
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    ui_width_next(ui_size_children_sum(1.0f));
                    ui_height_next(ui_size_children_sum(1.0f));
                    ui_row() {
                        ui_width_next(ui_size_ems(5.0f, 1.0f));
                        ui_slider(0.1f, &state->line_width, 1.0f, ui_key_from_string(ui_active_seed_key(), str8_literal("line_width")));
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                        ui_label(str8_literal("Line width"));
                    }
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    ui_label_format("Selected glyph: U+%.6X", top_context()->codepoint);
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
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

PANEL_BUILD_FUNCTION(view_preview) {
    prof_function_begin();
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    typedef struct ViewState ViewState;
    struct ViewState {
        U64 cursor;
        U64 mark;
        U64 size;
        U8 buffer[1024];
    };

    B32 is_new_tab = tab->view_state == 0;
    ViewState *state = (ViewState *) tab_get_state(tab, sizeof(ViewState));

    if (is_new_tab) {
        Str8 starter_text = str8_literal("Sphinx of black quartz, judge my vow.");
        memory_copy(state->buffer, starter_text.data, starter_text.size);
        state->size = starter_text.size;
    }

    ui_focus(UI_Focus_Active)
    ui_palette(palette_from_code(PaletteCode_Button))
    ui_width(ui_size_fill())
    ui_height(ui_size_ems(1.5f, 1.0f))
    ui_text_x_padding(ui_size_ems(0.5f, 1.0f).value) {
        ui_line_edit(state->buffer, &state->size, array_count(state->buffer), &state->cursor, &state->mark, ui_key_from_string(ui_active_seed_key(), str8_literal("preview_text")));
    }

    ui_palette_next(palette_from_code(PaletteCode_Button));
    ui_width_next(ui_size_fill());
    ui_height_next(ui_size_fill());
    UI_Box *canvas = ui_create_box_from_string(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder, str8_literal("##preview"));

    FontCache_Text text = { 0 };

    // NOTE(simon): Build laid out text.
    {
        TTF_Font *font = global_state->ttf_font;

        text.letters = arena_push_array(scratch.arena, FontCache_Letter, state->size);

        text.ascent  = (F32) font->ascent  / (F32) font->funits_per_em;
        text.descent = (F32) font->descent / (F32) font->funits_per_em;
        text.size.height = text.ascent - text.descent;

        U8 *ptr = state->buffer;
        U8 *opl = state->buffer + state->size;
        while (ptr < opl) {
            StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
            ptr += decode.size;

            MSDFCache_Glyph *msdf_glyph = msdf_cache_get_glyph(font, decode.codepoint);

            // TODO(simon): Verify that this is the correct layouting.
            FontCache_Letter *letter = &text.letters[text.letter_count++];
            letter->texture = msdf_glyph->texture;
            letter->offset  = msdf_glyph->rectangle_pt.min;
            letter->size    = r2f32_size(msdf_glyph->rectangle_pt);
            letter->source = r2f32(
                msdf_glyph->uv.min.x,
                msdf_glyph->uv.max.y,
                msdf_glyph->uv.max.x,
                msdf_glyph->uv.min.y
            );
            letter->advance = msdf_glyph->advance_pt;
            letter->decode_size = decode.size;

            text.size.width += msdf_glyph->advance_pt;
        }
    }

    Draw_List *draw_list = draw_list_create();
    draw_list_scope(draw_list) {
        F32   padding      = 2.0f * (F32) ui_font_size_top();
        V2F32 box_size     = r2f32_size(canvas->calculated_rectangle);
        M3F32 center_box   = m3f32_translation(v2f32_scale(box_size, 0.5f));
        M3F32 center_text = m3f32_translation(v2f32(-text.size.width * 0.5f, 0.0f));
        F32   scale        = f32_max(0.0f, f32_min((box_size.x - 2.0f * padding) / text.size.width, (box_size.y - 2.0f * padding) / text.size.height));
        M3F32 scale_to_box = m3f32_scale(v2f32(scale, -scale));
        M3F32 transform    = m3f32_multiply_m3f32(center_box, m3f32_multiply_m3f32(scale_to_box, center_text));

        draw_transform(transform) {
            F32 advance = 0.0f;
            for (U64 i = 0; i < text.letter_count; ++i) {
                FontCache_Letter *letter = &text.letters[i];
                draw_msdf(
                    r2f32(
                        letter->offset.x + advance,
                        letter->offset.y,
                        letter->offset.x + advance + letter->size.x,
                        letter->offset.y + letter->size.y
                    ),
                    letter->source,
                    letter->texture,
                    canvas->palette.text
                );

                advance += letter->advance;
            }
        }
    }

    ui_box_set_draw_list(canvas, draw_list);

    arena_end_temporary(scratch);
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
