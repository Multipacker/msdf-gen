#ifndef X11_INCLUDE_H
#define X11_INCLUDE_H

#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>

#include <xkbcommon/xkbcommon.h>

// NOTE(simon): Structures required for ICCCM.
// https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html
typedef enum {
    X11_IcccmWmSizeHint_UserSpecifiedPosition    = 1,
    X11_IcccmWmSizeHint_UserSpecifiedSize        = 2,
    X11_IcccmWmSizeHint_ProgramSpecifiedPosition = 4,
    X11_IcccmWmSizeHint_ProgramSpecifiedSize     = 8,
    X11_IcccmWmSizeHint_MinSize                  = 16,
    X11_IcccmWmSizeHint_MaxSize                  = 32,
    X11_IcccmWmSizeHint_ResizeIncrement          = 64,
    X11_IcccmWmSizeHint_Aspect                   = 128,
    X11_IcccmWmSizeHint_BaseSize                 = 256,
    X11_IcccmWmSizeHint_WindowGravity            = 512,
} X11_IcccmWmSizeHintsFlags;

typedef struct X11_IcccmWmSizeHints X11_IcccmWmSizeHints;
struct X11_IcccmWmSizeHints {
    U32 flags;
    U32 padding[4];
    S32 min_width;
    S32 min_height;
    S32 max_width;
    S32 max_height;
    S32 width_increment;
    S32 height_increment;
    S32 min_aspect_numerator;
    S32 min_aspect_denominator;
    S32 max_aspect_numerator;
    S32 max_aspect_denominator;
    S32 base_width;
    S32 base_height;
    S32 window_gravity;
};

typedef enum {
    X11_IcccmWmHint_Input        = 1,
    X11_IcccmWmHint_State        = 2,
    X11_IcccmWmHint_IconPixmap   = 4,
    X11_IcccmWmHint_IconWindow   = 8,
    X11_IcccmWmHint_IconPosition = 16,
    X11_IcccmWmHint_IconMask     = 32,
    X11_IcccmWmHint_WindowGroup  = 64,
    //X11_IcccmWmHint_Messagehint = 128, // NOTE(simon): Obsolete
    X11_IcccmWmHint_UrgencyHint  = 256,
} X11_IcccmWmHintsFlags;

typedef struct X11_IcccmWmHints X11_IcccmWmHints;
struct X11_IcccmWmHints {
    U32 flags;
    U32 input;
    U32 initial_state;
    xcb_pixmap_t icon_pixmap;
    xcb_window_t icon_window;
    S32 icon_x;
    S32 icon_y;
    xcb_pixmap_t icon_mask;
    xcb_window_t window_group;
};

typedef enum {
    X11_IcccmWmState_Withdrawn = 0,
    X11_IcccmWmState_Normal    = 1,
    X11_IcccmWmState_Iconic    = 3,
} X11_IcccmWmState;



// NOTE(simon): Atoms that we need to intern ourselves.
#define X11_ATOMS                               \
    X(utf8_string,        "UTF8_STRING")        \
    X(clipboard,          "CLIPBOARD")          \
    X(clipboard_property, "CLIPBOARD_PROPERTY") \
    X(incremental,        "INCR")               \
    X(targets,            "TARGETS")            \
    X(multiple,           "MULTIPLE")           \
    X(timestamp,          "TIMESTAMP")          \
    X(wm_protocols,       "WM_PROTOCOLS")       \
    X(wm_delete_window,   "WM_DELETE_WINDOW")

typedef struct X11_EventNode X11_EventNode;
struct X11_EventNode {
    X11_EventNode *next;
    xcb_generic_event_t *event;
};

typedef struct X11_State X11_State;
struct X11_State {
    xcb_connection_t     *connection;
    xcb_screen_t         *screen;
    xcb_cursor_context_t *cursor_context;

    struct xkb_context   *xkb_context;
    struct xkb_keymap    *xkb_keymap;
    struct xkb_state     *xkb_state;
    U8                    xkb_first_event;
    S32                   xkb_core_keyboard_id;

    Arena *event_arena;
    X11_EventNode *first_event;
    X11_EventNode *last_event;

#define X(name, atom_name) xcb_atom_t name##_atom;
    X11_ATOMS
#undef X

    Str8 copy_text;
    Arena *copy_arena;

    xcb_window_t window;

    VoidFunction *update;
};

#endif // X11_INCLUDE_H
