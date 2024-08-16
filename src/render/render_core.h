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
    Render_RectangleFlags_Texture      = 1 << 0,
    Render_RectangleFlags_MSDF         = 1 << 1,
    Render_RectangleFlags_AlphaMask    = 1 << 2,
} Render_RectangleFlags;

typedef struct Render_Rectangle Render_Rectangle;
struct Render_Rectangle {
    R2F32 rectangle;
    R2F32 uvs;
    V4F32 colors[4];
    F32   radies[4];
    F32   thickness;
    F32   softness;
    U32   flags;
};

typedef struct Render_RectangleChunk Render_RectangleChunk;
struct Render_RectangleChunk {
    Render_RectangleChunk *next;
    Render_Rectangle      *rectangles;
    U64                    count;
    U64                    capacity;
};

typedef struct Render_RectangleList Render_RectangleList;
struct Render_RectangleList {
    Render_RectangleChunk *first;
    Render_RectangleChunk *last;
    U64                    rectangle_count;
    U64                    chunk_count;
};

typedef struct Render_Batch Render_Batch;
struct Render_Batch {
    Render_Batch        *next;
    Render_RectangleList rectangles;
    Render_Texture       texture;
    R2F32                clip;
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
    U64 rectangle_count;
    U64 batch_count;
};

internal Render_Rectangle *render_rectangle_list_push(Arena *arena, Render_RectangleList *rectangles);

internal B32  render_init(Void);
internal Void render_create(Gfx_Context *gfx);

internal Void render_begin(V2U32 resolution);
internal Void render_submit(Render_BatchList batches);
internal Void render_end(Void);

// NOTE(simon): Texture API
internal Render_Texture render_texture_null(Void);
internal B32            render_texture_equal(Render_Texture a, Render_Texture b);
internal Render_Texture render_texture_create(V2U32 size, Render_TextureFormat format, U8 *data);
internal Void           render_texture_destroy(Render_Texture texture);
internal Void           render_texture_update(Render_Texture texture, V2U32 position, V2U32 size, U8 *data);
internal V2U32          render_size_from_texture(Render_Texture texture);

internal Render_Stats render_get_stats(Void);

#endif // RENDER_CORE_H
