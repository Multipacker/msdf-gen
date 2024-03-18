#ifndef BASE_VECTOR_H
#define BASE_VECTOR_H

typedef union {
    struct {
        F32 x;
        F32 y;
    };
    struct {
        F32 u;
        F32 v;
    };
} V2F32;

typedef union {
    struct {
        F32 x;
        F32 y;
        F32 z;
    };
    struct {
        F32 r;
        F32 g;
        F32 b;
    };
} V3F32;

internal V2F32 v2f32(F32 x, F32 y);

internal V3F32 v3f32(F32 x, F32 y, F32 z);

internal V2F32 v2f32_add(V2F32 a, V2F32 b);
internal V2F32 v2f32_subtract(V2F32 a, V2F32 b);
internal V2F32 v2f32_scale(V2F32 vector, F32 scale);
internal F32   v2f32_length_squared(V2F32 vector);
internal F32   v2f32_length(V2F32 vector);
internal V2F32 v2f32_normalize(V2F32 vector);
internal F32   v2f32_dot(V2F32 a, V2F32 b);
internal F32   v2f32_cross(V2F32 a, V2F32 b);
internal V2F32 v2f32_negate(V2F32 vector);
internal V2F32 v2f32_perpendicular(V2F32 vector);
internal V2F32 v2f32_min(V2F32 a, V2F32 b);
internal V2F32 v2f32_max(V2F32 a, V2F32 b);

#endif // BASE_VECTOR_H
