internal F32 f32_linear_from_srgb(F32 srgb) {
    F32 linear = 0.0f;

    if (srgb <= 0.04045f) {
        linear = srgb / 12.92f;
    } else {
        linear = f32_pow((srgb + 0.055f) / 1.055f, 2.4f);
    }

    return linear;
}

internal F32 f32_srgb_from_linear(F32 linear) {
    F32 srgb = 0.0f;

    if (linear <= 0.0031308f) {
        srgb = 12.92f * linear;
    } else {
        srgb = 1.055f * f32_pow(linear, 1.0f / 2.4f) - 0.055f;
    }

    return srgb;
}



internal V4F32 color_from_srgba_u8(U8 red, U8 green, U8 blue, U8 alpha) {
    V4F32 result = v4f32(
        f32_linear_from_srgb((F32) red   / 255.0f),
        f32_linear_from_srgb((F32) green / 255.0f),
        f32_linear_from_srgb((F32) blue  / 255.0f),
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



internal U32 u32_srgba_from_color(V4F32 color) {
    V4F32 clamped = v4f32(
        f32_clamp(color.r, 0.0f, 1.0f),
        f32_clamp(color.g, 0.0f, 1.0f),
        f32_clamp(color.b, 0.0f, 1.0f),
        f32_clamp(color.a, 0.0f, 1.0f)
    );

    U32 red   = (U32) f32_round(f32_srgb_from_linear(clamped.r) * 255.0f);
    U32 green = (U32) f32_round(f32_srgb_from_linear(clamped.g) * 255.0f);
    U32 blue  = (U32) f32_round(f32_srgb_from_linear(clamped.b) * 255.0f);
    U32 alpha = (U32) f32_round(clamped.a * 255.0f);

    U32 srgba = red << 24 | blue << 16 | green << 8 | alpha;

    return srgba;
}
