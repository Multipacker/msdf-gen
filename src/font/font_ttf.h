#ifndef TTF_H
#define TTF_H

#define TTF_MAGIC_NUMBER 0x5F0F3CF5

typedef enum {
    TTF_SimpleGlyphFlags_OnCurve         = 1 << 0,
    TTF_SimpleGlyphFlags_ShortX          = 1 << 1,
    TTF_SimpleGlyphFlags_ShortY          = 1 << 2,
    TTF_SimpleGlyphFlags_Repeat          = 1 << 3,
    TTF_SimpleGlyphFlags_SameOrPositiveX = 1 << 4,
    TTF_SimpleGlyphFlags_SameOrPositiveY = 1 << 5,
} TTF_SimpleGlyphFlags;

typedef enum {
    TTF_CompoundGlyphFlag_Arg1And2AreWords        = 1 << 0,
    TTF_CompoundGlyphFlag_ArgsAreXYValues         = 1 << 1,
    TTF_CompoundGlyphFlag_RoundXYToGrid           = 1 << 2,
    TTF_CompoundGlyphFlag_WeHaveAScale            = 1 << 3,
    // NOTE(simon): 1 << 4 is obsolete.
    TTF_CompoundGlyphFlag_MoreComponents          = 1 << 5,
    TTF_CompoundGlyphFlag_WeHaveAnXAndYScale      = 1 << 6,
    TTF_CompoundGlyphFlag_WeHaveATwoByTwo         = 1 << 7,
    TTF_CompoundGlyphFlag_WeHaveInstructions      = 1 << 8,
    TTF_CompoundGlyphFlag_UseMyMetrics            = 1 << 9,
    TTF_CompoundGlyphFlag_OverlapCompund          = 1 << 10,
    TTF_CompoundGlyphFlag_ScaledComponentOffset   = 1 << 11,
    TTF_CompoundGlyphFlag_UnscaledComponentOffset = 1 << 12,
} TTF_CompoundGlyphFlags;

#define TTF_MAKE_TAG(a, b, c, d) ((U32) a << 24 | (U32) b << 16 | (U32) c << 8 | (U32) d)
#define TTF_MAKE_VERSION(major, minor) (((major) & 0xFFFF) << 16 | ((minor) & 0xFFFF))

#define TTF_SCALER_TYPE_TRUE       0x74727565
#define TTF_SCALER_TYPE_1          0x00010000
#define TTF_SCALER_TYPE_OPEN       0x4F54544F
#define TTF_SCALER_TYPE_COLLECTION TTF_MAKE_TAG('t', 't', 'c', 'f')

typedef U16 TTF_ShortFrac;
typedef U32 TTF_Fixed;
typedef S16 TTF_FWord;
typedef U16 TTF_UFWord;
typedef U16 TTF_F2Dot14;
typedef U64 TTF_LongDateTime;

#define TTF_TABLES                      \
    X(Cmap, 'c', 'm', 'a', 'p')         \
    X(Glyf, 'g', 'l', 'y', 'f')         \
    X(Head, 'h', 'e', 'a', 'd')         \
    X(Hhea, 'h', 'h', 'e', 'a')         \
    X(Hmtx, 'h', 'm', 't', 'x')         \
    X(Loca, 'l', 'o', 'c', 'a')         \
    X(Maxp, 'm', 'a', 'x', 'p')         \
    /* NOTE(simon): Optional tables. */ \
    X(Fpgm, 'f', 'p', 'g', 'm')

#define X(name, ...) TTF_Table_##name,
typedef enum {
    TTF_TABLES
    TTF_Table_COUNT,
    TTF_Table_MaxRequired = TTF_Table_Maxp,
} TTF_Tables;
#undef X

#define X(name, a, b, c, d) [TTF_Table_##name] = TTF_MAKE_TAG(a, b, c, d),
global U32 ttf_table_tags[TTF_Table_COUNT] = {
    TTF_TABLES
};
#undef X

// NOTE: The TrueType spec states that platform IDs other than 0, 1, and 3 are
// allowed but ignored. Thus, we only list the ones we are interested in.
typedef enum {
    TTF_CmapPlatform_Unicode = 0,
    TTF_CmapPlatform_Windows = 3,
    TTF_CmapPlatform_COUNT,
} TTF_CmapPlatformId;

typedef enum {
    TTF_CmapUnicode_1_0                = 0,
    TTF_CmapUnicode_1_1                = 1,
    TTF_CmapUnicode_Deprecated         = 2,
    TTF_CmapUnicode_2_0_Bmp            = 3,
    TTF_CmapUnicode_2_0_NonBmp         = 4,
    TTF_CmapUnicode_VariationSequences = 5,
    TTF_CmapUnicode_LastResort         = 6,
} TTF_CmapUnicodeId;

typedef enum {
    TTF_CmapWindows_Symbol     = 0,
    TTF_CmapWindows_UnicodeBmp = 1,
    TTF_CmapWindows_ShiftJis   = 2,
    TTF_CmapWindows_Prc        = 3,
    TTF_CmapWindows_BigFive    = 4,
    TTF_CmapWindows_Johab      = 5,
    TTF_CmapWindows_Unicode4   = 10,
} TTF_CmapWindowsId;

#define TTF_CMAP_MAX_PLATFORM_SPECIFIC_ID 11

global U32 ttf_cmap_subtable_ids_to_rank[TTF_CmapPlatform_COUNT][TTF_CMAP_MAX_PLATFORM_SPECIFIC_ID] = {
    [TTF_CmapPlatform_Unicode] = {
        [TTF_CmapUnicode_1_0]                = 3,
        [TTF_CmapUnicode_1_1]                = 4,
        [TTF_CmapUnicode_Deprecated]         = 0,
        [TTF_CmapUnicode_2_0_Bmp]            = 2,
        [TTF_CmapUnicode_2_0_NonBmp]         = 6,
        [TTF_CmapUnicode_VariationSequences] = 0,
        [TTF_CmapUnicode_LastResort]         = 0,
    },
    [TTF_CmapPlatform_Windows] = {
        [TTF_CmapWindows_Symbol]     = 0,
        [TTF_CmapWindows_UnicodeBmp] = 1,
        [TTF_CmapWindows_ShiftJis]   = 0,
        [TTF_CmapWindows_Prc]        = 0,
        [TTF_CmapWindows_BigFive]    = 0,
        [TTF_CmapWindows_Johab]      = 0,
        [TTF_CmapWindows_Unicode4]   = 5,
    },
};

typedef struct {
    TTF_UFWord advance_width;
    TTF_FWord  left_side_bearing;
} TTF_HmtxMetrics;

typedef struct TTF_Glyph TTF_Glyph;
struct TTF_Glyph {
    // NOTE(simon): Bounds.
    V2F32 min;
    V2F32 max;

    // NOTE(simon): Compound glyph data.
    S32        component_count;
    TTF_Glyph *components;
    M3F32     *transforms;
    V2S32     *alignment;

    // NOTE(simon): Simple glyph data.
    S32    contour_count;
    S32    point_count;
    U16   *contour_end_points;
    U8    *point_flags;
    V2F32 *point_coordinates;
};

typedef struct TTF_CodepointRange TTF_CodepointRange;
struct TTF_CodepointRange {
    U32 first_codepoint;
    U32 first_glyph_index;
    U32 size;
};

typedef struct TTF_CodepointMap TTF_CodepointMap;
struct TTF_CodepointMap {
    TTF_CodepointRange *ranges;
    U32 range_count;
    U32 codepoint_count;
};

typedef struct TTF_CodepointRangeNode TTF_CodepointRangeNode;
struct TTF_CodepointRangeNode {
    TTF_CodepointRangeNode *next;
    TTF_CodepointRangeNode *previous;
    TTF_CodepointRange range;
};

typedef struct TTF_CodepointRangeList TTF_CodepointRangeList;
struct TTF_CodepointRangeList {
    TTF_CodepointRangeNode *first;
    TTF_CodepointRangeNode *last;
    U32 range_count;
};

typedef struct TTF_Font TTF_Font;
struct TTF_Font {
    Str8 tables[TTF_Table_COUNT];

    Str8 *raw_glyph_data;
    TTF_HmtxMetrics *metrics;

    U16 glyph_count;

    TTF_FWord ascent;
    TTF_FWord descent;
    U16 funits_per_em;
    U16 lowest_rec_ppem;

    TTF_CodepointMap codepoint_map;
};

global TTF_Font ttf_font_nil = { 0 };

internal U32 ttf_glyph_index_from_font_codepoint(TTF_Font *font, U32 codepoint);

internal TTF_Font *ttf_load(Arena *arena, Str8 font_path);

#endif // TTF_H
