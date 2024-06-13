#include "src/base/base_include.h"
#include "src/graphics/graphics_include.h"
#include "src/render/render_include.h"
#include "src/font/font_include.h"

#include "src/base/base_include.c"
#include "src/graphics/graphics_include.c"
#include "src/render/render_include.c"
#include "src/font/font_include.c"

typedef struct {
    V2F32 min_pt;
    V2F32 max_pt;
    F32   advance_pt;
    V2F32 uv_min;
    V2F32 uv_max;
} Glyph;

typedef struct {
    Render_Texture atlas;
    Glyph glyphs[128];
} Font;

internal Void load_font(Render_Context *render, Str8 font_path, Font *result) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    U32 glyph_size     = 32;
    U32 glyphs_per_row = 16;
    U32 atlas_size     = glyph_size * glyphs_per_row;
    result->atlas = render_texture_create(render, v2u32(atlas_size, atlas_size), 0);

    TTF_Font *font = ttf_load(scratch.arena, font_path);
    if (font->errors.node_count == 0) {
        // Generate glyphs
        for (U32 codepoint = 0; codepoint < 128; ++codepoint) {
            Arena_Temporary glyph_scratch = arena_get_scratch(&scratch.arena, 1);

            MSDF_RasterResult raster_result = msdf_generate(glyph_scratch.arena, font, codepoint, glyph_size);

            V2U32 atlas_position = v2u32(
                glyph_size * (codepoint % glyphs_per_row),
                glyph_size * (codepoint / glyphs_per_row)
            );
            render_texture_update(render, result->atlas, atlas_position, v2u32(glyph_size, glyph_size), raster_result.data);

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

internal S32 os_run(Str8List arguments) {
    if (!arguments.first->next) {
        os_console_print(str8_literal("You have to pass a file\n"));
        os_exit(1);
    }

    Arena *arena = arena_create();

    render_init();

    Gfx_Context *gfx = gfx_create(arena, str8_literal("MSDF-gen"), 1280, 720);
    if (gfx->errors.node_count) {
        os_console_print(str8_join(arena, &gfx->errors));
        return -1;
    }
    Render_Context *render = render_create(gfx);

    Font font = { 0 };
    load_font(render, arguments.first->next->string, &font);

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
        for (Gfx_Event *event = events.first; event; event = event->next) {
            if (event->kind == Gfx_EventKind_Quit) {
                running = false;
            } else if (event->kind == Gfx_EventKind_Scroll) {
                F32 old_zoom = zoom;

                zoom *= f32_pow(0.97f, event->scroll.y);

                offset = v2f32_subtract(mouse, v2f32_scale(v2f32_subtract(mouse, offset), old_zoom / zoom));
            } else if (event->kind == Gfx_EventKind_KeyRelease && event->key == Gfx_Key_Tab) {
                render_msdf = !render_msdf;
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
        render_begin(render, client_area);

        V2U32 texture_size = render_size_from_texture(font.atlas);
        render_rectangle(
            render,
            offset, v2f32_add(offset, v2f32((F32) texture_size.width / zoom, (F32) texture_size.height / zoom)),
            .uv_min = v2f32(0, 0), .uv_max = v2f32(1, 1),
            .texture = font.atlas,
            .color = v4f32(1.0f, 1.0f, 1.0f, 1.0f),
            .flags = (render_msdf ? Render_RectangleFlags_MSDF : Render_RectangleFlags_Texture)
        );

        Str8 string = str8_literal("MSDF-based text rendering");
        V2F32 text_point = offset;
        F32 point_size = 50.0f / zoom;
        for (U8 *ptr = string.data, *opl = string.data + string.size; ptr < opl; ) {
            StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
            ptr += decode.size;

            Glyph *glyph = &font.glyphs[decode.codepoint];

            render_rectangle(
                render,
                v2f32_add(text_point, v2f32_scale(glyph->min_pt, point_size)), v2f32_add(text_point, v2f32_scale(glyph->max_pt, point_size)),
                .uv_min = glyph->uv_min, .uv_max = glyph->uv_max,
                .texture = font.atlas,
                .color = v4f32(1.0f, 1.0f, 1.0f, 1.0f),
                .flags = (render_msdf ? Render_RectangleFlags_MSDF : Render_RectangleFlags_Texture)
            );

            text_point.x += glyph->advance_pt * point_size;
        }

        render_end(render);

        arena_reset(previous_arena);
        swap(current_arena, previous_arena, Arena *);
    }

    return 0;
}
