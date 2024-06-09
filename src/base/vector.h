#ifndef BASE_VECTOR_H
#define BASE_VECTOR_H

typedef union {
    struct {
        U32 x;
        U32 y;
    };
    struct {
        U32 u;
        U32 v;
    };
    struct {
        U32 width;
        U32 height;
    };
} V2U32;

typedef union {
    struct {
        F32 x;
        F32 y;
    };
    struct {
        F32 u;
        F32 v;
    };
    struct {
        F32 width;
        F32 height;
    };
} V2F32;

typedef union V3F32 V3F32;
union V3F32 {
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
    struct {
        F32 width;
        F32 height;
        F32 depth;
    };
};

typedef union V4F32 V4F32;
union V4F32 {
    struct {
        F32 x;
        F32 y;
        F32 z;
        F32 w;
    };
    struct {
        F32 r;
        F32 g;
        F32 b;
        F32 a;
    };
};

typedef struct M4F32 M4F32;
struct M4F32 {
    F32 m[4][4];
};

internal V2U32 v2u32(U32 x, U32 y);

internal V2F32 v2f32(F32 x, F32 y);

internal V3F32 v3f32(F32 x, F32 y, F32 z);

internal V4F32 v4f32(F32 x, F32 y, F32 z, F32 w);

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

internal M4F32 m4f32_ortho(F32 left, F32 right, F32 top, F32 bottom, F32 near_plane, F32 far_plane);

#endif // BASE_VECTOR_H
