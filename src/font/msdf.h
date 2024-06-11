#ifndef MSDF_H
#define MSDF_H

typedef enum {
    MSDF_SEGMENT_NULL,
    MSDF_SEGMENT_LINE,
    MSDF_SEGMENT_QUADRATIC_BEZIER,
    MSDF_SEGMENT_KIND_COUNT,
} MSDF_SegmentKind;

typedef enum {
    MSDF_COLOR_RED   = 0x01,
    MSDF_COLOR_GREEN = 0x02,
    MSDF_COLOR_BLUE  = 0x04,

    MSDF_EDGE_START = 0x08,
    MSDF_EDGE_END   = 0x10,
} MSDF_ColorFlags;

typedef enum {
    MSDF_ContourFlags_Flip = 0x01,
    MSDF_ContourFlags_Keep = 0x02,
} MSDF_ContourFlags;

typedef struct MSDF_Segment MSDF_Segment;
struct MSDF_Segment {
    MSDF_SegmentKind kind;
    MSDF_Segment *next;
    MSDF_Segment *previous;
    V2F32 p0;
    V2F32 p1;
    V2F32 p2;
    MSDF_ColorFlags flags;

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

    S32 x_min;
    S32 y_min;
    S32 x_max;
    S32 y_max;
};

typedef struct {
    F32 distance;
    F32 orthogonality;
    F32 unclamped_t;
} MSDF_Distance;

typedef struct {
    F32 x_min;
    F32 y_min;
    F32 x_max;
    F32 y_max;

    F32 advance_width;
    F32 left_side_bearing;

    U8 *data;
} MSDF_RasterResult;

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
internal Void msdf_correct_contour_orientation(MSDF_Glyph *glyph);

internal MSDF_RasterResult msdf_generate(Arena *arena, TTF_Font *font, U32 codepoint, U32 render_size);

#endif // MSDF_H
