#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "opengl_bindings.h"

#if OS_WINDOWS
#include "win32_opengl.h"
#endif

#define RENDER_BATCH_SIZE 1024

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

#endif // OPENGL_INCLUDE_H
