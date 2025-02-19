global Wayland_OpenGLState global_wayland_opengl_state;

internal B32 render_init(Void) {
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

            EGL_RED_SIZE,     8,
            EGL_GREEN_SIZE,   8,
            EGL_BLUE_SIZE,    8,
            EGL_ALPHA_SIZE,   8,
            EGL_DEPTH_SIZE,   24,
            EGL_STENCIL_SIZE, 8,

            EGL_NONE,
        };

        EGLConfig config = { 0 };
        EGLint count = 0;
        if (eglChooseConfig(opengl_state->display, config_attributes, &config, 1, &count) && count >= 1) {
            EGLint context_attributes[] = {
                EGL_CONTEXT_MAJOR_VERSION, 4,
                EGL_CONTEXT_MINOR_VERSION, 5,
                EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
#if DEBUG_BUILD
                EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
#endif
                EGL_NONE,
            };

            EGLContext *context = eglCreateContext(opengl_state->display, config, EGL_NO_CONTEXT, context_attributes);
            if (context) {
                const EGLAttrib surface_attributes[] = {
                    EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
                    EGL_NONE,
                };

                opengl_state->window = wl_egl_window_create(wayland_state->surface->surface, wayland_state->width, wayland_state->height);

                opengl_state->surface = eglCreatePlatformWindowSurface(opengl_state->display, config, opengl_state->window, surface_attributes);
                if (opengl_state->surface) {
                    eglMakeCurrent(opengl_state->display, opengl_state->surface, opengl_state->surface, context);

#define X(type, name) name = (type) eglGetProcAddress(#name); assert(name);
                    GL_LINUX_FUNCTIONS(X)
                    GL_FUNCTIONS(X)
#undef X

                    eglSwapInterval(opengl_state->display, 1);
                    wayland_state->swap_buffers = wayland_opengl_swap_buffers;
                }
            }
        }
    }

    return true;
}

internal Void wayland_opengl_swap_buffers(Void) {
    Wayland_State *wayland_state = &global_wayland_state;
    Wayland_OpenGLState *opengl_state = &global_wayland_opengl_state;
    eglSwapBuffers(opengl_state->display, opengl_state->surface);
}

internal Void opengl_resize(V2U32 resolution) {
    Wayland_OpenGLState *opengl_state = &global_wayland_opengl_state;
    wl_egl_window_resize(opengl_state->window, (S32) resolution.width, (S32) resolution.height, 0, 0);
}

internal Void opengl_backend_init(Void) {
}
