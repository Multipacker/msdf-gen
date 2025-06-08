#ifndef FONT_CORE_H
#define FONT_CORE_H

typedef struct Font_Raster Font_Raster;

typedef struct Font_Metrics Font_Metrics;
struct Font_Metrics {
    S32 ascent;
    S32 descent;
    U32 units_per_em;
};

internal Font_Raster *raster_load(Arena *arena, Str8 data);
internal Font_Metrics raster_get_font_metrics(Font_Raster *font);
internal MSDF_RasterResult raster_generate(Arena *arena, Font_Raster *font, U32 codepoint, U32 size);

#endif // FONT_CORE_H
