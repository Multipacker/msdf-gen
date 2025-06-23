#include "generated.h"

embed_file(opengl_vertex_shader,   "shader.vert");
embed_file(opengl_fragment_shader, "shader.frag");

#include <stdio.h>

#if LINUX_WAYLAND
#  include "wayland_opengl.c"
#elif LINUX_X11
#  include "x11_opengl.c"
#endif

global OpenGL_Context global_opengl_context;

internal GLuint opengl_texture_id_from_texture(Render_Texture texture) {
    GLuint result = texture.u32[0];
    return result;
}

internal Render_Texture render_texture_null(Void) {
    Render_Texture result = { 0 };
    return result;
}

internal B32 render_texture_equal(Render_Texture a, Render_Texture b) {
    B32 result = opengl_texture_id_from_texture(a) == opengl_texture_id_from_texture(b);
    return result;
}

internal Render_Texture render_texture_create(V2U32 size, Render_TextureFormat format, U8 *data) {
    GLint gl_internal_format = 0;
    GLenum gl_format = 0;
    switch (format) {
        case Render_TextureFormat_R8: {
            gl_internal_format = GL_R8;
            gl_format          = GL_RED;
        } break;
        case Render_TextureFormat_RGBA8: {
            gl_internal_format = GL_RGBA8;
            gl_format          = GL_RGBA;
        } break;
    }

    GLuint texture_id = 0;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        gl_internal_format,
        (GLsizei) size.width,
        (GLsizei) size.height,
        0,
        gl_format,
        GL_UNSIGNED_BYTE,
        data
    );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);

    Render_Texture result = { 0 };
    result.u32[0] = texture_id;
    result.u32[1] = size.width;
    result.u32[2] = size.height;
    result.u32[3] = format;

    return result;
}

internal Void render_texture_destroy(Render_Texture texture) {
    GLuint texture_id = opengl_texture_id_from_texture(texture);
    glDeleteTextures(1, &texture_id);
}

internal V2U32 render_size_from_texture(Render_Texture texture) {
    V2U32 result = { 0 };
    result.width  = texture.u32[1];
    result.height = texture.u32[2];
    return result;
}

internal Void render_texture_update(Render_Texture texture, V2U32 position, V2U32 size, U8 *data) {
    Render_TextureFormat format = (Render_TextureFormat) texture.u32[3];

    GLenum gl_format = 0;
    switch (format) {
        case Render_TextureFormat_R8: {
            gl_format = GL_RED;
        } break;
        case Render_TextureFormat_RGBA8: {
            gl_format = GL_RGBA;
        } break;
    }

    glBindTexture(GL_TEXTURE_2D, opengl_texture_id_from_texture(texture));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        (GLint) position.x, (GLint) position.y,
        (GLsizei) size.width, (GLsizei) size.height,
        gl_format, GL_UNSIGNED_BYTE,
        data
    );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
}

internal B32 render_init(Void) {
    OpenGL_Context *state = &global_opengl_context;
    state->arena = arena_create();

    opengl_backend_init();

    glEnable(GL_FRAMEBUFFER_SRGB);

    glGenSamplers(array_count(state->samplers), state->samplers);

    glSamplerParameteri(state->samplers[Render_Filtering_Nearest], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(state->samplers[Render_Filtering_Nearest], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(state->samplers[Render_Filtering_Nearest], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(state->samplers[Render_Filtering_Nearest], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glSamplerParameteri(state->samplers[Render_Filtering_Linear], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(state->samplers[Render_Filtering_Linear], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(state->samplers[Render_Filtering_Linear], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(state->samplers[Render_Filtering_Linear], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    typedef struct ShaderSpecification ShaderSpecification;
    struct ShaderSpecification {
        Str8   source;
        GLenum kind;
        GLuint shader;
        Str8   errors;
    };
    ShaderSpecification shaders[] = {
        { opengl_vertex_shader,   GL_VERTEX_SHADER,   },
        { opengl_fragment_shader, GL_FRAGMENT_SHADER, },
    };

    // NOTE(simon): Compile.
    for (U32 i = 0; i < array_count(shaders); ++i) {
        shaders[i].shader = glCreateShader(shaders[i].kind);

        // NOTE(simon): Attach source.
        const GLchar *source_data = (const GLchar *) shaders[i].source.data;
        GLint         source_size = (GLint) shaders[i].source.size;
        glShaderSource(shaders[i].shader, 1, &source_data, &source_size);

        glCompileShader(shaders[i].shader);

        // NOTE(simon): Get compilation logs.
        GLint log_length = 0;
        glGetShaderiv(shaders[i].shader, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length != 0) {
            shaders[i].errors.data = arena_push_array_no_zero(state->arena, U8, (U64) log_length);
            shaders[i].errors.size = (U64) log_length;
            glGetShaderInfoLog(shaders[i].shader, log_length, 0, (GLchar *) shaders[i].errors.data);

            gfx_message(true, str8_literal("Failed to compile OpenGL shader"), shaders[i].errors);
            os_exit(1);
        }
    }

    state->program = glCreateProgram();
    for (U32 i = 0; i < array_count(shaders); ++i) {
        glAttachShader(state->program, shaders[i].shader);
    }

    glLinkProgram(state->program);
    glValidateProgram(state->program);

    // NOTE(simon): Get linking logs.
    GLint log_length = 0;
    glGetProgramiv(state->program, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length) {
        Str8 errors = { 0 };
        errors.data = arena_push_array_no_zero(state->arena, U8, (U64) log_length);
        errors.size = (U64) log_length;
        glGetProgramInfoLog(state->program, log_length, 0, (GLchar *) errors.data);

        gfx_message(true, str8_literal("Failed to link OpenGL program"), errors);
        os_exit(1);
    }



    state->uniform_projection_location = glGetUniformLocation(state->program, "uniform_projection");
    state->uniform_sampler_location    = glGetUniformLocation(state->program, "uniform_sampler");
    state->uniform_transform_location  = glGetUniformLocation(state->program, "uniform_transform");

    glGenVertexArrays(1, &state->vao);
    glBindVertexArray(state->vao);

    glGenBuffers(1, &state->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);

    // NOTE(simon): Setup the float attributes.
    typedef struct VertexAttributes VertexAttributes;
    struct VertexAttributes {
        GLint     components;
        GLenum    type;
        GLboolean normalized;
        U64       offset;
    };
    VertexAttributes attributes[] = {
        { 4, GL_FLOAT, GL_FALSE, member_offset(Render_Shape, position),  },
        { 4, GL_FLOAT, GL_FALSE, member_offset(Render_Shape, source),    },
        { 4, GL_FLOAT, GL_FALSE, member_offset(Render_Shape, colors[0]), },
        { 4, GL_FLOAT, GL_FALSE, member_offset(Render_Shape, colors[1]), },
        { 4, GL_FLOAT, GL_FALSE, member_offset(Render_Shape, colors[2]), },
        { 4, GL_FLOAT, GL_FALSE, member_offset(Render_Shape, colors[3]), },
        { 4, GL_FLOAT, GL_FALSE, member_offset(Render_Shape, radies),    },
        { 1, GL_FLOAT, GL_FALSE, member_offset(Render_Shape, thickness), },
        { 1, GL_FLOAT, GL_FALSE, member_offset(Render_Shape, softness),  },
    };
    for (U64 i = 0; i < array_count(attributes); ++i) {
        glVertexAttribPointer((GLuint) i, attributes[i].components, attributes[i].type, attributes[i].normalized, sizeof(Render_Shape), pointer_from_integer(attributes[i].offset));
        glVertexAttribDivisor((GLuint) i, 1);
        glEnableVertexAttribArray((GLuint) i);
    }

    // NOTE(simon): Set up the integer attribute.
    glVertexAttribIPointer(9, 1, GL_UNSIGNED_INT, sizeof(Render_Shape), pointer_from_integer(member_offset(Render_Shape, flags)));
    glVertexAttribDivisor(9, 1);
    glEnableVertexAttribArray(9);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glUseProgram(state->program);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    glEnable(GL_SCISSOR_TEST);

    return false;
}



internal Void render_begin(Void) {
}

internal Void render_end(Void) {
}



internal Render_Window render_create(Gfx_Window handle) {
    Render_Window result = opengl_backend_create(handle);
    return result;
}

internal Void render_destroy(Gfx_Window graphics_handle, Render_Window render_handle) {
    opengl_backend_destroy(graphics_handle, render_handle);
}

internal Void render_window_begin(Gfx_Window graphics_handle, Render_Window render_handle) {
    prof_function_begin();
    OpenGL_Context *gfx = &global_opengl_context;

    opengl_window_resize(graphics_handle, render_handle);
    opengl_window_select(graphics_handle, render_handle);

    V2U32 resolution = gfx_client_area_from_window(graphics_handle);
    gfx->resolution = resolution;

    glViewport(0, 0, (GLsizei) resolution.width, (GLsizei) resolution.height);

    M4F32 projection = m4f32_ortho(
        0.0f, (F32) resolution.width,
        0.0f, (F32) resolution.height,
        1.0f, -1.0f
    );
    glUniformMatrix4fv(gfx->uniform_projection_location, 1, GL_TRUE, &projection.m[0][0]);

    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    prof_function_end();
}

internal Void render_window_submit(Gfx_Window graphics_handle, Render_Window render_handle, Render_BatchList batches) {
    prof_function_begin();

    OpenGL_Context *gfx = &global_opengl_context;

    for (Render_Batch *batch = batches.first; batch; batch = batch->next) {
        prof_zone_begin(prof_batch, "batch");

        // NOTE(simon): Find clip size.
        GLsizei width  = (GLsizei) (batch->clip.max.x - batch->clip.min.x);
        GLsizei height = (GLsizei) (batch->clip.max.y - batch->clip.min.y);

        // NOTE(simon): Do we even need to render this batch?
        if (width > 0 && height > 0) {
            U64 byte_size = batch->shapes.shape_count * sizeof(Render_Shape);

            // NOTE(simon): Update stats.
            gfx->current_stats.shape_count += batch->shapes.shape_count;
            gfx->current_stats.bytes_uploaded_to_gpu += byte_size;
            ++gfx->current_stats.batch_count;

            // NOTE(simon): Upload data.
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) byte_size, 0, GL_STREAM_DRAW);
            U8 *mapped_buffer = (U8 *) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            U8 *ptr = mapped_buffer;
            for (Render_ShapeChunk *chunk = batch->shapes.first; chunk; chunk = chunk->next) {
                memory_copy(ptr, chunk->shapes, chunk->count * sizeof(Render_Shape));
                ptr += chunk->count * sizeof(Render_Shape);
            }
            glUnmapBuffer(GL_ARRAY_BUFFER);

            glScissor(
                (GLint) batch->clip.min.x,
                (GLint) gfx->resolution.y - (GLint) batch->clip.max.y,
                width,
                height
            );

            glBindSampler(0, gfx->samplers[batch->filtering]);
            glBindTexture(GL_TEXTURE_2D, opengl_texture_id_from_texture(batch->texture));
            glUniformMatrix3fv(gfx->uniform_transform_location, 1, GL_TRUE, &batch->transform.m[0][0]);

            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei) batch->shapes.shape_count);
        }

        prof_zone_end(prof_batch);
    }

    prof_function_end();
}

internal Void render_window_end(Gfx_Window graphics_handle, Render_Window render_handle) {
    OpenGL_Context *gfx = &global_opengl_context;

    opengl_swap_buffers(graphics_handle, render_handle);

    gfx->previous_stats = gfx->current_stats;
    memory_zero_struct(&gfx->current_stats);
}

internal Render_Stats render_get_stats(Void) {
    OpenGL_Context *gfx = &global_opengl_context;
    Render_Stats result = gfx->previous_stats;
    return result;
}
