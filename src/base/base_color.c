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



internal V4F32 color_from_srgba(V4F32 srgba) {
    V4F32 result = v4f32(
        f32_linear_from_srgb(srgba.r),
        f32_linear_from_srgb(srgba.g),
        f32_linear_from_srgb(srgba.b),
        srgba.a
    );
    return result;
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



internal V4F32 srgba_from_color(V4F32 color) {
    V4F32 result = v4f32(
        f32_srgb_from_linear(color.r),
        f32_srgb_from_linear(color.g),
        f32_srgb_from_linear(color.b),
        color.a
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



internal V4F32 srgba_from_hsva(V4F32 hsva) {
    F32 hue        = hsva.x;
    F32 saturation = hsva.y;
    F32 value      = hsva.z;
    F32 alpha      = hsva.w;

    F32 chroma    = value * saturation;
    F32 hue_prime = hue / 60.0f;
    F32 x         = chroma * (1.0f - f32_abs(f32_mod(hue_prime, 2.0f) - 1.0f));

    S32 sector = s32_min(s32_max(0, (S32) f32_floor(hue_prime)), 5);
    F32 reds[]   = { chroma, x,      0.0f,   0.0f,   x,      chroma, };
    F32 greens[] = { x,      chroma, chroma, x,      0.0f,   0.0f,   };
    F32 blues[]  = { 0.0f,   0.0f,   x,      chroma, chroma, x,      };

    F32 m = value - chroma;

    V4F32 result = v4f32(
        m + reds[sector],
        m + greens[sector],
        m + blues[sector],
        alpha
    );
    return result;
}

internal V4F32 hsva_from_srgba(V4F32 srgba) {
    F32 red   = srgba.r;
    F32 green = srgba.g;
    F32 blue  = srgba.b;
    F32 alpha = srgba.a;

    // NOTE(simon): This chosen to smaller than 1 / 256 with the reasoning that
    // humans have a hard time differentiating between colors that are closer
    // together than by that amount. The reason we need an epsilon at all is
    // because we otherwise would run into division by zero giving us either
    // NaN or inifity.
    F32 epsilon = 1.0f / 512.0f;

    F32 min = f32_min(f32_min(red, green), blue);
    F32 max = f32_max(f32_max(red, green), blue);

    F32 chroma     = max - min;
    F32 value      = max;
    F32 saturation = value < epsilon ? 0.0f : (chroma / value);

    F32 hue = 60.0f;
    if (chroma < epsilon) {
        hue *= 0.0f;
    } else if (value == red) {
        hue *= f32_mod(6.0f + (green - blue) / chroma, 6.0f);
    } else if (value == green) {
        hue *= 2.0f + (blue - red)   / chroma;
    } else if (value == blue) {
        hue *= 4.0f + (red  - green) / chroma;
    }

    V4F32 result = v4f32(
        hue,
        saturation,
        value,
        alpha
    );
    return result;
}
