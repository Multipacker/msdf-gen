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

#endif // FONT_H
