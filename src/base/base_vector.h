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
    U8 values[2];
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
    U16 values[2];
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
    U32 values[2];
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
    U64 values[2];
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
    S8 values[2];
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
    S16 values[2];
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
    S32 values[2];
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
    S64 values[2];
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

typedef union {
    struct {
        F64 x;
        F64 y;
    };
    struct {
        F64 u;
        F64 v;
    };
    struct {
        F64 width;
        F64 height;
    };
    F64 values[2];
} V2F64;

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
    F32 values[3];
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
    F32 values[4];
};

typedef struct M2F32 M2F32;
struct M2F32 {
    F32 m[2][2];
};

typedef struct M3F32 M3F32;
struct M3F32 {
    F32 m[3][3];
};

typedef struct M4F32 M4F32;
struct M4F32 {
    F32 m[4][4];
};



typedef union R1U8 R1U8;
union R1U8 {
    struct {
        U8 min;
        U8 max;
    };
    U8 values[2];
};

typedef union R1U16 R1U16;
union R1U16 {
    struct {
        U16 min;
        U16 max;
    };
    U16 values[2];
};

typedef union R1U32 R1U32;
union R1U32 {
    struct {
        U32 min;
        U32 max;
    };
    U32 values[2];
};

typedef union R1U64 R1U64;
union R1U64 {
    struct {
        U64 min;
        U64 max;
    };
    U64 values[2];
};

typedef union R1S8 R1S8;
union R1S8 {
    struct {
        S8 min;
        S8 max;
    };
    S8 values[2];
};

typedef union R1S16 R1S16;
union R1S16 {
    struct {
        S16 min;
        S16 max;
    };
    S16 values[2];
};

typedef union R1S32 R1S32;
union R1S32 {
    struct {
        S32 min;
        S32 max;
    };
    S32 values[2];
};

typedef union R1S64 R1S64;
union R1S64 {
    struct {
        S64 min;
        S64 max;
    };
    S64 values[2];
};

typedef union R1F32 R1F32;
union R1F32 {
    struct {
        F32 min;
        F32 max;
    };
    F32 values[2];
};

typedef union R1F64 R1F64;
union R1F64 {
    struct {
        F64 min;
        F64 max;
    };
    F64 values[2];
};



typedef union R2U8 R2U8;
union R2U8 {
    struct {
        V2U8 min;
        V2U8 max;
    };
    V2U8 values[2];
};

typedef union R2U16 R2U16;
union R2U16 {
    struct {
        V2U16 min;
        V2U16 max;
    };
    V2U16 values[2];
};

typedef union R2U32 R2U32;
union R2U32 {
    struct {
        V2U32 min;
        V2U32 max;
    };
    V2U32 values[2];
};

typedef union R2U64 R2U64;
union R2U64 {
    struct {
        V2U64 min;
        V2U64 max;
    };
    V2U64 values[2];
};

typedef union R2S8 R2S8;
union R2S8 {
    struct {
        V2S8 min;
        V2S8 max;
    };
    V2S8 values[2];
};

typedef union R2S16 R2S16;
union R2S16 {
    struct {
        V2S16 min;
        V2S16 max;
    };
    V2S16 values[2];
};

typedef union R2S32 R2S32;
union R2S32 {
    struct {
        V2S32 min;
        V2S32 max;
    };
    V2S32 values[2];
};

typedef union R2S64 R2S64;
union R2S64 {
    struct {
        V2S64 min;
        V2S64 max;
    };
    V2S64 values[2];
};

typedef union R2F32 R2F32;
union R2F32 {
    struct {
        V2F32 min;
        V2F32 max;
    };
    V2F32 values[2];
};

typedef union R2F64 R2F64;
union R2F64 {
    struct {
        V2F64 min;
        V2F64 max;
    };
    V2F64 values[2];
};

internal V2U8 v2u8(U8 x, U8 y);
internal V2U8 v2u8_add(V2U8 a, V2U8 b);
internal V2U8 v2u8_subtract(V2U8 a, V2U8 b);
internal V2U8 v2u8_min(V2U8 a, V2U8 b);
internal V2U8 v2u8_max(V2U8 a, V2U8 b);

internal V2U16 v2u16(U16 x, U16 y);
internal V2U16 v2u16_add(V2U16 a, V2U16 b);
internal V2U16 v2u16_subtract(V2U16 a, V2U16 b);
internal V2U16 v2u16_min(V2U16 a, V2U16 b);
internal V2U16 v2u16_max(V2U16 a, V2U16 b);

internal V2U32 v2u32(U32 x, U32 y);
internal V2U32 v2u32_add(V2U32 a, V2U32 b);
internal V2U32 v2u32_subtract(V2U32 a, V2U32 b);
internal V2U32 v2u32_min(V2U32 a, V2U32 b);
internal V2U32 v2u32_max(V2U32 a, V2U32 b);

internal V2U64 v2u64(U64 x, U64 y);
internal V2U64 v2u64_add(V2U64 a, V2U64 b);
internal V2U64 v2u64_subtract(V2U64 a, V2U64 b);
internal V2U64 v2u64_min(V2U64 a, V2U64 b);
internal V2U64 v2u64_max(V2U64 a, V2U64 b);

internal V2S8 v2s8(S8 x, S8 y);
internal V2S8 v2s8_add(V2S8 a, V2S8 b);
internal V2S8 v2s8_subtract(V2S8 a, V2S8 b);
internal V2S8 v2s8_min(V2S8 a, V2S8 b);
internal V2S8 v2s8_max(V2S8 a, V2S8 b);

internal V2S16 v2s16(S16 x, S16 y);
internal V2S16 v2s16_add(V2S16 a, V2S16 b);
internal V2S16 v2s16_subtract(V2S16 a, V2S16 b);
internal V2S16 v2s16_min(V2S16 a, V2S16 b);
internal V2S16 v2s16_max(V2S16 a, V2S16 b);

internal V2S32 v2s32(S32 x, S32 y);
internal V2S32 v2s32_add(V2S32 a, V2S32 b);
internal V2S32 v2s32_subtract(V2S32 a, V2S32 b);
internal V2S32 v2s32_min(V2S32 a, V2S32 b);
internal V2S32 v2s32_max(V2S32 a, V2S32 b);

internal V2S64 v2s64(S64 x, S64 y);
internal V2S64 v2s64_add(V2S64 a, V2S64 b);
internal V2S64 v2s64_subtract(V2S64 a, V2S64 b);
internal V2S64 v2s64_min(V2S64 a, V2S64 b);
internal V2S64 v2s64_max(V2S64 a, V2S64 b);

internal V2F32 v2f32(F32 x, F32 y);
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

internal V2F64 v2f64(F64 x, F64 y);
internal V2F64 v2f64_add(V2F64 a, V2F64 b);
internal V2F64 v2f64_subtract(V2F64 a, V2F64 b);
internal V2F64 v2f64_scale(V2F64 vector, F64 scale);
internal F64   v2f64_length_squared(V2F64 vector);
internal F64   v2f64_length(V2F64 vector);
internal V2F64 v2f64_normalize(V2F64 vector);
internal F64   v2f64_dot(V2F64 a, V2F64 b);
internal F64   v2f64_cross(V2F64 a, V2F64 b);
internal V2F64 v2f64_negate(V2F64 vector);
internal V2F64 v2f64_perpendicular(V2F64 vector);
internal V2F64 v2f64_min(V2F64 a, V2F64 b);
internal V2F64 v2f64_max(V2F64 a, V2F64 b);

internal V3F32 v3f32(F32 x, F32 y, F32 z);
internal V3F32 v3f32_add(V3F32 a, V3F32 b);
internal V3F32 v3f32_subtract(V3F32 a, V3F32 b);
internal V3F32 v3f32_scale(V3F32 vector, F32 scale);
internal F32   v3f32_length_squared(V3F32 vector);
internal F32   v3f32_length(V3F32 vector);
internal V3F32 v3f32_normalize(V3F32 vector);
internal F32   v3f32_dot(V3F32 a, V3F32 b);
internal V3F32 v3f32_cross(V3F32 a, V3F32 b);
internal V3F32 v3f32_negate(V3F32 vector);
internal V3F32 v3f32_min(V3F32 a, V3F32 b);
internal V3F32 v3f32_max(V3F32 a, V3F32 b);

internal V4F32 v4f32(F32 x, F32 y, F32 z, F32 w);

internal M2F32 m2f32(F32 m00, F32 m01, F32 m10, F32 m11);
internal V2F32 m2f32_multiply_v2f32(M2F32 matrix, V2F32 vector);

internal M3F32 m3f32(F32 m00, F32 m01, F32 m02, F32 m10, F32 m11, F32 m12, F32 m20, F32 m21, F32 m22);
internal M3F32 m3f32_identity(Void);
internal M3F32 m3f32_translation(V2F32 offset);
internal M3F32 m3f32_scale(V2F32 scale);
internal M3F32 m3f32_uniform_scale(F32 scale);
internal M3F32 m3f32_multiply_m3f32(M3F32 a, M3F32 b);
internal V2F32 m3f32_multiply_v2f32(M3F32 matrix, V2F32 vector);
internal V3F32 m3f32_multiply_v3f32(M3F32 matrix, V3F32 vector);

internal M4F32 m4f32_ortho(F32 left, F32 right, F32 top, F32 bottom, F32 near_plane, F32 far_plane);



internal R1U8 r1u8(U8 min, U8 max);
internal R1U8 r1u8_intersect(R1U8 a, R1U8 b);
internal B32  r1u8_contains_u8(R1U8 bounds, U8 point);
internal B32  r1u8_contains_r1u8(R1U8 a, R1U8 b);
internal R1U8 r1u8_pad(R1U8 range, U8 pad);
internal U8   r1u8_size(R1U8 range);
internal U8   r1u8_center(R1U8 range);

internal R1U16 r1u16(U16 min, U16 max);
internal R1U16 r1u16_intersect(R1U16 a, R1U16 b);
internal B32   r1u16_contains_u16(R1U16 bounds, U16 point);
internal B32   r1u16_contains_r1u16(R1U16 a, R1U16 b);
internal R1U16 r1u16_pad(R1U16 range, U16 pad);
internal U16   r1u16_size(R1U16 range);
internal U16   r1u16_center(R1U16 range);

internal R1U32 r1u32(U32 min, U32 max);
internal R1U32 r1u32_intersect(R1U32 a, R1U32 b);
internal B32   r1u32_contains_u32(R1U32 bounds, U32 point);
internal B32   r1u32_contains_r1u32(R1U32 a, R1U32 b);
internal R1U32 r1u32_pad(R1U32 range, U32 pad);
internal U32   r1u32_size(R1U32 range);
internal U32   r1u32_center(R1U32 range);

internal R1U64 r1u64(U64 min, U64 max);
internal R1U64 r1u64_intersect(R1U64 a, R1U64 b);
internal B32   r1u64_contains_u64(R1U64 bounds, U64 point);
internal B32   r1u64_contains_r1u64(R1U64 a, R1U64 b);
internal R1U64 r1u64_pad(R1U64 range, U64 pad);
internal U64   r1u64_size(R1U64 range);
internal U64   r1u64_center(R1U64 range);

internal R1S8 r1s8(S8 min, S8 max);
internal R1S8 r1s8_intersect(R1S8 a, R1S8 b);
internal B32  r1s8_contains_s8(R1S8 bounds, S8 point);
internal B32  r1s8_contains_r1s8(R1S8 a, R1S8 b);
internal R1S8 r1s8_pad(R1S8 range, S8 pad);
internal S8   r1s8_size(R1S8 range);
internal S8   r1s8_center(R1S8 range);

internal R1S16 r1s16(S16 min, S16 max);
internal R1S16 r1s16_intersect(R1S16 a, R1S16 b);
internal B32   r1s16_contains_s16(R1S16 bounds, S16 point);
internal B32   r1s16_contains_r1s16(R1S16 a, R1S16 b);
internal R1S16 r1s16_pad(R1S16 range, S16 pad);
internal S16   r1s16_size(R1S16 range);
internal S16   r1s16_center(R1S16 range);

internal R1S32 r1s32(S32 min, S32 max);
internal R1S32 r1s32_intersect(R1S32 a, R1S32 b);
internal B32   r1s32_contains_s32(R1S32 bounds, S32 point);
internal B32   r1s32_contains_r1s32(R1S32 a, R1S32 b);
internal R1S32 r1s32_pad(R1S32 range, S32 pad);
internal S32   r1s32_size(R1S32 range);
internal S32   r1s32_center(R1S32 range);

internal R1S64 r1s64(S64 min, S64 max);
internal R1S64 r1s64_intersect(R1S64 a, R1S64 b);
internal B32   r1s64_contains_s64(R1S64 bounds, S64 point);
internal B32   r1s64_contains_r1s64(R1S64 a, R1S64 b);
internal R1S64 r1s64_pad(R1S64 range, S64 pad);
internal S64   r1s64_size(R1S64 range);
internal S64   r1s64_center(R1S64 range);

internal R1F32 r1f32(F32 min, F32 max);
internal R1F32 r1f32_intersect(R1F32 a, R1F32 b);
internal B32   r1f32_contains_f32(R1F32 bounds, F32 point);
internal B32   r1f32_contains_r1f32(R1F32 a, R1F32 b);
internal R1F32 r1f32_pad(R1F32 range, F32 pad);
internal F32   r1f32_size(R1F32 range);
internal F32   r1f32_center(R1F32 range);

internal R1F64 r1f64(F64 min, F64 max);
internal R1F64 r1f64_intersect(R1F64 a, R1F64 b);
internal B32   r1f64_contains_f64(R1F64 bounds, F64 point);
internal B32   r1f64_contains_r1f64(R1F64 a, R1F64 b);
internal R1F64 r1f64_pad(R1F64 range, F64 pad);
internal F64   r1f64_size(R1F64 range);
internal F64   r1f64_center(R1F64 range);



internal R2U8 r2u8(U8 min_x, U8 min_y, U8 max_x, U8 max_y);
internal R2U8 r2u8_from_position_size(V2U8 position, V2U8 size);
internal R2U8 r2u8_intersect(R2U8 a, R2U8 b);
internal B32  r2u8_contains_v2u8(R2U8 bounds, V2U8 point);
internal B32  r2u8_contains_r2u8(R2U8 a, R2U8 b);
internal R2U8 r2u8_pad(R2U8 range, U8 pad);
internal V2U8 r2u8_size(R2U8 range);
internal V2U8 r2u8_center(R2U8 range);

internal R2U16 r2u16(U16 min_x, U16 min_y, U16 max_x, U16 max_y);
internal R2U16 r2u16_from_position_size(V2U16 position, V2U16 size);
internal R2U16 r2u16_intersect(R2U16 a, R2U16 b);
internal B32   r2u16_contains_v2u16(R2U16 bounds, V2U16 point);
internal B32   r2u16_contains_r2u16(R2U16 a, R2U16 b);
internal R2U16 r2u16_pad(R2U16 range, U16 pad);
internal V2U16 r2u16_size(R2U16 range);
internal V2U16 r2u16_center(R2U16 range);

internal R2U32 r2u32(U32 min_x, U32 min_y, U32 max_x, U32 max_y);
internal R2U32 r2u32_from_position_size(V2U32 position, V2U32 size);
internal R2U32 r2u32_intersect(R2U32 a, R2U32 b);
internal B32   r2u32_contains_v2u32(R2U32 bounds, V2U32 point);
internal B32   r2u32_contains_r2u32(R2U32 a, R2U32 b);
internal R2U32 r2u32_pad(R2U32 range, U32 pad);
internal V2U32 r2u32_size(R2U32 range);
internal V2U32 r2u32_center(R2U32 range);

internal R2U64 r2u64(U64 min_x, U64 min_y, U64 max_x, U64 max_y);
internal R2U64 r2u64_from_position_size(V2U64 position, V2U64 size);
internal R2U64 r2u64_intersect(R2U64 a, R2U64 b);
internal B32   r2u64_contains_v2u64(R2U64 bounds, V2U64 point);
internal B32   r2u64_contains_r2u64(R2U64 a, R2U64 b);
internal R2U64 r2u64_pad(R2U64 range, U64 pad);
internal V2U64 r2u64_size(R2U64 range);
internal V2U64 r2u64_center(R2U64 range);

internal R2S8 r2s8(S8 min_x, S8 min_y, S8 max_x, S8 max_y);
internal R2S8 r2s8_from_position_size(V2S8 position, V2S8 size);
internal R2S8 r2s8_intersect(R2S8 a, R2S8 b);
internal B32  r2s8_contains_v2s8(R2S8 bounds, V2S8 point);
internal B32  r2s8_contains_r2s8(R2S8 a, R2S8 b);
internal R2S8 r2s8_pad(R2S8 range, S8 pad);
internal V2S8 r2s8_size(R2S8 range);
internal V2S8 r2s8_center(R2S8 range);

internal R2S16 r2s16(S16 min_x, S16 min_y, S16 max_x, S16 max_y);
internal R2S16 r2s16_from_position_size(V2S16 position, V2S16 size);
internal R2S16 r2s16_intersect(R2S16 a, R2S16 b);
internal B32   r2s16_contains_v2s16(R2S16 bounds, V2S16 point);
internal B32   r2s16_contains_r2s16(R2S16 a, R2S16 b);
internal R2S16 r2s16_pad(R2S16 range, S16 pad);
internal V2S16 r2s16_size(R2S16 range);
internal V2S16 r2s16_center(R2S16 range);

internal R2S32 r2s32(S32 min_x, S32 min_y, S32 max_x, S32 max_y);
internal R2S32 r2s32_from_position_size(V2S32 position, V2S32 size);
internal R2S32 r2s32_intersect(R2S32 a, R2S32 b);
internal B32   r2s32_contains_v2s32(R2S32 bounds, V2S32 point);
internal B32   r2s32_contains_r2s32(R2S32 a, R2S32 b);
internal R2S32 r2s32_pad(R2S32 range, S32 pad);
internal V2S32 r2s32_size(R2S32 range);
internal V2S32 r2s32_center(R2S32 range);

internal R2S64 r2s64(S64 min_x, S64 min_y, S64 max_x, S64 max_y);
internal R2S64 r2s64_from_position_size(V2S64 position, V2S64 size);
internal R2S64 r2s64_intersect(R2S64 a, R2S64 b);
internal B32   r2s64_contains_v2s64(R2S64 bounds, V2S64 point);
internal B32   r2s64_contains_r2s64(R2S64 a, R2S64 b);
internal R2S64 r2s64_pad(R2S64 range, S64 pad);
internal V2S64 r2s64_size(R2S64 range);
internal V2S64 r2s64_center(R2S64 range);

internal R2F32 r2f32(F32 min_x, F32 min_y, F32 max_x, F32 max_y);
internal R2F32 r2f32_from_position_size(V2F32 position, V2F32 size);
internal R2F32 r2f32_intersect(R2F32 a, R2F32 b);
internal B32   r2f32_contains_v2f32(R2F32 bounds, V2F32 point);
internal B32   r2f32_contains_r2f32(R2F32 a, R2F32 b);
internal R2F32 r2f32_pad(R2F32 range, F32 pad);
internal V2F32 r2f32_size(R2F32 range);
internal V2F32 r2f32_center(R2F32 range);

internal R2F64 r2f64(F64 min_x, F64 min_y, F64 max_x, F64 max_y);
internal R2F64 r2f64_from_position_size(V2F64 position, V2F64 size);
internal R2F64 r2f64_intersect(R2F64 a, R2F64 b);
internal B32   r2f64_contains_v2f64(R2F64 bounds, V2F64 point);
internal B32   r2f64_contains_r2f64(R2F64 a, R2F64 b);
internal R2F64 r2f64_pad(R2F64 range, F64 pad);
internal V2F64 r2f64_size(R2F64 range);
internal V2F64 r2f64_center(R2F64 range);

#endif // BASE_VECTOR_H
