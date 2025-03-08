#ifndef MSDF_CACHE_INCLUDE_H
#define MSDF_CACHE_INCLUDE_H

typedef struct MSDFCache_Glyph MSDFCache_Glyph;
struct MSDFCache_Glyph {
    MSDFCache_Glyph *next;
    MSDFCache_Glyph *previous;

    U32 glyph_index;

    R2F32 rectangle_pt;
    F32   advance_pt;
    R2F32 uv;
    Render_Texture texture;

    MSDF_Log log;

    B32 loaded;
};

global MSDFCache_Glyph global_glyph_null = { 0 };

typedef struct MSDFCache_GlyphList MSDFCache_GlyphList;
struct MSDFCache_GlyphList {
    MSDFCache_Glyph *first;
    MSDFCache_Glyph *last;
};

// NOTE(simon): Must be a power of 2
#define ATLAS_GLYPHS_PER_SIDE (1 << 6)
typedef struct MSDFCache_Atlas MSDFCache_Atlas;
struct MSDFCache_Atlas {
    MSDFCache_Atlas *next;
    MSDFCache_Atlas *previous;

    Render_Texture texture;
    U64 occupancy[(ATLAS_GLYPHS_PER_SIDE * ATLAS_GLYPHS_PER_SIDE + 63) / 64];
};

// TODO(simon): This should be done using a handle to the font and not a
// pointer.
typedef struct MSDFCache_Request MSDFCache_Request;
struct MSDFCache_Request {
    U32 glyph_index;
    TTF_Font *font;
};

typedef struct MSDFCache_Result MSDFCache_Result;
struct MSDFCache_Result {
    Arena *arena;
    MSDF_RasterResult raster;
};

typedef struct MSDFCache_State MSDFCache_State;
struct MSDFCache_State {
    Arena *arena;
    Arena *glyph_arena;

    U32 glyph_size;
    U32 glyphs_per_side;

    MSDFCache_Atlas *first_atlas;
    MSDFCache_Atlas *last_atlas;

    MSDFCache_GlyphList glyph_lists[2048];

    MSDFCache_Request *request_buffer;
    U32                request_size;
    U32                request_write;
    U32                request_read;
    OS_Mutex             request_mutex;
    OS_ConditionVariable request_condition_variable;

    MSDFCache_Result *result_buffer;
    U32               result_size;
    U32               result_write;
    U32               result_read;
    OS_Mutex             result_mutex;
    OS_ConditionVariable result_condition_variable;

    VoidFunction *wakeup;
};

internal Void msdf_font_create(U32 glyph_size, VoidFunction *wakeup);
internal Void msdf_cache_update(Void);
internal Void msdf_cache_clear(Void);

internal Void msdf_cache_generate_glyphs_thread_entry(Void *data);

internal MSDFCache_Glyph *msdf_cache_get_glyph(TTF_Font *font, U32 codepoint);

#endif // MSDF_CACHE_INCLUDE_H
