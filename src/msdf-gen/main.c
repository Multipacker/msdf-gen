#include "src/base/base_include.h"
#include "src/graphics/graphics_include.h"
#include "src/font/font_include.h"

#include "src/base/base_include.c"
#include "src/graphics/graphics_include.c"
#include "src/font/font_include.c"

/*
 * /usr/share/fonts/TTF/FiraMono-Regular.ttf
 * /usr/share/fonts/TTF/Inconsolata-Regular.ttf
 * /usr/share/fonts/ttf-linux-libertine/LinLibertine_Mah.ttf
 * /usr/share/fonts/noto/NotoSerif-Regular.ttf
 */
internal S32 os_run(Str8List arguments) {
    Arena arena = arena_create();

    if (!arguments.first->next) {
        os_console_print(str8_literal("You have to pass a file\n"));
        os_exit(1);
    }

    Str8 font_path = arguments.first->next->string;
    FontDescription font_description = { 0 };
    font_description.codepoint_first = 0;
    font_description.codepoint_last  = 127;

    TTF_Font font = { 0 };
    if (!ttf_load(&arena, font_path, &font)) {
        os_console_print(error_get_error_message());
    }
    if (!ttf_get_character_map(&arena, &font, &font_description)) {
        os_console_print(error_get_error_message());
    }

    U32 glyph_width  = u32_ceil_to_power_of_2((U32) f32_ceil(f32_sqrt(font.internal_glyph_count)));
    U32 glyph_height = u32_ceil_to_power_of_2(font.internal_glyph_count / glyph_width);
    U32 glyph_size   = 32;

    U32 atlas_width  = glyph_width  * glyph_size;
    U32 atlas_height = glyph_height * glyph_size;

    U8 *font_atlas = arena_push_array_zero(&arena, U8, atlas_width * atlas_height * sizeof(U32));
    Font msdf_font = { 0 };
    {
        B32 success = true;

        msdf_font.funits_per_em = font.funits_per_em;

        // Copy over the character map.
        msdf_font.codepoint_count = font.codepoint_count;
        msdf_font.codepoints      = arena_push_array(&arena, U32, msdf_font.codepoint_count);
        msdf_font.glyph_indicies  = arena_push_array(&arena, U32, msdf_font.codepoint_count);
        memory_copy(msdf_font.codepoints, font.codepoints, msdf_font.codepoint_count * sizeof(*msdf_font.codepoints));
        memory_copy(msdf_font.glyph_indicies, font.glyph_indicies, msdf_font.codepoint_count * sizeof(*msdf_font.glyph_indicies));

        if (success) {
            msdf_font.glyph_count = font.internal_glyph_count;
            msdf_font.glyphs = arena_push_array(&arena, Font_Glyph, msdf_font.glyph_count);
            success = ttf_parse_metric_data(&font, &msdf_font);
        }

        for (U32 i = 0; i < font.internal_glyph_count; ++i) {
            Arena_Temporary scratch = arena_get_scratch(0, 0);
            MSDF_Glyph msdf_glyph = ttf_expand_contours_to_msdf(scratch.arena, &font, font.internal_to_ttf_glyph_indicies[i]);
            msdf_resolve_contour_overlap(scratch.arena, &msdf_glyph);
            msdf_convert_to_simple_polygons(scratch.arena, &msdf_glyph);
            msdf_correct_contour_orientation(&msdf_glyph);
            msdf_color_edges(msdf_glyph);

            Font_Glyph *glyph = &msdf_font.glyphs[i];

            U32 x = glyph_size * (i % glyph_width);
            U32 y = glyph_size * (i / glyph_width);

            F32 width_adjustment  = (F32) (msdf_glyph.x_max - msdf_glyph.x_min) * 0.5f / (F32) (atlas_width  - 2);
            F32 height_adjustment = (F32) (msdf_glyph.y_max - msdf_glyph.y_min) * 0.5f / (F32) (atlas_height - 2);
            glyph->x_min = msdf_glyph.x_min - width_adjustment;
            glyph->y_min = msdf_glyph.y_min - height_adjustment;
            glyph->x_max = msdf_glyph.x_max + width_adjustment;
            glyph->y_max = msdf_glyph.y_max + height_adjustment;
            msdf_generate(msdf_glyph, font_atlas, atlas_width, x, y, glyph_size, glyph_size);

            arena_end_temporary(scratch);
        }
    }

    Gfx_Context *gfx = gfx_create(&arena, str8_literal("MSDF-gen"), 1280, 720);
    if (!gfx) {
        os_console_print(error_get_error_message());
        return -1;
    }

    Render_Texture texture = render_texture_create(gfx, v2u32(atlas_width, atlas_height), font_atlas);

    V2F32 offset    = { 0 };
    F32 zoom        = 2.0f;
    B32 running     = true;
    B32 render_msdf = true;
    V2F32 grab      = { 0 };
    B32 dragging    = false;

    while (running) {
        Arena_Temporary restore_point = arena_begin_temporary(&arena);
        Gfx_EventList events = gfx_get_events(&arena, gfx);
        V2F32 mouse = gfx_get_mouse_position(gfx);
        for (Gfx_Event *event = events.first; event; event = event->next) {
            if (event->kind == Gfx_EventKind_Quit) {
                running = false;
            } else if (event->kind == Gfx_EventKind_Scroll) {
                F32 old_zoom = zoom;

                zoom *= f32_pow(0.99, -event->scroll.y);

                offset.x = mouse.x - old_zoom / zoom * (mouse.x - offset.x);
                offset.y = mouse.y - old_zoom / zoom * (mouse.y - offset.y);
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
        arena_end_temporary(restore_point);

        render_begin(gfx);

        render_rectangle(
            gfx,
            offset, v2f32(offset.x + 100.0 / zoom, offset.y + 100.0 / zoom),
            .uv_min = v2f32(0, 0), .uv_max = v2f32(1, 1),
            .texture = texture,
            .color = v4f32(1.0, 1.0, 1.0, 1.0),
            .flags = (render_msdf ? Render_RectangleFlags_MSDF : Render_RectangleFlags_Texture)
        );

        render_end(gfx);
    }

    return 0;
}
