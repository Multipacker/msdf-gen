#ifndef X11_INCLUDE_H
#define X11_INCLUDE_H

#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>

#include <xkbcommon/xkbcommon.h>

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

    xcb_atom_t utf8_string_atom;
    xcb_atom_t clipboard_atom;
    xcb_atom_t clipboard_property_atom;
    xcb_atom_t incremental_atom;
    xcb_atom_t targets_atom;
    xcb_atom_t multiple_atom;
    xcb_atom_t timestamp_atom;
    xcb_atom_t wm_protocols_atom;
    xcb_atom_t wm_delete_window_atom;

    Str8 copy_text;
    Arena *copy_arena;

    xcb_window_t window;

    VoidFunction *update;
};

#endif // X11_INCLUDE_H
