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
