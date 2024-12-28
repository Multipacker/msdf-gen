#include <stdio.h>

#if OS_WINDOWS
#include "win32_opengl.c"
#elif OS_LINUX
#include "sdl_opengl.c"
#endif

global OpenGL_Context global_opengl_context;

internal GLuint opengl_texture_id_from_texture(Render_Texture texture) {
    GLuint result = texture.u32[0];
    return result;
}

internal Void opengl_debug_output(GLenum source, GLenum type, U32 id, GLenum severity, GLsizei length, const char *message, const Void *userParam) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }

    Str8 source_string = { 0 };
    switch (source) {
        case GL_DEBUG_SOURCE_API:             source_string = str8_literal("API");             break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_string = str8_literal("Window System");   break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: source_string = str8_literal("Shader Compiler"); break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     source_string = str8_literal("Third Party");     break;
        case GL_DEBUG_SOURCE_APPLICATION:     source_string = str8_literal("Application");     break;
        case GL_DEBUG_SOURCE_OTHER:           source_string = str8_literal("Other");           break;
    }

    Str8 type_string = { 0 };
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               type_string = str8_literal("Error");                break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_string = str8_literal("Deprecated Behaviour"); break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_string = str8_literal("Undefined Behaviour");  break;
        case GL_DEBUG_TYPE_PORTABILITY:         type_string = str8_literal("Portability");          break;
        case GL_DEBUG_TYPE_PERFORMANCE:         type_string = str8_literal("Performance");          break;
        case GL_DEBUG_TYPE_MARKER:              type_string = str8_literal("Marker");               break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          type_string = str8_literal("Push Group");           break;
        case GL_DEBUG_TYPE_POP_GROUP:           type_string = str8_literal("Pop Group");            break;
        case GL_DEBUG_TYPE_OTHER:               type_string = str8_literal("Other");                break;
    }

    Str8 severity_string = { 0 };
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         severity_string = str8_literal("High");         break;
        case GL_DEBUG_SEVERITY_MEDIUM:       severity_string = str8_literal("Medium");       break;
        case GL_DEBUG_SEVERITY_LOW:          severity_string = str8_literal("Low");          break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severity_string = str8_literal("Notification"); break;
    }

    Arena_Temporary scratch = arena_get_scratch(0, 0);
    Str8 final_message = str8_format(
        scratch.arena,
        "OpenGL 0x%X(%.*s %.*s, %.*s): %s\n",
        id,
        str8_expand(source_string),
        str8_expand(type_string),
        str8_expand(severity_string),
        message
    );
    os_console_print(final_message);
    arena_end_temporary(scratch);
}

internal OpenGL_Result opengl_create_shader(Arena *arena, Str8 path, GLenum shader_type) {
    OpenGL_Result result = { 0 };

    result.handle = glCreateShader(shader_type);
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    Str8 shader_source = { 0 };
    if (os_file_read(scratch.arena, path, &shader_source)) {
        const GLchar *source_data = (const GLchar *) shader_source.data;
        GLint         source_size = (GLint) shader_source.size;

        glShaderSource(result.handle, 1, &source_data, &source_size);

        glCompileShader(result.handle);

        GLint compile_status = 0;
        glGetShaderiv(result.handle, GL_COMPILE_STATUS, &compile_status);
        if (!compile_status) {
            GLint log_length = 0;
            glGetShaderiv(result.handle, GL_INFO_LOG_LENGTH, &log_length);

            GLchar *raw_log = arena_push_array(scratch.arena, GLchar, (U64) log_length);
            glGetShaderInfoLog(result.handle, log_length, 0, raw_log);

            Str8 log = str8((U8 *) raw_log, (U64) log_length);

            str8_list_push(arena, &result.errors, str8_format(arena, "Could not compile shader '%.*s'. Shader log:\n%.*s\n", str8_expand(path), str8_expand(log)));

            glDeleteShader(result.handle);
            result.handle = 0;
        }
    } else {
        str8_list_push(arena, &result.errors, str8_format(arena, "Could not read file '%.*s'\n", str8_expand(path)));
        glDeleteShader(result.handle);
        result.handle = 0;
    }

    arena_end_temporary(scratch);
    return result;
}

internal OpenGL_Result opengl_create_program(Arena *arena, OpenGL_ShaderSpecification *shaders, U32 shader_count) {
    OpenGL_Result result = { 0 };
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    result.handle = glCreateProgram();
    GLuint *shader_handles = arena_push_array_zero(scratch.arena, GLuint, shader_count);

    for (U32 i = 0; i < shader_count; ++i) {
        OpenGL_Result compiled_shader = opengl_create_shader(arena, shaders[i].source, shaders[i].kind);

        if (compiled_shader.handle) {
            glAttachShader(result.handle, compiled_shader.handle);
            shader_handles[i] = compiled_shader.handle;
        } else {
            str8_list_append(arena, &result.errors, compiled_shader.errors);
        }
    }

    if (result.errors.node_count == 0) {
        glLinkProgram(result.handle);

        for (U32 i = 0; i < shader_count; ++i) {
            glDetachShader(result.handle, shader_handles[i]);
            glDeleteShader(shader_handles[i]);
        }

        GLint link_status = 0;
        glGetProgramiv(result.handle, GL_LINK_STATUS, &link_status);
        if (!link_status) {
            GLint log_length = 0;
            glGetProgramiv(result.handle, GL_INFO_LOG_LENGTH, &log_length);

            GLchar *raw_log = arena_push_array(scratch.arena, GLchar, (U64) log_length);
            glGetProgramInfoLog(result.handle, log_length, 0, raw_log);

            Str8 log = str8((U8 *) raw_log, (U64) log_length);

            str8_list_push(arena, &result.errors, str8_format(arena, "Could not link program. Program log:\n%.*s\n", str8_expand(log)));

            glDeleteProgram(result.handle);
            result.handle = 0;
        }
    } else {
        for (U32 i = 0; i < shader_count; ++i) {
            if (shader_handles[i]) {
                glDetachShader(result.handle, shader_handles[i]);
                glDeleteShader(shader_handles[i]);
            }
        }

        glDeleteProgram(result.handle);
    }

    arena_end_temporary(scratch);
    return result;
}

internal Void opengl_vertex_array_instance_attribute_float(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset, GLuint bindingindex) {
    glVertexArrayAttribFormat(vaobj,   attribindex, size, type, normalized, relativeoffset);
    glVertexArrayAttribBinding(vaobj,  attribindex, bindingindex);
    glVertexArrayBindingDivisor(vaobj, attribindex, 1);
    glEnableVertexArrayAttrib(vaobj,   attribindex);
}

internal Void opengl_vertex_array_instance_attribute_integer(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset, GLuint bindingindex) {
    glVertexArrayAttribIFormat(vaobj,  attribindex, size, type, relativeoffset);
    glVertexArrayAttribBinding(vaobj,  attribindex, bindingindex);
    glVertexArrayBindingDivisor(vaobj, attribindex, 1);
    glEnableVertexArrayAttrib(vaobj,   attribindex);
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
    GLuint texture_id = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);

    Render_Texture result = { 0 };
    result.u32[0] = texture_id;
    result.u32[1] = size.width;
    result.u32[2] = size.height;
    result.u32[3] = format;

    GLenum gl_internal_format = 0;
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

    glTextureStorage2D(texture_id, 1, gl_internal_format, (GLsizei) size.width, (GLsizei) size.height);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (data) {
        if (format == Render_TextureFormat_R8) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }

        glTextureSubImage2D(texture_id, 0, 0, 0, (GLsizei) size.width, (GLsizei) size.height, gl_format, GL_UNSIGNED_BYTE, data);

        if (format == Render_TextureFormat_R8) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        }
    }

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
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        } break;
        case Render_TextureFormat_RGBA8: {
            gl_format = GL_RGBA;
        } break;
    }

    glTextureSubImage2D(
        opengl_texture_id_from_texture(texture),
        0,
        (GLint) position.x, (GLint) position.y,
        (GLsizei) size.width, (GLsizei) size.height,
        gl_format, GL_UNSIGNED_BYTE,
        data
    );

    if (format == Render_TextureFormat_R8) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
}

internal Void render_create(Void) {
    OpenGL_Context *result = &global_opengl_context;

    Arena *arena = arena_create();
    result->arena = arena;
    opengl_backend_init();

    glDebugMessageCallback(&opengl_debug_output, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_FRAMEBUFFER_SRGB);

    OpenGL_ShaderSpecification shaders[] = {
        { str8_literal("src/render/opengl/shader.vert"), GL_VERTEX_SHADER,    },
        { str8_literal("src/render/opengl/shader.frag"), GL_FRAGMENT_SHADER,  },
    };
    OpenGL_Result program = opengl_create_program(arena, shaders, array_count(shaders));
    if (program.errors.node_count) {
        os_console_print(str8_join(arena, &program.errors));
    }

    result->program = program.handle;

    result->uniform_projection_location = glGetUniformLocation(result->program, "uniform_projection");
    result->uniform_sampler_location    = glGetUniformLocation(result->program, "uniform_sampler");
    result->uniform_transform_location  = glGetUniformLocation(result->program, "uniform_transform");

    GLuint vbos[4] = { 0 };
    glCreateBuffers(array_count(vbos), vbos);
    glNamedBufferData(vbos[0], kilobytes(64),  0, GL_DYNAMIC_DRAW);
    glNamedBufferData(vbos[1], kilobytes(256), 0, GL_DYNAMIC_DRAW);
    glNamedBufferData(vbos[2], megabytes(1),   0, GL_DYNAMIC_DRAW);
    glNamedBufferData(vbos[3], megabytes(4),   0, GL_DYNAMIC_DRAW);
    result->vbo_64kb  = vbos[0];
    result->vbo_256kb = vbos[1];
    result->vbo_1mb   = vbos[2];
    result->vbo_4mb   = vbos[3];

    glCreateVertexArrays(1, &result->vao);

    opengl_vertex_array_instance_attribute_float(result->vao,   0, 4, GL_FLOAT,        GL_FALSE, member_offset(Render_Shape, position),  0);
    opengl_vertex_array_instance_attribute_float(result->vao,   1, 4, GL_FLOAT,        GL_FALSE, member_offset(Render_Shape, colors[0]), 0);
    opengl_vertex_array_instance_attribute_float(result->vao,   2, 4, GL_FLOAT,        GL_FALSE, member_offset(Render_Shape, colors[1]), 0);
    opengl_vertex_array_instance_attribute_float(result->vao,   3, 4, GL_FLOAT,        GL_FALSE, member_offset(Render_Shape, colors[2]), 0);
    opengl_vertex_array_instance_attribute_float(result->vao,   4, 4, GL_FLOAT,        GL_FALSE, member_offset(Render_Shape, colors[3]), 0);
    opengl_vertex_array_instance_attribute_float(result->vao,   5, 4, GL_FLOAT,        GL_FALSE, member_offset(Render_Shape, source),    0);
    opengl_vertex_array_instance_attribute_integer(result->vao, 6, 1, GL_UNSIGNED_INT,           member_offset(Render_Shape, flags),     0);
    opengl_vertex_array_instance_attribute_float(result->vao,   7, 1, GL_FLOAT,        GL_FALSE, member_offset(Render_Shape, thickness), 0);
    opengl_vertex_array_instance_attribute_float(result->vao,   8, 1, GL_FLOAT,        GL_FALSE, member_offset(Render_Shape, softness),  0);
    opengl_vertex_array_instance_attribute_float(result->vao,   9, 4, GL_FLOAT,        GL_FALSE, member_offset(Render_Shape, radies),    0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glUseProgram(result->program);
    glBindVertexArray(result->vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_SCISSOR_TEST);
}

internal Void render_begin(V2U32 resolution) {
    OpenGL_Context *gfx = &global_opengl_context;
    gfx->resolution = resolution;

    glViewport(0, 0, (GLsizei) resolution.width, (GLsizei) resolution.height);

    M4F32 projection = m4f32_ortho(
        0.0f, (F32) resolution.width,
        0.0f, (F32) resolution.height,
        1.0f, -1.0f
    );
    glProgramUniformMatrix4fv(gfx->program, gfx->uniform_projection_location, 1, GL_TRUE, &projection.m[0][0]);

    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
}

internal Void render_submit(Render_BatchList batches) {
    prof_function_begin();

    OpenGL_Context *gfx = &global_opengl_context;

    for (Render_Batch *batch = batches.first; batch; batch = batch->next) {
        prof_zone_begin(prof_batch, "batch");

        ++gfx->current_stats.batch_count;
        gfx->current_stats.shape_count += batch->shapes.shape_count;

        GLsizei width  = (GLsizei) (batch->clip.max.x - batch->clip.min.x);
        GLsizei height = (GLsizei) (batch->clip.max.y - batch->clip.min.y);

        if (width > 0 && height > 0) {
            glScissor(
                (GLint) batch->clip.min.x,
                (GLint) gfx->resolution.y - (GLint) batch->clip.max.y,
                width,
                height
            );

            glBindTextureUnit(0, opengl_texture_id_from_texture(batch->texture));
            glProgramUniformMatrix3fv(gfx->program, gfx->uniform_transform_location, 1, GL_TRUE, &batch->transform.m[0][0]);

            GLuint vbo = 0;
            B32 specifically_sized = false;
            U64 byte_size = batch->shapes.shape_count * sizeof(Render_Shape);
            gfx->current_stats.bytes_uploaded_to_gpu += byte_size;

            // NOTE(simon): Select an appropriate buffer.
            if (byte_size <= kilobytes(64)) {
                vbo = gfx->vbo_64kb;
            } else if (byte_size <= kilobytes(256)) {
                vbo = gfx->vbo_256kb;
            } else if (byte_size <= megabytes(1)) {
                vbo = gfx->vbo_1mb;
            } else if (byte_size <= megabytes(4)) {
                vbo = gfx->vbo_4mb;
            } else {
                specifically_sized = true;
                glCreateBuffers(1, &vbo);
                glNamedBufferData(vbo, (GLsizeiptr) byte_size, 0, GL_STREAM_DRAW);
            }

            // NOTE(simon): Update buffer data
            U8 *mapped_buffer = (U8 *) glMapNamedBuffer(vbo, GL_WRITE_ONLY);
            U8 *ptr = mapped_buffer;
            for (Render_ShapeChunk *chunk = batch->shapes.first; chunk; chunk = chunk->next) {
                memory_copy(ptr, chunk->shapes, chunk->count * sizeof(Render_Shape));
                ptr += chunk->count * sizeof(Render_Shape);
            }
            glUnmapNamedBuffer(vbo);

            glVertexArrayVertexBuffer(gfx->vao, 0, vbo, 0, sizeof(Render_Shape));

            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei) batch->shapes.shape_count);

            // NOTE(simon): Delete specifically sized buffer.
            if (specifically_sized) {
                glDeleteBuffers(1, &vbo);
            }
        }

        prof_zone_end(prof_batch);
    }

    prof_function_end();
}

internal Void render_end(Void) {
    OpenGL_Context *gfx = &global_opengl_context;

    gfx_swap_buffers();

    gfx->previous_stats = gfx->current_stats;
    memory_zero_struct(&gfx->current_stats);
}

internal Render_Stats render_get_stats(Void) {
    OpenGL_Context *gfx = &global_opengl_context;
    Render_Stats result = gfx->previous_stats;
    return result;
}
