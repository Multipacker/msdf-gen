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



internal V2F64 v2f64(F64 x, F64 y) {
    V2F64 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V2F64 v2f64_add(V2F64 a, V2F64 b) {
    V2F64 result = { 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal V2F64 v2f64_subtract(V2F64 a, V2F64 b) {
    V2F64 result = { 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal V2F64 v2f64_scale(V2F64 vector, F64 scale) {
    V2F64 result = { 0 };

    result.x = scale * vector.x;
    result.y = scale * vector.y;

    return result;
}

internal F64 v2f64_length_squared(V2F64 vector) {
    F64 result = vector.x * vector.x + vector.y * vector.y;
    return result;
}

internal F64 v2f64_length(V2F64 vector) {
    F64 result = f64_sqrt(vector.x * vector.x + vector.y * vector.y);
    return result;
}

internal V2F64 v2f64_normalize(V2F64 vector) {
    F64 length = v2f64_length(vector);

    V2F64 result = { 0 };

    if (length > F64_EPSILON) {
        result.x = vector.x / length;
        result.y = vector.y / length;
    }

    return result;
}

internal F64 v2f64_dot(V2F64 a, V2F64 b) {
    return a.x * b.x + a.y * b.y;
}

internal F64 v2f64_cross(V2F64 a, V2F64 b) {
    return a.x * b.y - a.y * b.x;
}

internal V2F64 v2f64_negate(V2F64 vector) {
    V2F64 result = { 0 };

    result.x = -vector.x;
    result.y = -vector.y;

    return result;
}

internal V2F64 v2f64_perpendicular(V2F64 vector) {
    V2F64 result = { 0 };

    result.x =  vector.y;
    result.y = -vector.x;

    return result;
}

internal V2F64 v2f64_min(V2F64 a, V2F64 b) {
    V2F64 result = { 0 };

    result.x = f64_min(a.x, b.x);
    result.y = f64_min(a.y, b.y);

    return result;
}

internal V2F64 v2f64_max(V2F64 a, V2F64 b) {
    V2F64 result = { 0 };

    result.x = f64_max(a.x, b.x);
    result.y = f64_max(a.y, b.y);

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

internal M3F32 m3f32_uniform_scale(F32 scale) {
    M3F32 result = {
        .m = {
            { scale,  0.0f, 0.0f, },
            {  0.0f, scale, 0.0f, },
            {  0.0f,  0.0f, 1.0f, },
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



internal R1U8 r1u8(U8 min, U8 max) {
    R1U8 result = { 0 };

    result.min = min;
    result.max = max;

    return result;
}

internal R1U8 r1u8_intersect(R1U8 a, R1U8 b) {
    R1U8 result = { 0 };

    result.min = u8_max(a.min, b.min);
    result.max = u8_min(a.max, b.max);

    return result;
}

internal B32 r1u8_contains_u8(R1U8 bounds, U8 point) {
    B32 result = bounds.min <= point && point < bounds.max;
    return result;
}

internal B32 r1u8_contains_r1u8(R1U8 a, R1U8 b) {
    B32 result = a.min <= b.min && b.max <= a.max;
    return result;
}

internal R1U8 r1u8_pad(R1U8 range, U8 pad) {
    R1U8 result = { 0 };

    result.min = range.min - pad;
    result.max = range.max + pad;

    return result;
}

internal U8 r1u8_size(R1U8 range) {
    U8 result = range.max - range.min;
    return result;
}

internal U8 r1u8_center(R1U8 range) {
    U8 result = (range.min + range.max) / 2;
    return result;
}




internal R1U16 r1u16(U16 min, U16 max) {
    R1U16 result = { 0 };

    result.min = min;
    result.max = max;

    return result;
}

internal R1U16 r1u16_intersect(R1U16 a, R1U16 b) {
    R1U16 result = { 0 };

    result.min = u16_max(a.min, b.min);
    result.max = u16_min(a.max, b.max);

    return result;
}

internal B32 r1u16_contains_u16(R1U16 bounds, U16 point) {
    B32 result = bounds.min <= point && point < bounds.max;
    return result;
}

internal B32 r1u16_contains_r1u16(R1U16 a, R1U16 b) {
    B32 result = a.min <= b.min && b.max <= a.max;
    return result;
}

internal R1U16 r1u16_pad(R1U16 range, U16 pad) {
    R1U16 result = { 0 };

    result.min = range.min - pad;
    result.max = range.max + pad;

    return result;
}

internal U16 r1u16_size(R1U16 range) {
    U16 result = range.max - range.min;
    return result;
}

internal U16 r1u16_center(R1U16 range) {
    U16 result = (range.min + range.max) / 2;
    return result;
}



internal R1U32 r1u32(U32 min, U32 max) {
    R1U32 result = { 0 };

    result.min = min;
    result.max = max;

    return result;
}

internal R1U32 r1u32_intersect(R1U32 a, R1U32 b) {
    R1U32 result = { 0 };

    result.min = u32_max(a.min, b.min);
    result.max = u32_min(a.max, b.max);

    return result;
}

internal B32 r1u32_contains_u32(R1U32 bounds, U32 point) {
    B32 result = bounds.min <= point && point < bounds.max;
    return result;
}

internal B32 r1u32_contains_r1u32(R1U32 a, R1U32 b) {
    B32 result = a.min <= b.min && b.max <= a.max;
    return result;
}

internal R1U32 r1u32_pad(R1U32 range, U32 pad) {
    R1U32 result = { 0 };

    result.min = range.min - pad;
    result.max = range.max + pad;

    return result;
}

internal U32 r1u32_size(R1U32 range) {
    U32 result = range.max - range.min;
    return result;
}

internal U32 r1u32_center(R1U32 range) {
    U32 result = (range.min + range.max) / 2;
    return result;
}



internal R1U64 r1u64(U64 min, U64 max) {
    R1U64 result = { 0 };

    result.min = min;
    result.max = max;

    return result;
}

internal R1U64 r1u64_intersect(R1U64 a, R1U64 b) {
    R1U64 result = { 0 };

    result.min = u64_max(a.min, b.min);
    result.max = u64_min(a.max, b.max);

    return result;
}

internal B32 r1u64_contains_u64(R1U64 bounds, U64 point) {
    B32 result = bounds.min <= point && point < bounds.max;
    return result;
}

internal B32 r1u64_contains_r1u64(R1U64 a, R1U64 b) {
    B32 result = a.min <= b.min && b.max <= a.max;
    return result;
}

internal R1U64 r1u64_pad(R1U64 range, U64 pad) {
    R1U64 result = { 0 };

    result.min = range.min - pad;
    result.max = range.max + pad;

    return result;
}

internal U64 r1u64_size(R1U64 range) {
    U64 result = range.max - range.min;
    return result;
}

internal U64 r1u64_center(R1U64 range) {
    U64 result = (range.min + range.max) / 2;
    return result;
}



internal R1S8 r1s8(S8 min, S8 max) {
    R1S8 result = { 0 };

    result.min = min;
    result.max = max;

    return result;
}

internal R1S8 r1s8_intersect(R1S8 a, R1S8 b) {
    R1S8 result = { 0 };

    result.min = s8_max(a.min, b.min);
    result.max = s8_min(a.max, b.max);

    return result;
}

internal B32 r1s8_contains_s8(R1S8 bounds, S8 point) {
    B32 result = bounds.min <= point && point < bounds.max;
    return result;
}

internal B32 r1s8_contains_r1s8(R1S8 a, R1S8 b) {
    B32 result = a.min <= b.min && b.max <= a.max;
    return result;
}

internal R1S8 r1s8_pad(R1S8 range, S8 pad) {
    R1S8 result = { 0 };

    result.min = range.min - pad;
    result.max = range.max + pad;

    return result;
}

internal S8 r1s8_size(R1S8 range) {
    S8 result = range.max - range.min;
    return result;
}

internal S8 r1s8_center(R1S8 range) {
    S8 result = (range.min + range.max) / 2;
    return result;
}




internal R1S16 r1s16(S16 min, S16 max) {
    R1S16 result = { 0 };

    result.min = min;
    result.max = max;

    return result;
}

internal R1S16 r1s16_intersect(R1S16 a, R1S16 b) {
    R1S16 result = { 0 };

    result.min = s16_max(a.min, b.min);
    result.max = s16_min(a.max, b.max);

    return result;
}

internal B32 r1s16_contains_s16(R1S16 bounds, S16 point) {
    B32 result = bounds.min <= point && point < bounds.max;
    return result;
}

internal B32 r1s16_contains_r1s16(R1S16 a, R1S16 b) {
    B32 result = a.min <= b.min && b.max <= a.max;
    return result;
}

internal R1S16 r1s16_pad(R1S16 range, S16 pad) {
    R1S16 result = { 0 };

    result.min = range.min - pad;
    result.max = range.max + pad;

    return result;
}

internal S16 r1s16_size(R1S16 range) {
    S16 result = range.max - range.min;
    return result;
}

internal S16 r1s16_center(R1S16 range) {
    S16 result = (range.min + range.max) / 2;
    return result;
}



internal R1S32 r1s32(S32 min, S32 max) {
    R1S32 result = { 0 };

    result.min = min;
    result.max = max;

    return result;
}

internal R1S32 r1s32_intersect(R1S32 a, R1S32 b) {
    R1S32 result = { 0 };

    result.min = s32_max(a.min, b.min);
    result.max = s32_min(a.max, b.max);

    return result;
}

internal B32 r1s32_contains_s32(R1S32 bounds, S32 point) {
    B32 result = bounds.min <= point && point < bounds.max;
    return result;
}

internal B32 r1s32_contains_r1s32(R1S32 a, R1S32 b) {
    B32 result = a.min <= b.min && b.max <= a.max;
    return result;
}

internal R1S32 r1s32_pad(R1S32 range, S32 pad) {
    R1S32 result = { 0 };

    result.min = range.min - pad;
    result.max = range.max + pad;

    return result;
}

internal S32 r1s32_size(R1S32 range) {
    S32 result = range.max - range.min;
    return result;
}

internal S32 r1s32_center(R1S32 range) {
    S32 result = (range.min + range.max) / 2;
    return result;
}



internal R1S64 r1s64(S64 min, S64 max) {
    R1S64 result = { 0 };

    result.min = min;
    result.max = max;

    return result;
}

internal R1S64 r1s64_intersect(R1S64 a, R1S64 b) {
    R1S64 result = { 0 };

    result.min = s64_max(a.min, b.min);
    result.max = s64_min(a.max, b.max);

    return result;
}

internal B32 r1s64_contains_s64(R1S64 bounds, S64 point) {
    B32 result = bounds.min <= point && point < bounds.max;
    return result;
}

internal B32 r1s64_contains_r1s64(R1S64 a, R1S64 b) {
    B32 result = a.min <= b.min && b.max <= a.max;
    return result;
}

internal R1S64 r1s64_pad(R1S64 range, S64 pad) {
    R1S64 result = { 0 };

    result.min = range.min - pad;
    result.max = range.max + pad;

    return result;
}

internal S64 r1s64_size(R1S64 range) {
    S64 result = range.max - range.min;
    return result;
}

internal S64 r1s64_center(R1S64 range) {
    S64 result = (range.min + range.max) / 2;
    return result;
}



internal R1F32 r1f32(F32 min, F32 max) {
    R1F32 result = { 0 };

    result.min = min;
    result.max = max;

    return result;
}

internal R1F32 r1f32_intersect(R1F32 a, R1F32 b) {
    R1F32 result = { 0 };

    result.min = f32_max(a.min, b.min);
    result.max = f32_min(a.max, b.max);

    return result;
}

internal B32 r1f32_contains_f32(R1F32 bounds, F32 point) {
    B32 result = bounds.min <= point && point < bounds.max;
    return result;
}

internal B32 r1f32_contains_r1f32(R1F32 a, R1F32 b) {
    B32 result = a.min <= b.min && b.max <= a.max;
    return result;
}

internal R1F32 r1f32_pad(R1F32 range, F32 pad) {
    R1F32 result = { 0 };

    result.min = range.min - pad;
    result.max = range.max + pad;

    return result;
}

internal F32 r1f32_size(R1F32 range) {
    F32 result = range.max - range.min;
    return result;
}

internal F32 r1f32_center(R1F32 range) {
    F32 result = (range.min + range.max) / 2;
    return result;
}



internal R1F64 r1f64(F64 min, F64 max) {
    R1F64 result = { 0 };

    result.min = min;
    result.max = max;

    return result;
}

internal R1F64 r1f64_intersect(R1F64 a, R1F64 b) {
    R1F64 result = { 0 };

    result.min = f64_max(a.min, b.min);
    result.max = f64_min(a.max, b.max);

    return result;
}

internal B32 r1f64_contains_f64(R1F64 bounds, F64 point) {
    B32 result = bounds.min <= point && point < bounds.max;
    return result;
}

internal B32 r1f64_contains_r1f64(R1F64 a, R1F64 b) {
    B32 result = a.min <= b.min && b.max <= a.max;
    return result;
}

internal R1F64 r1f64_pad(R1F64 range, F64 pad) {
    R1F64 result = { 0 };

    result.min = range.min - pad;
    result.max = range.max + pad;

    return result;
}

internal F64 r1f64_size(R1F64 range) {
    F64 result = range.max - range.min;
    return result;
}

internal F64 r1f64_center(R1F64 range) {
    F64 result = (range.min + range.max) / 2;
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

internal R2U8 r2u8_from_position_size(V2U8 position, V2U8 size) {
    R2U8 result = { 0 };

    result.min = position;
    result.max = v2u8_add(position, size);

    return result;
}

internal R2U8 r2u8_intersect(R2U8 a, R2U8 b) {
    R2U8 result = { 0 };

    result.min.x = u8_max(a.min.x, b.min.x);
    result.min.y = u8_max(a.min.y, b.min.y);
    result.max.x = u8_min(a.max.x, b.max.x);
    result.max.y = u8_min(a.max.y, b.max.y);

    return result;
}

internal B32 r2u8_contains_v2u8(R2U8 bounds, V2U8 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
    return result;
}

internal B32 r2u8_contains_r2u8(R2U8 a, R2U8 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
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

internal V2U8 r2u8_center(R2U8 range) {
    V2U8 result = v2u8(
        (range.min.x + range.max.x) / 2,
        (range.min.y + range.max.y) / 2
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

internal R2U16 r2u16_from_position_size(V2U16 position, V2U16 size) {
    R2U16 result = { 0 };

    result.min = position;
    result.max = v2u16_add(position, size);

    return result;
}

internal R2U16 r2u16_intersect(R2U16 a, R2U16 b) {
    R2U16 result = { 0 };

    result.min.x = u16_max(a.min.x, b.min.x);
    result.min.y = u16_max(a.min.y, b.min.y);
    result.max.x = u16_min(a.max.x, b.max.x);
    result.max.y = u16_min(a.max.y, b.max.y);

    return result;
}

internal B32 r2u16_contains_v2u16(R2U16 bounds, V2U16 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
    return result;
}

internal B32 r2u16_contains_r2u16(R2U16 a, R2U16 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
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

internal V2U16 r2u16_center(R2U16 range) {
    V2U16 result = v2u16(
        (range.min.x + range.max.x) / 2,
        (range.min.y + range.max.y) / 2
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

internal R2U32 r2u32_intersect(R2U32 a, R2U32 b) {
    R2U32 result = { 0 };

    result.min.x = u32_max(a.min.x, b.min.x);
    result.min.y = u32_max(a.min.y, b.min.y);
    result.max.x = u32_min(a.max.x, b.max.x);
    result.max.y = u32_min(a.max.y, b.max.y);

    return result;
}

internal B32 r2u32_contains_v2u32(R2U32 bounds, V2U32 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
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

internal V2U32 r2u32_center(R2U32 range) {
    V2U32 result = v2u32(
        (range.min.x + range.max.x) / 2,
        (range.min.y + range.max.y) / 2
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

internal R2U64 r2u64_from_position_size(V2U64 position, V2U64 size) {
    R2U64 result = { 0 };

    result.min = position;
    result.max = v2u64_add(position, size);

    return result;
}

internal R2U64 r2u64_intersect(R2U64 a, R2U64 b) {
    R2U64 result = { 0 };

    result.min.x = u64_max(a.min.x, b.min.x);
    result.min.y = u64_max(a.min.y, b.min.y);
    result.max.x = u64_min(a.max.x, b.max.x);
    result.max.y = u64_min(a.max.y, b.max.y);

    return result;
}

internal B32 r2u64_contains_v2u64(R2U64 bounds, V2U64 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
    return result;
}

internal B32 r2u64_contains_r2u64(R2U64 a, R2U64 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
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

internal V2U64 r2u64_center(R2U64 range) {
    V2U64 result = v2u64(
        (range.min.x + range.max.x) / 2,
        (range.min.y + range.max.y) / 2
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

internal R2S8 r2s8_from_position_size(V2S8 position, V2S8 size) {
    R2S8 result = { 0 };

    result.min = position;
    result.max = v2s8_add(position, size);

    return result;
}

internal R2S8 r2s8_intersect(R2S8 a, R2S8 b) {
    R2S8 result = { 0 };

    result.min.x = s8_max(a.min.x, b.min.x);
    result.min.y = s8_max(a.min.y, b.min.y);
    result.max.x = s8_min(a.max.x, b.max.x);
    result.max.y = s8_min(a.max.y, b.max.y);

    return result;
}

internal B32 r2s8_contains_v2s8(R2S8 bounds, V2S8 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
    return result;
}

internal B32 r2s8_contains_r2s8(R2S8 a, R2S8 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
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

internal V2S8 r2s8_center(R2S8 range) {
    V2S8 result = v2s8(
        (range.min.x + range.max.x) / 2,
        (range.min.y + range.max.y) / 2
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

internal R2S16 r2s16_from_position_size(V2S16 position, V2S16 size) {
    R2S16 result = { 0 };

    result.min = position;
    result.max = v2s16_add(position, size);

    return result;
}

internal R2S16 r2s16_intersect(R2S16 a, R2S16 b) {
    R2S16 result = { 0 };

    result.min.x = s16_max(a.min.x, b.min.x);
    result.min.y = s16_max(a.min.y, b.min.y);
    result.max.x = s16_min(a.max.x, b.max.x);
    result.max.y = s16_min(a.max.y, b.max.y);

    return result;
}

internal B32 r2s16_contains_v2s16(R2S16 bounds, V2S16 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
    return result;
}

internal B32 r2s16_contains_r2s16(R2S16 a, R2S16 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
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

internal V2S16 r2s16_center(R2S16 range) {
    V2S16 result = v2s16(
        (range.min.x + range.max.x) / 2,
        (range.min.y + range.max.y) / 2
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

internal R2S32 r2s32_from_position_size(V2S32 position, V2S32 size) {
    R2S32 result = { 0 };

    result.min = position;
    result.max = v2s32_add(position, size);

    return result;
}

internal R2S32 r2s32_intersect(R2S32 a, R2S32 b) {
    R2S32 result = { 0 };

    result.min.x = s32_max(a.min.x, b.min.x);
    result.min.y = s32_max(a.min.y, b.min.y);
    result.max.x = s32_min(a.max.x, b.max.x);
    result.max.y = s32_min(a.max.y, b.max.y);

    return result;
}

internal B32 r2s32_contains_v2s32(R2S32 bounds, V2S32 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
    return result;
}

internal B32 r2s32_contains_r2s32(R2S32 a, R2S32 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
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

internal V2S32 r2s32_center(R2S32 range) {
    V2S32 result = v2s32(
        (range.min.x + range.max.x) / 2,
        (range.min.y + range.max.y) / 2
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

internal R2S64 r2s64_from_position_size(V2S64 position, V2S64 size) {
    R2S64 result = { 0 };

    result.min = position;
    result.max = v2s64_add(position, size);

    return result;
}

internal R2S64 r2s64_intersect(R2S64 a, R2S64 b) {
    R2S64 result = { 0 };

    result.min.x = s64_max(a.min.x, b.min.x);
    result.min.y = s64_max(a.min.y, b.min.y);
    result.max.x = s64_min(a.max.x, b.max.x);
    result.max.y = s64_min(a.max.y, b.max.y);

    return result;
}

internal B32 r2s64_contains_v2s64(R2S64 bounds, V2S64 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
    return result;
}

internal B32 r2s64_contains_r2s64(R2S64 a, R2S64 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
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

internal V2S64 r2s64_center(R2S64 range) {
    V2S64 result = v2s64(
        (range.min.x + range.max.x) / 2,
        (range.min.y + range.max.y) / 2
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

internal R2F32 r2f32_from_position_size(V2F32 position, V2F32 size) {
    R2F32 result = { 0 };

    result.min = position;
    result.max = v2f32_add(position, size);

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

internal B32 r2f32_contains_v2f32(R2F32 bounds, V2F32 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
    return result;
}

internal B32 r2f32_contains_r2f32(R2F32 a, R2F32 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
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

internal V2F32 r2f32_center(R2F32 range) {
    V2F32 result = v2f32(
        (range.min.x + range.max.x) / 2,
        (range.min.y + range.max.y) / 2
    );
    return result;
}



internal R2F64 r2f64(F64 min_x, F64 min_y, F64 max_x, F64 max_y) {
    R2F64 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

    return result;
}

internal R2F64 r2f64_from_position_size(V2F64 position, V2F64 size) {
    R2F64 result = { 0 };

    result.min = position;
    result.max = v2f64_add(position, size);

    return result;
}

internal R2F64 r2f64_intersect(R2F64 a, R2F64 b) {
    R2F64 result = { 0 };

    result.min.x = f64_max(a.min.x, b.min.x);
    result.min.y = f64_max(a.min.y, b.min.y);
    result.max.x = f64_min(a.max.x, b.max.x);
    result.max.y = f64_min(a.max.y, b.max.y);

    return result;
}

internal B32 r2f64_contains_v2f64(R2F64 bounds, V2F64 point) {
    B32 contains_x = bounds.min.x <= point.x && point.x < bounds.max.x;
    B32 contains_y = bounds.min.y <= point.y && point.y < bounds.max.y;
    B32 result     = contains_x && contains_y;
    return result;
}

internal B32 r2f64_contains_r2f64(R2F64 a, R2F64 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
}

internal R2F64 r2f64_pad(R2F64 range, F64 pad) {
    R2F64 result = { 0 };

    result.min.x = range.min.x - pad;
    result.min.y = range.min.y - pad;
    result.max.x = range.max.x + pad;
    result.max.y = range.max.y + pad;

    return result;
}

internal V2F64 r2f64_size(R2F64 range) {
    V2F64 result = v2f64(
        range.max.x - range.min.x,
        range.max.y - range.min.y
    );
    return result;
}

internal V2F64 r2f64_center(R2F64 range) {
    V2F64 result = v2f64(
        (range.min.x + range.max.x) / 2,
        (range.min.y + range.max.y) / 2
    );
    return result;
}
