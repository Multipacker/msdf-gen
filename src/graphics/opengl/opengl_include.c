internal GLuint opengl_create_shader(Str8 path, GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    Str8 shader_source = { 0 };
    if (os_file_read(scratch.arena, path, &shader_source)) {
        const GLchar *source_data = (const GLchar *) shader_source.data;
        GLint         source_size = (GLint) shader_source.size;

        glShaderSource(shader, 1, &source_data, &source_size);

        glCompileShader(shader);

        GLint compile_status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
        if (!compile_status) {
            GLint log_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

            GLchar *raw_log = arena_push_array(scratch.arena, GLchar, (U64) log_length);
            glGetShaderInfoLog(shader, log_length, 0, raw_log);

            Str8 log = str8((U8 *) raw_log, (U64) log_length);

            fprintf(stderr, "ERROR: Could not compile shader. Shader log:\n%.*s\n", str8_expand(log));
            error_emit(str8_literal("Could not compile shader."));

            glDeleteShader(shader);
            shader = 0;
        }
    } else {
        error_emit(str8_literal("Could not compile shader."));
        glDeleteShader(shader);
        shader = 0;
    }

    arena_end_temporary(scratch);

    return shader;
}

internal GLuint opengl_create_program(GLuint *shaders, U32 shader_count) {
    GLuint program = glCreateProgram();

    for (U32 i = 0; i < shader_count; ++i) {
        glAttachShader(program, shaders[i]);
    }

    glLinkProgram(program);

    for (U32 i = 0; i < shader_count; ++i) {
        glDetachShader(program, shaders[i]);
        glDeleteShader(shaders[i]);
    }

    GLint link_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        Arena_Temporary scratch = arena_get_scratch(0, 0);

        GLint log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

        GLchar *raw_log = arena_push_array(scratch.arena, GLchar, (U64) log_length);
        glGetProgramInfoLog(program, log_length, 0, raw_log);

        Str8 log = str8((U8 *) raw_log, (U64) log_length);

        fprintf(stderr, "ERROR: Could not link program. Program log:\n%.*s\n", str8_expand(log));
        error_emit(str8_literal("Could not link program."));

        glDeleteProgram(program);
        program = 0;

        arena_end_temporary(scratch);
    }

    return program;
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

internal Void opengl_debug_output(
    GLenum source,
    GLenum type,
    U32 id,
    GLenum severity,
    GLsizei length,
    const char *message,
    const Void *userParam
) {
    printf("%s\n", message);
}

internal Void render_rectangle_internal(Render_Context *gfx, Render_RectangleParams *parameters) {
    Render_Batch *batch = gfx->batches.last;

    if (!batch || batch->size >= RENDER_BATCH_SIZE || (batch->texture_id && batch->texture_id != parameters->texture.texture_id)) {
        batch = arena_push_struct_zero(&gfx->arena, Render_Batch);
        dll_push_back(gfx->batches.first, gfx->batches.last, batch);
    }

    // NOTE(simon): Either it is the same texture id, or there is no texture for this batch.
    batch->texture_id = parameters->texture.texture_id;

    Render_Rectangle *rect = &batch->rectangles[batch->size++];

    rect->min    = v2f32(parameters->min.x, parameters->min.y);
    rect->max    = v2f32(parameters->max.x, parameters->max.y);
    rect->color  = parameters->color;
    rect->uv_min = parameters->uv_min;
    rect->uv_max = parameters->uv_max;
    rect->flags  = parameters->flags;
}

internal Void render_begin(Render_Context *gfx, V2U32 resolution) {
    gfx->frame_restore = arena_begin_temporary(&gfx->arena);

    glViewport(0, 0, resolution.width, resolution.height);

    M4F32 projection = m4f32_ortho(0.0f, (F32) resolution.width, 0.0f, (F32) resolution.height, 1.0f, -1.0f);
    glProgramUniformMatrix4fv(gfx->program, gfx->uniform_projection_location, 1, GL_FALSE, &projection.m[0][0]);
}

internal Void render_end(Render_Context *gfx) {
    glProgramUniform1i(gfx->program, gfx->uniform_sampler_location, 0);

    glClear(GL_COLOR_BUFFER_BIT);

    for (Render_Batch *batch = gfx->batches.first; batch; batch = batch->next) {
        glBindTextureUnit(0, batch->texture_id);
        glNamedBufferSubData(gfx->vbo, 0, batch->size * sizeof(Render_Rectangle), batch->rectangles);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei) batch->size);
    }

    gfx->batches.first = 0;
    gfx->batches.last  = 0;
    arena_end_temporary(gfx->frame_restore);

    SDL_GL_SwapWindow(gfx->gfx->window);
}

internal Render_Texture render_texture_create(Render_Context *gfx, V2U32 size, U8 *data) {
    Render_Texture result = { 0 };

    glCreateTextures(GL_TEXTURE_2D, 1, &result.texture_id);
    result.size = size;

    glTextureStorage2D(result.texture_id, 1, GL_RGBA8, (GLsizei) size.width, (GLsizei) size.height);
    glTextureParameteri(result.texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(result.texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(result.texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(result.texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureSubImage2D(result.texture_id, 0, 0, 0, (GLsizei) size.width, (GLsizei) size.height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    return result;
}

internal Void render_texture_destroy(Render_Context *gfx, Render_Texture texture) {
    glDeleteTextures(1, &texture.texture_id);
}

internal Void render_texture_update(Render_Context *gfx, Render_Texture texture, V2U32 position, V2U32 size, U8 *data) {
    glTextureSubImage2D(
        texture.texture_id,
        0,
        0, 0,
        (GLsizei) size.width, (GLsizei) size.height,
        GL_RGBA, GL_UNSIGNED_BYTE,
        data
    );
}

internal Render_Context *render_create(Gfx_Context *gfx) {
    Arena arena = arena_create();
    Render_Context *result = arena_push_struct_zero(&arena, Render_Context);
    result->arena = arena;
    result->gfx = gfx;

    glDebugMessageCallback(&opengl_debug_output, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_FRAMEBUFFER_SRGB);

    GLuint shaders[] = {
        opengl_create_shader(str8_literal("src/graphics/opengl/shader.vert"), GL_VERTEX_SHADER),
        opengl_create_shader(str8_literal("src/graphics/opengl/shader.frag"), GL_FRAGMENT_SHADER),
    };
    result->program = opengl_create_program(shaders, array_count(shaders));

    result->uniform_projection_location = glGetUniformLocation(result->program, "uniform_projection");
    result->uniform_sampler_location    = glGetUniformLocation(result->program, "uniform_sampler");

    glCreateBuffers(1, &result->vbo);
    glNamedBufferData(result->vbo, RENDER_BATCH_SIZE * sizeof(Render_Rectangle), 0, GL_DYNAMIC_DRAW);

    glCreateVertexArrays(1, &result->vao);

    opengl_vertex_array_instance_attribute_float(result->vao,   0, 2, GL_FLOAT,        GL_FALSE, member_offset(Render_Rectangle, min),    0);
    opengl_vertex_array_instance_attribute_float(result->vao,   1, 2, GL_FLOAT,        GL_FALSE, member_offset(Render_Rectangle, max),    0);
    opengl_vertex_array_instance_attribute_float(result->vao,   2, 4, GL_FLOAT,        GL_FALSE, member_offset(Render_Rectangle, color),  0);
    opengl_vertex_array_instance_attribute_float(result->vao,   3, 2, GL_FLOAT,        GL_FALSE, member_offset(Render_Rectangle, uv_min), 0);
    opengl_vertex_array_instance_attribute_float(result->vao,   4, 2, GL_FLOAT,        GL_FALSE, member_offset(Render_Rectangle, uv_max), 0);
    opengl_vertex_array_instance_attribute_integer(result->vao, 5, 1, GL_UNSIGNED_INT,           member_offset(Render_Rectangle, flags),  0);

    glVertexArrayVertexBuffer(result->vao, 0, result->vbo, 0, sizeof(Render_Rectangle));

    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glUseProgram(result->program);
    glBindVertexArray(result->vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return result;
}
