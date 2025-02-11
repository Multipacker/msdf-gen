#ifndef X11_OPENGL_H
#define X11_OPENGL_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef struct X11_OpenGLState X11_OpenGLState;
struct X11_OpenGLState {
    EGLDisplay display;
    EGLSurface *surface;
};

internal Void x11_opengl_swap_buffers(Void);

#endif // X11_OPENGL_H
