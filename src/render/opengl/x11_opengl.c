global X11_OpenGLState global_x11_opengl_state;

internal B32 render_init(Void) {
    X11_State *x11_state = &global_x11_state;
    X11_OpenGLState *opengl_state = &global_x11_opengl_state;

    const EGLint attributes[] = {
        EGL_PLATFORM_XCB_SCREEN_EXT,
        0, // TODO(simon): Replace with screen from connection
        EGL_NONE,
    };
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
    opengl_state->display = eglGetPlatformDisplayEXT(EGL_PLATFORM_XCB_EXT, x11_state->connection, attributes);

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
                EGLint surface_attributes[] = {
                    EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
                    EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
                    EGL_NONE,
                };

                opengl_state->surface = eglCreateWindowSurface(opengl_state->display, config, x11_state->window, surface_attributes);
                if (opengl_state->surface) {
                    eglMakeCurrent(opengl_state->display, opengl_state->surface, opengl_state->surface, context);

#define X(type, name) name = (type) eglGetProcAddress(#name); assert(name);
                    GL_LINUX_FUNCTIONS(X)
                    GL_FUNCTIONS(X)
#undef X

                    eglSwapInterval(opengl_state->display, 1);
                    x11_state->swap_buffers = x11_opengl_swap_buffers;
                }
            }
        }
    }

    return true;
}

internal Void x11_opengl_swap_buffers(Void) {
    X11_OpenGLState *opengl_state = &global_x11_opengl_state;
    eglSwapBuffers(opengl_state->display, opengl_state->surface);
}

internal Void opengl_backend_init(Void) {
}
