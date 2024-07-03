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

typedef struct {
    V2F32 min_pt;
    V2F32 max_pt;
    F32   advance_pt;
    V2F32 uv_min;
    V2F32 uv_max;
} Glyph;

typedef struct {
    Render_Texture atlas;
    Glyph glyphs[256];
} Font;

internal Void load_font(Str8 font_path, Font *result) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    U32 glyph_size     = 32;
    U32 glyphs_per_row = 16;
    U32 atlas_size     = glyph_size * glyphs_per_row;
    result->atlas = render_texture_create(v2u32(atlas_size, atlas_size), Render_TextureFormat_RGBA8, 0);

    TTF_Font *font = ttf_load(scratch.arena, font_path);
    if (font->errors.node_count == 0) {
        // Generate glyphs
        for (U32 codepoint = 0; codepoint < 256; ++codepoint) {
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
                ((F32) atlas_position.x + 0.5f) / atlas_size,
                ((F32) atlas_position.y + 0.5f) / atlas_size
            );
            result->glyphs[codepoint].uv_max = v2f32(
                ((F32) atlas_position.x + glyph_size - 0.5f) / atlas_size,
                ((F32) atlas_position.y + glyph_size - 0.5f) / atlas_size
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

internal Void draw_ui(UI_Box *box) {
    if (box->flags & UI_BoxFlags_DrawBackground) {
        render_rectangle(
            box->calculated_rectangle.min, box->calculated_rectangle.max,
            .color = box->color
        );
    }

    if (box->flags & UI_BoxFlags_DrawText) {
        V2F32 origin = v2f32(
            box->calculated_rectangle.min.x + box->size[Axis2_X].value,
            box->calculated_rectangle.min.y + box->size[Axis2_Y].value
        );
        F32 advance = 0.0f;
        for (U64 i = 0; i < box->text.letter_count; ++i) {
            FontCache_Letter *letter = &box->text.letters[i];
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
    }

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

    if (box->flags & UI_BoxFlags_Disabled) {
        Render_Rectangle *rect = render_rectangle(
            box->calculated_rectangle.min, box->calculated_rectangle.max,
            .color = v4f32(0.2f, 0.2f, 0.2f, 0.75f)
        );
    }

    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        draw_ui(child);
    }
}

internal S32 os_run(Str8List arguments) {
    if (!arguments.first->next) {
        os_console_print(str8_literal("You have to pass a file\n"));
        os_exit(1);
    }

    Arena *arena = arena_create();

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

    V2F32 offset      = { 0 };
    F32   zoom        = 2.0f;
    B32   running     = true;
    B32   render_msdf = true;
    V2F32 grab        = { 0 };
    B32   dragging    = false;

    Arena *current_arena  = arena_create();
    Arena *previous_arena = arena_create();

    while (running) {
        Gfx_EventList events = gfx_get_events(current_arena, gfx);
        V2F32 mouse = gfx_get_mouse_position(gfx);
        local U32 test = 0;
        for (Gfx_Event *event = events.first; event; event = event->next) {
            if (event->kind == Gfx_EventKind_Quit) {
                running = false;
            } else if (event->kind == Gfx_EventKind_Scroll) {
                F32 old_zoom = zoom;

                zoom *= f32_pow(0.97f, event->scroll.y);

                offset = v2f32_subtract(mouse, v2f32_scale(v2f32_subtract(mouse, offset), old_zoom / zoom));
            } else if (event->kind == Gfx_EventKind_KeyRelease && event->key == Gfx_Key_Tab) {
                render_msdf = !render_msdf;
            } else if (event->kind == Gfx_EventKind_KeyRelease && event->key == Gfx_Key_A) {
                test = (test + 1) % 10;
            } else if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_MouseLeft) {
                dragging = true;
                grab = v2f32_subtract(offset, mouse);
            } else if (event->kind == Gfx_EventKind_KeyRelease && event->key == Gfx_Key_MouseLeft) {
                dragging = false;
            }
        }
        if (dragging) {
            offset = v2f32_add(grab, mouse);
        }

        V2U32 client_area = gfx_get_window_client_area(gfx);
        render_begin(client_area);

        V2U32 texture_size = render_size_from_texture(font.atlas);
        /*for (U32 y = 0; y < 16; ++y) {
            for (U32 x = 0; x < 16; ++x) {
                U32 codepoint = x + y * 16;
                Glyph *glyph = &font.glyphs[codepoint];
                render_rectangle(
                    v2f32_add(offset, v2f32(40 * x / zoom, 40 * y / zoom)),
                    v2f32_add(offset, v2f32(40 * (x + 1.0f) / zoom, 40 * (y + 1.0f) / zoom)),
                    .uv_min = glyph->uv_min, .uv_max = glyph->uv_max,
                    .texture = font.atlas,
                    .flags = (render_msdf ? Render_RectangleFlags_MSDF : Render_RectangleFlags_Texture)
                );
            }
        }*/

        FontCache_Font *test_font = font_cache_font_from_path(str8_literal("/usr/share/fonts/noto/NotoSerif-Regular.ttf"));
        FontCache_Text text = font_cache_text(current_arena, test_font, str8_literal("Text Örendering!"), 50);
        render_rectangle(
            offset,
            v2f32_add(offset, text.size),
            .color = v4f32(1, 0, 0, 1)
        );
        draw_text(test_font, offset, str8_literal("Text Örendering!"), 50);

        //draw_text_msdf(&font, offset, 50.0f / zoom, str8_literal("MSDF-based text rendering"));

        ui_begin(gfx, ui, 1.0f / 60.0f);
        ui_spacer_sized(ui, ui_size_fill());
        ui_width_push(ui, ui_size_parent_percent(render_msdf ? 0.25f : 0.0f, 1.0f));
        ui_height_push(ui, ui_size_parent_percent(1.0f, 1.0f));
        ui_color_push(ui, v4f32(0.4f, 0.4f, 0.4f, 1.0f));
        ui_extra_box_flags_next(ui, UI_BoxFlags_DrawBackground | UI_BoxFlags_AnimatePosition);
        ui_row_string(ui, str8_literal("test")) {
            ui_spacer_sized(ui, ui_size_fill());
            ui_width_push(ui, ui_size_children_sum(1.0f));
            ui_height_push(ui, ui_size_parent_percent(1.0f, 1.0f));
            ui_column(ui) {
                ui_spacer_sized(ui, ui_size_pixels(25.0f, 1.0f));
                ui_color_push(ui, v4f32(0.3f, 0.3f, 0.3f, 1.0f));
                ui_width_push(ui, ui_size_text_content(5.0f, 1.0f));
                ui_height_push(ui, ui_size_text_content(5.0f, 1.0f));
                for (U32 i = 0; i < 10; ++i) {
                    if (i == test) {
                        //Arena_Temporary scratch = arena_get_scratch(0, 0);
                        //ui->active_key = ui_key_from_string(str8_format(scratch.arena, "%u", i));
                        //arena_end_temporary(scratch);
                        ui_extra_box_flags_next(ui, UI_BoxFlags_Disabled);
                    }
                    UI_Box *item = ui_create_box_from_string_format(ui, UI_BoxFlags_DrawBackground | UI_BoxFlags_DrawText | UI_BoxFlags_DrawHot | UI_BoxFlags_DrawActive, "%u", i);
                    ui_box_set_string(ui, item, str8_literal("Hello"));
                    ui_spacer_sized(ui, ui_size_pixels(5.0f, 1.0f));
                }
            }
            ui_spacer_sized(ui, ui_size_fill());
        }
        ui_end(ui);

        draw_ui(ui->root);

        render_end();

        arena_reset(previous_arena);
        swap(current_arena, previous_arena, Arena *);
    }

    return 0;
}
