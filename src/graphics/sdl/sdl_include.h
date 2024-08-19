#ifndef SDL_INCLUDE_H
#define SDL_INCLUDE_H

#include <SDL2/SDL.h>

typedef struct SDL_State SDL_State;
struct SDL_State {
    SDL_Window   *window;
    SDL_GLContext gl_context;
};

#endif // SDL_INCLUDE_H
