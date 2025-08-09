global Wayland_OpenGLState global_wayland_opengl_state;

internal Render_Window opengl_handle_from_window(OpenGL_Window *window) {
    Render_Window handle = { 0 };
    handle.u64[0] = integer_from_pointer(window);
    return handle;
}

internal OpenGL_Window *opengl_window_from_handle(Render_Window handle) {
    OpenGL_Window *window = (OpenGL_Window *) pointer_from_integer(handle.u64[0]);
    return window;
}

internal B32 opengl_backend_init(Void) {
    Wayland_State *wayland_state = &global_wayland_state;
    Wayland_OpenGLState *opengl_state = &global_wayland_opengl_state;
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    opengl_state->permanent_arena = arena_create();

    // NOTE(simon): Get display.
    opengl_state->display = eglGetDisplay((EGLNativeDisplayType) wayland_state->display);
    if (opengl_state->display == EGL_NO_DISPLAY) {
        gfx_message(true, str8_literal("Failed to initialize OpenGL"), str8_literal("Could not acquire EGL display."));
        os_exit(1);
    }

    // NOTE(simon): Initialize.
    EGLint major = 0, minor = 0;
    if (!eglInitialize(opengl_state->display, &major, &minor)) {
        gfx_message(true, str8_literal("Failed to initialize OpenGL"), str8_literal("Could not initialize EGL."));
        os_exit(1);
    }

    // NOTE(simon): Bind OpenGL API.
    if (!eglBindAPI(EGL_OPENGL_API)) {
        gfx_message(true, str8_literal("Failed to initialize OpenGL"), str8_literal("Could not bind OpenGL API."));
        os_exit(1);
    }

    // NOTE(simon): Create context.
    EGLint context_attributes[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE,
    };
    opengl_state->context = eglCreateContext(opengl_state->display, 0, EGL_NO_CONTEXT, context_attributes);
    if (opengl_state->context == EGL_NO_CONTEXT) {
        gfx_message(true, str8_literal("Failed to initialize OpenGL"), str8_literal("Could not create a context."));
        os_exit(1);
    }

    // NOTE(simon): Gather configs.
    EGLint config_attributes[] = {
        EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
        EGL_CONFORMANT,        EGL_OPENGL_BIT,
        EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
        EGL_LEVEL,             0,
        EGL_ALPHA_SIZE,        8,
        EGL_RED_SIZE,          8,
        EGL_GREEN_SIZE,        8,
        EGL_BLUE_SIZE,         8,
        EGL_DEPTH_SIZE,        24,
        EGL_STENCIL_SIZE,      8,
        EGL_NONE,
    };
    EGLint available_config_count = 0;
    eglChooseConfig(opengl_state->display, config_attributes, 0, 0, &available_config_count);
    EGLConfig *available_configs = arena_push_array(scratch.arena, EGLConfig, (U64) available_config_count);
    eglChooseConfig(opengl_state->display, config_attributes, available_configs, available_config_count, &available_config_count);

    // NOTE(simon): Choose config.
    for (EGLint i = 0; i < available_config_count; ++i) {
        EGLConfig config = available_configs[i];

        if (1) {
            opengl_state->config = config;
            break;
        }
    }

    eglMakeCurrent(opengl_state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, opengl_state->context);

#define X(type, name) name = (type) eglGetProcAddress(#name); assert(name);
    GL_FUNCTIONS(X)
#undef X

    arena_end_temporary(scratch);
    return true;
}

internal Render_Window opengl_backend_create(Gfx_Window handle) {
    Wayland_OpenGLState *opengl_state = &global_wayland_opengl_state;
    Wayland_Window *graphics_window = wayland_window_from_handle(handle);

    OpenGL_Window *render_window = opengl_state->window_freelist;
    if (render_window) {
        sll_stack_pop(opengl_state->window_freelist);
        memory_zero_struct(render_window);
    } else {
        render_window = arena_push_struct(opengl_state->permanent_arena, OpenGL_Window);
    }

    render_window->window = wl_egl_window_create(graphics_window->surface->surface, graphics_window->surface->width, graphics_window->surface->height);

    const EGLint surface_attributes[] = {
        EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
        EGL_NONE,
    };

    render_window->surface = eglCreateWindowSurface(opengl_state->display, opengl_state->config, (EGLNativeWindowType) render_window->window, surface_attributes);

    if (render_window->surface == EGL_NO_SURFACE) {
        gfx_message(true, str8_literal("Failed to create OpenGL window"), str8_literal("Could not create a EGL window surface."));
        os_exit(1);
    }

    eglSwapInterval(opengl_state->display, 1);

    Render_Window result = opengl_handle_from_window(render_window);
    return result;
}

internal Void opengl_backend_destroy(Gfx_Window graphics_handle, Render_Window render_handle) {
    Wayland_OpenGLState *opengl_state = &global_wayland_opengl_state;
    OpenGL_Window *render_window = opengl_window_from_handle(render_handle);

    eglMakeCurrent(opengl_state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, opengl_state->context);
    eglDestroySurface(opengl_state->display, render_window->surface);
    wl_egl_window_destroy(render_window->window);

    sll_stack_push(opengl_state->window_freelist, render_window);
}

internal Void opengl_window_resize(Gfx_Window graphics_handle, Render_Window render_handle) {
    Wayland_OpenGLState *opengl_state = &global_wayland_opengl_state;
    OpenGL_Window *render_window = opengl_window_from_handle(render_handle);

    V2U32 resolution = gfx_client_area_from_window(graphics_handle);

    if (render_window->resolution.width != resolution.width || render_window->resolution.height != resolution.height) {
        render_window->resolution = resolution;
        wl_egl_window_resize(render_window->window, (S32) resolution.width, (S32) resolution.height, 0, 0);
    }
}

internal Void opengl_window_select(Gfx_Window graphics_handle, Render_Window render_handle) {
    Wayland_OpenGLState *opengl_state = &global_wayland_opengl_state;
    OpenGL_Window *render_window = opengl_window_from_handle(render_handle);

    eglMakeCurrent(opengl_state->display, render_window->surface, render_window->surface, opengl_state->context);

    // NOTE(simon): This doesn't automatically get set if our first
    // eglMakeCurrent doesn't have a default framebuffer. On my desktop using
    // Xwayland, I get a black screen if I don't run this with a surface bound.
    glDrawBuffer(GL_BACK);
}

internal Void opengl_swap_buffers(Gfx_Window graphics_handle, Render_Window render_handle) {
    Wayland_OpenGLState *state = &global_wayland_opengl_state;
    OpenGL_Window *render_window = opengl_window_from_handle(render_handle);
    Wayland_Window *graphics_window = wayland_window_from_handle(graphics_handle);

    if (graphics_window->is_configured) {
        eglSwapBuffers(state->display, render_window->surface);
    }
}
