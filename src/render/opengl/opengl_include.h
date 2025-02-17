#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "opengl_bindings.h"

#if OS_WINDOWS
#  include "win32_opengl.h"
#elif OS_LINUX
#  if LINUX_WAYLAND
#    include "wayland_opengl.h"
#  elif LINUX_X11
#    include "x11_opengl.h"
#  endif
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

typedef struct {
    Str8 source;
    GLenum kind;
} OpenGL_ShaderSpecification;

typedef struct {
    GLuint handle;
    Str8List errors;
} OpenGL_Result;

// NOTE(simon): Shaders compilation helpers.
internal OpenGL_Result opengl_create_shader(Arena *arena, Str8 path, GLenum shader_type);
internal OpenGL_Result opengl_create_program(Arena *arena, OpenGL_ShaderSpecification *shaders, U32 shader_count);

// NOTE(simon): Vertex attribute helpers.
internal Void opengl_vertex_array_instance_attribute_float(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset, GLuint bindingindex);
internal Void opengl_vertex_array_instance_attribute_integer(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset, GLuint bindingindex);

// NOTE(simon): Texture helpers
internal GLuint opengl_texture_id_from_texture(Render_Texture texture);

#endif // OPENGL_INCLUDE_H
