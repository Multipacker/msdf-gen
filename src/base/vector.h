#ifndef BASE_VECTOR_H
#define BASE_VECTOR_H

typedef union {
    struct {
        U8 x;
        U8 y;
    };
    struct {
        U8 width;
        U8 height;
    };
} V2U8;

typedef union {
    struct {
        U16 x;
        U16 y;
    };
    struct {
        U16 width;
        U16 height;
    };
} V2U16;

typedef union {
    struct {
        U32 x;
        U32 y;
    };
    struct {
        U32 width;
        U32 height;
    };
} V2U32;

typedef union {
    struct {
        U64 x;
        U64 y;
    };
    struct {
        U64 width;
        U64 height;
    };
} V2U64;

typedef union {
    struct {
        S8 x;
        S8 y;
    };
    struct {
        S8 width;
        S8 height;
    };
} V2S8;

typedef union {
    struct {
        S16 x;
        S16 y;
    };
    struct {
        S16 width;
        S16 height;
    };
} V2S16;

typedef union {
    struct {
        S32 x;
        S32 y;
    };
    struct {
        S32 width;
        S32 height;
    };
} V2S32;

typedef union {
    struct {
        S64 x;
        S64 y;
    };
    struct {
        S64 width;
        S64 height;
    };
} V2S64;

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
    F32 values[2];
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

typedef struct R2U8 R2U8;
struct R2U8 {
    V2U8 min;
    V2U8 max;
};

typedef struct R2U16 R2U16;
struct R2U16 {
    V2U16 min;
    V2U16 max;
};

typedef struct R2U32 R2U32;
struct R2U32 {
    V2U32 min;
    V2U32 max;
};

typedef struct R2U64 R2U64;
struct R2U64 {
    V2U64 min;
    V2U64 max;
};

typedef struct R2S8 R2S8;
struct R2S8 {
    V2S8 min;
    V2S8 max;
};

typedef struct R2S16 R2S16;
struct R2S16 {
    V2S16 min;
    V2S16 max;
};

typedef struct R2S32 R2S32;
struct R2S32 {
    V2S32 min;
    V2S32 max;
};

typedef struct R2S64 R2S64;
struct R2S64 {
    V2S64 min;
    V2S64 max;
};

typedef struct R2F32 R2F32;
struct R2F32 {
    V2F32 min;
    V2F32 max;
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

internal R2U8 r2u8(U8 min_x, U8 min_y, U8 max_x, U8 max_y);

internal R2U16 r2u16(U16 min_x, U16 min_y, U16 max_x, U16 max_y);

internal R2U32 r2u32(U32 min_x, U32 min_y, U32 max_x, U32 max_y);
internal R2U32 r2u32_from_position_size(V2U32 position, V2U32 size);
internal B32 r2u32_contains_r2u32(R2U32 a, R2U32 b);

internal R2U64 r2u64(U64 min_x, U64 min_y, U64 max_x, U64 max_y);

internal R2S8 r2s8(S8 min_x, S8 min_y, S8 max_x, S8 max_y);

internal R2S16 r2s16(S16 min_x, S16 min_y, S16 max_x, S16 max_y);

internal R2S32 r2s32(S32 min_x, S32 min_y, S32 max_x, S32 max_y);

internal R2S64 r2s64(S64 min_x, S64 min_y, S64 max_x, S64 max_y);

internal R2F32 r2f32(F32 min_x, F32 min_y, F32 max_x, F32 max_y);

#endif // BASE_VECTOR_H
