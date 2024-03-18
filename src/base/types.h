#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef int8_t  S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef U8  B8;
typedef U16 B16;
typedef U32 B32;
typedef U64 B64;

typedef float  F32;
typedef double F64;

typedef void Void;
typedef char *CStr;
typedef Void VoidFunction(Void);

typedef enum {
    Month_Jan,
    Month_Feb,
    Month_Mar,
    Month_Apr,
    Month_May,
    Month_Jun,
    Month_Jul,
    Month_Aug,
    Month_Sep,
    Month_Oct,
    Month_Nov,
    Month_Dec,
} Month;

typedef enum {
    Day_Monday,
    Day_Tuesday,
    Day_Wednesday,
    Day_Thursday,
    Day_Friday,
    Day_Saturday,
    Day_Sunday,
} Day;

typedef enum {
    OperatingSystem_Null,
    OperatingSystem_Windows,
    OperatingSystem_Linux,
    OperatingSystem_Mac,
    OperatingSystem_COUNT,
} OperatingSystem;

typedef enum {
    Architecture_Null,
    Architecture_X64,
    Architecture_X86,
    Architecture_Arm,
    Architecture_Arm64,
    Architecture_COUNT,
} Architecture;

typedef enum {
    DATA_ACCESS_FLAGS_READ    = (1 << 0),
    DATA_ACCESS_FLAGS_WRITE   = (1 << 1),
    DATA_ACCESS_FLAGS_EXECUTE = (1 << 2),
} DataAccessFlags;

typedef U64 DenseTime;

typedef struct {
    U16 millisecond;  // [0, 999]
    U8  second;       // [0, 60] 60 in the case of leap seconds
    U8  minute;       // [0, 59]
    U8  hour;         // [0, 23]
    U8  day;          // [0, 30]
    U8  month;        // [0, 11]
    S16 year;        // 1 = 1 CE; 2020 = 2020 CE, 0 = 1 BCE; -100 = 101 BCE; etc.
} DateTime;

typedef enum {
    FILE_PROPERTY_FLAGS_IS_FOLDER = (1 << 0),
} FilePropertyFlags;

// NOTE: DenseTime is in universal time by default.
typedef struct {
    U64 size;
    FilePropertyFlags flags;
    DenseTime create_time;
    DenseTime modify_time;
    DataAccessFlags access;
} FileProperties;

global S8  S8_MIN  = 0x80;
global S16 S16_MIN = 0x8000;
global S32 S32_MIN = 0x80000000;
global S64 S64_MIN = 0x8000000000000000ll;

global S8  S8_MAX  = 0x7F;
global S16 S16_MAX = 0x7FFF;
global S32 S32_MAX = 0x7FFFFFFF;
global S64 S64_MAX = 0x7FFFFFFFFFFFFFFFll;

global U8  U8_MAX  = 0xFF;
global U16 U16_MAX = 0xFFFF;
global U32 U32_MAX = 0xFFFFFFFF;
global U64 U64_MAX = 0xFFFFFFFFFFFFFFFFllu;

global F32 F32_EPSILON = 1.1920929e-7f;
global F32 F32_PI      = 3.14159265359f;
global F32 F32_TAU     = 6.28318530718f;
global F32 F32_E       = 2.71828182846f;
global U32 F32_BIAS    = 127;

global F64 F64_EPSILON = 2.220446e-16;
global F64 F64_PI      = 3.14159265359;
global F64 F64_TAU     = 6.28318530718;
global F64 F64_E       = 2.71828182846;
global U32 F64_BIAS    = 1023;

#define false 0
#define true  1

internal U8 u8_min(U8 a, U8 b);
internal U8 u8_max(U8 a, U8 b);
internal U8 u8_round_down_to_power_of_2(U8 value, U8 power);
internal U8 u8_round_up_to_power_of_2(U8 value, U8 power);
internal U8 u8_floor_to_power_of_2(U8 value);
internal U8 u8_rotate_right(U8 x, U8 amount);
internal U8 u8_rotate_left(U8 x, U8 amount);
internal U8 u8_ceil_to_power_of_2(U8 x);
internal U8 u8_reverse(U8 x);

internal U16 u16_min(U16 a, U16 b);
internal U16 u16_max(U16 a, U16 b);
internal U16 u16_round_down_to_power_of_2(U16 value, U16 power);
internal U16 u16_round_up_to_power_of_2(U16 value, U16 power);
internal U16 u16_floor_to_power_of_2(U16 value);
internal U16 u16_rotate_right(U16 x, U16 amount);
internal U16 u16_rotate_left(U16 x, U16 amount);
internal U16 u16_ceil_to_power_of_2(U16 x);
internal U16 u16_reverse(U16 x);
internal U16 u16_big_to_local_endian(U16 x);

internal U32 u32_min(U32 a, U32 b);
internal U32 u32_max(U32 a, U32 b);
internal U32 u32_round_down_to_power_of_2(U32 value, U32 power);
internal U32 u32_round_up_to_power_of_2(U32 value, U32 power);
internal U32 u32_floor_to_power_of_2(U32 value);
internal U32 u32_rotate_right(U32 x, U32 amount);
internal U32 u32_rotate_left(U32 x, U32 amount);
internal U32 u32_ceil_to_power_of_2(U32 x);
internal U32 u32_reverse(U32 x);
internal U32 u32_big_to_local_endian(U32 x);

internal U64 u64_min(U64 a, U64 b);
internal U64 u64_max(U64 a, U64 b);
internal U64 u64_round_down_to_power_of_2(U64 value, U64 power);
internal U64 u64_round_up_to_power_of_2(U64 value, U64 power);
internal U64 u64_floor_to_power_of_2(U64 value);
internal U64 u64_rotate_right(U64 x, U64 amount);
internal U64 u64_rotate_left(U64 x, U64 amount);
internal U64 u64_ceil_to_power_of_2(U64 x);
internal U64 u64_reverse(U64 x);
internal U64 u64_big_to_local_endian(U64 x);

internal S8 s8_min(S8 a, S8 B);
internal S8 s8_max(S8 a, S8 B);
internal S8 s8_abs(S8 x);

internal S16 s16_min(S16 a, S16 b);
internal S16 s16_max(S16 a, S16 b);
internal S16 s16_abs(S16 x);
internal S16 s16_big_to_local_endian(S16 x);

internal S32 s32_min(S32 a, S32 b);
internal S32 s32_max(S32 a, S32 b);
internal S32 s32_abs(S32 x);
internal S32 s32_big_to_local_endian(S32 x);

internal S64 s64_min(S64 a, S64 b);
internal S64 s64_max(S64 a, S64 b);
internal S64 s64_abs(S64 x);
internal S64 s64_big_to_local_endian(S64 x);

internal F32 f32_infinity(Void);
internal F32 f32_negative_infinity(Void);

internal F64 f64_infinity(Void);
internal F64 f64_negative_infinity(Void);

internal F32 f32_min(F32 a, F32 b);
internal F32 f32_max(F32 a, F32 b);
internal F32 f32_sign(F32 x);
internal F32 f32_abs(F32 x);
internal F32 f32_sqrt(F32 x);
internal F32 f32_cbrt(F32 x);
internal F32 f32_sin(F32 x);
internal F32 f32_cos(F32 x);
internal F32 f32_tan(F32 x);
internal F32 f32_arctan(F32 x);
internal F32 f32_arctan2(F32 y, F32 x);
internal F32 f32_ln(F32 x);
internal F32 f32_log(F32 x);
internal F32 f32_log2(F32 x);
internal F32 f32_lerp(F32 a, F32 b, F32 t);
internal F32 f32_unlerp(F32 a, F32 b, F32 x);
internal F32 f32_pow(F32 a, F32 b);
internal F32 f32_floor(F32 x);
internal F32 f32_ceil(F32 x);
internal U32 f32_round_to_u32(F32 x);
internal S32 f32_round_to_s32(F32 x);
internal U32 f32_solve_cubic(F32 a, F32 b, F32 c, F32 d, F32 *result_ts);

internal F64 f64_min(F64 a, F64 b);
internal F64 f64_max(F64 a, F64 b);
internal F64 f64_abs(F64 x);
internal F64 f64_sqrt(F64 x);
internal F64 f64_sin(F64 x);
internal F64 f64_cos(F64 x);
internal F64 f64_tan(F64 x);
internal F64 f64_ln(F64 x);
internal F64 f64_lg(F64 x);
internal F64 f64_lerp(F64 a, F64 b, F64 t);
internal F64 f64_unlerp(F64 a, F64 b, F64 x);
internal F64 f64_pow(F64 a, F64 b);
internal F64 f64_floor(F64 x);
internal F64 f64_ceil(F64 x);

internal DenseTime dense_time_from_date_time(DateTime *date_time);
internal DateTime  date_time_from_dense_time(DenseTime dense_time);

#endif // TYPES_H
