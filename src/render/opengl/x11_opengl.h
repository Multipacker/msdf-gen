#ifndef X11_OPENGL_H
#define X11_OPENGL_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef struct OpenGL_Window OpenGL_Window;
struct OpenGL_Window {
    OpenGL_Window *next;

    EGLSurface *surface;
};

typedef struct X11_OpenGLState X11_OpenGLState;
struct X11_OpenGLState {
    Arena     *permanent_arena;
    EGLDisplay display;
    EGLConfig  config;
    EGLContext context;

    OpenGL_Window *window_freelist;
};

#endif // X11_OPENGL_H
