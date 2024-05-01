global Gfx_Key sdl_to_gfx_keycode[128];

internal Gfx_Context *gfx_create(Arena *arena, Str8 title, U32 width, U32 height) {
    Arena_Temporary restore_point = arena_begin_temporary(arena);
    Gfx_Context *result = arena_push_struct_zero(arena, Gfx_Context);
    result->arena = arena_create();

    B32 error = 0;

    // NOTE(simon): Initialize SDL to gfx keycode table.
    for (SDL_KeyCode key = SDLK_0; key <= SDLK_9; ++key)
    {
        sdl_to_gfx_keycode[key] = (Gfx_Key) (Gfx_Key_0 + (key - SDLK_0));
    }
    for (SDL_KeyCode key = SDLK_a; key <= SDLK_z; ++key)
    {
        sdl_to_gfx_keycode[key] = (Gfx_Key) (Gfx_Key_A + (key - SDLK_a));
    }
    sdl_to_gfx_keycode[SDLK_BACKSPACE] = Gfx_Key_Backspace;
    sdl_to_gfx_keycode[SDLK_SPACE]     = Gfx_Key_Space;
    sdl_to_gfx_keycode[SDLK_TAB]       = Gfx_Key_Tab;
    sdl_to_gfx_keycode[SDLK_RETURN]    = Gfx_Key_Return;
    sdl_to_gfx_keycode[SDLK_ESCAPE]    = Gfx_Key_Escape;
    sdl_to_gfx_keycode[SDLK_DELETE]    = Gfx_Key_Delete;

    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        Arena_Temporary scratch = arena_get_scratch(&arena, 1);

        CStr cstr_title = cstr_from_str8(scratch.arena, title);

        result->window = SDL_CreateWindow(
            cstr_title,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            (int) width, (int) height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
        );

        arena_end_temporary(scratch);

        if (result->window) {
            result->gl_context = SDL_GL_CreateContext(result->window);
            SDL_GL_MakeCurrent(result->window, result->gl_context);

#define X(type, name) name = (type) SDL_GL_GetProcAddress(#name); assert(name);
            GL_LINUX_FUNCTION(X)
            GL_FUNCTIONS(X)
#undef X

            SDL_GL_SetSwapInterval(1);
        } else {
            error_emit(str8_literal("Could not create SDL window."));
            error = true;
        }
    } else {
        error_emit(str8_literal("Could not initialize SDL 2.0"));
        error = true;
    }

    if (error) {
        if (result->window) {
            SDL_DestroyWindow(result->window);
        }

        arena_end_temporary(restore_point);
        result = 0;
    }

    return result;
}

internal Gfx_EventList gfx_get_events(Arena *arena, Gfx_Context *gfx) {
    Gfx_EventList events = { 0 };

    for (SDL_Event sdl_event = { 0 }; SDL_PollEvent(&sdl_event);) {
        Gfx_Event *event = arena_push_struct_zero(arena, Gfx_Event);
        switch (sdl_event.type) {
            case SDL_QUIT: {
                event->kind = Gfx_EventKind_Quit;
            } break;
            case SDL_KEYDOWN: case SDL_KEYUP: {
                if (sdl_event.key.type == SDL_KEYDOWN) {
                    event->kind = Gfx_EventKind_KeyPress;
                } else {
                    event->kind = Gfx_EventKind_KeyRelease;
                }

                SDL_Keycode sdl_keycode = sdl_event.key.keysym.sym;

                switch (sdl_keycode) {
                    case SDLK_PAGEUP:   event->key = Gfx_Key_PageUp;   break;
                    case SDLK_PAGEDOWN: event->key = Gfx_Key_PageDown; break;
                    case SDLK_LEFT:     event->key = Gfx_Key_Left;     break;
                    case SDLK_RIGHT:    event->key = Gfx_Key_Right;    break;
                    case SDLK_UP:       event->key = Gfx_Key_Up;       break;
                    case SDLK_DOWN:     event->key = Gfx_Key_Down;     break;
                    case SDLK_LSHIFT:   event->key = Gfx_Key_Shift;    break;
                    case SDLK_RSHIFT:   event->key = Gfx_Key_Shift;    break;
                    case SDLK_END:      event->key = Gfx_Key_End;      break;
                    case SDLK_HOME:     event->key = Gfx_Key_Home;     break;
                    case SDLK_LCTRL:    event->key = Gfx_Key_Control;  break;
                    case SDLK_RCTRL:    event->key = Gfx_Key_Control;  break;
                    case SDLK_LALT:     event->key = Gfx_Key_Alt;      break;
                    case SDLK_RALT:     event->key = Gfx_Key_Alt;      break;
                    case SDLK_LGUI:     event->key = Gfx_Key_OS;       break;
                    case SDLK_RGUI:     event->key = Gfx_Key_OS;       break;
                    default: {
                        if (SDLK_F1 <= sdl_keycode && sdl_keycode <= SDLK_F12) {
                            event->key = (Gfx_Key) (Gfx_Key_F1 + (sdl_keycode - SDLK_F1));
                        } else {
                            event->key = sdl_to_gfx_keycode[sdl_keycode];
                        }
                    } break;
                }

                SDL_Keymod modifiers = SDL_GetModState();
                event->key_modifiers |= (modifiers & KMOD_SHIFT ? Gfx_KeyModifier_Shift   : 0);
                event->key_modifiers |= (modifiers & KMOD_CTRL  ? Gfx_KeyModifier_Control : 0);

                if (event->key == Gfx_Key_Null) {
                    event->kind = Gfx_EventKind_Null;
                }
            } break;
            case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: {
                if (sdl_event.button.type == SDL_MOUSEBUTTONDOWN) {
                    event->kind = Gfx_EventKind_KeyPress;
                } else {
                    event->kind = Gfx_EventKind_KeyRelease;
                }

                switch (sdl_event.button.button) {
                    case SDL_BUTTON_LEFT: {
                        if (sdl_event.button.clicks == 1) {
                            event->key = Gfx_Key_MouseLeft;
                        } else {
                            event->key = Gfx_Key_MouseLeftDouble;
                        }
                    } break;
                    case SDL_BUTTON_MIDDLE: {
                        if (sdl_event.button.clicks == 1) {
                            event->key = Gfx_Key_MouseMiddle;
                        } else {
                            event->key = Gfx_Key_MouseMiddleDouble;
                        }
                    } break;
                    case SDL_BUTTON_RIGHT: {
                        if (sdl_event.button.clicks == 1) {
                            event->key = Gfx_Key_MouseRight;
                        } else {
                            event->key = Gfx_Key_MouseRightDouble;
                        }
                    } break;
                    default: {
                    } break;
                }
            } break;
            case SDL_MOUSEWHEEL: {
                event->kind   = Gfx_EventKind_Scroll;
                event->scroll = v2f32(-sdl_event.wheel.preciseX, sdl_event.wheel.preciseY);
            } break;
        }

        if (event->kind != Gfx_EventKind_Null) {
            dll_push_back(events.first, events.last, event);
        }
    }

    return events;
}

internal V2F32 gfx_get_mouse_position(Gfx_Context *gfx) {
    int x = 0;
    int y = 0;
    SDL_GetMouseState(&x, &y);

    V2F32 result = v2f32(x, y);
    return result;
}

internal V2U32 gfx_get_window_client_area(Gfx_Context *gfx) {
    int width  = 0;
    int height = 0;
    SDL_GL_GetDrawableSize(gfx->window, &width, &height);
    V2U32 result = v2u32((U32) width, (U32) height);
    return result;
}
