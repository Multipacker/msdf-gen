#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "../opengl/opengl_bindings.h"

#define RENDER_BATCH_SIZE 1024

typedef enum {
    Render_RectangleFlags_Texture = 1 << 0,
    Render_RectangleFlags_MSDF    = 1 << 1,
} Render_RectangleFlags;

typedef struct Render_Rectangle Render_Rectangle;
struct Render_Rectangle {
    V2F32 min;
    V2F32 max;
    V4F32 color;
    V2F32 uv_min;
    V2F32 uv_max;
    U32   flags;
};

typedef struct Render_Batch Render_Batch;
struct Render_Batch {
    Render_Batch *next;
    Render_Batch *previous;

    GLuint texture_id;
    U32 size;
    Render_Rectangle rectangles[RENDER_BATCH_SIZE];
};

typedef struct Render_BatchList Render_BatchList;
struct Render_BatchList {
    Render_Batch *first;
    Render_Batch *last;
};

typedef struct Render_Texture Render_Texture;
struct Render_Texture {
    GLuint texture_id;
    V2U32  size;
};

typedef struct Render_Context Render_Context;
struct Render_Context {
    Arena           *arena;
    Arena_Temporary  frame_restore;
    Render_BatchList batches;
    GLuint           program;
    GLuint           vao;
    GLuint           vbo;
    GLint            uniform_projection_location;
    GLint            uniform_sampler_location;
    Gfx_Context     *gfx;
};

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

#endif // OPENGL_INCLUDE_H
