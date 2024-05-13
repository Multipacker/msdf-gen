#ifndef SDL_INCLUDE_H
#define SDL_INCLUDE_H

#include <SDL2/SDL.h>

typedef struct Gfx_Context Gfx_Context;
struct Gfx_Context {
    SDL_Window      *window;
    SDL_GLContext    gl_context;
};

#endif // SDL_INCLUDE_H
