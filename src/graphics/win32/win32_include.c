global Arena        *win32_event_arena;
global Gfx_EventList win32_event_list;

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
            } break;
            case WM_MOUSEWHEEL: {
                event->kind = Gfx_EventKind_Scroll;
                event->scroll.y = (F32) (GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA);
            } break;
            case WM_LBUTTONDBLCLK: {
                event->kind = Gfx_EventKind_KeyPress;
                event->key = Gfx_Key_MouseLeftDouble;
                event->key_modifiers |= (GetAsyncKeyState(VK_SHIFT)   & 0x8000) ? Gfx_KeyModifier_Shift   : 0;
                event->key_modifiers |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? Gfx_KeyModifier_Control : 0;
            } break;
            case WM_MBUTTONDBLCLK: {
                event->kind = Gfx_EventKind_KeyPress;
                event->key = Gfx_Key_MouseMiddleDouble;
                event->key_modifiers |= (GetAsyncKeyState(VK_SHIFT)   & 0x8000) ? Gfx_KeyModifier_Shift   : 0;
                event->key_modifiers |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? Gfx_KeyModifier_Control : 0;
            } break;
            case WM_RBUTTONDBLCLK: {
                event->kind = Gfx_EventKind_KeyPress;
                event->key = Gfx_Key_MouseRightDouble;
                event->key_modifiers |= (GetAsyncKeyState(VK_SHIFT)   & 0x8000) ? Gfx_KeyModifier_Shift   : 0;
                event->key_modifiers |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? Gfx_KeyModifier_Control : 0;
            } break;
            case WM_LBUTTONUP: case WM_LBUTTONDOWN: {
                event->kind = message == WM_LBUTTONUP ? Gfx_EventKind_KeyRelease : Gfx_EventKind_KeyPress;
                event->key = Gfx_Key_MouseLeft;
                event->key_modifiers |= (GetAsyncKeyState(VK_SHIFT)   & 0x8000) ? Gfx_KeyModifier_Shift   : 0;
                event->key_modifiers |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? Gfx_KeyModifier_Control : 0;
            } break;
            case WM_MBUTTONUP: case WM_MBUTTONDOWN: {
                event->kind = message == WM_MBUTTONUP ? Gfx_EventKind_KeyRelease : Gfx_EventKind_KeyPress;
                event->key = Gfx_Key_MouseMiddle;
                event->key_modifiers |= (GetAsyncKeyState(VK_SHIFT)   & 0x8000) ? Gfx_KeyModifier_Shift   : 0;
                event->key_modifiers |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? Gfx_KeyModifier_Control : 0;
            } break;
            case WM_RBUTTONUP: case WM_RBUTTONDOWN: {
                event->kind = message == WM_RBUTTONUP ? Gfx_EventKind_KeyRelease : Gfx_EventKind_KeyPress;
                event->key = Gfx_Key_MouseRight;
                event->key_modifiers |= (GetAsyncKeyState(VK_SHIFT)   & 0x8000) ? Gfx_KeyModifier_Shift   : 0;
                event->key_modifiers |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? Gfx_KeyModifier_Control : 0;
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

internal Gfx_Context *gfx_create(Arena *arena, Str8 title, U32 width, U32 height) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    Gfx_Context *result = arena_push_struct_zero(arena, Gfx_Context);

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
        result->hwnd = CreateWindow(
            window_class.lpszClassName, cstr16_title,
            WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            0, 0, instance, 0
        );

        if (result->hwnd) {
            result->hdc = GetDC(result->hwnd);
            ShowWindow(result->hwnd, SW_SHOW);
            win32_init_opengl(result);
        } else {
            // TODO: Error
        }
    } else {
        // TODO: Error
    }

    arena_end_temporary(scratch);
    return result;
}

internal Gfx_EventList gfx_get_events(Arena *arena, Gfx_Context *gfx) {
    win32_event_arena = arena;
    win32_event_list.first = 0;
    win32_event_list.last  = 0;

    for (MSG message; PeekMessage(&message, 0, 0, 0, PM_REMOVE);) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return win32_event_list;
}

internal V2F32 gfx_get_mouse_position(Gfx_Context *gfx) {
    POINT point = { 0 };
    GetCursorPos(&point);
    V2F32 result = v2f32((F32) point.x, (F32) point.y);
    return result;
}

internal V2U32 gfx_get_window_client_area(Gfx_Context *gfx) {
    RECT rect = { 0 };
    GetClientRect(gfx->hwnd, &rect);
    V2U32 result = v2u32(rect.right - rect.left, rect.bottom - rect.top);
    return result;
}
