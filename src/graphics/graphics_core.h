#ifndef GRAPHICS_CORE_H
#define GRAPHICS_CORE_H

typedef enum {
    Gfx_EventKind_Null,
    Gfx_EventKind_Quit,
    Gfx_EventKind_KeyPress,
    Gfx_EventKind_KeyRelease,
    Gfx_EventKind_Text,
    Gfx_EventKind_Scroll,
    Gfx_EventKind_Resize,

    Gfx_EventKind_COUNT,
} Gfx_EventKind;

typedef enum {
    Gfx_KeyModifier_Control = 1 << 0,
    Gfx_KeyModifier_Shift   = 1 << 1,
} Gfx_KeyModifier;

typedef enum {
    Gfx_Key_Null,

    Gfx_Key_A,
    Gfx_Key_B,
    Gfx_Key_C,
    Gfx_Key_D,
    Gfx_Key_E,
    Gfx_Key_F,
    Gfx_Key_G,
    Gfx_Key_H,
    Gfx_Key_I,
    Gfx_Key_J,
    Gfx_Key_K,
    Gfx_Key_L,
    Gfx_Key_M,
    Gfx_Key_N,
    Gfx_Key_O,
    Gfx_Key_P,
    Gfx_Key_Q,
    Gfx_Key_R,
    Gfx_Key_S,
    Gfx_Key_T,
    Gfx_Key_U,
    Gfx_Key_V,
    Gfx_Key_W,
    Gfx_Key_X,
    Gfx_Key_Y,
    Gfx_Key_Z,

    Gfx_Key_0,
    Gfx_Key_1,
    Gfx_Key_2,
    Gfx_Key_3,
    Gfx_Key_4,
    Gfx_Key_5,
    Gfx_Key_6,
    Gfx_Key_7,
    Gfx_Key_8,
    Gfx_Key_9,

    Gfx_Key_F1,
    Gfx_Key_F2,
    Gfx_Key_F3,
    Gfx_Key_F4,
    Gfx_Key_F5,
    Gfx_Key_F6,
    Gfx_Key_F7,
    Gfx_Key_F8,
    Gfx_Key_F9,
    Gfx_Key_F10,
    Gfx_Key_F11,
    Gfx_Key_F12,

    Gfx_Key_Backspace,
    Gfx_Key_Space,
    Gfx_Key_Alt,
    Gfx_Key_OS,
    Gfx_Key_Tab,
    Gfx_Key_Return,
    Gfx_Key_Shift,
    Gfx_Key_Control,
    Gfx_Key_Escape,
    Gfx_Key_PageUp,
    Gfx_Key_PageDown,
    Gfx_Key_End,
    Gfx_Key_Home,
    Gfx_Key_Left,
    Gfx_Key_Right,
    Gfx_Key_Up,
    Gfx_Key_Down,
    Gfx_Key_Delete,
    Gfx_Key_MouseLeft,
    Gfx_Key_MouseRight,
    Gfx_Key_MouseMiddle,
} Gfx_Key;

typedef struct Gfx_Event Gfx_Event;
struct Gfx_Event {
    Gfx_Event *next;
    Gfx_Event *previous;

    Gfx_EventKind   kind;
    Gfx_Key         key;
    Gfx_KeyModifier key_modifiers;
    V2F32           scroll;
    V2F32           position;
};

typedef struct Gfx_EventList Gfx_EventList;
struct Gfx_EventList {
    Gfx_Event *first;
    Gfx_Event *last;
};

typedef enum {
    Gfx_Cursor_Pointer,
    Gfx_Cursor_Hand,
    Gfx_Cursor_Beam,
    Gfx_Cursor_SizeNWSE,
    Gfx_Cursor_SizeNESW,
    Gfx_Cursor_SizeWE,
    Gfx_Cursor_SizeNS,
    Gfx_Cursor_SizeAll,
    Gfx_Cursor_Disabled,
    Gfx_Cursor_COUNT,
} Gfx_Cursor;

internal Void          gfx_create(Str8 title, U32 width, U32 height);
internal V2U32         gfx_get_window_client_area(Void);
internal Gfx_EventList gfx_get_events(Arena *arena);
internal V2F32         gfx_get_mouse_position(Void);
internal Void          gfx_swap_buffers(Void);
internal Void          gfx_set_cursor(Gfx_Cursor cursor);

#endif // GRAPHICS_CORE_H
