internal F32 f32_srgb_to_linear(F32 srgb) {
    F32 linear = 0.0f;

    if (srgb <= 0.04045f) {
        linear = srgb / 12.92f;
    } else {
        linear = f32_pow((srgb + 0.055f) / 1.055f, 2.4f);
    }

    return linear;
}

internal V4F32 color_from_srgba_u8(U8 red, U8 green, U8 blue, U8 alpha) {
    V4F32 result = v4f32(
        f32_srgb_to_linear((F32) red   / 255.0f),
        f32_srgb_to_linear((F32) green / 255.0f),
        f32_srgb_to_linear((F32) blue  / 255.0f),
        (F32) alpha / 255.0f
    );

    return result;
}

internal V4F32 color_from_srgba_u32(U32 rgba) {
    V4F32 result = color_from_srgba_u8(
        (rgba >> 24) & 0xFF,
        (rgba >> 16) & 0xFF,
        (rgba >>  8) & 0xFF,
        (rgba >>  0) & 0xFF
    );

    return result;
}
