#ifndef BASE_COLOR_H
#define BASE_COLOR_H

/*
 * NOTE(simon):
 * Color is assumed to have all components be in [0, 1]
 *
 * sRGBA is assumed to have all components be in [0, 1]
 *
 * HSVa is assumed to have its components in the following ranges
 *   Hue        [0, 360)
 *   Saturation [0, 1]
 *   Value      [0, 1]
 *   Alpha      [0, 1]
 *
 * When a color is represented as a U32, red occupies the highest 8 bits, then
 * green, blue, and alpha, in that order.
 */

internal F32 f32_linear_from_srgb(F32 srgb);
internal F32 f32_srgb_from_linear(F32 srgb);

internal V4F32 color_from_srgba(V4F32 srgba);
internal V4F32 color_from_srgba_u8(U8 red, U8 green, U8 blue, U8 alpha);
internal V4F32 color_from_srgba_u32(U32 rgba);

internal V4F32 srgba_from_color(V4F32 color);
internal U32   u32_srgba_from_color(V4F32 color);

internal V4F32 srgba_from_hsva(V4F32 hsva);
internal V4F32 hsva_from_srgba(V4F32 srgba);

#endif // BASE_COLOR_H
