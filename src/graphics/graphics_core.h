#ifndef GRAPHICS_CORE_H
#define GRAPHICS_CORE_H

typedef enum {
    Gfx_EventKind_Null,
    Gfx_EventKind_Quit,
    Gfx_EventKind_KeyPress,
    Gfx_EventKind_KeyRelease,
    Gfx_EventKind_MouseMove,
    Gfx_EventKind_Text,
    Gfx_EventKind_Scroll,
    Gfx_EventKind_Resize,
    Gfx_EventKind_Wakeup,
    Gfx_EventKind_FileDrop,

    Gfx_EventKind_COUNT,
} Gfx_EventKind;

typedef enum {
    Gfx_KeyModifier_Control = 1 << 0,
    Gfx_KeyModifier_Shift   = 1 << 1,
} Gfx_KeyModifier;

#define GFX_KEYS                          \
    X(Null,        "Null")                \
    X(A,           "A")                   \
    X(B,           "B")                   \
    X(C,           "C")                   \
    X(D,           "D")                   \
    X(E,           "E")                   \
    X(F,           "F")                   \
    X(G,           "G")                   \
    X(H,           "H")                   \
    X(I,           "I")                   \
    X(J,           "J")                   \
    X(K,           "K")                   \
    X(L,           "L")                   \
    X(M,           "M")                   \
    X(N,           "N")                   \
    X(O,           "O")                   \
    X(P,           "P")                   \
    X(Q,           "Q")                   \
    X(R,           "R")                   \
    X(S,           "S")                   \
    X(T,           "T")                   \
    X(U,           "U")                   \
    X(V,           "V")                   \
    X(W,           "W")                   \
    X(X,           "X")                   \
    X(Y,           "Y")                   \
    X(Z,           "Z")                   \
    X(0,           "0")                   \
    X(1,           "1")                   \
    X(2,           "2")                   \
    X(3,           "3")                   \
    X(4,           "4")                   \
    X(5,           "5")                   \
    X(6,           "6")                   \
    X(7,           "7")                   \
    X(8,           "8")                   \
    X(9,           "9")                   \
    X(F1,          "F1")                  \
    X(F2,          "F2")                  \
    X(F3,          "F3")                  \
    X(F4,          "F4")                  \
    X(F5,          "F5")                  \
    X(F6,          "F6")                  \
    X(F7,          "F7")                  \
    X(F8,          "F8")                  \
    X(F9,          "F9")                  \
    X(F10,         "F10")                 \
    X(F11,         "F11")                 \
    X(F12,         "F12")                 \
    X(Backspace,   "Backspace")           \
    X(Space,       "Space")               \
    X(Alt,         "Alt")                 \
    X(OS,          "Win")                 \
    X(Tab,         "Tab")                 \
    X(Return,      "Return")              \
    X(Shift,       "Shift")               \
    X(Control,     "Control")             \
    X(Escape,      "Escape")              \
    X(PageUp,      "Page Up")             \
    X(PageDown,    "Page Down")           \
    X(End,         "End")                 \
    X(Home,        "Home")                \
    X(Left,        "Left")                \
    X(Right,       "Right")               \
    X(Up,          "Up")                  \
    X(Down,        "Down")                \
    X(Delete,      "Delete")              \
    X(MouseLeft,   "Left Mouse Button")   \
    X(MouseRight,  "Right Mouse Button")  \
    X(MouseMiddle, "Middle Mouse Button") \

#define X(name, display_name) Gfx_Key_##name,
typedef enum {
    GFX_KEYS
    Gfx_Key_COUNT,
} Gfx_Key;
#undef X

#define X(name, display_name) str8_literal_compile(display_name),
global Str8 gfx_name_from_key[] = {
    GFX_KEYS
};
#undef X

typedef struct Gfx_Event Gfx_Event;
struct Gfx_Event {
    Gfx_Event *next;
    Gfx_Event *previous;

    Gfx_EventKind   kind;
    Gfx_Key         key;
    Gfx_KeyModifier key_modifiers;
    V2F32           scroll;
    V2F32           position;
    Str8            text;
    Str8            path;
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
internal Void          gfx_send_wakeup_event(Void);
internal Gfx_EventList gfx_get_events(Arena *arena, B32 wait);
internal V2F32         gfx_get_mouse_position(Void);
internal Void          gfx_swap_buffers(Void);
internal Void          gfx_set_cursor(Gfx_Cursor cursor);
internal Void          gfx_set_update_function(VoidFunction *update);
internal F32           gfx_dpi(Void);
internal Void          gfx_clear_custom_title_bar_data(Void);
internal Void          gfx_set_custom_title_bar_height(F32 height);
internal Void          gfx_push_cusomt_title_bar_client_area(R2F32 rectangle);
internal B32           gfx_has_os_title_bar(Void);
internal Void          gfx_minimize(Void);
internal B32           gfx_is_maximized(Void);
internal Void          gfx_set_maximized(B32 maximized);

// NOTE(simon): Clipboard
internal Void gfx_set_clipboard_text(Str8 text);
internal Str8 gfx_get_clipboard_text(Arena *arena);

#endif // GRAPHICS_CORE_H
