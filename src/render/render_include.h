#ifndef RENDER_INCLUDE_H
#define RENDER_INCLUDE_H

#include "opengl/opengl_include.h"

typedef struct Render_Context Render_Context;

typedef struct Render_Texture Render_Texture;

internal Render_Context *render_create(Gfx_Context *gfx);
internal Void render_begin(Render_Context *gfx, V2U32 resolution);
internal Void render_end(Render_Context *gfx);

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
