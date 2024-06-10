#ifndef GRAPHICS_INCLUDE_H
#define  GRAPHICS_INCLUDE_H

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
    Gfx_Key_MouseLeftDouble,
    Gfx_Key_MouseRightDouble,
    Gfx_Key_MouseMiddleDouble,
} Gfx_Key;

typedef struct Gfx_Event Gfx_Event;
struct Gfx_Event {
    Gfx_Event *next;
    Gfx_Event *previous;

    Gfx_EventKind   kind;
    Gfx_Key         key;
    Gfx_KeyModifier key_modifiers;
    V2F32           scroll;
};

typedef struct Gfx_EventList Gfx_EventList;
struct Gfx_EventList {
    Gfx_Event *first;
    Gfx_Event *last;
};

#if OS_LINUX
#  include "sdl/sdl_include.h"
#elif OS_WINDOWS
#  include "win32/win32_include.h"
#  include "win32/win32_opengl.h"
#else
# error no backend for graphics_include.c on this operating system
#endif

internal Gfx_EventList gfx_get_events(Arena *arena, Gfx_Context *gfx);
internal V2F32 gfx_get_mouse_position(Gfx_Context *gfx);
internal Void gfx_swap_buffers(Gfx_Context *gfx);

#endif // GRAPHICS_INCLUDE_H
