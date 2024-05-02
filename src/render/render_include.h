#ifndef RENDER_INCLUDE_H
#define RENDER_INCLUDE_H

#include "opengl/opengl_include.h"

typedef struct Render_Context Render_Context;

typedef union Render_Texture Render_Texture;
union Render_Texture {
    U32 u32[4];
};

internal Render_Context *render_create(Gfx_Context *gfx);
internal Void render_begin(Render_Context *gfx, V2U32 resolution);
internal Void render_end(Render_Context *gfx);

internal Render_Texture render_texture_create(Render_Context *gfx, V2U32 size, U8 *data);
internal Void           render_texture_destroy(Render_Context *gfx, Render_Texture texture);
internal Void           render_texture_update(Render_Context *gfx, Render_Texture texture, V2U32 position, V2U32 size, U8 *data);
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
#define render_rectangle(gfx, minimum, maximum, ...) render_rectangle_internal(gfx, &(Render_RectangleParams) { .min = minimum, .max = maximum, .color = v4f32(1.0f, 1.0f, 1.0f, 1.0f), __VA_ARGS__ })
internal Void render_rectangle_internal(Render_Context *gfx, Render_RectangleParams *parameters);

#endif // RENDER_INCLUDE_H
