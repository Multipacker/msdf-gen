#ifndef MSDF_H
#define MSDF_H

typedef enum {
    MSDF_Segment_Null,
    MSDF_Segment_Line,
    MSDF_Segment_QuadraticBezier,
    MSDF_Segment_COUNT,
} MSDF_SegmentKind;

typedef enum {
    MSDF_SegmentFlag_Red   = 1 << 0,
    MSDF_SegmentFlag_Green = 1 << 1,
    MSDF_SegmentFlag_Blue  = 1 << 2,
    MSDF_SegmentFlag_Start = 1 << 3,
    MSDF_SegmentFlag_End   = 1 << 4,
} MSDF_SegmentFlags;

typedef enum {
    MSDF_ContourFlag_Flip = 1 << 0,
    MSDF_ContourFlag_Keep = 1 << 1,
} MSDF_ContourFlags;

typedef struct MSDF_Segment MSDF_Segment;
struct MSDF_Segment {
    MSDF_SegmentKind kind;
    MSDF_Segment *next;
    MSDF_Segment *previous;
    V2F32 p0;
    V2F32 p1;
    V2F32 p2;
    MSDF_SegmentFlags flags;

    // NOTE(simon): Bounding circle for pruning
    V2F32 circle_center;
    F32   circle_radius;
};

typedef struct MSDF_SegmentList MSDF_SegmentList;
struct MSDF_SegmentList {
    MSDF_Segment *first;
    MSDF_Segment *last;
};

typedef struct MSDF_Contour MSDF_Contour;
struct MSDF_Contour {
    MSDF_Contour *next;
    MSDF_Contour *previous;
    MSDF_Segment *first_segment;
    MSDF_Segment *last_segment;
    MSDF_ContourFlags flags;
    S32 local_winding;
};

typedef struct MSDF_Glyph MSDF_Glyph;
struct MSDF_Glyph {
    MSDF_Contour *first_contour;
    MSDF_Contour *last_contour;

    V2F32 min;
    V2F32 max;
};

typedef struct {
    F32 distance;
    F32 orthogonality;
    F32 unclamped_t;
} MSDF_Distance;



// NOTE(simon): Structured logging for debugging.

typedef enum {
    MSDF_LogNodeFlag_DrawPoint  = 1 << 0,
    MSDF_LogNodeFlag_DrawLine   = 1 << 1,
    MSDF_LogNodeFlag_DrawBezier = 1 << 2,
} MSDF_LogNodeFlags;

typedef struct MSDF_LogNode MSDF_LogNode;
struct MSDF_LogNode {
    // NOTE(simon): Structure links.
    MSDF_LogNode *next;
    MSDF_LogNode *previous;
    MSDF_LogNode *first;
    MSDF_LogNode *last;
    MSDF_LogNode *parent;

    MSDF_LogNodeFlags flags;

    Str8 string;

    // NOTE(simon): Points for geometric data. Start from lowest, use more as
    // needed.
    V4F32 color;
    V2F32 p0;
    V2F32 p1;
    V2F32 p2;
};

typedef struct MSDF_LogNodeStack MSDF_LogNodeStack;
struct MSDF_LogNodeStack {
    MSDF_LogNodeStack *next;
    MSDF_LogNode      *node;
};

typedef struct MSDF_LogNodeIterator MSDF_LogNodeIterator;
struct MSDF_LogNodeIterator {
    MSDF_LogNode *next;
    U32           push_count;
    U32           pop_count;
};

typedef struct MSDF_LogState MSDF_LogState;
struct MSDF_LogState {
    Arena             *arena;
    Arena             *scratch_arena;
    MSDF_LogNodeStack *parent_stack;
    MSDF_LogNodeStack *stack_freelist;
};



typedef struct MSDF_RasterResult MSDF_RasterResult;
struct MSDF_RasterResult {
    V2F32 min;
    V2F32 max;

    U32 glyph_index;

    F32 advance_width;
    F32 left_side_bearing;

    V2U32 size;
    U8 *data;

    MSDF_LogNode *logs;
};

// NOTE(simon): Base functions for creating logs.
internal MSDF_LogNode        *msdf_log_create_node(Void);
internal MSDF_LogNode        *msdf_log_create_node_from_string(Str8 label);
internal MSDF_LogNode        *msdf_log_create_node_from_string_format(CStr format, ...);
internal MSDF_LogNode        *msdf_log_create_node_from_string_format_list(CStr format, va_list arguments);
internal Void                 msdf_log_node_set_string(MSDF_LogNode *node, Str8 string);
internal Void                 msdf_log_node_set_string_format(MSDF_LogNode *node, CStr format, ...);
internal Void                 msdf_log_push_parent(MSDF_LogNode *node);
internal Void                 msdf_log_pop_parent(Void);
internal MSDF_LogNodeIterator msdf_log_iterator_depth_first_pre_order(MSDF_LogNode *node);

// NOTE(simon): Helpers for basic geometry.
internal MSDF_LogNode *msdf_log_push_parent_string(Str8 string);
internal MSDF_LogNode *msdf_log_push_parent_string_format(CStr format, ...);
#define msdf_log_parent_string(string)             defer_loop(msdf_log_push_parent_string(string), msdf_log_pop_parent())
#define msdf_log_parent_string_format(string, ...) defer_loop(msdf_log_push_parent_string_format(string, __VA_ARGS__), msdf_log_pop_parent())
internal MSDF_LogNode *msdf_log_create_point(V2F32 p0, V4F32 color);
internal MSDF_LogNode *msdf_log_create_line(V2F32 p0, V2F32 p1, V4F32 color);
internal MSDF_LogNode *msdf_log_create_bezier(V2F32 p0, V2F32 p1, V2F32 p2, V4F32 color);

// NOTE(simon): Helpers for glyph geometry.
internal MSDF_LogNode *msdf_log_create_segment(MSDF_Segment *segment, V4F32 color);
internal MSDF_LogNode *msdf_log_create_contour(MSDF_Contour *contour, V4F32 color);
internal MSDF_LogNode *msdf_log_create_glyph(MSDF_Glyph *glyph, V4F32 color);



internal B32 msdf_distance_is_closer(MSDF_Distance a, MSDF_Distance b);

internal B32 msdf_is_corner(MSDF_Segment a, MSDF_Segment b, F32 threshold);

internal MSDF_Distance msdf_line_distance_orthogonality(V2F32 point, MSDF_Segment line);
internal MSDF_Distance msdf_quadratic_bezier_distance_orthogonality(V2F32 point, MSDF_Segment bezier);

internal F32 msdf_line_signed_pseudo_distance(V2F32 point, MSDF_Segment line);
internal F32 msdf_quadratic_bezier_signed_pseudo_distance(V2F32 point, MSDF_Segment bezier, F32 clamped_t);

internal Void msdf_segment_split(MSDF_Segment segment, F32 t, MSDF_Segment *result_a, MSDF_Segment *result_b);
internal U32 msdf_segment_intersect(MSDF_Segment a, MSDF_Segment b, F32 *result_ats, F32 *result_bts);

internal S32 msdf_contour_calculate_own_winding_number(MSDF_Contour *contour);
internal S32 msdf_contour_calculate_winding_number(MSDF_Contour *contour, V2F32 point);

internal Void msdf_resolve_contour_overlap(Arena *arena, MSDF_Glyph *glyph);
internal Void msdf_convert_to_simple_polygons(Arena *arena, MSDF_Glyph *glyph);
internal Void msdf_correct_contour_orientation(Arena *arena, MSDF_Glyph *glyph);

internal MSDF_RasterResult msdf_generate_from_glyph_index(Arena *arena, TTF_Font *font, U32 glyph_index, U32 render_size);
internal MSDF_RasterResult msdf_generate_from_codepoint(Arena *arena, TTF_Font *font, U32 codepoint, U32 render_size);

#endif // MSDF_H
