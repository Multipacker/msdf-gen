#ifndef TTF_H
#define TTF_H

#define TTF_SCALER_TYPE_TRUE 0x74727565
#define TTF_SCALER_TYPE_1    0x00010000

#define TTF_MAGIC_NUMBER 0x5F0F3CF5

#define TTF_SIMPLE_GLYPH_FLAGS_ON_CURVE           0x01
#define TTF_SIMPLE_GLYPH_FLAGS_SHORT_X            0x02
#define TTF_SIMPLE_GLYPH_FLAGS_SHORT_Y            0x04
#define TTF_SIMPLE_GLYPH_FLAGS_REPEAT             0x08
#define TTF_SIMPLE_GLYPH_FLAGS_SAME_OR_POSITIVE_X 0x10
#define TTF_SIMPLE_GLYPH_FLAGS_SAME_OR_POSITIVE_Y 0x20

#define TTF_COMPOUND_GLYPH_FLAGS_ARG_1_AND_2_ARE_WORDS    0x0001
#define TTF_COMPOUND_GLYPH_FLAGS_ARGS_ARE_XY_VALUES       0x0002
#define TTF_COMPOUND_GLYPH_FLAGS_ROUND_XY_TO_GRID         0x0004
#define TTF_COMPOUND_GLYPH_FLAGS_WE_HAVE_A_SCALE          0x0008
// NOTE: 0x0010 is obsolete.
#define TTF_COMPOUND_GLYPH_FLAGS_MORE_COMPONENTS           0x0020
#define TTF_COMPOUND_GLYPH_FLAGS_WE_HAVE_AN_X_AND_Y_SCALE  0x0040
#define TTF_COMPOUND_GLYPH_FLAGS_WE_HAVE_A_TWO_BY_TWO      0x0080
#define TTF_COMPOUND_GLYPH_FLAGS_WE_HAVE_INSTRUCTIONS      0x0100
#define TTF_COMPOUND_GLYPH_FLAGS_USE_MY_METRICS            0x0200
#define TTF_COMPOUND_GLYPH_FLAGS_OVERLAP_COMPUND           0x0400
#define TTF_COMPOUND_GLYPH_FLAGS_SCALED_COMPONENT_OFFSET   0x0800
#define TTF_COMPOUND_GLYPH_FLAGS_UNSCALED_COMPONENT_OFFSET 0x1000

#define TTF_MAKE_TAG(a, b, c, d) ((U32) a << 24 | (U32) b << 16 | (U32) c << 8 | (U32) d)
#define TTF_MAKE_VERSION(major, minor) (((major) & 0xFFFF) << 16 | ((minor) & 0xFFFF))

typedef U16 TTF_ShortFrac;
typedef U32 TTF_Fixed;
typedef S16 TTF_FWord;
typedef U16 TTF_UFWord;
typedef U16 TTF_F2Dot14;
typedef U64 TTF_LongDateTime;

typedef enum {
    TTF_TABLE_CMAP,
    TTF_TABLE_GLYF,
    TTF_TABLE_HEAD,
    TTF_TABLE_HHEA,
    TTF_TABLE_HMTX,
    TTF_TABLE_LOCA,
    TTF_TABLE_MAXP,
    TTF_TABLE_MAX_REQUIRED = TTF_TABLE_MAXP,
    TTF_TABLE_COUNT,
} TTF_Tables;

global U32 ttf_table_tags[TTF_TABLE_COUNT] = {
    TTF_MAKE_TAG('c', 'm', 'a', 'p'),
    TTF_MAKE_TAG('g', 'l', 'y', 'f'),
    TTF_MAKE_TAG('h', 'e', 'a', 'd'),
    TTF_MAKE_TAG('h', 'h', 'e', 'a'),
    TTF_MAKE_TAG('h', 'm', 't', 'x'),
    TTF_MAKE_TAG('l', 'o', 'c', 'a'),
    TTF_MAKE_TAG('m', 'a', 'x', 'p'),
};

typedef struct {
    U32 scaler_type;
    U16 num_tables;
    U16 search_range;
    U16 entry_selector;
    U16 range_shift;
} TTF_OffsetSubtable;

typedef struct {
    U32 tag;
    U32 check_sum;
    U32 offset;
    U32 length;
} TTF_TableDirectoryEntry;

// NOTE: The TrueType spec states that platform IDs other than 0, 1, and 3 are
// allowed but ignored. Thus, we only list the ones we are interested in.
typedef enum {
    TTF_CMAP_PLATFORM_UNICODE = 0,
    TTF_CMAP_PLATFORM_WINDOWS = 3,
} TTF_CmapPlatformId;

typedef enum {
    TTF_CMAP_UNICODE_1_0                 = 0,
    TTF_CMAP_UNICODE_1_1                 = 1,
    TTF_CMAP_UNICODE_DEPRECATED          = 2,
    TTF_CMAP_UNICODE_2_0_BMP             = 3,
    TTF_CMAP_UNICODE_2_0_NON_BMP         = 4,
    TTF_CMAP_UNICODE_VARIATION_SEQUENCES = 5,
    TTF_CMAP_UNICODE_LAST_RESORT         = 6,
} TTF_CmapUnicodeId;

typedef enum {
    TTF_CMAP_WINDOWS_SYMBOL      = 0,
    TTF_CMAP_WINDOWS_UNICODE_BMP = 1,
    TTF_CMAP_WINDOWS_SHIFT_JIS   = 2,
    TTF_CMAP_WINDOWS_PRC         = 3,
    TTF_CMAP_WINDOWS_BIG_FIVE    = 4,
    TTF_CMAP_WINDOWS_JOHAB       = 5,
    TTF_CMAP_WINDOWS_UNICODE_4   = 10,
} TTF_CmapWindowsId;

typedef struct {
    U16 version;
    U16 number_subtables;
} TTF_CmapTable;

typedef struct {
    U16 platform_id;
    U16 platform_specific_id;
    U32 offset;
} TTF_CmapSubtable;

typedef struct {
    U16 format;
    U16 length;
    U16 language;
    U8 glyph_index_array[256];
} TTF_CmapFormat0;

typedef struct {
    U16 format;
    U16 length;
    U16 language;
    U16 seg_count_x2;
    U16 search_range;
    U16 entry_selector;
    U16 range_shift;

    // NOTE: These are variable segments and can thus not be represented in the struct.
    // U16 end_code[seg_count];
    // U16 reserved_pad;
    // U16 start_code[seg_count];
    // U16 id_delta[seg_count];
    // U16 id_range_offset[seg_count];
    // U16 glyph_index_array[variable]; // UGH...
} TTF_CmapFormat4;

typedef struct {
    U16 format;
    U16 length;
    U16 language;
    U16 first_code;
    U16 entry_count;
    U16 glyph_index_array[];
} TTF_CmapFormat6;

typedef struct {
    U16 format;
    U16 reserved;
    U32 length;
    U32 language;
    U32 n_groups;
    struct {
        U32 start_char_code;
        U32 end_char_code;
        U32 start_glyph_code;
    } groups[];
} TTF_CmapFormat12;

typedef struct {
    S16 number_of_contours;
    TTF_FWord x_min;
    TTF_FWord y_min;
    TTF_FWord x_max;
    TTF_FWord y_max;
} TTF_GlyphHeader;

typedef packed_struct({
    TTF_Fixed        version;
    TTF_Fixed        font_revision;
    U32              check_sum_adjustment;
    U32              magic_number;
    U16              flags;
    U16              units_per_em;
    TTF_LongDateTime created;
    TTF_LongDateTime modified;
    TTF_FWord        x_min;
    TTF_FWord        y_min;
    TTF_FWord        x_max;
    TTF_FWord        y_max;
    U16              mac_style;
    U16              lowest_rec_ppem;
    S16              font_direction_hint;
    S16              index_to_loc_format;
    S16              glyph_data_format;
}) TTF_HeadTable;

typedef struct {
    TTF_Fixed  version;
    TTF_FWord  ascent;
    TTF_FWord  descent;
    TTF_FWord  line_gap;
    TTF_UFWord advance_width_max;
    TTF_FWord  min_left_side_bearing;
    TTF_FWord  min_right_side_bearing;
    TTF_FWord  x_max_extent;
    S16        caret_slope_rise;
    S16        caret_slope_run;
    TTF_FWord  caret_offset;
    S16        reserved0;
    S16        reserved1;
    S16        reserved2;
    S16        reserved3;
    S16        metric_data_format;
    U16        num_of_long_hor_metrics;
} TTF_HheaTable;

typedef struct {
    TTF_UFWord advance_width;
    TTF_FWord  left_side_bearing;
} TTF_HmtxMetrics;

typedef struct {
    TTF_Fixed version;
    U16 num_glyphs;
    U16 max_points;
    U16 max_contours;
    U16 max_component_points;
    U16 max_component_contours;
    U16 max_zones;
    U16 max_twilight_points;
    U16 max_storage;
    U16 max_function_defs;
    U16 max_instruction_defs;
    U16 max_stack_elements;
    U16 max_size_of_instructions;
    U16 max_component_elements;
    U16 max_component_depth;
} TTF_MaxpTable;

typedef struct {
    TTF_FWord x_min;
    TTF_FWord y_min;
    TTF_FWord x_max;
    TTF_FWord y_max;
    U32 contour_count;
    U32 point_count;
    U16       *contour_end_points;
    U8        *flags;
    TTF_FWord *x_coordinates;
    TTF_FWord *y_coordinates;
} TTF_Glyph;

// Used ONLY for parsing
typedef struct {
    Str8 tables[TTF_TABLE_COUNT];

    B32 is_long_loca_format;

    U16 glyph_count;

    U32        contour_capacity;
    U16       *contour_end_points_buffer;
    U32        point_capacity;
    U8        *flag_buffer;
    TTF_FWord *x_buffer;
    TTF_FWord *y_buffer;

    U16 *ttf_to_internal_glyph_indicies; // NOTE: The stored indicies are 1-based.
    U16 *internal_to_ttf_glyph_indicies;
    U16 internal_glyph_count;

    U32 codepoint_count;
    U32 *codepoints;
    U32 *glyph_indicies;

    U16 funits_per_em;
    U16 lowest_rec_ppem;
} TTF_Font;

internal B32 ttf_load(Arena *arena, Arena *scratch, FontDescription *font_description, TTF_Font *ttf_font);
internal B32  ttf_parse_metric_data(TTF_Font *font, Font *result_font);

#endif // TTF_H
