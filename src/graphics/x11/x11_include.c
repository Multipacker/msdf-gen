#include <stdlib.h>

#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon-x11.h>

#include <linux/input-event-codes.h>

/*
 * This implementation was derived from a lot of different sources and
 * experimentation as the "official documentation" is outdated and
 * underspecified in some cases. This a mostly complete list of references
 * used:
 *
 * https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html
 * https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html
 * https://www.uninformativ.de/blog/postings/2017-04-02/0/POSTING-en.html
 * https://cscene.sourceforge.net/CS7/CS7-06.html
 * https://freedesktop.org/wiki/Specifications/
 */

global X11_State global_x11_state;

internal Void gfx_create(Str8 title, U32 width, U32 height) {
    X11_State *state = &global_x11_state;
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    state->event_arena = arena_create();
    state->copy_arena  = arena_create();

    int screen_number = 0;
    state->connection = xcb_connect(0, &screen_number);
    if (state->connection) {
        const xcb_setup_t *setup = xcb_get_setup(state->connection);

        xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
        for (int i = 0; i < screen_number; ++i) {
            xcb_screen_next(&iter);
        }

        state->screen = iter.data;

        xcb_cursor_context_new(state->connection, state->screen, &state->cursor_context);

        // NOTE(simon): Intern atoms.
        {
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

            // NOTE(simon): Send requests.
#define X(name, atom) xcb_intern_atom_cookie_t name##_cookie = xcb_intern_atom(state->connection, false, sizeof(atom) - 1, atom);
            X11_ATOMS
#undef X

            // NOTE(simon): Get atoms.
#define X(name, atom_name)                                                                              \
    xcb_intern_atom_reply_t *name##_reply = xcb_intern_atom_reply(state->connection, name##_cookie, 0); \
    if (name##_reply) {                                                                                 \
        state->name##_atom = name##_reply->atom;                                                        \
        free(name##_reply);                                                                             \
    }
            X11_ATOMS
#undef X
        }

        U32 value_mask = XCB_CW_EVENT_MASK;
        U32 value_list[] = {
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_PROPERTY_CHANGE,
        };

        state->window = xcb_generate_id(state->connection);
        xcb_create_window(
            state->connection,
            XCB_COPY_FROM_PARENT,
            state->window,
            state->screen->root,
            0, 0,
            (U16) width, (U16) height,
            1,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            state->screen->root_visual,
            value_mask, value_list
        );

        xcb_change_property(
            state->connection,
            XCB_PROP_MODE_REPLACE,
            state->window,
            XCB_ATOM_WM_NAME,
            state->utf8_string_atom,
            8,
            (U32) title.size,
            title.data
        );

        xcb_change_property(
            state->connection,
            XCB_PROP_MODE_REPLACE,
            state->window,
            XCB_ATOM_WM_ICON_NAME,
            state->utf8_string_atom,
            8,
            (U32) title.size,
            title.data
        );

        xcb_change_property(
            state->connection,
            XCB_PROP_MODE_REPLACE,
            state->window,
            state->wm_protocols_atom,
            XCB_ATOM_ATOM,
            32,
            1,
            &state->wm_delete_window_atom
        );

        xcb_map_window(state->connection, state->window);

        xcb_flush(state->connection);
    }

    xkb_x11_setup_xkb_extension(
        state->connection,
        1, 0,                        // NOTE(simon): Requested version
        XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
        0, 0,                        // NOTE(simon): Activve version
        &state->xkb_first_event, 0  // NOTE(simon): First event codes
    );

    state->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    state->xkb_core_keyboard_id = xkb_x11_get_core_keyboard_device_id(state->connection);

    state->xkb_keymap = xkb_x11_keymap_new_from_device(state->xkb_context, state->connection, state->xkb_core_keyboard_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
    state->xkb_state = xkb_x11_state_new_from_device(state->xkb_keymap, state->connection, state->xkb_core_keyboard_id);

    // NOTE(simon): I have no idea what this functions does or what the
    // parameters mean, it is taken straight from this example:
    // https://github.com/xkbcommon/libxkbcommon/blob/e7570bcb78a48c0e3fb48087991a85af95943e98/tools/interactive-x11.c#L236
    // TODO(simon): Maybe check the result of the request.
    xcb_xkb_select_events_details_t details = { 0 };
    details.affectNewKeyboard  = XCB_XKB_NKN_DETAIL_KEYCODES;
    details.newKeyboardDetails = XCB_XKB_NKN_DETAIL_KEYCODES;
    details.affectState        = XCB_XKB_STATE_PART_MODIFIER_BASE | XCB_XKB_STATE_PART_MODIFIER_LATCH | XCB_XKB_STATE_PART_MODIFIER_LOCK | XCB_XKB_STATE_PART_GROUP_BASE | XCB_XKB_STATE_PART_GROUP_LATCH | XCB_XKB_STATE_PART_GROUP_LOCK;
    details.stateDetails       = XCB_XKB_STATE_PART_MODIFIER_BASE | XCB_XKB_STATE_PART_MODIFIER_LATCH | XCB_XKB_STATE_PART_MODIFIER_LOCK | XCB_XKB_STATE_PART_GROUP_BASE | XCB_XKB_STATE_PART_GROUP_LATCH | XCB_XKB_STATE_PART_GROUP_LOCK;
    xcb_xkb_select_events_aux(
        state->connection,
        (U16) state->xkb_core_keyboard_id,
        XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY | XCB_XKB_EVENT_TYPE_MAP_NOTIFY | XCB_XKB_EVENT_TYPE_STATE_NOTIFY,
        0,
        0,
        XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS | XCB_XKB_MAP_PART_MODIFIER_MAP | XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS | XCB_XKB_MAP_PART_KEY_ACTIONS | XCB_XKB_MAP_PART_VIRTUAL_MODS | XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP,
        XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS | XCB_XKB_MAP_PART_MODIFIER_MAP | XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS | XCB_XKB_MAP_PART_KEY_ACTIONS | XCB_XKB_MAP_PART_VIRTUAL_MODS | XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP,
        &details
    );

    arena_end_temporary(scratch);
}

internal V2U32 gfx_get_window_client_area(Void) {
    X11_State *state = &global_x11_state;

    xcb_get_geometry_cookie_t cookie = xcb_get_geometry(state->connection, state->window);
    xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(state->connection, cookie, 0);

    V2U32 result = { 0 };
    if (reply) {
        result.x = reply->width;
        result.y = reply->height;
        free(reply);
    }

    return result;
}

// TODO(simon): Double check that this works.
internal Void gfx_send_wakeup_event(Void) {
    X11_State *state = &global_x11_state;
    xcb_client_message_event_t client_message = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 8,
        .window = state->window,
        .type = XCB_ATOM_NONE,
    };
    xcb_send_event(state->connection, false, state->window, 0, (const char *) &client_message);
    xcb_flush(state->connection);
}

internal Gfx_EventList gfx_get_events(Arena *arena, B32 wait) {
    X11_State *state = &global_x11_state;
    Gfx_EventList events = { 0 };

    // NOTE(simon): Collect events for this frame.
    xcb_generic_event_t *xcb_event = { 0 };
    if (!wait || state->first_event || (xcb_event = xcb_wait_for_event(state->connection))) {
        for (B32 first_wait = wait; first_wait || (xcb_event = xcb_poll_for_event(state->connection)); first_wait = false) {
            X11_EventNode *event_node = arena_push_struct_zero(state->event_arena, X11_EventNode);
            event_node->event = xcb_event;
            sll_queue_push(state->first_event, state->last_event, event_node);
        }
    }

    // NOTE(simon): Process events.
    for (X11_EventNode *event_node = state->first_event; event_node; event_node = event_node->next) {
        Gfx_Event *event = arena_push_struct_zero(arena, Gfx_Event);
        switch (event_node->event->response_type & ~0x80) {
            case XCB_EXPOSE: {
                xcb_expose_event_t *expose = (xcb_expose_event_t *) event_node->event;

                if (state->update) {
                    state->update();
                }
            } break;
            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE: {
                xcb_button_press_event_t *button = (xcb_button_press_event_t *) event_node->event;

                Gfx_EventKind button_action = (button->response_type == XCB_BUTTON_PRESS ? Gfx_EventKind_KeyPress : Gfx_EventKind_KeyRelease);

                switch (button->detail) {
                    case XCB_BUTTON_INDEX_1: {
                        event->kind = button_action;
                        event->key = Gfx_Key_MouseLeft;
                    } break;
                    case XCB_BUTTON_INDEX_2: {
                        event->kind = button_action;
                        event->key = Gfx_Key_MouseMiddle;
                    } break;
                    case XCB_BUTTON_INDEX_3: {
                        event->kind = button_action;
                        event->key = Gfx_Key_MouseRight;
                    } break;
                    case XCB_BUTTON_INDEX_4: {
                        if (button_action == Gfx_EventKind_KeyPress) {
                            event->kind = Gfx_EventKind_Scroll;
                            event->scroll = v2f32(0.0f, 1.0f);
                        }
                    } break;
                    case XCB_BUTTON_INDEX_5: {
                        if (button_action == Gfx_EventKind_KeyPress) {
                            event->kind = Gfx_EventKind_Scroll;
                            event->scroll = v2f32(0.0f, -1.0f);
                        }
                    } break;
                    // NOTE(simon): These are not documented and are not
                    // part of the headers, but through testing have been
                    // determined to be horizontal scrolling.
                    case 6: {
                        if (button_action == Gfx_EventKind_KeyPress) {
                            event->kind = Gfx_EventKind_Scroll;
                            event->scroll = v2f32(1.0f, 0.0f);
                        }
                    } break;
                    case 7: {
                        if (button_action == Gfx_EventKind_KeyPress) {
                            event->kind = Gfx_EventKind_Scroll;
                            event->scroll = v2f32(-1.0f, 0.0f);
                        }
                    } break;
                }

                event->position.x = (F32) button->event_x;
                event->position.y = (F32) button->event_y;
            } break;
            case XCB_KEY_PRESS: {
                xcb_key_press_event_t *key = (xcb_key_press_event_t *) event_node->event;

                int required_length = xkb_state_key_get_utf8(state->xkb_state, key->detail, 0, 0) + 1;
                CStr buffer = arena_push_array_zero(arena, char, (U64) required_length);
                int length = xkb_state_key_get_utf8(state->xkb_state, key->detail, buffer, (size_t) required_length);

                if (length) {
                    Gfx_Event *text_event = arena_push_struct_zero(arena, Gfx_Event);
                    text_event->kind = Gfx_EventKind_Text;
                    text_event->text = str8_cstr(buffer);
                    dll_push_back(events.first, events.last, text_event);
                }
            }
            // NOTE(simon): Fallthrough
            case XCB_KEY_RELEASE: {
                xcb_key_release_event_t *key = (xcb_key_release_event_t *) event_node->event;

                // NOTE(simon): Get keysyms without modifiers.
                xkb_keysym_t *keysyms = 0;
                int keysym_count = xkb_keymap_key_get_syms_by_level(state->xkb_keymap, key->detail, 0, 0, (const xkb_keysym_t **) &keysyms);

                for (int i = 0; i < keysym_count; ++i) {
                    os_console_print(str8_format(arena, "keysym: %u\n", keysyms[i]));

                    Gfx_Key event_key = Gfx_Key_Null;
                    switch (keysyms[i]) {
                        case XKB_KEY_BackSpace:  event_key = Gfx_Key_Backspace; break;
                        case XKB_KEY_Tab:        event_key = Gfx_Key_Tab;       break;
                        case XKB_KEY_Return:     event_key = Gfx_Key_Return;    break;
                        case XKB_KEY_Escape:     event_key = Gfx_Key_Escape;    break;
                        case XKB_KEY_Delete:     event_key = Gfx_Key_Delete;    break;
                        case XKB_KEY_F1:         event_key = Gfx_Key_F1;        break;
                        case XKB_KEY_F2:         event_key = Gfx_Key_F2;        break;
                        case XKB_KEY_F3:         event_key = Gfx_Key_F3;        break;
                        case XKB_KEY_F4:         event_key = Gfx_Key_F4;        break;
                        case XKB_KEY_F5:         event_key = Gfx_Key_F5;        break;
                        case XKB_KEY_F6:         event_key = Gfx_Key_F6;        break;
                        case XKB_KEY_F7:         event_key = Gfx_Key_F7;        break;
                        case XKB_KEY_F8:         event_key = Gfx_Key_F8;        break;
                        case XKB_KEY_F9:         event_key = Gfx_Key_F9;        break;
                        case XKB_KEY_F10:        event_key = Gfx_Key_F10;       break;
                        case XKB_KEY_F11:        event_key = Gfx_Key_F11;       break;
                        case XKB_KEY_F12:        event_key = Gfx_Key_F12;       break;
                        case XKB_KEY_Shift_L:    event_key = Gfx_Key_Shift;     break;
                        case XKB_KEY_Shift_R:    event_key = Gfx_Key_Shift;     break;
                        case XKB_KEY_Control_L:  event_key = Gfx_Key_Control;   break;
                        case XKB_KEY_Control_R:  event_key = Gfx_Key_Control;   break;
                        case XKB_KEY_Meta_L:     event_key = Gfx_Key_OS;        break;
                        case XKB_KEY_Meta_R:     event_key = Gfx_Key_OS;        break;
                        case XKB_KEY_Alt_L:      event_key = Gfx_Key_Alt;       break;
                        case XKB_KEY_Alt_R:      event_key = Gfx_Key_Alt;       break;
                        case XKB_KEY_space:      event_key = Gfx_Key_Space;     break;
                        case XKB_KEY_0:          event_key = Gfx_Key_0;         break;
                        case XKB_KEY_1:          event_key = Gfx_Key_1;         break;
                        case XKB_KEY_2:          event_key = Gfx_Key_2;         break;
                        case XKB_KEY_3:          event_key = Gfx_Key_3;         break;
                        case XKB_KEY_4:          event_key = Gfx_Key_4;         break;
                        case XKB_KEY_5:          event_key = Gfx_Key_5;         break;
                        case XKB_KEY_6:          event_key = Gfx_Key_6;         break;
                        case XKB_KEY_7:          event_key = Gfx_Key_7;         break;
                        case XKB_KEY_8:          event_key = Gfx_Key_8;         break;
                        case XKB_KEY_9:          event_key = Gfx_Key_9;         break;
                        case XKB_KEY_a:          event_key = Gfx_Key_A;         break;
                        case XKB_KEY_b:          event_key = Gfx_Key_B;         break;
                        case XKB_KEY_c:          event_key = Gfx_Key_C;         break;
                        case XKB_KEY_d:          event_key = Gfx_Key_D;         break;
                        case XKB_KEY_e:          event_key = Gfx_Key_E;         break;
                        case XKB_KEY_f:          event_key = Gfx_Key_F;         break;
                        case XKB_KEY_g:          event_key = Gfx_Key_G;         break;
                        case XKB_KEY_h:          event_key = Gfx_Key_H;         break;
                        case XKB_KEY_i:          event_key = Gfx_Key_I;         break;
                        case XKB_KEY_j:          event_key = Gfx_Key_J;         break;
                        case XKB_KEY_k:          event_key = Gfx_Key_K;         break;
                        case XKB_KEY_l:          event_key = Gfx_Key_L;         break;
                        case XKB_KEY_m:          event_key = Gfx_Key_M;         break;
                        case XKB_KEY_n:          event_key = Gfx_Key_N;         break;
                        case XKB_KEY_o:          event_key = Gfx_Key_O;         break;
                        case XKB_KEY_p:          event_key = Gfx_Key_P;         break;
                        case XKB_KEY_q:          event_key = Gfx_Key_Q;         break;
                        case XKB_KEY_r:          event_key = Gfx_Key_R;         break;
                        case XKB_KEY_s:          event_key = Gfx_Key_S;         break;
                        case XKB_KEY_t:          event_key = Gfx_Key_T;         break;
                        case XKB_KEY_u:          event_key = Gfx_Key_U;         break;
                        case XKB_KEY_v:          event_key = Gfx_Key_V;         break;
                        case XKB_KEY_w:          event_key = Gfx_Key_W;         break;
                        case XKB_KEY_x:          event_key = Gfx_Key_X;         break;
                        case XKB_KEY_y:          event_key = Gfx_Key_Y;         break;
                        case XKB_KEY_z:          event_key = Gfx_Key_Z;         break;
                        case XKB_KEY_Home:       event_key = Gfx_Key_Home;      break;
                        case XKB_KEY_Left:       event_key = Gfx_Key_Left;      break;
                        case XKB_KEY_Up:         event_key = Gfx_Key_Up;        break;
                        case XKB_KEY_Right:      event_key = Gfx_Key_Right;     break;
                        case XKB_KEY_Down:       event_key = Gfx_Key_Down;      break;
                        case XKB_KEY_Prior:      event_key = Gfx_Key_PageUp;    break;
                        case XKB_KEY_Next:       event_key = Gfx_Key_PageDown;  break;
                        case XKB_KEY_End:        event_key = Gfx_Key_End;       break;
                    }

                    if (event_key != Gfx_Key_Null) {
                        Gfx_Event *key_event = arena_push_struct_zero(arena, Gfx_Event);
                        if ((event_node->event->response_type & ~0x80) == XCB_KEY_PRESS) {
                            key_event->kind = Gfx_EventKind_KeyPress;
                        } else {
                            key_event->kind = Gfx_EventKind_KeyRelease;
                        }
                        key_event->key = event_key;
                        event->key_modifiers |= (key->state & XCB_MOD_MASK_SHIFT   ? Gfx_KeyModifier_Shift   : 0);
                        event->key_modifiers |= (key->state & XCB_MOD_MASK_CONTROL ? Gfx_KeyModifier_Control : 0);

                        dll_push_back(events.first, events.last, key_event);
                    }
                }
            } break;
            case XCB_CLIENT_MESSAGE: {
                xcb_client_message_event_t *client = (xcb_client_message_event_t *) event_node->event;

                if (client->type == state->wm_protocols_atom && client->format == 32 && client->data.data32[0] == state->wm_delete_window_atom) {
                    event->kind = Gfx_EventKind_Quit;
                }
            } break;
            case XCB_SELECTION_REQUEST: {
                xcb_selection_request_event_t *request = (xcb_selection_request_event_t *) event_node->event;

                xcb_atom_t property_atom = (request->property == XCB_ATOM_NONE ? request->target : request->property);
                xcb_selection_notify_event_t selection_notify = {
                    .response_type = XCB_SELECTION_NOTIFY,
                    .time          = request->time,
                    .requestor     = request->requestor,
                    .selection     = request->selection,
                    .target        = request->target,
                    .property      = XCB_ATOM_NONE,
                };

                if (
                    request->property != XCB_ATOM_NONE &&
                    request->selection == state->clipboard_atom &&
                    request->owner == state->window
                ) {
                    xcb_atom_t type_atom = XCB_ATOM_NONE;
                    U8 format = 0;
                    Str8 data = { 0 };

                    xcb_atom_t targets[] = {
                        state->targets_atom,
                        state->utf8_string_atom,
                    };

                    if (request->target == state->targets_atom) {
                        type_atom = XCB_ATOM_ATOM;
                        format    = 32;
                        data      = str8((U8 *) targets, array_count(targets));
                    } else if (request->property != XCB_ATOM_NONE && request->target == state->multiple_atom) {
                        // TODO(simon): Implement this when I have something that requires it. Hard to test otherwise
                        os_console_print(str8_literal("XCB: Selection requests with multiple targets is not implemented yet.\n"));
                        assert(false);
                    } else if (request->target == state->timestamp_atom) {
                        // TODO(simon): Implement this when I have a good way
                        // to acquire timestamps when we become the owner of
                        // the selection.
                        os_console_print(str8_literal("XCB: Selection requests with timestamp target is not implemented yet.\n"));
                        assert(false);
                    } else if (request->target == state->utf8_string_atom) {
                        type_atom = state->utf8_string_atom;
                        format    = 8;
                        data      = state->copy_text;
                    }

                    if (type_atom != XCB_ATOM_NONE) {
                        // TODO(simon): Support incremental copies.
                        xcb_void_cookie_t property_change_cookie = xcb_change_property_checked(
                            state->connection,
                            XCB_PROP_MODE_REPLACE,
                            request->requestor,
                            property_atom,
                            type_atom,
                            format,
                            (U32) data.size, data.data
                        );
                        xcb_generic_error_t *error = xcb_request_check(state->connection, property_change_cookie);
                        if (error) {
                            free(error);
                        } else {
                            selection_notify.property = property_atom;
                        }
                    }
                }

                xcb_send_event(state->connection, false, request->requestor, 0, (const char *) &selection_notify);
                xcb_flush(state->connection);
            } break;
            default: {
                if ((event_node->event->response_type & ~0x80) == state->xkb_first_event) {
                    // NOTE(simon): HOW does this type not exist in the standard headers???
                    typedef struct XKB_Event XKB_Event;
                    struct XKB_Event {
                        U8 repsone_type;
                        U8 type;
                        U16 sequence;
                        xcb_timestamp_t time;
                        U8 deviceID;
                    };
                    XKB_Event *xkb_event = (XKB_Event *) event_node->event;

                    // TODO(simon): Check the device id

                    switch (xkb_event->type) {
                        case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
                            xcb_xkb_new_keyboard_notify_event_t *keyboard_notify = (xcb_xkb_new_keyboard_notify_event_t *) xkb_event;
                            if (keyboard_notify->changed & XCB_XKB_NKN_DETAIL_KEYCODES) {
                                xkb_keymap_unref(state->xkb_keymap);
                                xkb_state_unref(state->xkb_state);
                                state->xkb_keymap = xkb_x11_keymap_new_from_device(state->xkb_context, state->connection, state->xkb_core_keyboard_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
                                state->xkb_state = xkb_x11_state_new_from_device(state->xkb_keymap, state->connection, state->xkb_core_keyboard_id);
                            }
                        } break;
                        case XCB_XKB_MAP_NOTIFY: {
                            xkb_keymap_unref(state->xkb_keymap);
                            xkb_state_unref(state->xkb_state);
                            state->xkb_keymap = xkb_x11_keymap_new_from_device(state->xkb_context, state->connection, state->xkb_core_keyboard_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
                            state->xkb_state = xkb_x11_state_new_from_device(state->xkb_keymap, state->connection, state->xkb_core_keyboard_id);
                        } break;
                        case XCB_XKB_STATE_NOTIFY: {
                            xcb_xkb_state_notify_event_t *state_notify = (xcb_xkb_state_notify_event_t *) xkb_event;
                            xkb_state_update_mask(
                                state->xkb_state,
                                state_notify->baseMods,
                                state_notify->latchedMods,
                                state_notify->lockedMods,
                                (xkb_layout_index_t) state_notify->baseGroup,
                                (xkb_layout_index_t) state_notify->latchedGroup,
                                (xkb_layout_index_t) state_notify->lockedGroup
                            );
                        } break;
                    }
                }
            } break;
        }

        if (event->kind != Gfx_EventKind_Null) {
            dll_push_back(events.first, events.last, event);
        }

        free(event_node->event);
    }

    state->first_event = 0;
    state->last_event = 0;
    arena_reset(state->event_arena);

    return events;
}

// TODO(simon): This doesn't follow the specification if the mouse is outside
// of the window, it returns last mouse position that was inside the window.
internal V2F32 gfx_get_mouse_position(Void) {
    X11_State *state = &global_x11_state;

    xcb_query_pointer_cookie_t cookie = xcb_query_pointer(state->connection, state->window);
    xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(state->connection, cookie, 0);

    V2F32 result = { 0 };
    if (reply) {
        if (reply->same_screen) {
            result.x = reply->win_x;
            result.y = reply->win_y;
        }

        free(reply);
    }

    return result;
}

internal Void gfx_swap_buffers(Void) {
    X11_State *state = &global_x11_state;
    // TODO(simon): Implement
}

internal Void gfx_set_cursor(Gfx_Cursor cursor) {
    X11_State *state = &global_x11_state;
    if (!state->cursor_context) {
        return;
    }

    xcb_cursor_t selected_cursor = XCB_CURSOR_NONE;

    // NOTE(simon): Note that we do use some names that are listed as up for
    // discussion, but seem to be implemented anyway.
    // https://freedesktop.org/wiki/Specifications/cursor-spec/
#define xcb_cursor_list(X)     \
    X(Pointer,  "default")     \
    X(Hand,     "pointer")     \
    X(Beam,     "text")        \
    X(SizeNWSE, "nwse-resize") \
    X(SizeNESW, "nesw-resize") \
    X(SizeWE,   "ew-resize")   \
    X(SizeNS,   "ns-resize")   \
    X(SizeAll,  "all-scroll")  \
    X(Disabled, "not-allowd")
#define xcb_load_cursor(gfx_kind, xcb_kind)                                       \
    case Gfx_Cursor_##gfx_kind: {                                                 \
        local xcb_cursor_t xcb_cursor = XCB_CURSOR_NONE;                          \
        if (xcb_cursor == XCB_CURSOR_NONE) {                                      \
            xcb_cursor = xcb_cursor_load_cursor(state->cursor_context, xcb_kind); \
        }                                                                         \
        selected_cursor = xcb_cursor;                                             \
    } break;

    switch (cursor) {
        xcb_cursor_list(xcb_load_cursor)
        case Gfx_Cursor_COUNT: break;
    }

    xcb_change_window_attributes(state->connection, state->window, XCB_CW_CURSOR, &selected_cursor);
    xcb_flush(state->connection);
}

internal Void gfx_set_update_function(VoidFunction *update) {
    X11_State *state = &global_x11_state;
    state->update = update;
}



// NOTE(simon): Clipboard
internal Void gfx_set_clipboard_text(Str8 text) {
    X11_State *state = &global_x11_state;

    arena_reset(state->copy_arena);
    state->copy_text = str8_copy(state->copy_arena, text);

    xcb_set_selection_owner(state->connection, state->window, state->clipboard_atom, XCB_CURRENT_TIME);
    xcb_flush(state->connection);
}

internal Str8 gfx_get_clipboard_text(Arena *arena) {
    X11_State *state = &global_x11_state;

    xcb_window_t owner = XCB_WINDOW_NONE;

    // NOTE(simon): Get the current owner of the clipboard selection.
    {
        xcb_get_selection_owner_cookie_t owner_cookie = xcb_get_selection_owner(state->connection, state->clipboard_atom);
        xcb_get_selection_owner_reply_t *owner_reply = xcb_get_selection_owner_reply(state->connection, owner_cookie, 0);

        if (owner_reply) {
            owner = owner_reply->owner;
            free(owner_reply);
        }
    }

    Str8 result = { 0 };

    if (owner == XCB_WINDOW_NONE) {
        // NOTE(simon): No one owns the clipboard selection, we cannot perform the copy.
    } else if (owner == state->window) {
        // NOTE(simon): We own the clipboard selection! Perform a cheap internal copy.
        result = str8_copy(arena, state->copy_text);
    } else {
        // NOTE(simon): Someone else owns the clipboard selection, and we are
        // incredibly sad :( There is no good solution to how you get selection
        // contents, other than waiting for "some amount of time", or doing
        // other dirty tricks.

        Arena_Temporary scratch = arena_get_scratch(&arena, 1);

        // NOTE(simon): Deleted so that the new contents can be put in the property.
        xcb_delete_property(state->connection, state->window, state->clipboard_property_atom);

        xcb_convert_selection(
            state->connection,
            state->window,
            state->clipboard_atom,
            state->utf8_string_atom,
            state->clipboard_property_atom,
            XCB_CURRENT_TIME
        );
        xcb_flush(state->connection);

        Str8List copy_parts = { 0 };
        B32 incremental = false;
        B32 done = false;

        // NOTE(simon): Poll events until we either complete the copy or have
        // gone more than a set amount of time between two copy events.
        for (U64 start_time = os_now_nanoseconds(); os_now_nanoseconds() - start_time < 1000000000 && !done;) {
            xcb_generic_event_t *xcb_event = xcb_poll_for_event(state->connection);
            if (!xcb_event) {
                continue;
            }

            B32 consumed = false;
            if ((xcb_event->response_type & ~0x80) == XCB_SELECTION_NOTIFY) {
                xcb_selection_notify_event_t *notify = (xcb_selection_notify_event_t *) xcb_event;

                xcb_get_property_reply_t *reply = 0;
                if (notify->property == state->clipboard_property_atom) {
                    consumed = true;

                    // NOTE(simon): We got interrupted by another copy. We can
                    // either abort the current one, or switch to the new one.
                    // Here we switch to the new copy.
                    if (incremental) {
                        arena_end_temporary(scratch);
                        scratch = arena_get_scratch(&arena, 1);

                        incremental = false;
                        memory_zero_struct(&copy_parts);
                    }

                    xcb_get_property_cookie_t cookie = xcb_get_property(
                        state->connection,
                        false,
                        state->window,
                        state->clipboard_property_atom,
                        state->utf8_string_atom,
                        0,
                        U32_MAX
                    );
                    reply = xcb_get_property_reply(state->connection, cookie, 0);
                }

                if (reply) {
                    if (reply->type == state->incremental_atom) {
                        incremental = true;
                    } else if (reply->type == state->utf8_string_atom) {
                        Str8 part = str8(xcb_get_property_value(reply), (U64) xcb_get_property_value_length(reply));
                        Str8 part_copy = str8_copy(scratch.arena, part);
                        str8_list_push(scratch.arena, &copy_parts, part_copy);
                        done = true;
                    } else {
                        done = true;
                    }
                    
                    xcb_delete_property(state->connection, state->window, state->clipboard_property_atom);
                    xcb_flush(state->connection);
                    free(reply);
                }
            } else if ((xcb_event->response_type & ~0x80) == XCB_PROPERTY_NOTIFY) {
                xcb_property_notify_event_t *notify = (xcb_property_notify_event_t *) xcb_event;

                xcb_get_property_reply_t *reply = 0;
                if (incremental && notify->atom == state->clipboard_property_atom && notify->state == XCB_PROPERTY_NEW_VALUE) {
                    consumed = true;

                    xcb_get_property_cookie_t cookie = xcb_get_property(
                        state->connection,
                        true,
                        state->window,
                        state->clipboard_property_atom,
                        state->utf8_string_atom,
                        0,
                        U32_MAX
                    );
                    reply = xcb_get_property_reply(state->connection, cookie, 0);
                }

                if (reply) {
                    if (reply->type == state->utf8_string_atom) {
                        Str8 part = str8(xcb_get_property_value(reply), (U64) xcb_get_property_value_length(reply));
                        Str8 part_copy = str8_copy(scratch.arena, part);
                        str8_list_push(scratch.arena, &copy_parts, part_copy);
                        done = part_copy.size == 0;
                    } else {
                        done = true;
                    }
                    
                    free(reply);
                }
            }

            if (consumed) {
                start_time = os_now_nanoseconds();
                free(xcb_event);
            } else {
                // NOTE(simon): Save the event for the next time someone calls
                // `gfx_get_events`.
                X11_EventNode *event_node = arena_push_struct_zero(state->event_arena, X11_EventNode);
                event_node->event = xcb_event;
                sll_queue_push(state->first_event, state->last_event, event_node);
            }
        }

        result = str8_join(arena, &copy_parts);
        arena_end_temporary(scratch);
    }

    return result;
}
