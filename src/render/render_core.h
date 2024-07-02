#ifndef RENDER_CORE_H
#define RENDER_CORE_H

typedef struct Render_Context Render_Context;

typedef enum {
    Render_RectangleFlags_Texture   = 1 << 0,
    Render_RectangleFlags_MSDF      = 1 << 1,
    Render_RectangleFlags_AlphaMask = 1 << 2,
} Render_RectangleFlags;

typedef struct Render_Rectangle Render_Rectangle;
struct Render_Rectangle {
    V2F32 min;
    V2F32 max;
    V4F32 colors[4];
    V2F32 uv_min;
    V2F32 uv_max;
    U32   flags;
};

typedef enum {
    Render_TextureFormat_R8,
    Render_TextureFormat_RGBA8,
} Render_TextureFormat;

typedef union Render_Texture Render_Texture;
union Render_Texture {
    U32 u32[4];
};

internal B32  render_init(Void);
internal Void render_create(Gfx_Context *gfx);

internal Void render_begin(V2U32 resolution);
internal Void render_end(Void);

internal Render_Texture render_texture_create(V2U32 size, Render_TextureFormat format, U8 *data);
internal Void           render_texture_destroy(Render_Texture texture);
internal Void           render_texture_update(Render_Texture texture, V2U32 position, V2U32 size, U8 *data);
internal V2U32          render_size_from_texture(Render_Texture texture);

typedef struct Render_RectangleParams Render_RectangleParams;
struct Render_RectangleParams {
    V2F32                 min;
    V2F32                 max;
    V4F32                 color;
    V2F32                 uv_min;
    V2F32                 uv_max;
    Render_Texture        texture;
    Render_RectangleFlags flags;
};
#define render_rectangle(minimum, maximum, ...) render_rectangle_internal(&(Render_RectangleParams) { .min = minimum, .max = maximum, .color = v4f32(1.0f, 1.0f, 1.0f, 1.0f), __VA_ARGS__ })
internal Render_Rectangle *render_rectangle_internal(Render_RectangleParams *parameters);

#endif // RENDER_CORE_H
