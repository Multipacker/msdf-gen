#ifndef FONT_INCLUDE_H
#define FONT_INCLUDE_H

#include "ttf.h"
#include "msdf.h"

typedef struct Font_Raster Font_Raster;

internal Font_Raster *raster_load(Arena *arena, Str8 path);
internal Void raster_get_font_metrics(Font_Raster *font);
internal MSDF_RasterResult raster_generate(Arena *arena, Font_Raster *font, U32 codepoint, U32 size);

#endif // FONT_INCLUDE_H
