#include "src/base/base_include.h"
#include "src/graphics/graphics_include.h"
#include "src/render/render_include.h"
#include "src/font/font_include.h"
#include "src/font_cache/font_cache_include.h"
#include "src/ui/ui_include.h"

#include "src/base/base_include.c"
#include "src/graphics/graphics_include.c"
#include "src/render/render_include.c"
#include "src/font/font_include.c"
#include "src/font_cache/font_cache_include.c"
#include "src/ui/ui_include.c"

/*
 * TODO:
 * Clipping to parent
 * Clipboard
 * Rounded corners
 * Focus and keyboard navigation / interaction
 * Draggin and more input information
 */
/* NOTE(simon): The old zoom equations, for reference.
 * F32 old_zoom = zoom;
 * zoom *= f32_pow(0.97f, event->scroll.y);
 * offset = v2f32_subtract(mouse, v2f32_scale(v2f32_subtract(mouse, offset), old_zoom / zoom));
*/

typedef struct {
    Str8 name;
    V4F32 background_color;
    V4F32 element_color;
    V4F32 border_color;
    V4F32 text_color;
} Theme;

global Theme global_themes[2];

typedef struct {
    V2F32 min_pt;
    V2F32 max_pt;
    F32   advance_pt;
    V2F32 uv_min;
    V2F32 uv_max;
} Glyph;

typedef struct {
    Render_Texture atlas;
    Glyph glyphs[2048];
} Font;

typedef struct UIDrawMSDF UIDrawMSDF;
struct UIDrawMSDF {
    Font *font;
    B32   render_raw;
};

internal Void load_font(Str8 font_path, Font *result) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    U32 glyph_size     = 32;
    U32 glyphs_per_row = 64;
    U32 atlas_size     = glyph_size * glyphs_per_row;
    result->atlas = render_texture_create(v2u32(atlas_size, atlas_size), Render_TextureFormat_RGBA8, 0);

    TTF_Font *font = ttf_load(scratch.arena, font_path);
    if (font->errors.node_count == 0) {
        // Generate glyphs
        for (U32 codepoint = 0; codepoint < 2048; ++codepoint) {
            Arena_Temporary glyph_scratch = arena_get_scratch(&scratch.arena, 1);

            MSDF_RasterResult raster_result = msdf_generate(glyph_scratch.arena, font, codepoint, glyph_size);

            V2U32 atlas_position = v2u32(
                glyph_size * (codepoint % glyphs_per_row),
                glyph_size * (codepoint / glyphs_per_row)
            );
            render_texture_update(result->atlas, atlas_position, v2u32(glyph_size, glyph_size), raster_result.data);

            // This adjustment increases the size of glyphs to acount for the
            // UVs needing to include a 1/2 texel border for rendering. This
            // makes sure that the glyphs have the same visual size.
            F32 scale = ((F32) glyph_size - 1.0f) / ((F32) glyph_size - 2.0f) - 1.0f;
            F32 width_adjustment  = (raster_result.x_max - raster_result.x_min) * scale * 0.5f;
            F32 height_adjustment = (raster_result.y_max - raster_result.y_min) * scale * 0.5f;

            result->glyphs[codepoint].advance_pt = raster_result.advance_width;
            result->glyphs[codepoint].min_pt = v2f32(raster_result.x_min - width_adjustment, raster_result.y_min - height_adjustment);
            result->glyphs[codepoint].max_pt = v2f32(raster_result.x_max + width_adjustment, raster_result.y_max + height_adjustment);
            result->glyphs[codepoint].uv_min = v2f32(
                ((F32) atlas_position.x + 0.5f) / (F32) atlas_size,
                ((F32) atlas_position.y + 0.5f) / (F32) atlas_size
            );
            result->glyphs[codepoint].uv_max = v2f32(
                ((F32) atlas_position.x + (F32) glyph_size - 0.5f) / (F32) atlas_size,
                ((F32) atlas_position.y + (F32) glyph_size - 0.5f) / (F32) atlas_size
            );

            arena_end_temporary(glyph_scratch);
        }
    } else {
        os_console_print(str8_join(scratch.arena, &font->errors));
    }

    arena_end_temporary(scratch);
}

internal Void draw_text_msdf(Font *font, V2F32 position, F32 point_size, Str8 text) {
    V2F32 text_point = position;
    for (U8 *ptr = text.data, *opl = text.data + text.size; ptr < opl; ) {
        StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
        ptr += decode.size;

        Glyph *glyph = &font->glyphs[decode.codepoint];

        render_rectangle(
            v2f32_add(text_point, v2f32_scale(glyph->min_pt, point_size)), v2f32_add(text_point, v2f32_scale(glyph->max_pt, point_size)),
            .uv_min = glyph->uv_min, .uv_max = glyph->uv_max,
            .texture = font->atlas,
            .color = v4f32(1.0f, 1.0f, 1.0f, 1.0f),
            .flags = Render_RectangleFlags_MSDF
        );

        text_point.x += glyph->advance_pt * point_size;
    }
}

internal Void draw_text(FontCache_Font *font, V2F32 origin, Str8 string, U32 size) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    FontCache_Text text = font_cache_text(scratch.arena, font, string, size);

    F32 advance = 0.0f;
    for (U64 i = 0; i < text.letter_count; ++i) {
        FontCache_Letter *letter = &text.letters[i];
        render_rectangle(
            v2f32(
                origin.x + letter->offset.x + advance,
                origin.y + letter->offset.y
            ),
            v2f32(
                origin.x + letter->offset.x + advance + letter->size.x,
                origin.y + letter->offset.y + letter->size.y
            ),
            .uv_min = letter->uvs.min, .uv_max = letter->uvs.max,
            .texture = letter->texture,
            .flags = Render_RectangleFlags_AlphaMask
        );
        advance += letter->advance;
    }

    arena_end_temporary(scratch);
}

internal UI_BOX_DRAW_FUNCTION(draw_ui_msdf) {
    UIDrawMSDF *draw_msdf = (UIDrawMSDF *) data;

    StringDecode decode = string_decode_utf8(box->string.data, box->string.size);
    U32 codepoint = decode.codepoint;
    if (codepoint > array_count(draw_msdf->font->glyphs)) {
        codepoint = 0;
    }

    Glyph *glyph = &draw_msdf->font->glyphs[codepoint];

    render_rectangle(
        box->calculated_rectangle.min,
        box->calculated_rectangle.max,
        .uv_min = glyph->uv_min, .uv_max = glyph->uv_max,
        .texture = draw_msdf->font->atlas,
        .flags = (draw_msdf->render_raw ? Render_RectangleFlags_Texture : Render_RectangleFlags_MSDF)
    );
}

internal Void draw_ui(UI_Box *box) {
    if (box->flags & UI_BoxFlags_DrawBackground) {
        render_rectangle(
            box->calculated_rectangle.min, box->calculated_rectangle.max,
            .color = box->color,
        );

        if (box->flags & UI_BoxFlags_DrawHot && box->hot_t > 0.0f) {
            Render_Rectangle *rect = render_rectangle(
                box->calculated_rectangle.min, box->calculated_rectangle.max
            );
            rect->colors[0] = v4f32(1.0f, 1.0f, 1.0f, 0.5f * box->hot_t);
            rect->colors[1] = v4f32(1.0f, 1.0f, 1.0f, 0.5f * box->hot_t);
            rect->colors[2] = v4f32(0.0f, 0.0f, 0.0f, 0.0f);
            rect->colors[3] = v4f32(0.0f, 0.0f, 0.0f, 0.0f);
        }

        if (box->flags & UI_BoxFlags_DrawActive && box->active_t > 0.0f) {
            Render_Rectangle *rect = render_rectangle(
                box->calculated_rectangle.min, box->calculated_rectangle.max
            );
            rect->colors[0] = v4f32(0.0f, 0.0f, 0.0f, 0.0f);
            rect->colors[1] = v4f32(0.0f, 0.0f, 0.0f, 0.0f);
            rect->colors[2] = v4f32(0.0f, 0.0f, 0.0f, 0.5f * box->active_t);
            rect->colors[3] = v4f32(0.0f, 0.0f, 0.0f, 0.5f * box->active_t);
        }
    }

    if (box->flags & UI_BoxFlags_DrawText) {
        V2F32 origin = box->calculated_rectangle.min;
        F32 advance = 0.0f;
        for (U64 i = 0; i < box->text.letter_count; ++i) {
            FontCache_Letter *letter = &box->text.letters[i];
            render_rectangle(
                v2f32(
                    f32_floor(origin.x + letter->offset.x + advance),
                    f32_floor(origin.y + letter->offset.y)
                ),
                v2f32(
                    f32_floor(origin.x + letter->offset.x + advance + letter->size.x),
                    f32_floor(origin.y + letter->offset.y + letter->size.y)
                ),
                .color = box->text_color,
                .uv_min = letter->uvs.min, .uv_max = letter->uvs.max,
                .texture = letter->texture,
                .flags = Render_RectangleFlags_AlphaMask
            );
            advance += letter->advance;
        }
    }

    if (box->draw_function) {
        box->draw_function(box, box->draw_data);
    }

    if (box->flags & UI_BoxFlags_DrawBorder) {
        render_rectangle(
            box->calculated_rectangle.min, box->calculated_rectangle.max,
            .color = box->border_color,
            .thickness = 1.0f,
            .softness = 1.0f
        );
    }

    if (box->flags & UI_BoxFlags_Disabled) {
        Render_Rectangle *rect = render_rectangle(
            box->calculated_rectangle.min, box->calculated_rectangle.max,
            .color = v4f32(0.2f, 0.2f, 0.2f, 0.75f)
        );
    }

    for (UI_Box *child = box->last; child != &global_ui_null_box; child = child->previous) {
        draw_ui(child);
    }
}

internal Void build_glyph_view(UI_Context *ui, Theme *theme, UIDrawMSDF *draw_msdf, U32 *selected_codepoint) {
    // NOTE(simon): Scroll region
    ui_width_next(ui, ui_size_parent_percent(1.0f, 0.0f));
    ui_height_next(ui, ui_size_children_sum(1.0f));
    ui_layout_axis_next(ui, Axis2_X);
    UI_Box *region = ui_create_box_from_string(ui, UI_BoxFlags_OverflowY | UI_BoxFlags_Scrollable, str8_literal("region"));
    ui_parent_push(ui, region);

    // NOTE(simon): Scroll container
    ui_color_next(ui, theme->background_color);
    ui_width_next(ui, ui_size_fill());
    ui_height_next(ui, ui_size_parent_percent(1.0f, 0.0f));
    ui_layout_axis_next(ui, Axis2_Y);
    UI_Box *container = ui_create_box_from_string(ui, UI_BoxFlags_DrawBackground, str8_literal("glyphs"));



    local U32 scroll_codepoint = 0;
    local F32 scroll_offset = 0.0f;

    U32 first_codepoint = 0x000000;
    U32 last_codepoint  = 2000;

    F32 preferred_width = 50.0f;
    U32 codepoints_per_row = (U32) f32_floor(container->calculated_size.width / preferred_width);
    if (!codepoints_per_row) {
        codepoints_per_row = 10;
    }
    F32 width = container->calculated_size.width / (F32) codepoints_per_row;
    F32 height = width * 2.0f;

    S32 first_row = 0;
    S32 last_row  = (S32) ((last_codepoint + codepoints_per_row - 1) / codepoints_per_row);

    S32 scroll_row = (S32) (scroll_codepoint / codepoints_per_row);
    S32 target_row = scroll_row;

    S32 top_row    = scroll_row + (S32) (scroll_offset < 0.0f ? f32_ceil(scroll_offset - 1.0f) : f32_floor(scroll_offset));
    S32 bottom_row = s32_min(top_row + (S32) f32_ceil(container->calculated_size.height / height) + 1, last_row);
    container->view_offset.y = height * (f32_mod(scroll_offset, 1.0f) + (scroll_offset < 0.0f));

    S32 selected_row = (S32) (*selected_codepoint / codepoints_per_row);



    // NOTE(simon): Scrollbar container
    ui_color_next(ui, theme->background_color);
    ui_border_color_next(ui, theme->border_color);
    ui_width_next(ui, ui_size_pixels(20.0f, 0.0f));
    ui_height_next(ui, ui_size_parent_percent(1.0f, 0.0f));
    ui_layout_axis_next(ui, Axis2_Y);
    UI_Box *scroll_container = ui_create_box_from_string(ui, UI_BoxFlags_DrawBackground | UI_BoxFlags_DrawBorder, str8_literal("scrollbar"));

    ui_width(ui, ui_size_parent_percent(1.0f, 1.0f))
    ui_parent(ui, scroll_container) {
        F32 rows_above   = (F32) (scroll_row - first_row) + scroll_offset;
        F32 visible_rows = container->calculated_size.height / height;
        F32 row_count    = (F32) (last_row - first_row) + visible_rows - 1.0f;
        F32 rows_below   = (F32) (last_row - first_row) - 1.0f - (F32) scroll_row - scroll_offset;

        ui_hover_cursor_next(ui, Gfx_Cursor_Hand);
        ui_height_next(ui, ui_size_parent_percent(rows_above / row_count, 1.0f));
        UI_Box *scroll_before = ui_create_box_from_string(ui, UI_BoxFlags_Clickable, str8_literal("before"));

        ui_hover_cursor_next(ui, Gfx_Cursor_Hand);
        ui_color_next(ui, theme->element_color);
        ui_border_color_next(ui, theme->border_color);
        ui_height_next(ui, ui_size_parent_percent(visible_rows / row_count, 1.0f));
        UI_Box *scroll = ui_create_box_from_string(ui, UI_BoxFlags_Clickable | UI_BoxFlags_DrawBackground | UI_BoxFlags_DrawBorder | UI_BoxFlags_DrawHot | UI_BoxFlags_DrawActive, str8_literal("scroll"));

        ui_hover_cursor_next(ui, Gfx_Cursor_Hand);
        ui_height_next(ui, ui_size_parent_percent(rows_below / row_count, 1.0f));
        UI_Box *scroll_after = ui_create_box_from_string(ui, UI_BoxFlags_Clickable, str8_literal("after"));

        UI_Input before_input = ui_input_from_box(ui, scroll_before);
        if (before_input.input_flags & UI_InputFlag_LeftClicked) {
            target_row -= (S32) f32_floor(visible_rows);
        }

        UI_Input scroll_input = ui_input_from_box(ui, scroll);
        if (scroll_input.input_flags & UI_InputFlag_Dragging) {
            local S32 start_row = 0;
            if (scroll_input.input_flags & UI_InputFlag_Pressed) {
                start_row = top_row;
            }

            F32 scroll_size = scroll_container->calculated_size.height - scroll->calculated_size.height;
            F32 drag_percent = ui_drag_delta(ui).y / scroll_size;
            target_row = start_row + (S32) f32_floor(drag_percent * row_count);
        }

        UI_Input after_input  = ui_input_from_box(ui, scroll_after);
        if (after_input.input_flags & UI_InputFlag_LeftClicked) {
            target_row += (S32) f32_floor(visible_rows);
        }
    }

    ui_parent_push(ui, container);

    ui_color_push(ui, theme->element_color);
    ui_border_color_push(ui, theme->border_color);
    for (S32 row = top_row; row < bottom_row; ++row) {
        ui_width_next(ui, ui_size_parent_percent(1.0f, 1.0f));
        ui_height_next(ui, ui_size_pixels(height, 1.0f));
        ui_row(ui) {
            ui_width(ui, ui_size_pixels(width, 1.0f))
            ui_height(ui, ui_size_parent_percent(1.0f, 1.0f))
            ui_draw_function(ui, draw_ui_msdf)
            ui_draw_data(ui, draw_msdf)
            ui_hover_cursor(ui, Gfx_Cursor_Hand)
            for (U32 column = 0; column < codepoints_per_row; ++column) {
                U32 codepoint = column + (U32) row * codepoints_per_row;
                if (codepoint > last_codepoint) {
                    break;
                }

                U8 buffer[4] = { 0 };
                U64 size = string_encode_utf8(buffer, codepoint);
                Str8 string = str8(buffer, size);

                UI_Box *box = ui_create_box_from_string(
                    ui,
                    UI_BoxFlags_DrawBackground | UI_BoxFlags_DrawBorder |
                    UI_BoxFlags_DrawHot | UI_BoxFlags_DrawActive |
                    UI_BoxFlags_Clickable,
                    string
                );
                UI_Input input = ui_input_from_box(ui, box);

                if (input.input_flags & UI_InputFlag_LeftClicked) {
                    *selected_codepoint = codepoint;
                }
            }
        }
    }
    ui_border_color_pop(ui);
    ui_color_pop(ui);

    // NOTE(simon): Container
    ui_parent_pop(ui);

    // NOTE(simon): Region
    ui_parent_pop(ui);

    UI_Input region_input = ui_input_from_box(ui, region);
    target_row -= (S32) region_input.scroll.y;

    // NOTE(simon): Updating scroll
    target_row = s32_min(s32_max(first_row, target_row), last_row - 1);
    scroll_offset += (F32) scroll_row - (F32) target_row;
    scroll_row = target_row;
    scroll_codepoint = (U32) scroll_row * codepoints_per_row;

    // NOTE(simon): Animation
    scroll_offset += -scroll_offset * ui->slow_rate;
}

internal S32 os_run(Str8List arguments) {
    if (!arguments.first->next) {
        os_console_print(str8_literal("You have to pass a file\n"));
        os_exit(1);
    }

    Arena *arena = arena_create();

    // NOTE(simon): Themes
    {
        global_themes[0].name = str8_literal("Light theme");
        global_themes[0].background_color = color_from_srgba_u32(0xF8F9FAFF); // OC Gray 0
        global_themes[0].element_color    = color_from_srgba_u32(0xE9ECEFFF); // OC Gray 2
        global_themes[0].border_color     = color_from_srgba_u32(0xCED4DAFF); // OC Gray 4
        global_themes[0].text_color       = color_from_srgba_u32(0x495057FF); // OC Gray 7

        global_themes[1].name = str8_literal("Dark theme"),
        global_themes[1].background_color = color_from_srgba_u32(0x212529FF); // OC Gray 9
        global_themes[1].element_color    = color_from_srgba_u32(0x343A40FF); // OC Gray 8
        global_themes[1].border_color     = color_from_srgba_u32(0x495057FF); // OC Gray 7
        global_themes[1].text_color       = color_from_srgba_u32(0xF8F9FAFF); // OC Gray 0
    }

    render_init();
    UI_Context *ui = ui_create();

    Gfx_Context *gfx = gfx_create(arena, str8_literal("MSDF-gen"), 1280, 720);
    if (gfx->errors.node_count) {
        os_console_print(str8_join(arena, &gfx->errors));
        return -1;
    }
    render_create(gfx);

    Font font = { 0 };
    load_font(arguments.first->next->string, &font);

    font_cache_create();

    B32 running = true;

    Arena *current_arena  = arena_create();
    Arena *previous_arena = arena_create();

    UIDrawMSDF draw_msdf = { 0 };
    draw_msdf.font = &font;

    while (running) {
        Gfx_EventList events = gfx_get_events(current_arena, gfx);

        for (Gfx_Event *event = events.first, *next = 0; event; event = next) {
            next = event->next;

            if (event->kind == Gfx_EventKind_KeyRelease && event->key == Gfx_Key_Tab) {
                draw_msdf.render_raw ^= 1;
                dll_remove(events.first, events.last, event);
            }
        }


        // NOTE(simon): UI build
        {
            prof_zone_begin(prof_ui_build, "ui_build");
            ui_begin(gfx, ui, &events, 1.0f / 60.0f);

            Theme *theme = &global_themes[1];

            local U32 selected_codepoint = 0;
            build_glyph_view(ui, theme, &draw_msdf, &selected_codepoint);

            ui_width_next(ui, ui_size_parent_percent(0.25f, 1.0f));
            ui_height_next(ui, ui_size_parent_percent(1.0f, 1.0f));
            ui_color_next(ui, theme->background_color);
            ui_extra_box_flags_next(ui, UI_BoxFlags_DrawBackground);
            ui_column(ui) {
                ui_width(ui, ui_size_text_content(0.0f, 1.0f))
                ui_height(ui, ui_size_text_content(0.0f, 1.0f))
                ui_color(ui, theme->element_color)
                ui_border_color(ui, theme->border_color) {
                    ui_width_next(ui, ui_size_parent_percent(1.0f, 0.0f));
                    ui_height_next(ui, ui_size_parent_percent(1.0f, 0.0f));
                    ui_draw_function_next(ui, draw_ui_msdf);
                    ui_draw_data_next(ui, &draw_msdf);
                    U8 buffer[4] = { 0 };
                    U64 size = string_encode_utf8(buffer, selected_codepoint);
                    Str8 string = str8(buffer, size);
                    ui_create_box_from_string(ui, 0, string);

                    ui_label_format(ui, "Selected glyph: U+%.6X", selected_codepoint);

                    Render_Stats stats = render_get_stats();
                    ui_label_format(ui, "Batches: %u", stats.batch_count);
                    ui_label_format(ui, "Rectangles: %u", stats.rectangle_count);
                }
            }

            ui_end(gfx, ui);
            prof_zone_end(prof_ui_build);
        }

        for (Gfx_Event *event = events.first, *next; event; event = next) {
            next = event->next;
            B32 consumed = false;

            if (event->kind == Gfx_EventKind_Quit) {
                running = false;
                consumed = true;
            }

            if (consumed) {
                dll_remove(events.first, events.last, event);
            }
        }

        {
            prof_zone_begin(prof_render, "render");
            V2U32 client_area = gfx_get_window_client_area(gfx);
            render_begin(client_area);
            draw_ui(ui->root);
            render_end();
            prof_zone_end(prof_render);
        }

        arena_reset(previous_arena);
        swap(current_arena, previous_arena, Arena *);
        prof_frame_done();
    }

    return 0;
}
