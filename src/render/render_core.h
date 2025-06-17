#ifndef RENDER_CORE_H
#define RENDER_CORE_H

typedef enum {
    Render_TextureFormat_R8,
    Render_TextureFormat_RGBA8,
} Render_TextureFormat;

typedef struct Render_Texture Render_Texture;
struct Render_Texture {
    U32 u32[4];
};

typedef enum {
    Render_ShapeFlag_Texture   = 1 << 0,
    Render_ShapeFlag_MSDF      = 1 << 1,
    Render_ShapeFlag_AlphaMask = 1 << 2,
    Render_ShapeFlag_Line      = 1 << 3,
} Render_ShapeFlags;

typedef struct Render_Shape Render_Shape;
struct Render_Shape {
    R2F32 position;
    R2F32 source;
    V4F32 colors[4];
    F32   radies[4];
    F32   thickness;
    F32   softness;
    U32   flags;
};

typedef struct Render_ShapeChunk Render_ShapeChunk;
struct Render_ShapeChunk {
    Render_ShapeChunk *next;
    Render_Shape      *shapes;
    U64                count;
    U64                capacity;
};

typedef struct Render_ShapeList Render_ShapeList;
struct Render_ShapeList {
    Render_ShapeChunk *first;
    Render_ShapeChunk *last;
    U64                shape_count;
    U64                chunk_count;
};

typedef enum {
    Render_Filtering_Nearest,
    Render_Filtering_Linear,
    Render_Filtering_COUNT,
} Render_Filtering;

typedef struct Render_Batch Render_Batch;
struct Render_Batch {
    Render_Batch    *next;
    Render_ShapeList shapes;
    Render_Texture   texture;
    R2F32            clip;
    M3F32            transform;
    Render_Filtering filtering;
};

typedef struct Render_BatchList Render_BatchList;
struct Render_BatchList {
    Render_Batch *first;
    Render_Batch *last;
    U64           count;
};

typedef struct Render_Stats Render_Stats;
struct Render_Stats {
    U64 bytes_uploaded_to_gpu;
    U64 shape_count;
    U64 batch_count;
};

typedef struct Render_Window Render_Window;
struct Render_Window {
    U64 u64[1];
};

internal Render_Shape *render_shape_list_push(Arena *arena, Render_ShapeList *shapes);

internal B32  render_init(Void);
internal Void render_begin(Void);
internal Void render_end(Void);

internal Render_Window render_create(Gfx_Window handle);
internal Void          render_destroy(Gfx_Window graphics_handle, Render_Window render_handle);
internal Void          render_window_begin(Gfx_Window graphics_handle, Render_Window render_handle);
internal Void          render_window_submit(Gfx_Window graphics_handle, Render_Window render_handle, Render_BatchList batches);
internal Void          render_window_end(Gfx_Window graphics_handle, Render_Window render_handle);

// NOTE(simon): Texture API
internal Render_Texture render_texture_null(Void);
internal B32            render_texture_equal(Render_Texture a, Render_Texture b);
internal Render_Texture render_texture_create(V2U32 size, Render_TextureFormat format, U8 *data);
internal Void           render_texture_destroy(Render_Texture texture);
internal Void           render_texture_update(Render_Texture texture, V2U32 position, V2U32 size, U8 *data);
internal V2U32          render_size_from_texture(Render_Texture texture);

internal Render_Stats render_get_stats(Void);

#endif // RENDER_CORE_H
