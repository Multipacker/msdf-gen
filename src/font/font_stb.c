#if COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#if COMPILER_CLANG
#pragma clang diagnostic pop
#endif

struct Font_Raster {
    stbtt_fontinfo font_info;
    B32 initialized;
};

internal Font_Raster *raster_load(Arena *arena, Str8 data) {
    Font_Raster *result = arena_push_struct(arena, Font_Raster);

    int loaded = stbtt_InitFont(&result->font_info, data.data, stbtt_GetFontOffsetForIndex(data.data, 0));
    result->initialized = loaded != 0;

    return result;
}

internal Font_Metrics raster_get_font_metrics(Font_Raster *font) {
    Font_Metrics result = { 0 };
    if (font->initialized) {
        int ascent = 0;
        int descent = 0;
        int line_gap = 0;
        stbtt_GetFontVMetrics(&font->font_info, &ascent, &descent, &line_gap);

        result.ascent       = ascent;
        result.descent      = descent;
        result.units_per_em = (U32) (1.0f / stbtt_ScaleForMappingEmToPixels(&font->font_info, 1));
    }

    return result;
}

internal MSDF_RasterResult raster_generate(Arena *arena, Font_Raster *font, U32 codepoint, U32 size) {
    MSDF_RasterResult result = { 0 };

    if (font->initialized) {
        int glyph_index = stbtt_FindGlyphIndex(&font->font_info, (int) codepoint);
        result.glyph_index = (U32) glyph_index;

        F32 scale = stbtt_ScaleForMappingEmToPixels(&font->font_info, (F32) size);
        int x_min = 0;
        int y_min = 0;
        int x_max = 0;
        int y_max = 0;
        stbtt_GetGlyphBitmapBox(&font->font_info, glyph_index, scale, scale, &x_min, &y_min, &x_max, &y_max);

        int width  = x_max - x_min;
        int height = y_max - y_min;
        result.data = arena_push_array(arena, U8, (U64) (width * height));
        stbtt_MakeGlyphBitmap(&font->font_info, result.data, width, height, width, scale, scale, glyph_index);

        int advance_width     = 0;
        int left_side_bearing = 0;
        stbtt_GetGlyphHMetrics(&font->font_info, glyph_index, &advance_width, &left_side_bearing);

        result.min               = v2f32((F32) x_min, (F32) y_min);
        result.max               = v2f32((F32) x_max, (F32) y_max);
        result.size              = v2u32((U32) width, (U32) height);
        result.advance_width     = f32_floor(scale * (F32) advance_width);
        result.left_side_bearing = f32_floor(scale * (F32) left_side_bearing);
    }

    return result;
}
