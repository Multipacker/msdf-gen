internal V2U32 v2u32(U32 x, U32 y) {
    V2U32 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V2F32 v2f32(F32 x, F32 y) {
    V2F32 result;

    result.x = x;
    result.y = y;

    return result;
}

internal V3F32 v3f32(F32 x, F32 y, F32 z) {
    V3F32 result;

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

internal V4F32 v4f32(F32 x, F32 y, F32 z, F32 w) {
    V4F32 result;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

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
    return vector.x * vector.x + vector.y * vector.y;
}

internal F32 v2f32_length(V2F32 vector) {
    return f32_sqrt(vector.x * vector.x + vector.y * vector.y);
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

internal R2U16 r2u16(U16 min_x, U16 min_y, U16 max_x, U16 max_y) {
    R2U16 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

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
    result.max = v2u32(
        position.x + size.x,
        position.y + size.y
    );

    return result;
}

internal B32 r2u32_contains_r2u32(R2U32 a, R2U32 b) {
    B32 contains_x = a.min.x <= b.min.x && b.max.x <= a.max.x;
    B32 contains_y = a.min.y <= b.min.y && b.max.y <= a.max.y;
    B32 contains = contains_x && contains_y;
    return contains;
}

internal R2U64 r2u64(U64 min_x, U64 min_y, U64 max_x, U64 max_y) {
    R2U64 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

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

internal R2S16 r2s16(S16 min_x, S16 min_y, S16 max_x, S16 max_y) {
    R2S16 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

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

internal R2S64 r2s64(S64 min_x, S64 min_y, S64 max_x, S64 max_y) {
    R2S64 result = { 0 };

    result.min.x = min_x;
    result.min.y = min_y;
    result.max.x = max_x;
    result.max.y = max_y;

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
