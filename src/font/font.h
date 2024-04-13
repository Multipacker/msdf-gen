#ifndef FONT_H
#define FONT_H

typedef struct {
    U32 advance_width;
    S32 left_side_bearing;

    F32 x_min;
    F32 y_min;
    F32 x_max;
    F32 y_max;

    V2F32 uv0;
    V2F32 uv1;
} Font_Glyph;

typedef struct {
    U32 funits_per_em;

    U32 codepoint_count;
    U32 *codepoints;
    U32 *glyph_indicies;

    U32 glyph_count;
    Font_Glyph *glyphs;
} Font;

typedef struct {
    U32 codepoint_first;
    U32 codepoint_last;
} FontDescription;

internal U32 font_codepoint_to_glyph_index(Font *font, U32 codepoint);

#endif // FONT_H
