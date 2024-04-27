global Gfx_Key sdl_to_gfx_keycode[128];

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

internal Void opengl_vertex_array_instance_attribute(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset, GLuint bindingindex) {
    glVertexArrayAttribFormat(vaobj,   attribindex, size, type, normalized, relativeoffset);
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

internal Gfx_Context *gfx_create(Arena *arena, Str8 title, U32 width, U32 height) {
    Arena_Temporary restore_point = arena_begin_temporary(arena);
    Gfx_Context *result = arena_push_struct_zero(arena, Gfx_Context);
    result->arena = arena_create();

    B32 error = 0;

    // NOTE(simon): Initialize SDL to gfx keycode table.
    for (SDL_KeyCode key = SDLK_0; key <= SDLK_9; ++key)
    {
        sdl_to_gfx_keycode[key] = (Gfx_Key) (Gfx_Key_0 + (key - SDLK_0));
    }
    for (SDL_KeyCode key = SDLK_a; key <= SDLK_z; ++key)
    {
        sdl_to_gfx_keycode[key] = (Gfx_Key) (Gfx_Key_A + (key - SDLK_a));
    }
    sdl_to_gfx_keycode[SDLK_BACKSPACE] = Gfx_Key_Backspace;
    sdl_to_gfx_keycode[SDLK_SPACE]     = Gfx_Key_Space;
    sdl_to_gfx_keycode[SDLK_TAB]       = Gfx_Key_Tab;
    sdl_to_gfx_keycode[SDLK_RETURN]    = Gfx_Key_Return;
    sdl_to_gfx_keycode[SDLK_ESCAPE]    = Gfx_Key_Escape;
    sdl_to_gfx_keycode[SDLK_DELETE]    = Gfx_Key_Delete;

    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

        Arena_Temporary scratch = arena_get_scratch(&arena, 1);

        CStr cstr_title = cstr_from_str8(scratch.arena, title);

        result->window = SDL_CreateWindow(
            cstr_title,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            (int) width, (int) height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
        );

        arena_end_temporary(scratch);

        if (result->window) {
            result->gl_context = SDL_GL_CreateContext(result->window);
            SDL_GL_MakeCurrent(result->window, result->gl_context);

#define X(type, name) name = (type) SDL_GL_GetProcAddress(#name); assert(name);
            GL_LINUX_FUNCTION(X)
            GL_FUNCTIONS(X)
#undef X

            SDL_GL_SetSwapInterval(1);

            glDebugMessageCallback(&opengl_debug_output, NULL);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

            GLuint shaders[] = {
                opengl_create_shader(str8_literal("src/graphics/sdl/shader.vert"), GL_VERTEX_SHADER),
                opengl_create_shader(str8_literal("src/graphics/sdl/shader.frag"), GL_FRAGMENT_SHADER),
            };
            result->program = opengl_create_program(shaders, array_count(shaders));

            result->uniform_projection_location = glGetUniformLocation(result->program, "uniform_projection");

            glCreateBuffers(1, &result->vbo);
            glNamedBufferData(result->vbo, RENDER_BATCH_SIZE * sizeof(Render_Rectangle), 0, GL_DYNAMIC_DRAW);

            glCreateVertexArrays(1, &result->vao);

            opengl_vertex_array_instance_attribute(result->vao, 0, 2, GL_FLOAT, GL_FALSE, member_offset(Render_Rectangle, min),   0);
            opengl_vertex_array_instance_attribute(result->vao, 1, 2, GL_FLOAT, GL_FALSE, member_offset(Render_Rectangle, max),   0);
            opengl_vertex_array_instance_attribute(result->vao, 2, 4, GL_FLOAT, GL_FALSE, member_offset(Render_Rectangle, color), 0);

            glVertexArrayVertexBuffer(result->vao, 0, result->vbo, 0, sizeof(Render_Rectangle));

            glClearColor(1, 0, 1, 1);
            glUseProgram(result->program);
            glBindVertexArray(result->vao);
        } else {
            error_emit(str8_literal("Could not create SDL window."));
            error = true;
        }
    } else {
        error_emit(str8_literal("Could not initialize SDL 2.0"));
        error = true;
    }

    if (error) {
        if (result->window) {
            SDL_DestroyWindow(result->window);
        }

        arena_end_temporary(restore_point);
        result = 0;
    }

    return result;
}

internal Gfx_EventList gfx_get_events(Arena *arena, Gfx_Context *gfx) {
    Gfx_EventList events = { 0 };

    for (SDL_Event sdl_event = { 0 }; SDL_PollEvent(&sdl_event);) {
        Gfx_Event *event = arena_push_struct_zero(arena, Gfx_Event);
        switch (sdl_event.type) {
            case SDL_QUIT: {
                event->kind = Gfx_EventKind_Quit;
            } break;
            case SDL_KEYDOWN: case SDL_KEYUP: {
                if (sdl_event.key.type == SDL_KEYDOWN) {
                    event->kind = Gfx_EventKind_KeyPress;
                } else {
                    event->kind = Gfx_EventKind_KeyRelease;
                }

                SDL_Keycode sdl_keycode = sdl_event.key.keysym.sym;

                switch (sdl_keycode) {
                    case SDLK_PAGEUP:   event->key = Gfx_Key_PageUp;   break;
                    case SDLK_PAGEDOWN: event->key = Gfx_Key_PageDown; break;
                    case SDLK_LEFT:     event->key = Gfx_Key_Left;     break;
                    case SDLK_RIGHT:    event->key = Gfx_Key_Right;    break;
                    case SDLK_UP:       event->key = Gfx_Key_Up;       break;
                    case SDLK_DOWN:     event->key = Gfx_Key_Down;     break;
                    case SDLK_LSHIFT:   event->key = Gfx_Key_Shift;    break;
                    case SDLK_RSHIFT:   event->key = Gfx_Key_Shift;    break;
                    case SDLK_END:      event->key = Gfx_Key_End;      break;
                    case SDLK_HOME:     event->key = Gfx_Key_Home;     break;
                    case SDLK_LCTRL:    event->key = Gfx_Key_Control;  break;
                    case SDLK_RCTRL:    event->key = Gfx_Key_Control;  break;
                    case SDLK_LALT:     event->key = Gfx_Key_Alt;      break;
                    case SDLK_RALT:     event->key = Gfx_Key_Alt;      break;
                    case SDLK_LGUI:     event->key = Gfx_Key_OS;       break;
                    case SDLK_RGUI:     event->key = Gfx_Key_OS;       break;
                    default: {
                        if (SDLK_F1 <= sdl_keycode && sdl_keycode <= SDLK_F12) {
                            event->key = (Gfx_Key) (Gfx_Key_F1 + (sdl_keycode - SDLK_F1));
                        } else {
                            event->key = sdl_to_gfx_keycode[sdl_keycode];
                        }
                    } break;
                }

                SDL_Keymod modifiers = SDL_GetModState();
                event->key_modifiers |= (modifiers & KMOD_SHIFT ? Gfx_KeyModifier_Shift   : 0);
                event->key_modifiers |= (modifiers & KMOD_CTRL  ? Gfx_KeyModifier_Control : 0);

                if (event->key == Gfx_Key_Null) {
                    event->kind = Gfx_EventKind_Null;
                }
            } break;
            case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: {
                if (sdl_event.button.type == SDL_MOUSEBUTTONDOWN) {
                    event->kind = Gfx_EventKind_KeyPress;
                } else {
                    event->kind = Gfx_EventKind_KeyRelease;
                }

                switch (sdl_event.button.button) {
                    case SDL_BUTTON_LEFT: {
                        if (sdl_event.button.clicks == 1) {
                            event->key = Gfx_Key_MouseLeft;
                        } else {
                            event->key = Gfx_Key_MouseLeftDouble;
                        }
                    } break;
                    case SDL_BUTTON_MIDDLE: {
                        if (sdl_event.button.clicks == 1) {
                            event->key = Gfx_Key_MouseMiddle;
                        } else {
                            event->key = Gfx_Key_MouseMiddleDouble;
                        }
                    } break;
                    case SDL_BUTTON_RIGHT: {
                        if (sdl_event.button.clicks == 1) {
                            event->key = Gfx_Key_MouseRight;
                        } else {
                            event->key = Gfx_Key_MouseRightDouble;
                        }
                    } break;
                    default: {
                    } break;
                }
            } break;
            case SDL_MOUSEWHEEL: {
                event->kind   = Gfx_EventKind_Scroll;
                event->scroll = v2f32(-sdl_event.wheel.preciseX, sdl_event.wheel.preciseY);
            } break;
        }

        if (event->kind != Gfx_EventKind_Null) {
            dll_push_back(events.first, events.last, event);
        }
    }

    return events;
}

internal V2U32 gfx_get_window_client_area(Gfx_Context *gfx) {
    int width  = 0;
    int height = 0;
    SDL_GL_GetDrawableSize(gfx->window, &width, &height);
    V2U32 result = v2u32((U32) width, (U32) height);
    return(result);
}

internal Void render_rectangle_internal(Gfx_Context *gfx, Render_RectangleParams *parameters) {
    Render_Batch *batch = gfx->batches.last;

    if (!batch || batch->size >= RENDER_BATCH_SIZE) {
        batch = arena_push_struct_zero(&gfx->arena, Render_Batch);
        dll_push_back(gfx->batches.first, gfx->batches.last, batch);
    }

    Render_Rectangle *rect = &batch->rectangles[batch->size++];

    rect->min   = v2f32(parameters->min.x, parameters->min.y);
    rect->max   = v2f32(parameters->max.x, parameters->max.y);
    rect->color = parameters->color;
}

internal Void render_begin(Gfx_Context *gfx) {
    gfx->frame_restore = arena_begin_temporary(&gfx->arena);
}

internal Void render_end(Gfx_Context *gfx) {
    V2U32 client_area = gfx_get_window_client_area(gfx);

    glViewport(0, 0, client_area.width, client_area.height);

    M4F32 projection = m4f32_ortho(0.0f, (F32) client_area.width, 0.0f, (F32) client_area.height, 1.0f, -1.0f);
    glProgramUniformMatrix4fv(gfx->program, gfx->uniform_projection_location, 1, GL_FALSE, &projection.m[0][0]);

    glClear(GL_COLOR_BUFFER_BIT);

    for (Render_Batch *batch = gfx->batches.first; batch; batch = batch->next) {
        glNamedBufferSubData(gfx->vbo, 0, batch->size * sizeof(Render_Rectangle), batch->rectangles);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei) batch->size);
    }

    gfx->batches.first = 0;
    gfx->batches.last  = 0;
    arena_end_temporary(gfx->frame_restore);

    SDL_GL_SwapWindow(gfx->window);
}
