#ifndef WAYLAND_OPENGL_H
#define WAYLAND_OPENGL_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-egl.h>

typedef struct Wayland_OpenGLState Wayland_OpenGLState;
struct Wayland_OpenGLState {
    EGLDisplay display;
    EGLConfig  config;
    EGLContext context;

    struct wl_egl_window *window;
    EGLSurface *surface;
};

internal Void wayland_opengl_swap_buffers(Void);
internal Void wayland_opengl_resize(Void);

#endif // WAYLAND_OPENGL_H
