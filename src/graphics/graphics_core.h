#ifndef GRAPHICS_CORE_H
#define GRAPHICS_CORE_H

typedef enum {
    UriFlag_HasAuthority = 1 << 0,
    UriFlag_HasQuery     = 1 << 1,
    UriFlag_HasFragment  = 1 << 2,
} UriFlags;

typedef struct Uri Uri;
struct Uri {
    UriFlags flags;

    Str8 scheme;
    Str8 authority;
    Str8 path;
    Str8 query;
    Str8 fragment;
};

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

typedef struct Gfx_Window Gfx_Window;
struct Gfx_Window {
    U64 u64[2];
};

typedef struct Gfx_Event Gfx_Event;
struct Gfx_Event {
    Gfx_Event *next;
    Gfx_Event *previous;

    Gfx_Window      window;
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
    Gfx_Cursor_SizeW,
    Gfx_Cursor_SizeN,
    Gfx_Cursor_SizeE,
    Gfx_Cursor_SizeS,
    Gfx_Cursor_SizeNW,
    Gfx_Cursor_SizeNE,
    Gfx_Cursor_SizeSE,
    Gfx_Cursor_SizeSW,
    Gfx_Cursor_SizeWE,
    Gfx_Cursor_SizeNS,
    Gfx_Cursor_SizeNWSE,
    Gfx_Cursor_SizeNESW,
    Gfx_Cursor_SizeAll,
    Gfx_Cursor_Disabled,
    Gfx_Cursor_COUNT,
} Gfx_Cursor;

internal Void gfx_init(Void);

// NOTE(simon): Events.
internal Void          gfx_send_wakeup_event(Void);
internal Gfx_EventList gfx_get_events(Arena *arena, B32 wait);
internal Void          gfx_set_update_function(VoidFunction *update);

internal Void gfx_set_cursor(Gfx_Cursor cursor);

// NOTE(simon): Windows.
internal B32        gfx_window_equal(Gfx_Window handle_a, Gfx_Window handle_b);
internal Gfx_Window gfx_window_create(Str8 title, U32 width, U32 height);
internal Void       gfx_window_close(Gfx_Window handle);
internal V2U32      gfx_client_area_from_window(Gfx_Window handle);
internal V2F32      gfx_mouse_position_from_window(Gfx_Window handle);
internal F32        gfx_dpi_from_window(Gfx_Window window);
internal Void       gfx_window_clear_custom_title_bar_data(Gfx_Window handle);
internal Void       gfx_window_set_custom_title_bar_height(Gfx_Window handle, F32 height);
internal Void       gfx_window_push_custom_title_bar_client_area(Gfx_Window handle, R2F32 rectangle);
internal Void       gfx_window_set_custom_border_width(Gfx_Window handle, F32 width);
internal B32        gfx_window_has_os_title_bar(Gfx_Window handle);
internal Void       gfx_window_minimize(Gfx_Window handle);
internal B32        gfx_window_is_maximized(Gfx_Window handle);
internal Void       gfx_window_set_maximized(Gfx_Window handle, B32 maximized);

internal Void gfx_message(B32 error, Str8 title, Str8 message);

// NOTE(simon): Clipboard
internal Void gfx_set_clipboard_text(Str8 text);
internal Str8 gfx_get_clipboard_text(Arena *arena);

internal Uri uri_from_string(Str8 string);

#endif // GRAPHICS_CORE_H
