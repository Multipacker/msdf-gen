global Arena        *win32_event_arena;
global Gfx_EventList win32_event_list;

global Gfx_Win32State gfx_win32_state;



// NOTE(simon): Modern Windows APIs that could be missing from older SDKs, so
// we have to load them dynamically.
#define WIN32_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((Void *) -4)
#define WIN32_PROCESS_PER_MONITOR_DPI_AWARE 2
#define WIN32_MDT_EFFECTIVE_DPI 0

typedef HRESULT Win32_SetProcessDpiAwareness(int);
typedef HRESULT Win32_GetDpiForMonitor(HMONITOR hmonitor, int dpiType, UINT *dpiX, UINT *dpiY);
typedef BOOL    Win32_SetProcessDpiAwarenessContext(Void *value);
typedef UINT    Win32_GetDpiForWindow(HWND hwnd);

global Win32_GetDpiForMonitor *win32_get_dpi_for_monitor = 0;
global Win32_GetDpiForWindow  *win32_get_dpi_for_window  = 0;



// NOTE(simon): Helpers for converting to and from handles.
internal Gfx_Window win32_handle_from_window(Gfx_Win32Window *window) {
    Gfx_Window result = { 0 };
    result.u64[0] = integer_from_pointer(window);
    reuslt.u64[1] = window->generation;
    return result;
}

internal Gfx_Win32Window *win32_window_from_handle(Gfx_Window handle) {
    Gfx_Win32Window *window = (Gfx_Win32Window *) pointer_from_integer(handle.u64[0]);
    if (!window || window->generation != handle.u64[1]) {
        window = 0;
    }
    return window;
}

internal Gfx_Win32Window *win32_window_from_hwnd(HWND hwnd) {
    Gfx_Win32State *state = &gfx_win32_state;

    Gfx_Win32Window *result = 0;
    for (Gfx_Win32Window *window = state->first_window; window; window = window->next) {
        if (window->hwnd == hwnd) {
            result = window;
            break;
        }
    }

    return result;
}



internal LRESULT CALLBACK win32_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    Gfx_Win32State *state = &gfx_win32_state;
    LRESULT result = 0;

    if (win32_event_arena) {
        Gfx_Win32Window *window        = win32_window_from_hwnd(hwnd);
        Gfx_Window       window_handle = win32_handle_from_window(window);

        switch (message) {
            case WM_CLOSE: case WM_QUIT: case WM_DESTROY: {
                Gfx_Event *event = arena_push_struct(win32_event_arena, Gfx_Event);
                event->kind = Gfx_EventKind_Quit;
                event->window = window_handle;
                dll_push_back(win32_event_list.first, win32_event_list.last, event);
            } break;
            case WM_SIZE:
            case WM_PAINT: {
                if (state->update) {
                    PAINTSTRUCT ps = { 0 };
                    BeginPaint(hwnd, &ps);
                    state->update();
                    EndPaint(hwnd, &ps);
                }
            } break;
            case WM_SETCURSOR: {
                RECT rect = { 0 };
                GetClientRect(hwnd, &rect);
                R2F32 window_rectangle = r2f32(
                    (F32) rect.left,
                    (F32) rect.top,
                    (F32) rect.right,
                    (F32) rect.bottom
                );

                V2F32 mouse = gfx_mouse_position_from_window(window_handle);
                if (r2f32_contains_v2f32(window_rectangle, mouse)) {
                    SetCursor(state->cursor);
                } else {
                    result = DefWindowProc(hwnd, message, wparam, lparam);
                }
            } break;
            case WM_DPICHANGED: {
                window->dpi = (F32) LOWORD(wparam);

                RECT *new_window = (RECT *) lparam;
                SetWindowPos(
                    hwnd,
                    0,
                    new_window->left,
                    new_window->top,
                    new_window->right - new_window->left,
                    new_window->bottom - new_window->top,
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
            } break;
            case WM_MOUSEWHEEL: {
                Gfx_Event *event = arena_push_struct(win32_event_arena, Gfx_Event);
                event->kind = Gfx_EventKind_Scroll;
                event->scroll.y = (F32) ((S16) GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA);
                POINT point = { 0 };
                point.x = (S32) (S16) LOWORD(lparam);
                point.y = (S32) (S16) HIWORD(lparam);
                ScreenToClient(hwnd, &point);
                event->position.x = (F32) point.x;
                event->position.y = (F32) point.y;
                event->window = window_handle;
                dll_push_back(win32_event_list.first, win32_event_list.last, event);
            } break;
            case WM_MOUSEHWHEEL: {
                Gfx_Event *event = arena_push_struct(win32_event_arena, Gfx_Event);
                event->kind = Gfx_EventKind_Scroll;
                event->scroll.x = (F32) ((S16) GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA);
                POINT point = { 0 };
                point.x = (S32) (S16) LOWORD(lparam);
                point.y = (S32) (S16) HIWORD(lparam);
                ScreenToClient(hwnd, &point);
                event->position.x = (F32) point.x;
                event->position.y = (F32) point.y;
                event->window = window_handle;
                dll_push_back(win32_event_list.first, win32_event_list.last, event);
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

                Gfx_Event *event = arena_push_struct(win32_event_arena, Gfx_Event);
                event->kind = pressed ? Gfx_EventKind_KeyPress : Gfx_EventKind_KeyRelease;
                event->key = buttons[button];
                event->key_modifiers |= (GetAsyncKeyState(VK_SHIFT)   & 0x8000) ? Gfx_KeyModifier_Shift   : 0;
                event->key_modifiers |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? Gfx_KeyModifier_Control : 0;
                event->position.x = (F32) (S16) LOWORD(lparam);
                event->position.y = (F32) (S16) HIWORD(lparam);
                event->window = window_handle;
                dll_push_back(win32_event_list.first, win32_event_list.last, event);

                // NOTE(simon): Determine whether or not the mouse captured.
                window->buttons_pressed &= ~(1 << button);
                window->buttons_pressed |= pressed << button;
                if (window->buttons_pressed) {
                    SetCapture(hwnd);
                } else {
                    ReleaseCapture();
                }
            } break;
            case WM_SYSKEYUP: case WM_SYSKEYDOWN: case WM_KEYUP: case WM_KEYDOWN: {
                U32 vk_code = (U32) wparam;
                B32 is_up   = lparam & (1 << 31);

                if (win32_key_table[vk_code] != 0) {
                    Gfx_Event *event = arena_push_struct(win32_event_arena, Gfx_Event);
                    event->kind = is_up ? Gfx_EventKind_KeyRelease : Gfx_EventKind_KeyPress;
                    event->key = win32_key_table[vk_code];
                    event->key_modifiers |= (GetAsyncKeyState(VK_SHIFT)   & 0x8000) ? Gfx_KeyModifier_Shift   : 0;
                    event->key_modifiers |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? Gfx_KeyModifier_Control : 0;
                    event->window = window_handle;
                    dll_push_back(win32_event_list.first, win32_event_list.last, event);
                }
            } break;
            case WM_CHAR: {
                U32 codepoint = wparam;
                B32 is_c0_control = codepoint <= 0x1F || codepoint == 0x7F;
                B32 is_c1_control = (0x80 <= codepoint && codepoint <= 0x9F);

                if (!is_c0_control && !is_c1_control) {
                    U8 *buffer = arena_push_array(win32_event_arena, U8, 4);
                    U64 length = string_encode_utf8(buffer, codepoint);

                    Gfx_Event *event = arena_push_struct(win32_event_arena, Gfx_Event);
                    event->kind = Gfx_EventKind_Text;
                    event->text = str8(buffer, length);
                    event->window = window_handle;
                    dll_push_back(win32_event_list.first, win32_event_list.last, event);
                }
            } break;
            case WM_DROPFILES: {
                HDROP drop = (HDROP) wparam;
                POINT drop_point = { 0 };
                DragQueryPoint(drop, &drop_point);
                U64 file_count = DragQueryFileW(drop, 0xFFFFFFFF, 0, 0);
                for (U64 i = 0; i < file_count; ++i) {
                    U64 name_size = DragQueryFileW(drop, i, 0, 0) + 1;
                    U16 *name_buffer = arena_push_array(win32_event_arena, U16, name_size);
                    DragQueryFileW(drop, i, name_buffer, name_size);

                    Gfx_Event *drop_event = arena_push_struct(win32_event_arena, Gfx_Event);
                    drop_event->kind = Gfx_EventKind_FileDrop;
                    drop_event->position = v2f32((F32) drop_point.x, (F32) drop_point.y);
                    drop_event->path = str8_from_str16(win32_event_arena, str16(name_buffer, name_size - 1));
                    dll_push_back(win32_event_list.first, win32_event_list.last, drop_event);
                }
                DragFinish(drop);
            } break;
            default: {
                result = DefWindowProc(hwnd, message, wparam, lparam);
            } break;
        }
    } else {
        result = DefWindowProc(hwnd, message, wparam, lparam);
    }

    return result;
}

internal Void gfx_init(Void) {
    Gfx_Win32State *state = &gfx_win32_state;

    state->permanent_arena = arena_create();

    state->graphics_thread = GetCurrentThreadId();

    // NOTE(simon): Set Windows 10 DPI awareness.
    Win32_SetProcessDpiAwarenessContext *set_process_dpi_awareness_context = 0;
    HMODULE user32_module = LoadLibraryA("user32.dll");
    if (user32_module) {
        set_process_dpi_awareness_context = (Win32_SetProcessDpiAwarenessContext *) GetProcAddress(user32_module, "SetProcessDpiAwarenessContext");
        win32_get_dpi_for_window = (Win32_GetDpiForWindow *) GetProcAddress(user32_module, "GetDpiForWindow");
        FreeLibrary(user32_module);
    }
    if (set_process_dpi_awareness_context) {
        set_process_dpi_awareness_context(WIN32_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    } else {
        // NOTE(simon): Fallback to Windows 8.1 DPI awareness.
        HMODULE shcore_module = LoadLibraryA("shcore.dll");
        Win32_SetProcessDpiAwareness *set_process_dpi_awareness = 0;
        if (shcore_module) {
            set_process_dpi_awareness = (Win32_SetProcessDpiAwareness *) GetProcAddress(shcore_module, "SetProcessDpiAwareness");
            win32_get_dpi_for_monitor = (Win32_GetDpiForMonitor *) GetProcAddress(shcore_module, "GetDpiForMonitor");

            // NOTE(simon): Don't free the library, we will need
            // win32_get_dpi_for_monitor throughout the rest of the program
            // execution.
        }

        if (set_process_dpi_awareness) {
            set_process_dpi_awareness(WIN32_PROCESS_PER_MONITOR_DPI_AWARE);
        } else {
            // TODO(simon): Fallback to Windows Vista DPI awareness.
        }
    }

    state->instance = GetModuleHandle(0);
    CStr16 class_name = cstr16_from_str8(state->permanent_arena, str8_literal("ApplicationWindowClasssName"));
    state->window_class.lpfnWndProc   = win32_window_proc;
    state->window_class.hInstance     = state->instance;
    state->window_class.lpszClassName = class_name;
    state->window_class.hCursor       = LoadCursor(0, IDC_ARROW);
    ATOM register_class_result = RegisterClass(&state->window_class);
}



// NOTE(simon): Events.
internal Void gfx_send_wakeup_event(Void) {
    Gfx_Win32State *state = &gfx_win32_state;
    PostThreadMessage(state->graphics_thread, WM_USER, 0, 0);
}

internal Gfx_EventList gfx_get_events(Arena *arena, B32 wait) {
    win32_event_arena = arena;
    win32_event_list.first = 0;
    win32_event_list.last  = 0;

    MSG message = { 0 };
    if (!wait || GetMessage(&message, 0, 0, 0)) {
        for (B32 first_wait = wait; first_wait || PeekMessage(&message, 0, 0, 0, PM_REMOVE); first_wait = false) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    return win32_event_list;
}

internal Void gfx_set_update_function(VoidFunction *update) {
    gfx_win32_state.update = update;
}



internal Void gfx_set_cursor(Gfx_Cursor cursor) {
    Gfx_Win32State *state = &gfx_win32_state;
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
        PostMessage(0, WM_SETCURSOR, 0, 0);
        state->cursor = selected_cursor;
    }
}




// NOTE(simon): Windows.
internal B32 gfx_window_equal(Gfx_Window handle_a, Gfx_Window handle_b) {
    Gfx_Win32Window *window_a = win32_window_from_handle(handle_a);
    Gfx_Win32Window *window_b = win32_window_from_handle(handle_b);

    B32 result = window_a == window_b;
    return result;
}

internal Gfx_Window gfx_window_create(Str8 title, U32 width, U32 height) {
    Gfx_Win32State *state = &gfx_win32_state;
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    Gfx_Win32Window *window = state->window_freelist;
    if (window) {
        sll_stack_pop(state->window_freelist);
        U64 generation = window->generation;
        memory_zero_struct(window);
        window->generation = generation;
    } else {
        window = arena_push_struct(state->permanent_arena, Gfx_Win32Window);
    }
    dll_push_back(state->first_window, state->last_window, window);

    CStr16 cstr16_title = cstr16_from_str8(scratch.arena, title);
    window->hwnd = CreateWindow(
        state->window_class.lpszClassName, cstr16_title,
        WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        0, 0, state->instance, 0
    );

    if (window->hwnd) {
        if (win32_get_dpi_for_window) {
            // NOTE(simon): Windows 10 DPI awareness.
            window->dpi = win32_get_dpi_for_window(window->hwnd);
        } else if (win32_get_dpi_for_monitor) {
            // NOTE(simon): Windows 8.1 DPI awareness.
            HMONITOR monitor = MonitorFromWindow(window->hwnd, MONITOR_DEFAULTTONEAREST);
            UINT dpi_x = 0;
            UINT dpi_y = 0;
            win32_get_dpi_for_monitor(monitor, WIN32_MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
            window->dpi = dpi_x;
        } else {
            // NOTE(simon): No HiDPI awareness.
            window->dpi = USER_DEFAULT_SCREEN_DPI;
        }

        DragAcceptFiles(window->hwnd, true);
        window->hdc = GetDC(window->hwnd);
        ShowWindow(window->hwnd, SW_SHOW);
    } else {
        // TODO: Error
    }

    arena_end_temporary(scratch);
    Gfx_Window result = win32_handle_from_window(window);
    return result;
}

internal Void gfx_window_close(Gfx_Window handle) {
    Gfx_Win32State  *state  = &gfx_win32_state;
    Gfx_Win32Window *window = win32_window_from_handle(handle);

    if (window) {
        ReleaseDC(window->hwnd, window->hdc);
        DestroyWindow(window->hwnd);
        ++window->generation;
        dll_remove(state->first_window, state->last_window, window);
        sll_stack_push(state->window_freelist, window);
    }
}

internal V2U32 gfx_client_area_from_window(Gfx_Window handle) {
    Gfx_Win32Window *window = win32_window_from_handle(handle);

    RECT rect = { 0 };
    if (window) {
        GetClientRect(window->hwnd, &rect);
    }
    V2U32 result = v2u32(rect.right - rect.left, rect.bottom - rect.top);

    return result;
}

internal V2F32 gfx_mouse_position_from_window(Gfx_Window handle) {
    Gfx_Win32Window *window = win32_window_from_handle(handle);

    POINT point = { 0 };
    if (window) {
        GetCursorPos(&point);
        ScreenToClient(window->hwnd, &point);
    }
    V2F32 result = v2f32((F32) point.x, (F32) point.y);

    return result;
}

internal F32 gfx_dpi_from_window(Gfx_Window handle) {
    Gfx_Win32Window *window = win32_window_from_handle(handle);

    F32 dpi = 0;
    if (window) {
        dpi = window->dpi;
    }

    return dpi;
}

internal Void gfx_window_clear_custom_title_bar_data(Gfx_Window handle) {
}

internal Void gfx_window_set_custom_title_bar_height(Gfx_Window handle, F32 height) {
}

internal Void gfx_window_push_custom_title_bar_client_area(Gfx_Window handle, R2F32 rectangle) {
}

internal Void gfx_window_set_custom_border_width(Gfx_Window handle, F32 width) {
}

internal B32 gfx_window_has_os_title_bar(Gfx_Window handle) {
    B32 result = true;
    return result;
}

internal Void gfx_window_minimize(Gfx_Window handle) {
}

internal B32 gfx_window_is_maximized(Gfx_Window handle) {
    B32 result = false;
    return result;
}

internal Void gfx_window_set_maximized(Gfx_Window handle, B32 maximized) {
}



internal Void gfx_message(B32 error, Str8 title, Str8 message) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    CStr16 cstr16_title   = cstr16_from_str8(scratch.arena, title);
    CStr16 cstr16_message = cstr16_from_str8(scratch.arena, message);
    UINT   type           = MB_OK | (error ? MB_ICONERROR : MB_ICONINFORMATION);

    MessageBoxW(0, cstr16_message, cstr16_title, type);

    arena_end_temporary(scratch);
}



// NOTE(simon): Clipboard
internal Void gfx_set_clipboard_text(Str8 text) {
    Gfx_Win32State *state = &gfx_win32_state;
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    Str16 str16 = str16_from_str8(scratch.arena, text);

    if (OpenClipboard(0)) {
        EmptyClipboard();
        HANDLE global_memory = GlobalAlloc(GMEM_MOVEABLE, (str16.size + 1) * sizeof(U16));
        if (global_memory) {
            U16 *global_memory_ptr = GlobalLock(global_memory);
            memory_copy(global_memory_ptr, str16.data, str16.size * sizeof(U16));
            global_memory_ptr[str16.size] = 0;
            GlobalUnlock(global_memory_ptr);
            SetClipboardData(CF_UNICODETEXT, global_memory);
        }
        CloseClipboard();
    }

    arena_end_temporary(scratch);
}

internal Str8 gfx_get_clipboard_text(Arena *arena) {
    Str8 result = { 0 };

    if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(0)) {
        HANDLE global_memory = GetClipboardData(CF_UNICODETEXT);
        if (global_memory) {
            Void *global_memory_ptr = GlobalLock(global_memory);
            if (global_memory_ptr) {
                result = str8_from_str16(arena, str16_cstr16(global_memory_ptr));
                GlobalUnlock(global_memory);
            }
        }
        CloseClipboard();
    }

    return result;
}
