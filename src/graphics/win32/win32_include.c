global Arena        *win32_event_arena;
global Gfx_EventList win32_event_list;

typedef struct Gfx_Win32State Gfx_Win32State;
struct Gfx_Win32State {
    HWND hwnd;
    HDC  hdc;
    U32 buttons_pressed;
    VoidFunction *update;
};

global Gfx_Win32State global_gfx_win32_state;

internal LRESULT CALLBACK win32_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;

    if (win32_event_arena) {
        Gfx_Event *event = arena_push_struct_zero(win32_event_arena, Gfx_Event);

        switch (message) {
            case WM_CLOSE: case WM_QUIT: case WM_DESTROY: {
                event->kind = Gfx_EventKind_Quit;
            } break;
            case WM_SIZE: {
                event->kind = Gfx_EventKind_Resize;
                if (global_gfx_win32_state.update) {
                    PAINTSTRUCT ps = { 0 };
                    BeginPaint(hwnd, &ps);
                    global_gfx_win32_state.update();
                    EndPaint(hwnd, &ps);
                }
            } break;
            case WM_PAINT: {
                if (global_gfx_win32_state.update) {
                    PAINTSTRUCT ps = { 0 };
                    BeginPaint(hwnd, &ps);
                    global_gfx_win32_state.update();
                    EndPaint(hwnd, &ps);
                }
            } break;
            case WM_MOUSEWHEEL: {
                event->kind = Gfx_EventKind_Scroll;
                event->scroll.y = (F32) ((S16) GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA);
                event->position.x = LOWORD(lparam);
                event->position.y = HIWORD(lparam);
            } break;
            case WM_MOUSEHWHEEL: {
                event->kind = Gfx_EventKind_Scroll;
                event->scroll.x = (F32) ((S16) GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA);
                event->position.x = LOWORD(lparam);
                event->position.y = HIWORD(lparam);
            } break;
            case WM_LBUTTONUP: case WM_LBUTTONDOWN:
            case WM_MBUTTONUP: case WM_MBUTTONDOWN:
            case WM_RBUTTONUP: case WM_RBUTTONDOWN: {
                B32 pressed = false;
                switch (message) {
                    case WM_LBUTTONUP:   case WM_MBUTTONUP:   case WM_RBUTTONUP:   pressed = false; break;
                    case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN: pressed = true;  break;
                    default: {
                        // NOTE(simon): Impossible to reach.
                    } break;
                }

                U32 button = 0;
                switch (message) {
                    case WM_LBUTTONUP: case WM_LBUTTONDOWN: button = 0; break;
                    case WM_MBUTTONUP: case WM_MBUTTONDOWN: button = 1; break;
                    case WM_RBUTTONUP: case WM_RBUTTONDOWN: button = 2; break;
                    default: {
                        // NOTE(simon): Impossible to reach.
                    } break;
                }

                Gfx_Key buttons[] = {
                    Gfx_Key_MouseLeft,
                    Gfx_Key_MouseRight,
                    Gfx_Key_MouseMiddle,
                };

                event->kind = pressed ? Gfx_EventKind_KeyPress : Gfx_EventKind_KeyRelease;
                event->key = buttons[button];
                event->key_modifiers |= (GetAsyncKeyState(VK_SHIFT)   & 0x8000) ? Gfx_KeyModifier_Shift   : 0;
                event->key_modifiers |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? Gfx_KeyModifier_Control : 0;
                event->position.x = LOWORD(lparam);
                event->position.y = HIWORD(lparam);

                // NOTE(simon): Determine whether or not the mouse captured.
                global_gfx_win32_state.buttons_pressed &= ~(1 << button);
                global_gfx_win32_state.buttons_pressed |= pressed << button;
                if (global_gfx_win32_state.buttons_pressed) {
                    SetCapture(hwnd);
                } else {
                    ReleaseCapture();
                }
            } break;
            case WM_SYSKEYUP: case WM_SYSKEYDOWN: case WM_KEYUP: case WM_KEYDOWN: {
                U32 vk_code = (U32) wparam;
                B32 is_up   = lparam & (1 << 31);

                if (win32_key_table[vk_code] != 0) {
                    event->kind = is_up ? Gfx_EventKind_KeyRelease : Gfx_EventKind_KeyPress;
                    event->key = win32_key_table[vk_code];
                    event->key_modifiers |= (GetAsyncKeyState(VK_SHIFT)   & 0x8000) ? Gfx_KeyModifier_Shift   : 0;
                    event->key_modifiers |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? Gfx_KeyModifier_Control : 0;
                }
            } break;
            default: {
                result = DefWindowProc(hwnd, message, wparam, lparam);
            } break;
        }

        if (event->kind != Gfx_EventKind_Null) {
            dll_push_back(win32_event_list.first, win32_event_list.last, event);
        }
    } else {
        result = DefWindowProc(hwnd, message, wparam, lparam);
    }

    return result;
}

internal Void gfx_create(Str8 title, U32 width, U32 height) {
    Gfx_Win32State *state = &global_gfx_win32_state;
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    HINSTANCE instance = GetModuleHandle(0);
    CStr16 class_name = cstr16_from_str8(scratch.arena, str8_literal("ApplicationWindowClasssName"));
    WNDCLASS window_class = { 0 };
    window_class.lpfnWndProc = win32_window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = class_name;
    window_class.hCursor = LoadCursor(0, IDC_ARROW);

    ATOM register_class_result = RegisterClass(&window_class);
    if (register_class_result) {
        CStr16 cstr16_title = cstr16_from_str8(scratch.arena, title);
        state->hwnd = CreateWindow(
            window_class.lpszClassName, cstr16_title,
            WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            0, 0, instance, 0
        );

        if (state->hwnd) {
            state->hdc = GetDC(state->hwnd);
            ShowWindow(state->hwnd, SW_SHOW);
        } else {
            // TODO: Error
        }
    } else {
        // TODO: Error
    }

    arena_end_temporary(scratch);
}

internal Void gfx_send_wakeup_event(Void) {
}

internal Gfx_EventList gfx_get_events(Arena *arena, B32 wait) {
    win32_event_arena = arena;
    win32_event_list.first = 0;
    win32_event_list.last  = 0;

    for (MSG message; PeekMessage(&message, 0, 0, 0, PM_REMOVE);) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return win32_event_list;
}

internal V2F32 gfx_get_mouse_position(Void) {
    Gfx_Win32State *state = &global_gfx_win32_state;
    POINT point = { 0 };
    GetCursorPos(&point);
    ScreenToClient(state->hwnd, &point);
    V2F32 result = v2f32((F32) point.x, (F32) point.y);
    return result;
}

internal V2U32 gfx_get_window_client_area(Void) {
    Gfx_Win32State *state = &global_gfx_win32_state;
    RECT rect = { 0 };
    GetClientRect(state->hwnd, &rect);
    V2U32 result = v2u32(rect.right - rect.left, rect.bottom - rect.top);
    return result;
}

internal Void gfx_swap_buffers(Void) {
    Gfx_Win32State *state = &global_gfx_win32_state;
    SwapBuffers(state->hdc);
}

internal Void gfx_set_cursor(Gfx_Cursor cursor) {
    HCURSOR selected_cursor = 0;

#define win32_cursor_list(X) \
    X(Pointer,  ARROW)       \
    X(Hand,     HAND)        \
    X(Beam,     IBEAM)       \
    X(SizeNWSE, SIZENWSE)    \
    X(SizeNESW, SIZENESW)    \
    X(SizeWE,   SIZEWE)      \
    X(SizeNS,   SIZENS)      \
    X(SizeAll,  SIZEALL)     \
    X(Disabled, NO)
#define win32_load_cursor(gfx_kind, win32_kind)             \
    case Gfx_Cursor_##gfx_kind: {                           \
        local HCURSOR win32_cursor = 0;                     \
        if (!win32_cursor) {                                \
            win32_cursor = LoadCursor(0, IDC_##win32_kind); \
        }                                                   \
        selected_cursor = win32_cursor;                     \
    } break;

    switch (cursor) {
        win32_cursor_list(win32_load_cursor)
        case Gfx_Cursor_COUNT: break;
    }

#undef win32_load_cursor
#undef win32_cursor_list

    if (selected_cursor) {
        SetCursor(selected_cursor);
    }
}

internal Void gfx_set_update_function(VoidFunction *update) {
    global_gfx_win32_state.update = update;
}
