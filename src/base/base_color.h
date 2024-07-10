#ifndef BASE_COLOR_H
#define BASE_COLOR_H

internal F32 f32_srgb_to_linear(F32 srgb);

internal V4F32 color_from_srgb_u8(U8 red, U8 green, U8 blue, U8 alpha);
internal V4F32 color_from_srgb_u32(U32 rgba);

#endif // BASE_COLOR_H
