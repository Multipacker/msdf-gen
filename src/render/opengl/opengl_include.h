#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "opengl_bindings.h"

#if LINUX_WAYLAND
#  include "wayland_opengl.h"
#elif LINUX_X11
#  include "x11_opengl.h"
#endif

typedef struct OpenGL_Context OpenGL_Context;
struct OpenGL_Context {
    Arena           *arena;
    Arena_Temporary  frame_restore;
    Render_BatchList batches;
    GLuint           program;
    GLuint           vbo;
    GLuint           vao;
    GLuint           samplers[Render_Filtering_COUNT];
    GLint            uniform_projection_location;
    GLint            uniform_sampler_location;
    GLint            uniform_transform_location;
    V2U32            resolution;
    Render_Stats     previous_stats;
    Render_Stats     current_stats;
};

// NOTE(simon): Vertex attribute helpers.
internal Void opengl_vertex_array_instance_attribute_float(GLuint index, GLint components, GLenum type, GLboolean normalized, GLsizei size, U64 relativeoffset);
internal Void opengl_vertex_array_instance_attribute_integer(GLuint index, GLint components, GLenum type, GLsizei size, U64 relativeoffset);

// NOTE(simon): Texture helpers
internal GLuint opengl_texture_id_from_texture(Render_Texture texture);

internal Void opengl_resize(Render_Window handle, V2U32 resolution);

#endif // OPENGL_INCLUDE_H
