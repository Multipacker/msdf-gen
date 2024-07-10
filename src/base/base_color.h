#ifndef BASE_COLOR_H
#define BASE_COLOR_H

internal F32 f32_linear_from_srgb(F32 srgb);
internal F32 f32_srgb_from_linear(F32 srgb);

internal V4F32 color_from_srgba_u8(U8 red, U8 green, U8 blue, U8 alpha);
internal V4F32 color_from_srgba_u32(U32 rgba);

internal U32 u32_srgba_from_color(V4F32 color);

#endif // BASE_COLOR_H
