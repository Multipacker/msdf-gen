global Wayland_OpenGLState global_wayland_opengl_state;

internal B32 render_init(Void) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    Wayland_State *wayland_state = &global_wayland_state;
    Wayland_OpenGLState *opengl_state = &global_wayland_opengl_state;

    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");

    const EGLint attributes[] = {
        EGL_NONE,
    };

    opengl_state->display = eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_EXT, wayland_state->display, attributes);

    EGLint major = 0, minor = 0;
    if (eglInitialize(opengl_state->display, &major, &minor)) {
        eglBindAPI(EGL_OPENGL_API);

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

        for (EGLint i = 0; i < available_config_count; ++i) {
            EGLConfig config = available_configs[i];

            // TODO(simon): Use some more logic to determine which config to use.
            if (1) {
                opengl_state->config = config;
                break;
            }
        }

        if (opengl_state->config) {
            EGLint context_attributes[] = {
                EGL_CONTEXT_MAJOR_VERSION, 4,
                EGL_CONTEXT_MINOR_VERSION, 5,
                EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
#if DEBUG_BUILD
                EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
#endif
                EGL_NONE,
            };

            opengl_state->context = eglCreateContext(opengl_state->display, opengl_state->config, EGL_NO_CONTEXT, context_attributes);
        }
    }

    // NOTE(simon): Create window resources.
    if (opengl_state->context) {
        const EGLAttrib surface_attributes[] = {
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
            EGL_NONE,
        };

        opengl_state->window = wl_egl_window_create(wayland_state->surface->surface, wayland_state->width, wayland_state->height);

        opengl_state->surface = eglCreatePlatformWindowSurface(opengl_state->display, opengl_state->config, opengl_state->window, surface_attributes);
        if (opengl_state->surface) {
            eglMakeCurrent(opengl_state->display, opengl_state->surface, opengl_state->surface, opengl_state->context);

#define X(type, name) name = (type) eglGetProcAddress(#name); assert(name);
            GL_LINUX_FUNCTIONS(X)
            GL_FUNCTIONS(X)
#undef X

            eglSwapInterval(opengl_state->display, 1);
            wayland_state->swap_buffers = wayland_opengl_swap_buffers;
        }
    }

    arena_end_temporary(scratch);
    return true;
}

internal Void wayland_opengl_swap_buffers(Void) {
    Wayland_State *wayland_state = &global_wayland_state;
    Wayland_OpenGLState *opengl_state = &global_wayland_opengl_state;
    eglSwapBuffers(opengl_state->display, opengl_state->surface);
}

internal Void opengl_resize(V2U32 resolution) {
    Wayland_OpenGLState *opengl_state = &global_wayland_opengl_state;
    if (opengl_state->resolution.width != resolution.width || opengl_state->resolution.height != resolution.height) {
        opengl_state->resolution = resolution;
        wl_egl_window_resize(opengl_state->window, (S32) resolution.width, (S32) resolution.height, 0, 0);
    }
}

internal Void opengl_backend_init(Void) {
}
