#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct Font_Raster {
    stbtt_fontinfo font_info;
    B32 initialized;
};

internal Font_Raster *raster_load(Arena *arena, Str8 path) {
    Font_Raster *result = arena_push_struct_zero(arena, Font_Raster);

    Str8 buffer = { 0 };
    if (os_file_read(arena, path, &buffer)) {
        int loaded = stbtt_InitFont(&result->font_info, buffer.data, 0);
        result->initialized = loaded != 0;
    }

    return result;
}

// TODO(simon): Which units do we return this in?
internal Void raster_get_font_metrics(Font_Raster *font) {
    if (font->initialized) {
        int ascent = 0;
        int descent = 0;
        int line_gap = 0;
        stbtt_GetFontVMetrics(&font->font_info, &ascent, &descent, &line_gap);
    }
}

internal MSDF_RasterResult raster_generate(Arena *arena, Font_Raster *font, U32 codepoint, V2F32 scale) {
    MSDF_RasterResult result = { 0 };

    if (font->initialized) {
        int glyph_index = stbtt_FindGlyphIndex(&font->font_info, (int) codepoint);

        int x_min = 0;
        int y_min = 0;
        int x_max = 0;
        int y_max = 0;
        stbtt_GetGlyphBitmapBox(&font->font_info, glyph_index, scale.x, scale.y, &x_min, &y_min, &x_max, &y_max);

        int width  = x_max - x_min;
        int height = y_max - y_min;
        result.data = arena_push_array_zero(arena, U8, width * height);
        stbtt_MakeGlyphBitmap(&font->font_info, result.data, width, height, width, scale.x, scale.y, glyph_index);

        int advance_width     = 0;
        int left_side_bearing = 0;
        stbtt_GetGlyphHMetrics(&font->font_info, glyph_index, &advance_width, &left_side_bearing);

        result.x_min             = x_min;
        result.y_min             = y_min;
        result.x_max             = x_max;
        result.y_max             = y_max;
        result.advance_width     = scale.x * advance_width;
        result.left_side_bearing = scale.x * left_side_bearing;
    }

    return result;
}
