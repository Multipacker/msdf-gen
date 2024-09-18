internal V2U8 v2u8(U8 x, U8 y) {
    V2U8 result = { 0 };

    result.x = x;
    result.y = y;

    return result;
}

internal V2U8 v2u8_add(V2U8 a, V2U8 b) {
    V2U8 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal V2U8 v2u8_subtract(V2U8 a, V2U8 b) {
    V2U8 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal V2U8 v2u8_min(V2U8 a, V2U8 b) {
    V2U8 result = { 0 };

    result.x = u8_min(a.x, b.x);
    result.y = u8_min(a.y, b.y);

    return result;
}

internal V2U8 v2u8_max(V2U8 a, V2U8 b) {
    V2U8 result = { 0 };

    result.x = u8_max(a.x, b.x);
    result.y = u8_max(a.y, b.y);

    return result;
}



internal V2U16 v2u16(U16 x, U16 y) {
    V2U16 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V2U16 v2u16_add(V2U16 a, V2U16 b) {
    V2U16 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal V2U16 v2u16_subtract(V2U16 a, V2U16 b) {
    V2U16 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal V2U16 v2u16_min(V2U16 a, V2U16 b) {
    V2U16 result = { 0 };

    result.x = u16_min(a.x, b.x);
    result.y = u16_min(a.y, b.y);

    return result;
}

internal V2U16 v2u16_max(V2U16 a, V2U16 b) {
    V2U16 result = { 0 };

    result.x = u16_max(a.x, b.x);
    result.y = u16_max(a.y, b.y);

    return result;
}



internal V2U32 v2u32(U32 x, U32 y) {
    V2U32 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V2U32 v2u32_add(V2U32 a, V2U32 b) {
    V2U32 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal V2U32 v2u32_subtract(V2U32 a, V2U32 b) {
    V2U32 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal V2U32 v2u32_min(V2U32 a, V2U32 b) {
    V2U32 result = { 0 };

    result.x = u32_min(a.x, b.x);
    result.y = u32_min(a.y, b.y);

    return result;
}

internal V2U32 v2u32_max(V2U32 a, V2U32 b) {
    V2U32 result = { 0 };

    result.x = u32_max(a.x, b.x);
    result.y = u32_max(a.y, b.y);

    return result;
}




internal V2U64 v2u64(U64 x, U64 y) {
    V2U64 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V2U64 v2u64_add(V2U64 a, V2U64 b) {
    V2U64 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal V2U64 v2u64_subtract(V2U64 a, V2U64 b) {
    V2U64 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal V2U64 v2u64_min(V2U64 a, V2U64 b) {
    V2U64 result = { 0 };

    result.x = u64_min(a.x, b.x);
    result.y = u64_min(a.y, b.y);

    return result;
}

internal V2U64 v2u64_max(V2U64 a, V2U64 b) {
    V2U64 result = { 0 };

    result.x = u64_max(a.x, b.x);
    result.y = u64_max(a.y, b.y);

    return result;
}



internal V2S8 v2s8(S8 x, S8 y) {
    V2S8 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V2S8 v2s8_add(V2S8 a, V2S8 b) {
    V2S8 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal V2S8 v2s8_subtract(V2S8 a, V2S8 b) {
    V2S8 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal V2S8 v2s8_min(V2S8 a, V2S8 b) {
    V2S8 result = { 0 };

    result.x = s8_min(a.x, b.x);
    result.y = s8_min(a.y, b.y);

    return result;
}

internal V2S8 v2s8_max(V2S8 a, V2S8 b) {
    V2S8 result = { 0 };

    result.x = s8_max(a.x, b.x);
    result.y = s8_max(a.y, b.y);

    return result;
}



internal V2S16 v2s16(S16 x, S16 y) {
    V2S16 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V2S16 v2s16_add(V2S16 a, V2S16 b) {
    V2S16 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal V2S16 v2s16_subtract(V2S16 a, V2S16 b) {
    V2S16 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal V2S16 v2s16_min(V2S16 a, V2S16 b) {
    V2S16 result = { 0 };

    result.x = s16_min(a.x, b.x);
    result.y = s16_min(a.y, b.y);

    return result;
}

internal V2S16 v2s16_max(V2S16 a, V2S16 b) {
    V2S16 result = { 0 };

    result.x = s16_max(a.x, b.x);
    result.y = s16_max(a.y, b.y);

    return result;
}



internal V2S32 v2s32(S32 x, S32 y) {
    V2S32 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V2S32 v2s32_add(V2S32 a, V2S32 b) {
    V2S32 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal V2S32 v2s32_subtract(V2S32 a, V2S32 b) {
    V2S32 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal V2S32 v2s32_min(V2S32 a, V2S32 b) {
    V2S32 result = { 0 };

    result.x = s32_min(a.x, b.x);
    result.y = s32_min(a.y, b.y);

    return result;
}

internal V2S32 v2s32_max(V2S32 a, V2S32 b) {
    V2S32 result = { 0 };

    result.x = s32_max(a.x, b.x);
    result.y = s32_max(a.y, b.y);

    return result;
}



internal V2S64 v2s64(S64 x, S64 y) {
    V2S64 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V2S64 v2s64_add(V2S64 a, V2S64 b) {
    V2S64 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal V2S64 v2s64_subtract(V2S64 a, V2S64 b) {
    V2S64 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal V2S64 v2s64_min(V2S64 a, V2S64 b) {
    V2S64 result = { 0 };

    result.x = s64_min(a.x, b.x);
    result.y = s64_min(a.y, b.y);

    return result;
}

internal V2S64 v2s64_max(V2S64 a, V2S64 b) {
    V2S64 result = { 0 };

    result.x = s64_max(a.x, b.x);
    result.y = s64_max(a.y, b.y);

    return result;
}



internal V2F32 v2f32(F32 x, F32 y) {
    V2F32 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V2F32 v2f32_add(V2F32 a, V2F32 b) {
    V2F32 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal V2F32 v2f32_subtract(V2F32 a, V2F32 b) {
    V2F32 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal V2F32 v2f32_scale(V2F32 vector, F32 scale) {
    V2F32 result = { 0 };

    result.x = scale * vector.x;
    result.y = scale * vector.y;

    return result;
}

internal F32 v2f32_length_squared(V2F32 vector) {
    F32 result = vector.x * vector.x + vector.y * vector.y;
    return result;
}

internal F32 v2f32_length(V2F32 vector) {
    F32 result = f32_sqrt(vector.x * vector.x + vector.y * vector.y);
    return result;
}

internal V2F32 v2f32_normalize(V2F32 vector) {
    F32 length = v2f32_length(vector);

    V2F32 result = { 0 };

    if (length > F32_EPSILON) {
        result.x = vector.x / length;
        result.y = vector.y / length;
    }

    return result;
}

internal F32 v2f32_dot(V2F32 a, V2F32 b) {
    return a.x * b.x + a.y * b.y;
}

internal F32 v2f32_cross(V2F32 a, V2F32 b) {
    return a.x * b.y - a.y * b.x;
}

internal V2F32 v2f32_negate(V2F32 vector) {
    V2F32 result = { 0 };

    result.x = -vector.x;
    result.y = -vector.y;

    return result;
}

internal V2F32 v2f32_perpendicular(V2F32 vector) {
    V2F32 result = { 0 };

    result.x =  vector.y;
    result.y = -vector.x;

    return result;
}

internal V2F32 v2f32_min(V2F32 a, V2F32 b) {
    V2F32 result = { 0 };

    result.x = f32_min(a.x, b.x);
    result.y = f32_min(a.y, b.y);

    return result;
}

internal V2F32 v2f32_max(V2F32 a, V2F32 b) {
    V2F32 result = { 0 };

    result.x = f32_max(a.x, b.x);
    result.y = f32_max(a.y, b.y);

    return result;
}



internal V3F32 v3f32(F32 x, F32 y, F32 z) {
    V3F32 result = { 0 };

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

internal V3F32 v3f32_add(V3F32 a, V3F32 b) {
    V3F32 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

internal V3F32 v3f32_subtract(V3F32 a, V3F32 b) {
    V3F32 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

internal V3F32 v3f32_scale(V3F32 vector, F32 scale) {
    V3F32 result = { 0 };

    result.x = scale * vector.x;
    result.y = scale * vector.y;
    result.z = scale * vector.z;

    return result;
}

internal F32 v3f32_length_squared(V3F32 vector) {
    F32 result = vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
    return result;
}

internal F32 v3f32_length(V3F32 vector) {
    F32 result = f32_sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
    return result;
}

internal V3F32 v3f32_normalize(V3F32 vector) {
    F32 length = v3f32_length(vector);

    V3F32 result = { 0 };

    if (length > F32_EPSILON) {
        result.x = vector.x / length;
        result.y = vector.y / length;
        result.z = vector.z / length;
    }

    return result;
}

internal F32 v3f32_dot(V3F32 a, V3F32 b) {
    F32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

internal V3F32 v3f32_cross(V3F32 a, V3F32 b) {
    V3F32 result = { 0 };

    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;

    return result;
}

internal V3F32 v3f32_negate(V3F32 vector) {
    V3F32 result = { 0 };

    result.x = -vector.x;
    result.y = -vector.y;
    result.z = -vector.z;

    return result;
}

internal V3F32 v3f32_min(V3F32 a, V3F32 b) {
    V3F32 result = { 0 };

    result.x = f32_min(a.x, b.x);
    result.y = f32_min(a.y, b.y);
    result.z = f32_min(a.z, b.z);

    return result;
}

internal V3F32 v3f32_max(V3F32 a, V3F32 b) {
    V3F32 result = { 0 };

    result.x = f32_max(a.x, b.x);
    result.y = f32_max(a.y, b.y);
    result.z = f32_max(a.z, b.z);

    return result;
}



internal V4F32 v4f32(F32 x, F32 y, F32 z, F32 w) {
    V4F32 result = { 0 };

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}



internal M2F32 m2f32(F32 m00, F32 m01, F32 m10, F32 m11) {
    M2F32 result = {
        .m = {
            { m00, m01, },
            { m10, m11, },
        },
    };

    return result;
}

internal V2F32 m2f32_multiply_v2f32(M2F32 matrix, V2F32 vector) {
    V2F32 result = {
        .x = vector.x * matrix.m[0][0] + vector.y * matrix.m[0][1],
        .y = vector.x * matrix.m[1][0] + vector.y * matrix.m[1][1],
    };
    return result;
}



internal M3F32 m3f32(F32 m00, F32 m01, F32 m02, F32 m10, F32 m11, F32 m12, F32 m20, F32 m21, F32 m22) {
    M3F32 result = {
        .m = {
            { m00, m01, m02, },
            { m10, m11, m12, },
            { m20, m21, m22, },
        },
    };

    return result;
}

internal M3F32 m3f32_identity(Void) {
    M3F32 result = {
        .m = {
            { 1.0f, 0.0f, 0.0f, },
            { 0.0f, 1.0f, 0.0f, },
            { 0.0f, 0.0f, 1.0f, },
        },
    };

    return result;
}

internal M3F32 m3f32_translation(V2F32 offset) {
    M3F32 result = {
        .m = {
            { 1.0f, 0.0f, offset.x, },
            { 0.0f, 1.0f, offset.y, },
            { 0.0f, 0.0f,     1.0f, },
        },
    };

    return result;
}

internal M3F32 m3f32_scale(V2F32 scale) {
    M3F32 result = {
        .m = {
            { scale.x,    0.0f, 0.0f, },
            {    0.0f, scale.y, 0.0f, },
            {    0.0f,    0.0f, 1.0f, },
        },
    };

    return result;
}

internal M3F32 m3f32_multiply_m3f32(M3F32 a, M3F32 b) {
    M3F32 result = { 0 };

    result.m[0][0] = a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0] + a.m[0][2] * b.m[2][0];
    result.m[0][1] = a.m[0][0] * b.m[0][1] + a.m[0][1] * b.m[1][1] + a.m[0][2] * b.m[2][1];
    result.m[0][2] = a.m[0][0] * b.m[0][2] + a.m[0][1] * b.m[1][2] + a.m[0][2] * b.m[2][2];

    result.m[1][0] = a.m[1][0] * b.m[0][0] + a.m[1][1] * b.m[1][0] + a.m[1][2] * b.m[2][0];
    result.m[1][1] = a.m[1][0] * b.m[0][1] + a.m[1][1] * b.m[1][1] + a.m[1][2] * b.m[2][1];
    result.m[1][2] = a.m[1][0] * b.m[0][2] + a.m[1][1] * b.m[1][2] + a.m[1][2] * b.m[2][2];

    result.m[2][0] = a.m[2][0] * b.m[0][0] + a.m[2][1] * b.m[1][0] + a.m[2][2] * b.m[2][0];
    result.m[2][1] = a.m[2][0] * b.m[0][1] + a.m[2][1] * b.m[1][1] + a.m[2][2] * b.m[2][1];
    result.m[2][2] = a.m[2][0] * b.m[0][2] + a.m[2][1] * b.m[1][2] + a.m[2][2] * b.m[2][2];

    return result;
}

internal V2F32 m3f32_multiply_v2f32(M3F32 matrix, V2F32 vector) {
    V2F32 result = {
        .x = vector.x * matrix.m[0][0] + vector.y * matrix.m[0][1] + matrix.m[0][2],
        .y = vector.x * matrix.m[1][0] + vector.y * matrix.m[1][1] + matrix.m[1][2],
    };
    return result;
}

internal V3F32 m3f32_multiply_v3f32(M3F32 matrix, V3F32 vector) {
    V3F32 result = {
        .x = vector.x * matrix.m[0][0] + vector.y * matrix.m[0][1] + vector.z * matrix.m[0][2],
        .y = vector.x * matrix.m[1][0] + vector.y * matrix.m[1][1] + vector.z * matrix.m[1][2],
        .z = vector.x * matrix.m[2][0] + vector.y * matrix.m[2][1] + vector.z * matrix.m[2][2],
    };
    return result;
}



internal M4F32 m4f32_ortho(F32 left, F32 right, F32 top, F32 bottom, F32 near_plane, F32 far_plane) {
    M4F32 result = { 0 };

    result.m[0][0] = 2.0f / (right - left);
    result.m[0][1] = 0.0f;
    result.m[0][2] = 0.0f;
    result.m[0][3] = (left + right) / (left - right);

    result.m[1][0] = 0.0f;
    result.m[1][1] = 2.0f / (top - bottom);
    result.m[1][2] = 0.0f;
    result.m[1][3] = (top + bottom) / (bottom - top);

    result.m[2][0] = 0.0f;
    result.m[2][1] = 0.0f;
    result.m[2][2] = 1.0f / (near_plane - far_plane);
    result.m[2][3] = near_plane / (near_plane - far_plane);

    result.m[3][0] = 0.0f;
    result.m[3][1] = 0.0f;
    result.m[3][2] = 0.0f;
    result.m[3][3] = 1.0f;

    return result;
}



internal R2U8 r2u8(U8 min_x, U8 min_y, U8 max_x, U8 max_y) {
    R2U8 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

    return result;
}

internal R2U8 r2u8_pad(R2U8 range, U8 pad) {
    R2U8 result = { 0 };

    result.min.x = range.min.x - pad;
    result.min.y = range.min.y - pad;
    result.max.x = range.max.x + pad;
    result.max.y = range.max.y + pad;

    return result;
}

internal V2U8 r2u8_size(R2U8 range) {
    V2U8 result = v2u8(
        range.max.x - range.min.x,
        range.max.y - range.min.y
    );
    return result;
}



internal R2U16 r2u16(U16 min_x, U16 min_y, U16 max_x, U16 max_y) {
    R2U16 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

    return result;
}

internal R2U16 r2u16_pad(R2U16 range, U16 pad) {
    R2U16 result = { 0 };

    result.min.x = range.min.x - pad;
    result.min.y = range.min.y - pad;
    result.max.x = range.max.x + pad;
    result.max.y = range.max.y + pad;

    return result;
}

internal V2U16 r2u16_size(R2U16 range) {
    V2U16 result = v2u16(
        range.max.x - range.min.x,
        range.max.y - range.min.y
    );
    return result;
}



internal R2U32 r2u32(U32 min_x, U32 min_y, U32 max_x, U32 max_y) {
    R2U32 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

    return result;
}

internal R2U32 r2u32_from_position_size(V2U32 position, V2U32 size) {
    R2U32 result = { 0 };

    result.min = position;
    result.max = v2u32_add(position, size);

    return result;
}

internal B32 r2u32_contains_r2u32(R2U32 a, R2U32 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
}

internal R2U32 r2u32_pad(R2U32 range, U32 pad) {
    R2U32 result = { 0 };

    result.min.x = range.min.x - pad;
    result.min.y = range.min.y - pad;
    result.max.x = range.max.x + pad;
    result.max.y = range.max.y + pad;

    return result;
}

internal V2U32 r2u32_size(R2U32 range) {
    V2U32 result = v2u32(
        range.max.x - range.min.x,
        range.max.y - range.min.y
    );
    return result;
}



internal R2U64 r2u64(U64 min_x, U64 min_y, U64 max_x, U64 max_y) {
    R2U64 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

    return result;
}

internal R2U64 r2u64_pad(R2U64 range, U64 pad) {
    R2U64 result = { 0 };

    result.min.x = range.min.x - pad;
    result.min.y = range.min.y - pad;
    result.max.x = range.max.x + pad;
    result.max.y = range.max.y + pad;

    return result;
}

internal V2U64 r2u64_size(R2U64 range) {
    V2U64 result = v2u64(
        range.max.x - range.min.x,
        range.max.y - range.min.y
    );
    return result;
}



internal R2S8 r2s8(S8 min_x, S8 min_y, S8 max_x, S8 max_y) {
    R2S8 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

    return result;
}

internal R2S8 r2s8_pad(R2S8 range, S8 pad) {
    R2S8 result = { 0 };

    result.min.x = range.min.x - pad;
    result.min.y = range.min.y - pad;
    result.max.x = range.max.x + pad;
    result.max.y = range.max.y + pad;

    return result;
}

internal V2S8 r2s8_size(R2S8 range) {
    V2S8 result = v2s8(
        range.max.x - range.min.x,
        range.max.y - range.min.y
    );
    return result;
}



internal R2S16 r2s16(S16 min_x, S16 min_y, S16 max_x, S16 max_y) {
    R2S16 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

    return result;
}

internal R2S16 r2s16_pad(R2S16 range, S16 pad) {
    R2S16 result = { 0 };

    result.min.x = range.min.x - pad;
    result.min.y = range.min.y - pad;
    result.max.x = range.max.x + pad;
    result.max.y = range.max.y + pad;

    return result;
}

internal V2S16 r2s16_size(R2S16 range) {
    V2S16 result = v2s16(
        range.max.x - range.min.x,
        range.max.y - range.min.y
    );
    return result;
}



internal R2S32 r2s32(S32 min_x, S32 min_y, S32 max_x, S32 max_y) {
    R2S32 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

    return result;
}

internal R2S32 r2s32_pad(R2S32 range, S32 pad) {
    R2S32 result = { 0 };

    result.min.x = range.min.x - pad;
    result.min.y = range.min.y - pad;
    result.max.x = range.max.x + pad;
    result.max.y = range.max.y + pad;

    return result;
}

internal V2S32 r2s32_size(R2S32 range) {
    V2S32 result = v2s32(
        range.max.x - range.min.x,
        range.max.y - range.min.y
    );
    return result;
}



internal R2S64 r2s64(S64 min_x, S64 min_y, S64 max_x, S64 max_y) {
    R2S64 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

    return result;
}

internal R2S64 r2s64_pad(R2S64 range, S64 pad) {
    R2S64 result = { 0 };

    result.min.x = range.min.x - pad;
    result.min.y = range.min.y - pad;
    result.max.x = range.max.x + pad;
    result.max.y = range.max.y + pad;

    return result;
}

internal V2S64 r2s64_size(R2S64 range) {
    V2S64 result = v2s64(
        range.max.x - range.min.x,
        range.max.y - range.min.y
    );
    return result;
}



internal R2F32 r2f32(F32 min_x, F32 min_y, F32 max_x, F32 max_y) {
    R2F32 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

    return result;
}

internal R2F32 r2f32_intersect(R2F32 a, R2F32 b) {
    R2F32 result = { 0 };

    result.min.x = f32_max(a.min.x, b.min.x);
    result.min.y = f32_max(a.min.y, b.min.y);
    result.max.x = f32_min(a.max.x, b.max.x);
    result.max.y = f32_min(a.max.y, b.max.y);

    return result;
}

internal B32 r2f32_contains(R2F32 bounds, V2F32 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
    return result;
}

internal R2F32 r2f32_pad(R2F32 range, F32 pad) {
    R2F32 result = { 0 };

    result.min.x = range.min.x - pad;
    result.min.y = range.min.y - pad;
    result.max.x = range.max.x + pad;
    result.max.y = range.max.y + pad;

    return result;
}

internal V2F32 r2f32_size(R2F32 range) {
    V2F32 result = v2f32(
        range.max.x - range.min.x,
        range.max.y - range.min.y
    );
    return result;
}
