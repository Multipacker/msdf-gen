#include "src/base/base_include.h"
#include "src/graphics/graphics_include.h"
#include "src/render/render_include.h"
#include "src/font/font_include.h"

#include "src/base/base_include.c"
#include "src/graphics/graphics_include.c"
#include "src/render/render_include.c"
#include "src/font/font_include.c"

/*
 * /usr/share/fonts/TTF/FiraMono-Regular.ttf
 * /usr/share/fonts/TTF/Inconsolata-Regular.ttf
 * /usr/share/fonts/ttf-linux-libertine/LinLibertine_Mah.ttf
 * /usr/share/fonts/noto/NotoSerif-Regular.ttf
 */
internal S32 os_run(Str8List arguments) {
    Arena *arena = arena_create();

    Gfx_Context *gfx = gfx_create(arena, str8_literal("MSDF-gen"), 1280, 720);
    if (!gfx) {
        os_console_print(error_get_error_message());
        return -1;
    }
    Render_Context *render = render_create(gfx);

    U32 glyph_size     = 32;
    U32 glyphs_per_row = 16;
    U32 atlas_size     = glyph_size  * glyphs_per_row;
    Render_Texture texture = render_texture_create(render, v2u32(atlas_size, atlas_size), 0);

    if (!arguments.first->next) {
        os_console_print(str8_literal("You have to pass a file\n"));
        os_exit(1);
    }

    Str8 font_path = arguments.first->next->string;

    TTF_Font font = { 0 };
    if (!ttf_load(arena, font_path, &font)) {
        os_console_print(error_get_error_message());
        return -1;
    }

    // Generate glyphs
    for (U32 codepoint = 0; codepoint < 127; ++codepoint) {
        Arena_Temporary scratch = arena_get_scratch(0, 0);

        MSDF_RasterResult raster_result = msdf_generate(scratch.arena, &font, codepoint, glyph_size);

        V2U32 atlas_position = v2u32(
            glyph_size * (codepoint % glyphs_per_row),
            glyph_size * (codepoint / glyphs_per_row)
        );
        render_texture_update(render, texture, atlas_position, v2u32(glyph_size, glyph_size), raster_result.data);

        arena_end_temporary(scratch);
    }

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

                zoom *= f32_pow(0.97, event->scroll.y);

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

        render_rectangle(
            render,
            offset, v2f32_add(offset, v2f32(atlas_size / zoom, atlas_size / zoom)),
            .uv_min = v2f32(0, 0), .uv_max = v2f32(1, 1),
            .texture = texture,
            .color = v4f32(1.0, 1.0, 1.0, 1.0),
            .flags = (render_msdf ? Render_RectangleFlags_MSDF : Render_RectangleFlags_Texture)
        );

        render_end(render);

        arena_reset(previous_arena);
        swap(current_arena, previous_arena, Arena *);
    }

    return 0;
}
