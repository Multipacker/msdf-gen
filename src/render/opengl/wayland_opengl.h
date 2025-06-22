#ifndef WAYLAND_OPENGL_H
#define WAYLAND_OPENGL_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-egl.h>

typedef struct OpenGL_Window OpenGL_Window;
struct OpenGL_Window {
    OpenGL_Window *next;

    struct wl_egl_window *window;
    EGLSurface *surface;
    V2U32 resolution;
};

typedef struct Wayland_OpenGLState Wayland_OpenGLState;
struct Wayland_OpenGLState {
    Arena *permanent_arena;
    EGLDisplay display;
    EGLConfig  config;
    EGLContext context;

    OpenGL_Window *window_freelist;
};

internal Void wayland_opengl_swap_buffers(Render_Window handle);

#endif // WAYLAND_OPENGL_H
