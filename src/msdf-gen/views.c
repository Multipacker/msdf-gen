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

typedef struct UIDrawMSDF UIDrawMSDF;
struct UIDrawMSDF {
    Font *font;
    B32   render_raw;
};

internal UI_BOX_DRAW_FUNCTION(draw_ui_msdf) {
    UIDrawMSDF *ui_draw_msdf = (UIDrawMSDF *) data;

    StringDecode decode = string_decode_utf8(box->string.data, box->string.size);

    Glyph *glyph = font_get_glyph(ui_draw_msdf->font, decode.codepoint);

    V2F32 box_size = v2f32_subtract(box->calculated_rectangle.max, box->calculated_rectangle.min);
    V2F32 glyph_size = r2f32_size(glyph->rectangle_pt);
    F32 scale_to_fit = f32_min(box_size.x / glyph_size.x, box_size.y / glyph_size.y);

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
        ui_draw_msdf->render_raw ? Render_ShapeFlag_Texture : Render_ShapeFlag_MSDF
    );
}

PANEL_BUILD_FUNCTION(view_glyph_list) {
    V2F32 panel_size = r2f32_size(panel_rectangle);
    F32 scrollbar_width = (F32) ui_font_size_top();
    F32 container_width = panel_size.x - scrollbar_width;

    typedef struct {
        U32 scroll_codepoint;
        F32 scroll_offset;
    } GlyphListState;

    GlyphListState *state = tab_get_state(tab, sizeof(*state));

    // NOTE(simon): Scroll region
    ui_width_next(ui_size_pixels(panel_size.x, 1.0f));
    ui_height_next(ui_size_pixels(panel_size.y, 1.0f));
    ui_layout_axis_next(Axis2_X);
    UI_Box *region = ui_create_box_from_string(UI_BoxFlag_OverflowY | UI_BoxFlag_Scrollable, str8_literal("region"));
    ui_parent_push(region);

    // NOTE(simon): Scroll container
    ui_width_next(ui_size_pixels(container_width, 1.0f));
    ui_height_next(ui_size_pixels(panel_size.y, 1.0f));
    ui_layout_axis_next(Axis2_Y);
    UI_Box *container = ui_create_box_from_string(0, str8_literal("glyphs"));

    U32 first_codepoint = 0x000000;
    U32 last_codepoint  = 0x10FFFF;

    F32 preferred_width = 3.0f * (F32) ui_font_size_top();
    U32 codepoints_per_row = (U32) f32_floor(container_width / preferred_width);
    if (!codepoints_per_row) {
        codepoints_per_row = 10;
    }
    F32 width = container_width / (F32) codepoints_per_row;
    F32 height = width * 2.0f;

    S32 first_row = 0;
    S32 last_row  = (S32) ((last_codepoint + codepoints_per_row - 1) / codepoints_per_row);

    S32 scroll_row = (S32) (state->scroll_codepoint / codepoints_per_row);
    S32 target_row = scroll_row;

    S32 top_row    = scroll_row + (S32) (state->scroll_offset < 0.0f ? f32_ceil(state->scroll_offset - 1.0f) : f32_floor(state->scroll_offset));
    S32 bottom_row = s32_min(top_row + (S32) f32_ceil(panel_size.y / height) + 1, last_row);
    container->view_offset.y = height * (f32_mod(state->scroll_offset, 1.0f) + (state->scroll_offset < 0.0f));

    S32 selected_row = (S32) (global_state->selected_codepoint / codepoints_per_row);

    // NOTE(simon): Scrollbar container
    ui_width_next(ui_size_pixels(scrollbar_width, 1.0f));
    ui_height_next(ui_size_pixels(panel_size.y, 1.0f));
    ui_layout_axis_next(Axis2_Y);
    UI_Box *scroll_container = ui_create_box_from_string(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder, str8_literal("scrollbar"));

    ui_width(ui_size_parent_percent(1.0f, 1.0f))
    ui_parent(scroll_container) {
        F32 rows_above   = (F32) (scroll_row - first_row) + state->scroll_offset;
        F32 visible_rows = container->calculated_size.height / height;
        F32 row_count    = (F32) (last_row - first_row) + visible_rows - 1.0f;
        F32 rows_below   = (F32) (last_row - first_row) - 1.0f - (F32) scroll_row - state->scroll_offset;

        ui_hover_cursor_next(Gfx_Cursor_Hand);
        ui_height_next(ui_size_parent_percent(rows_above / row_count, 0.0f));
        UI_Box *scroll_before = ui_create_box_from_string(UI_BoxFlag_Clickable, str8_literal("before"));

        ui_palette_next(palette_from_code(PaletteCode_Button));
        ui_hover_cursor_next(Gfx_Cursor_Hand);
        ui_height_next(ui_size_parent_percent(f32_max(0.01f, visible_rows / row_count), 1.0f));
        UI_Box *scroll = ui_create_box_from_string(UI_BoxFlag_Clickable | UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive, str8_literal("scroll"));

        ui_hover_cursor_next(Gfx_Cursor_Hand);
        ui_height_next(ui_size_parent_percent(rows_below / row_count, 0.0f));
        UI_Box *scroll_after = ui_create_box_from_string(UI_BoxFlag_Clickable, str8_literal("after"));

        UI_Input before_input = ui_input_from_box(scroll_before);
        if (before_input.input_flags & UI_InputFlag_LeftClicked) {
            target_row -= (S32) f32_floor(visible_rows);
        }

        UI_Input scroll_input = ui_input_from_box(scroll);
        if (scroll_input.input_flags & UI_InputFlag_LeftDragging) {
            if (scroll_input.input_flags & UI_InputFlag_LeftPressed) {
                ui_set_drag_data(&top_row);
            }

            S32 start_row = *ui_get_drag_data(S32);

            F32 scroll_size = panel_size.y - scroll->calculated_size.height;
            F32 drag_percent = ui_drag_delta().y / scroll_size;
            target_row = start_row + (S32) f32_floor(drag_percent * (row_count - visible_rows));
        }

        UI_Input after_input  = ui_input_from_box(scroll_after);
        if (after_input.input_flags & UI_InputFlag_LeftClicked) {
            target_row += (S32) f32_floor(visible_rows);
        }
    }

    ui_parent_push(container);

    UIDrawMSDF *draw_msdf = arena_push_struct_zero(ui_frame_arena(), UIDrawMSDF);
    draw_msdf->font = global_state->font;

    ui_palette_push(palette_from_code(PaletteCode_Button));
    for (S32 row = top_row; row < bottom_row; ++row) {
        ui_width_next(ui_size_parent_percent(1.0f, 1.0f));
        ui_height_next(ui_size_pixels(height, 1.0f));
        ui_row() {
            ui_width(ui_size_pixels(width, 1.0f))
            ui_height(ui_size_parent_percent(1.0f, 1.0f))
            ui_draw_function(draw_ui_msdf)
            ui_draw_data(draw_msdf)
            ui_hover_cursor(Gfx_Cursor_Hand)
            for (U32 column = 0; column < codepoints_per_row; ++column) {
                U32 codepoint = column + (U32) row * codepoints_per_row;
                if (codepoint > last_codepoint) {
                    break;
                }

                U8 buffer[4] = { 0 };
                U64 size = string_encode_utf8(buffer, codepoint);
                Str8 string = str8(buffer, size);

                UI_Box *box = ui_create_box_from_string(
                    UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder |
                    UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
                    UI_BoxFlag_Clickable,
                    string
                );
                UI_Input input = ui_input_from_box(box);

                if (input.input_flags & UI_InputFlag_LeftClicked) {
                    global_state->selected_codepoint = codepoint;
                }
            }
        }
    }
    ui_palette_pop();

    // NOTE(simon): Container
    ui_parent_pop();

    // NOTE(simon): Region
    ui_parent_pop();

    UI_Input region_input = ui_input_from_box(region);
    target_row -= (S32) region_input.scroll.y;

    // NOTE(simon): Updating scroll
    target_row = s32_min(s32_max(first_row, target_row), last_row - 1);
    state->scroll_offset += (F32) scroll_row - (F32) target_row;
    scroll_row = target_row;
    state->scroll_codepoint = (U32) scroll_row * codepoints_per_row;

    // NOTE(simon): Animation
    state->scroll_offset += -state->scroll_offset * ui_animation_slow_rate();
    if (f32_abs(state->scroll_offset) < 0.001f) {
        state->scroll_offset = 0.0f;
    } else {
        request_frame();
    }
}

PANEL_BUILD_FUNCTION(view_glyph) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    typedef struct ViewState ViewState;
    struct ViewState {
        B32 render_outline;
        B32 render_points;
        B32 render_raw;
    };

    ViewState *state = (ViewState *) tab_get_state(tab, sizeof(ViewState));

    ui_width(ui_size_parent_percent(1.0f, 1.0f))
    ui_height(ui_size_parent_percent(1.0f, 1.0f))
    ui_column() {
        ui_width(ui_size_text_content(0.0f, 1.0f))
        ui_height(ui_size_text_content(0.0f, 1.0f))
        ui_palette(palette_from_code(PaletteCode_Button)) {
            ui_width_next(ui_size_parent_percent(1.0f, 0.0f));
            ui_height_next(ui_size_parent_percent(1.0f, 0.0f));
            UI_Box *box = ui_create_box_from_string(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clip, str8_literal("glyph_viewer"));

            Draw_List *draw_list = draw_list_create();
            draw_list_scope(draw_list) {
                F32 padding = 2.0f * (F32) ui_font_size_top();
                F32 point_size = 0.4f * (F32) ui_font_size_top();

                U32 glyph_index = ttf_get_glyph_index(global_state->ttf_font, global_state->selected_codepoint);
                MSDF_Glyph glyph = ttf_expand_contours_to_msdf(scratch.arena, global_state->ttf_font, glyph_index);

                V2F32 box_size = r2f32_size(box->calculated_rectangle);
                R2F32 glyph_rectangle = r2f32((F32) glyph.min.x, (F32) glyph.min.y, (F32) glyph.max.x, (F32) glyph.max.y);
                V2F32 glyph_size = r2f32_size(glyph_rectangle);

                M3F32 center_glyph = m3f32_translation(v2f32_negate(r2f32_center(glyph_rectangle)));
                F32 scale = f32_max(0.0f, f32_min((box_size.x - 2.0f * padding) / glyph_size.x, (box_size.y - 2.0f * padding) / glyph_size.y));
                M3F32 scale_to_box = m3f32_scale(v2f32(scale, -scale));
                M3F32 center_box = m3f32_translation(v2f32_scale(r2f32_size(box->calculated_rectangle), 0.5f));

                M3F32 transform = m3f32_multiply_m3f32(center_box, m3f32_multiply_m3f32(scale_to_box, center_glyph));

                draw_transform(transform) {
                    Glyph *msdf_glyph = font_get_glyph(global_state->font, global_state->selected_codepoint);

                    V4F32 tint = box->palette.text;
                    Render_ShapeFlags flags = Render_ShapeFlag_MSDF;
                    if (state->render_raw) {
                        tint = v4f32(1.0f, 1.0f, 1.0f, 1.0f);
                        flags = Render_ShapeFlag_Texture;
                    }

                    V2F32 uv_size = r2f32_size(msdf_glyph->uv);

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

                    if (state->render_outline) {
                        for (MSDF_Contour *contour = glyph.first_contour; contour; contour = contour->next) {
                            for (MSDF_Segment *segment = contour->first_segment; segment; segment = segment->next) {
                                switch (segment->kind) {
                                    case MSDF_Segment_Null: {
                                    } break;
                                    case MSDF_Segment_Line: {
                                        draw_line(segment->p0, segment->p1, color_from_theme(ThemeColor_Outline), 1.0f / scale, 0.0f, 1.0f);
                                    } break;
                                    case MSDF_Segment_QuadraticBezier: {
                                        draw_bezier(segment->p0, segment->p1, segment->p2, color_from_theme(ThemeColor_Outline), 1.0f / scale, 0.0f, 1.0f);
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
                                        draw_circle(segment->p0, point_size / scale, color_from_theme(ThemeColor_OnCurve), 0.0f, 1.0f);
                                        draw_circle(segment->p1, point_size / scale, color_from_theme(ThemeColor_OnCurve), 0.0f, 1.0f);
                                    } break;
                                    case MSDF_Segment_QuadraticBezier: {
                                        draw_circle(segment->p0, point_size / scale, color_from_theme(ThemeColor_OnCurve), 0.0f, 1.0f);
                                        draw_circle(segment->p1, point_size / scale, color_from_theme(ThemeColor_OffCurve), 0.0f, 1.0f);
                                        draw_circle(segment->p2, point_size / scale, color_from_theme(ThemeColor_OnCurve), 0.0f, 1.0f);
                                    } break;
                                    case MSDF_Segment_COUNT: {
                                    } break;
                                }
                            }
                        }
                    }
                }
            }
            ui_box_set_draw_list(box, draw_list);

            ui_palette(palette_from_code(PaletteCode_Button)) {
                ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                ui_checkbox_b32(&state->render_outline, str8_literal("Draw outlines"));
                ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                ui_checkbox_b32(&state->render_points, str8_literal("Draw points"));
                ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                ui_checkbox_b32(&state->render_raw, str8_literal("Draw raw"));
                ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                ui_label_format("Selected glyph: U+%.6X", global_state->selected_codepoint);
            }
        }
    }

    arena_end_temporary(scratch);
}

PANEL_BUILD_FUNCTION(view_stats) {
    ui_width(ui_size_parent_percent(1.0f, 1.0f))
    ui_height(ui_size_parent_percent(1.0f, 1.0f))
    ui_column() {
        ui_width(ui_size_text_content(0.0f, 1.0f))
        ui_height(ui_size_text_content(0.0f, 1.0f)) {
            Render_Stats stats = render_get_stats();
            ui_label(str8_literal("Render stats"));
            ui_label_format("Batches: %u", stats.batch_count);
            ui_label_format("Shapes: %u", stats.shape_count);
            ui_label_format("Bytes uploaded: %u", stats.bytes_uploaded_to_gpu);
        }
    }
}

PANEL_BUILD_FUNCTION(view_theme) {
    ui_width(ui_size_fill())
    ui_height(ui_size_fill())
    ui_column() {
        ui_width_next(ui_size_text_content(0.0f, 1.0f));
        ui_height_next(ui_size_text_content(0.0f, 1.0f));
        ui_label_format("Active theme: %.*s", str8_expand(global_themes[global_state->theme_index].name));
        ui_width(ui_size_ems(15.0f, 1.0f))
        ui_height(ui_size_ems(1.0f, 1.0f))
        for (ThemeColor color = 0; color < ThemeColor_COUNT; ++color) {
            ui_spacer_sized(ui_size_ems(0.5f, 1.0f));

            ui_row() {
                ui_text_align_next(UI_TextAlign_Right);
                ui_label(theme_color_names[color]);
                ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                UI_Palette display = ui_palette_top();
                display.background = color_from_theme(color);
                ui_palette_next(display);
                ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder);
            }
        }
    }
}
