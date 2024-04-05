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
    if (!arguments.first->next) {
        os_console_print(str8_literal("You have to pass a file\n"));
        os_exit(1);
    }

    FontDescription font_description = { 0 };
    font_description.path = arguments.first->next->string;
    font_description.codepoint_first = 0;
    font_description.codepoint_last  = 127;

    Arena_Temporary scratch = arena_get_scratch(0, 0);

    TTF_Font font = { 0 };
    ttf_load(scratch.arena, scratch.arena, &font_description, &font);

    U32 glyph_width  = u32_ceil_to_power_of_2((U32) f32_ceil(f32_sqrt(font.internal_glyph_count)));
    U32 glyph_height = u32_ceil_to_power_of_2(font.internal_glyph_count / glyph_width);
    U32 glyph_size   = 16;

    U32 atlas_width  = glyph_width  * glyph_size;
    U32 atlas_height = glyph_height * glyph_size;

    U8 *font_atlas = arena_push_array_zero(scratch.arena, U8, atlas_width * atlas_height * sizeof(U32));
    Font msdf_font = { 0 };
    {
        B32 success = true;

        msdf_font.funits_per_em = font.funits_per_em;

        // Copy over the character map.
        msdf_font.codepoint_count = font.codepoint_count;
        msdf_font.codepoints      = arena_push_array(scratch.arena, U32, msdf_font.codepoint_count);
        msdf_font.glyph_indicies  = arena_push_array(scratch.arena, U32, msdf_font.codepoint_count);
        memory_copy(msdf_font.codepoints, font.codepoints, msdf_font.codepoint_count * sizeof(*msdf_font.codepoints));
        memory_copy(msdf_font.glyph_indicies, font.glyph_indicies, msdf_font.codepoint_count * sizeof(*msdf_font.glyph_indicies));

        if (success) {
            msdf_font.glyph_count = font.internal_glyph_count;
            msdf_font.glyphs = arena_push_array(scratch.arena, Font_Glyph, msdf_font.glyph_count);
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

    Gfx_Image font_image = { .data = (U32 *) font_atlas, .width = atlas_width, .height = atlas_height, };
    GraphicsContext *context = graphics_create(str8_literal("MSDF-gen"), 1280, 720, font_image);
    arena_end_temporary(scratch);

    S32 offset_x = 0;
    S32 offset_y = 0;
    S32 grab_x   = 0;
    S32 grab_y   = 0;
    F32 zoom     = 2.0f;
    B32 running = true;
    B32 render_msdf = true;
    while (running) {
        graphics_begin_frame(context);

        if (context->is_grabbed) {
            offset_x += context->x - grab_x;
            offset_y += context->y - grab_y;
        }
        grab_x = context->x;
        grab_y = context->y;

        if (context->scroll_amount) {
            F32 old_zoom = zoom;

            zoom *= f32_pow(0.99, -context->scroll_amount);

            offset_x = context->x - old_zoom / zoom * (context->x - offset_x);
            offset_y = context->y - old_zoom / zoom * (context->y - offset_y);
        }

        if (context->tab_pressed) {
            render_msdf = !render_msdf;
        }
        graphics_set_render_msdf(context, render_msdf);

        graphics_msdf(context, v2f32(offset_x, offset_y), v2f32(100.0f / zoom, 100.0f / zoom), v3f32(1, 1, 1), v2f32(0, 0), v2f32(1, 1));

        if (context->exit_requested) {
            running = false;
        }

        graphics_end_frame(context);
    }

    graphics_destroy(context);
    return 0;
}
