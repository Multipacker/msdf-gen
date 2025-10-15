#include <stdlib.h>

#include <xcb/xkb.h>

#include <xkbcommon/xkbcommon-x11.h>

/*
 * This implementation was derived from a lot of different sources and
 * experimentation as the "official documentation" is outdated and
 * underspecified in some cases. This a mostly complete list of references
 * used:
 *
 * Core specification of protocols:
 *   https://www.x.org/releases/X11R7.7/doc/index.html
 * Specification of other standards used for X:
 *   https://freedesktop.org/wiki/Specifications/
 * Latest version of EWMH as the above two links only take you to older
 * versions of the specification:
 *   https://specifications.freedesktop.org/wm-spec/1.5/
 * Other references:
 *   https://www.uninformativ.de/blog/postings/2017-04-02/0/POSTING-en.html
 *   https://cscene.sourceforge.net/CS7/CS7-06.html
 *   https://www.systutorials.com/docs/linux/man/3-XrmGetStringDatabase/
 */

/* TODO(simon):
 * * We might want to support session management from ICCCM.
 * * Verify that we follow the ICCCM after major changes to this
 *   implementation, even though I think we do.
 * * How do we want key repeat to be exposed? Both key press and release
 *   events, or just press events?
 * * Do we care about ICE client rendezvous from ICCCM?
 * * Do we care about device color characterization from ICCCM?
 * * Implement Extended Window Manager Hints (EWMH).
 * * We sometimes crash the XWayland server, I don't really know why or how
 *   that happens. We really shouldn't though, fix it!
 * * XKB compose is missing, implement it!
 */

global X11_State global_x11_state;



internal Gfx_Window x11_handle_from_window(X11_Window *window) {
    Gfx_Window handle = { 0 };
    handle.u64[0] = integer_from_pointer(window);
    handle.u64[1] = window->generation;
    return handle;
}

internal X11_Window *x11_window_from_handle(Gfx_Window handle) {
    X11_Window *window = (X11_Window *) pointer_from_integer(handle.u64[0]);
    if (!window || window->generation != handle.u64[1]) {
        window = 0;
    }
    return window;
}

internal X11_Window *x11_window_from_id(xcb_window_t id) {
    X11_State *state = &global_x11_state;

    X11_Window *result = 0;
    for (X11_Window *window = state->first_window; window; window = window->next) {
        if (window->window == id) {
            result = window;
            break;
        }
    }

    return result;
}



internal Void x11_update_cursor(Void) {
    X11_State *state = &global_x11_state;

    if (!state->cursor_context) {
        return;
    }

    if (!state->pointer_window) {
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
    X(SizeW,    "w-resize")    \
    X(SizeN,    "n-resize")    \
    X(SizeE,    "e-resize")    \
    X(SizeS,    "s-resize")    \
    X(SizeNW,   "nw-resize")   \
    X(SizeNE,   "ne-resize")   \
    X(SizeSE,   "se-resize")   \
    X(SizeSW,   "sw-resize")   \
    X(SizeWE,   "ew-resize")   \
    X(SizeNS,   "ns-resize")   \
    X(SizeNWSE, "nwse-resize") \
    X(SizeNESW, "nesw-resize") \
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

    switch (state->cursor) {
        xcb_cursor_list(xcb_load_cursor)
        case Gfx_Cursor_COUNT: break;
    }

    xcb_change_window_attributes(state->connection, state->pointer_window->window, XCB_CW_CURSOR, &selected_cursor);
    xcb_flush(state->connection);
}

internal xcb_get_property_reply_t *x11_get_property(xcb_window_t window, xcb_atom_t property, xcb_atom_t type) {
    X11_State *state = &global_x11_state;
    xcb_get_property_cookie_t cookie = xcb_get_property(
        state->connection,
        false,
        window,
        property,
        type,
        0,
        U32_MAX
    );
    xcb_get_property_reply_t *reply = xcb_get_property_reply(state->connection, cookie, 0);
    return reply;
}

internal Void x11_read_dpi(Void) {
    X11_State *state = &global_x11_state;
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    // NOTE(simon): Default to 96 DPI.
    state->dpi = 96.0f;

    // NOTE(simon): Read RESOURCE_MANAGER property from root window.
    Str8 resource_string = { 0 };
    xcb_get_property_reply_t *resource_manager_reply = x11_get_property(state->screen->root, XCB_ATOM_RESOURCE_MANAGER, XCB_ATOM_STRING);
    if (resource_manager_reply) {
        Str8 raw_data = str8(xcb_get_property_value(resource_manager_reply), (U64) xcb_get_property_value_length(resource_manager_reply));
        resource_string = str8_copy(scratch.arena, raw_data);
        free(resource_manager_reply);
    }

    // NOTE(simon): Parse resource lines, use the last occurrence of each resource name.
    for (Str8Node *line = str8_split_by_codepoints(scratch.arena, resource_string, str8_literal("\n")).first; line; line = line->next) {
        U8 *ptr = line->string.data;
        U8 *opl = line->string.data + line->string.size;

        // NOTE(simon): Skip blank lines.
        if (opl - ptr == 0) {
            continue;
        }

        // NOTE(simon): Skip comments and includes.
        if (*ptr == '!' || *ptr == '#') {
            continue;
        }

        // NOTE(simon): Skip whitespace.
        while (ptr < opl && (*ptr == ' ' || *ptr == '\t')) {
            ++ptr;
        }

        // NOTE(simon): Skip if the resource name doesn't match.
        Str8 resource = str8_literal("Xft.dpi");
        if (str8_equal(resource, str8_prefix(str8_range(ptr, opl), resource.size))) {
            ptr += resource.size;
        } else {
            continue;
        }

        // NOTE(simon): Skip whitespace.
        while (ptr < opl && (*ptr == ' ' || *ptr == '\t')) {
            ++ptr;
        }

        // NOTE(simon): If we don't find a ':', the name probably didn't match, skip.
        if (ptr < opl && *ptr == ':') {
            ++ptr;
        } else {
            break;
        }

        // NOTE(simon): Skip whitespace.
        while (ptr < opl && (*ptr == ' ' || *ptr == '\t')) {
            ++ptr;
        }

        // NOTE(simon): Parse the value.
        Str8 value = str8_range(ptr, opl);
        CStr value_cstr = cstr_from_str8(scratch.arena, value);
        state->dpi = (F32) atof(value_cstr);
    }

    arena_end_temporary(scratch);
}



internal Void gfx_init(Void) {
    X11_State *state = &global_x11_state;
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    state->permanent_arena = arena_create();
    state->event_arena     = arena_create();
    state->copy_arena      = arena_create();

    state->connection = xcb_connect(0, &state->screen_index);
    if (!state->connection) {
        gfx_message(true, str8_literal("Failed to initialize X11"), str8_literal("Could not connect to the X server."));
        os_exit(1);
    }

    const xcb_setup_t *setup = xcb_get_setup(state->connection);

    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < state->screen_index; ++i) {
        xcb_screen_next(&iter);
    }

    state->screen = iter.data;

    // NOTE(simon): Create clipboard window
    {
        U32 value_mask = XCB_CW_EVENT_MASK;
        U32 value_list[] = {
            XCB_EVENT_MASK_PROPERTY_CHANGE,
        };
        state->clipboard_window = xcb_generate_id(state->connection);
        xcb_create_window(
            state->connection,
            0,
            state->clipboard_window,
            state->screen->root,
            0, 0,
            1, 1,
            0,
            XCB_WINDOW_CLASS_INPUT_ONLY,
            state->screen->root_visual,
            value_mask, value_list
        );
    }

    xcb_cursor_context_new(state->connection, state->screen, &state->cursor_context);

    // NOTE(simon): Intern atoms.
    {
        // NOTE(simon): Send requests.
#define X(name, atom) xcb_intern_atom_cookie_t name##_cookie = xcb_intern_atom(state->connection, false, sizeof(atom) - 1, atom);
        X11_ATOMS
#undef X

        // NOTE(simon): Get atoms.
#define X(name, atom_name)                                                                          \
xcb_intern_atom_reply_t *name##_reply = xcb_intern_atom_reply(state->connection, name##_cookie, 0); \
if (name##_reply) {                                                                                 \
    state->name##_atom = name##_reply->atom;                                                        \
    free(name##_reply);                                                                             \
}
        X11_ATOMS
#undef X
    }

    x11_read_dpi();
    // NOTE(simon): Register for property updates on the root window. This will
    // allow us to response immediately to DPI settings.
    {
        U32 root_event = XCB_EVENT_MASK_PROPERTY_CHANGE;
        xcb_change_window_attributes(state->connection, state->screen->root, XCB_CW_EVENT_MASK, &root_event);
    }

    // NOTE(simon): Setup XSync
    xcb_sync_initialize_cookie_t sync_cookie = xcb_sync_initialize(state->connection, 3, 1);
    xcb_sync_initialize_reply_t *sync_reply = xcb_sync_initialize_reply(state->connection, sync_cookie, 0);
    if (!sync_reply) {
        gfx_message(true, str8_literal("Failed to initialize X11"), str8_literal("Could not initialize Xsync extension."));
        os_exit(1);
    }
    if (sync_reply->major_version != 3 && sync_reply->minor_version != 1) {
        gfx_message(true, str8_literal("Failed to initialize X11"), str8_literal("Incompatible version of Xsync extension."));
        os_exit(1);
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



// NOTE(simon): Events.
// TODO(simon): We might want to use a custom atom for this.
internal Void gfx_send_wakeup_event(Void) {
    X11_State *state = &global_x11_state;
    xcb_client_message_event_t client_message = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 8,
        .window = state->clipboard_window,
        .type = XCB_ATOM_NONE,
    };
    xcb_send_event(state->connection, false, state->clipboard_window, 0, (const char *) &client_message);
    xcb_flush(state->connection);
}

internal Gfx_EventList gfx_get_events(Arena *arena, B32 wait) {
    X11_State *state = &global_x11_state;
    Gfx_EventList events = { 0 };

    // NOTE(simon): Collect events for this frame.
    xcb_generic_event_t *xcb_event = 0;
    if (!wait || state->first_event || (xcb_event = xcb_wait_for_event(state->connection))) {
        for (B32 first_wait = wait && !state->first_event; first_wait || (xcb_event = xcb_poll_for_event(state->connection)); first_wait = false) {
            X11_EventNode *event_node = arena_push_struct(state->event_arena, X11_EventNode);
            event_node->event = xcb_event;
            sll_queue_push(state->first_event, state->last_event, event_node);
        }
    }

    // NOTE(simon): Process events.
    for (X11_EventNode *event_node = state->first_event; event_node; event_node = event_node->next) {
        U8 response_type = event_node->event->response_type & ~0x80;
        switch (response_type) {
            case XCB_LEAVE_NOTIFY: {
                state->pointer_window = 0;
            } break;
            case XCB_ENTER_NOTIFY: {
                xcb_enter_notify_event_t *enter = (xcb_enter_notify_event_t *) event_node->event;
                state->pointer_window = x11_window_from_id(enter->event);
                x11_update_cursor();
            } break;
            case XCB_PROPERTY_NOTIFY: {
                xcb_property_notify_event_t *notify = (xcb_property_notify_event_t *) event_node->event;

                if (notify->window == state->screen->root && notify->atom == XCB_ATOM_RESOURCE_MANAGER) {
                    x11_read_dpi();
                }
            } break;
            case XCB_CONFIGURE_NOTIFY: {
                xcb_configure_notify_event_t *notify = (xcb_configure_notify_event_t *) event_node->event;
                X11_Window *window = x11_window_from_id(notify->window);
                window->width  = notify->width;
                window->height = notify->height;
            } break;
            case XCB_EXPOSE: {
                if (state->update) {
                    state->update();
                }

                for (X11_Window *window = state->first_window; window; window = window->next) {
                    xcb_sync_set_counter(state->connection, window->counter, window->counter_value);
                }
                xcb_flush(state->connection);
            } break;
            case XCB_SELECTION_NOTIFY: {
                Arena_Temporary scratch = arena_get_scratch(&arena, 1);
                xcb_selection_notify_event_t *notify = (xcb_selection_notify_event_t *) event_node->event;
                X11_Window *window = x11_window_from_id(notify->requestor);

                if (notify->property == state->x_dnd_selection_atom && state->drag_and_drop_target == notify->requestor) {
                    Str8 data = { 0 };

                    // NOTE(simon): Get drag-and-drop data.
                     // TODO(simon): Support for incremental copies using INCR. Probably try to reuse the code for clipboard handling.
                    xcb_get_property_reply_t *reply = x11_get_property(window->window, state->x_dnd_selection_atom, state->drag_and_drop_type);
                    if (reply) {
                        Str8 raw_data = str8((U8 *) xcb_get_property_value(reply), (U64) xcb_get_property_value_length(reply));
                        data = str8_copy(scratch.arena, raw_data);
                        free(reply);
                    }

                    // NOTE(simon): Implementation of https://freedesktop.org/wiki/Specifications/file-uri-spec/
                    // NOTE(simon): Iterate through lines.
                    // TODO(simon): We might want to handle file URIs of the form `file:/<path>`
                    for (U64 index = 0; index < data.size;) {
                        U64 next_index = str8_find(index, str8_literal("\r\n"), data);
                        Str8 line = str8_substring(data, index, next_index - index);
                        index = next_index + 2;

                        Uri uri = uri_from_string(line);

                        // NOTE(simon): We only accept files from localhost.
                        if (str8_equal(uri.scheme, str8_literal("file")) && (uri.authority.size == 0 || str8_equal(uri.authority, str8_literal("localhost")))) {
                            Gfx_Event *event = arena_push_struct(state->event_arena, Gfx_Event);
                            event->kind     = Gfx_EventKind_FileDrop;
                            event->position = state->drag_and_drop_position;
                            event->path     = str8_copy(state->event_arena, uri.path);
                            event->window   = x11_handle_from_window(window);
                            dll_push_back(events.first, events.last, event);
                        } else {
                            gfx_message(
                                false,
                                str8_literal("Could not receive drag-and-dropped URI"),
                                str8_format(scratch.arena, "Unsupported URI %.*s", str8_expand(line))
                            );
                        }
                    }

                    // NOTE(simon): Notify the source that we are done with the drag-and-drop data.
                    xcb_client_message_event_t finished_message = {
                        .response_type  = XCB_CLIENT_MESSAGE,
                        .format         = 32,
                        .window         = state->drag_and_drop_source,
                        .type           = state->x_dnd_finished_atom,
                        .data.data32[0] = window->window,
                        .data.data32[1] = data.size ? 0x01 : 0,
                        .data.data32[2] = state->x_dnd_action_copy_atom,
                    };
                    xcb_send_event(state->connection, false, state->drag_and_drop_source, 0, (const char *) &finished_message);
                    xcb_flush(state->connection);

                    // NOTE(simon): Reset drag-and-drop state.
                    state->drag_and_drop_source   = XCB_WINDOW_NONE;
                    state->drag_and_drop_target   = XCB_WINDOW_NONE;
                    state->drag_and_drop_version  = 0;
                    state->drag_and_drop_type     = XCB_ATOM_NONE;
                    state->drag_and_drop_position = v2f32(0.0f, 0.0f);
                }

                arena_end_temporary(scratch);
            } break;
            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE: {
                xcb_button_press_event_t *button = (xcb_button_press_event_t *) event_node->event;
                X11_Window *window = x11_window_from_id(button->event);

                Gfx_EventKind button_action = (response_type == XCB_BUTTON_PRESS ? Gfx_EventKind_KeyPress : Gfx_EventKind_KeyRelease);
                Gfx_EventKind scroll_action = (response_type == XCB_BUTTON_PRESS ? Gfx_EventKind_Scroll   : Gfx_EventKind_Null);

                typedef struct MouseButton MouseButton;
                struct MouseButton {
                    Gfx_EventKind kind;
                    Gfx_Key       key;
                    V2F32         scroll;
                };
                // NOTE(simon): Horizontal scrolling is not documented but was
                // determined through testing.
                MouseButton buttons[] = {
                    { Gfx_EventKind_Null, Gfx_Key_Null,        v2f32( 0.0f,  0.0f), },
                    { button_action,      Gfx_Key_MouseLeft,   v2f32( 0.0f,  0.0f), },
                    { button_action,      Gfx_Key_MouseMiddle, v2f32( 0.0f,  0.0f), },
                    { button_action,      Gfx_Key_MouseRight,  v2f32( 0.0f,  0.0f), },
                    { scroll_action,      Gfx_Key_Null,        v2f32( 0.0f,  1.0f), },
                    { scroll_action,      Gfx_Key_Null,        v2f32( 0.0f, -1.0f), },
                    { scroll_action,      Gfx_Key_Null,        v2f32( 1.0f,  0.0f), },
                    { scroll_action,      Gfx_Key_Null,        v2f32(-1.0f,  0.0f), },
                };

                if (button->detail < array_count(buttons) && buttons[button->detail].kind != Gfx_EventKind_Null) {
                    Gfx_Event *button_event = arena_push_struct(arena, Gfx_Event);
                    button_event->kind       = buttons[button->detail].kind;
                    button_event->key        = buttons[button->detail].key;
                    button_event->scroll     = buttons[button->detail].scroll;
                    button_event->position.x = (F32) button->event_x;
                    button_event->position.y = (F32) button->event_y;
                    button_event->window     = x11_handle_from_window(window);
                    dll_push_back(events.first, events.last, button_event);
                }
            } break;
            case XCB_KEY_PRESS: {
                xcb_key_press_event_t *key = (xcb_key_press_event_t *) event_node->event;
                X11_Window *window = x11_window_from_id(key->event);

                Gfx_KeyModifier modifiers = 0;
                modifiers |= (xkb_state_mod_name_is_active(state->xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0 ? Gfx_KeyModifier_Shift   : 0);
                modifiers |= (xkb_state_mod_name_is_active(state->xkb_state, XKB_MOD_NAME_CTRL,  XKB_STATE_MODS_EFFECTIVE) > 0 ? Gfx_KeyModifier_Control : 0);

                U32 codepoint = xkb_state_key_get_utf32(state->xkb_state, key->detail);

                B32 is_c0_control = codepoint <= 0x1F || codepoint == 0x7F;
                B32 is_c1_control = (0x80 <= codepoint && codepoint <= 0x9F);
                B32 is_newline    = codepoint == 0x0A || codepoint == 0x0D;

                // NOTE(simon): Translate carriage returns to line feeds.
                if (is_newline) {
                    codepoint = 0x0A;
                }

                B32 is_plain           = !(modifiers & Gfx_KeyModifier_Control);
                B32 is_valid_character = (!is_c0_control && !is_c1_control) || is_newline;

                if (is_plain && is_valid_character) {
                    Gfx_Event *text_event = arena_push_struct(arena, Gfx_Event);
                    text_event->kind = Gfx_EventKind_Text;
                    text_event->text.data = arena_push_array_no_zero(arena, U8, 4);
                    text_event->text.size = string_encode_utf8(text_event->text.data, codepoint);
                    text_event->window = x11_handle_from_window(window);
                    dll_push_back(events.first, events.last, text_event);
                }
            }
            // NOTE(simon): Fallthrough
            case XCB_KEY_RELEASE: {
                xcb_key_release_event_t *key = (xcb_key_release_event_t *) event_node->event;
                X11_Window *window = x11_window_from_id(key->event);

                // NOTE(simon): Get keysyms without modifiers.
                xkb_keysym_t *keysyms = 0;
                int keysym_count = xkb_state_key_get_syms(state->xkb_state, key->detail, (const xkb_keysym_t **) &keysyms);

                for (int i = 0; i < keysym_count; ++i) {
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

                        case XKB_KEY_KP_Space:   event_key = Gfx_Key_Space;     break;
                        case XKB_KEY_KP_Tab:     event_key = Gfx_Key_Tab;       break;
                        case XKB_KEY_KP_Enter:   event_key = Gfx_Key_Return;    break;
                        case XKB_KEY_KP_F1:      event_key = Gfx_Key_F1;        break;
                        case XKB_KEY_KP_F2:      event_key = Gfx_Key_F2;        break;
                        case XKB_KEY_KP_F3:      event_key = Gfx_Key_F3;        break;
                        case XKB_KEY_KP_F4:      event_key = Gfx_Key_F4;        break;
                        case XKB_KEY_KP_Home:    event_key = Gfx_Key_Home;      break;
                        case XKB_KEY_KP_Left:    event_key = Gfx_Key_Left;      break;
                        case XKB_KEY_KP_Up:      event_key = Gfx_Key_Up;        break;
                        case XKB_KEY_KP_Right:   event_key = Gfx_Key_Right;     break;
                        case XKB_KEY_KP_Down:    event_key = Gfx_Key_Down;      break;
                        case XKB_KEY_KP_Prior:   event_key = Gfx_Key_PageUp;    break;
                        case XKB_KEY_KP_Next:    event_key = Gfx_Key_PageDown;  break;
                        case XKB_KEY_KP_End:     event_key = Gfx_Key_End;       break;
                        case XKB_KEY_KP_Delete:  event_key = Gfx_Key_Delete;    break;
                        case XKB_KEY_KP_0:       event_key = Gfx_Key_0;         break;
                        case XKB_KEY_KP_1:       event_key = Gfx_Key_1;         break;
                        case XKB_KEY_KP_2:       event_key = Gfx_Key_2;         break;
                        case XKB_KEY_KP_3:       event_key = Gfx_Key_3;         break;
                        case XKB_KEY_KP_4:       event_key = Gfx_Key_4;         break;
                        case XKB_KEY_KP_5:       event_key = Gfx_Key_5;         break;
                        case XKB_KEY_KP_6:       event_key = Gfx_Key_6;         break;
                        case XKB_KEY_KP_7:       event_key = Gfx_Key_7;         break;
                        case XKB_KEY_KP_8:       event_key = Gfx_Key_8;         break;
                        case XKB_KEY_KP_9:       event_key = Gfx_Key_9;         break;
                    }

                    if (event_key != Gfx_Key_Null) {
                        Gfx_Event *key_event = arena_push_struct(arena, Gfx_Event);
                        key_event->kind = (response_type == XCB_KEY_PRESS ? Gfx_EventKind_KeyPress : Gfx_EventKind_KeyRelease);
                        key_event->key  = event_key;
                        key_event->key_modifiers |= (key->state & XCB_MOD_MASK_SHIFT   ? Gfx_KeyModifier_Shift   : 0);
                        key_event->key_modifiers |= (key->state & XCB_MOD_MASK_CONTROL ? Gfx_KeyModifier_Control : 0);
                        key_event->window = x11_handle_from_window(window);
                        dll_push_back(events.first, events.last, key_event);
                    }
                }
            } break;
            case XCB_CLIENT_MESSAGE: {
                xcb_client_message_event_t *client = (xcb_client_message_event_t *) event_node->event;
                X11_Window *window = x11_window_from_id(client->window);

                if (client->type == state->wm_protocols_atom && client->format == 32 && client->data.data32[0] == state->wm_delete_window_atom) {
                    Gfx_Event *quit_event = arena_push_struct(arena, Gfx_Event);
                    quit_event->kind = Gfx_EventKind_Quit;
                    quit_event->window = x11_handle_from_window(window);
                    dll_push_back(events.first, events.last, quit_event);
                } else if (client->type == state->wm_protocols_atom && client->format == 32 && client->data.data32[0] == state->net_wm_sync_request_atom) {
                    // NOTE(simon): Store these for updating the counter AFTER we have repainted.
                    window->counter_value.lo = (U32) client->data.data32[2];
                    window->counter_value.hi = (S32) client->data.data32[3];
                } else if (client->type == state->x_dnd_enter_atom && client->format == 32) {
                    // TODO(simon): Should we cancel any active drag-and-drop?
                    // How do we do it in that case? We could either send
                    // XdndStatus indicating we won't accept, or send
                    // XdndFinished.

                    // NOTE(simon): Reset drag-and-drop state.
                    state->drag_and_drop_source   = XCB_WINDOW_NONE;
                    state->drag_and_drop_target   = XCB_WINDOW_NONE;
                    state->drag_and_drop_version  = 0;
                    state->drag_and_drop_type     = XCB_ATOM_NONE;
                    state->drag_and_drop_position = v2f32(0.0f, 0.0f);

                    U8 version = (client->data.data32[1] >> 24) & 0xFF;
                    if (3 <= version && version <= X11_Xdnd_Version) {
                        // NOTE(ismon): Decode parameters.
                        state->drag_and_drop_source  = client->data.data32[0];
                        state->drag_and_drop_target  = client->window;
                        state->drag_and_drop_version = version;

                        B32 more_than_three_data_types = (client->data.data32[1] >>  0) & 0x01;

                        // NOTE(simon): Select preferred type.
                        state->drag_and_drop_type = XCB_ATOM_NONE;
                        if (more_than_three_data_types) {
                            xcb_get_property_reply_t *reply = x11_get_property(state->drag_and_drop_source, state->x_dnd_type_list_atom, XCB_ATOM_ATOM);
                            if (reply) {
                                // TODO(simon): Copying the results this way might not be entierly correct
                                xcb_atom_t *types      = xcb_get_property_value(reply);
                                U64         type_count = (U64) xcb_get_property_value_length(reply) / sizeof(xcb_atom_t);

                                for (U64 i = 0; i < type_count; ++i) {
                                    if (types[i] == state->text_uri_list_atom) {
                                        state->drag_and_drop_type = state->text_uri_list_atom;
                                        break;
                                    }
                                }

                                free(reply);
                            }
                        } else {
                            for (U64 i = 2; i < 5; ++i) {
                                if (client->data.data32[i] == state->text_uri_list_atom) {
                                    state->drag_and_drop_type = state->text_uri_list_atom;
                                    break;
                                }
                            }
                        }
                    }
                } else if (client->type == state->x_dnd_leave_atom && client->format == 32) {
                    // NOTE(simon): Always clear all state so that we don't become confused.
                    state->drag_and_drop_source   = XCB_WINDOW_NONE;
                    state->drag_and_drop_target   = XCB_WINDOW_NONE;
                    state->drag_and_drop_version  = 0;
                    state->drag_and_drop_type     = XCB_ATOM_NONE;
                    state->drag_and_drop_position = v2f32(0.0f, 0.0f);
                } else if (client->type == state->x_dnd_position_atom && client->format == 32) {
                    if (state->drag_and_drop_target == client->window) {
                        // NOTE(simon): Decode parameters.
                        S16 root_x = (S16) ((client->data.data32[2] >> 16) & 0xFFFF);
                        S16 root_y = (S16) ((client->data.data32[2] >>  0) & 0xFFFF);
                        U32 action = client->data.data32[4];

                        // NOTE(simon): Update local coordinates of drag.
                        xcb_translate_coordinates_cookie_t cookie = xcb_translate_coordinates(
                            state->connection,
                            state->screen->root,
                            state->drag_and_drop_target,
                            root_x,
                            root_y
                        );
                        xcb_translate_coordinates_reply_t *reply = xcb_translate_coordinates_reply(state->connection, cookie, 0);
                        if (reply) {
                            state->drag_and_drop_position = v2f32(reply->dst_x, reply->dst_y);
                            free(reply);
                        }

                        // NOTE(simon): If we have a common supported type,
                        // accept the drag with no rectangle. Otherwise,
                        // decline it.
                        xcb_client_message_event_t status_message = {
                            .response_type  = XCB_CLIENT_MESSAGE,
                            .format         = 32,
                            .window         = state->drag_and_drop_source,
                            .type           = state->x_dnd_status_atom,
                            .data.data32[0] = state->drag_and_drop_target,
                        };
                        if (state->drag_and_drop_type != XCB_ATOM_NONE) {
                            status_message.data.data32[1] = 0x01;
                            status_message.data.data32[4] = state->x_dnd_action_copy_atom;
                        }
                        xcb_send_event(state->connection, false, state->drag_and_drop_source, 0, (const char *) &status_message);
                        xcb_flush(state->connection);
                    }
                } else if (client->type == state->x_dnd_drop_atom && client->format == 32) {
                    if (state->drag_and_drop_target == client->window) {
                        // NOTE(simon): Decode parameters.
                        xcb_timestamp_t timestamp = client->data.data32[2];

                        if (state->drag_and_drop_type != XCB_ATOM_NONE) {
                            // NOTE(simon): Convert the selection if we have a
                            // common supported type.
                            xcb_convert_selection(
                                state->connection,
                                window->window,
                                state->x_dnd_selection_atom,
                                state->drag_and_drop_type,
                                state->x_dnd_selection_atom,
                                timestamp
                            );
                        } else {
                            // NOTE(simon): We could not agree on a common
                            // supported type, inform that we won't read the data.
                            xcb_client_message_event_t finished_message = {
                                .response_type = XCB_CLIENT_MESSAGE,
                                .format = 32,
                                .window = state->drag_and_drop_source,
                                .type = state->x_dnd_finished_atom,
                                .data.data32[0] = window->window,
                            };
                            xcb_send_event(state->connection, false, state->drag_and_drop_source, 0, (const char *) &finished_message);
                            xcb_flush(state->connection);

                            // NOTE(simon): Reset drag-and-drop state.
                            state->drag_and_drop_source   = XCB_WINDOW_NONE;
                            state->drag_and_drop_target   = XCB_WINDOW_NONE;
                            state->drag_and_drop_version  = 0;
                            state->drag_and_drop_type     = XCB_ATOM_NONE;
                            state->drag_and_drop_position = v2f32(0.0f, 0.0f);
                        }
                    }
                }
            } break;
            case XCB_SELECTION_REQUEST: {
                xcb_selection_request_event_t *request = (xcb_selection_request_event_t *) event_node->event;

                xcb_atom_t property_atom = (request->property == XCB_ATOM_NONE ? request->target : request->property);
                xcb_atom_t response_property_atom = XCB_ATOM_NONE;

                if (property_atom != XCB_ATOM_NONE && request->selection == state->clipboard_atom && request->owner == state->clipboard_window) {
                    xcb_atom_t targets[] = {
                        state->targets_atom,
                        state->utf8_string_atom,
                    };

                    xcb_atom_t type_atom = XCB_ATOM_NONE;
                    U8         format    = 0;
                    Str8       data      = { 0 };
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
                            response_property_atom = property_atom;
                        }
                    }
                }

                xcb_selection_notify_event_t selection_notify = {
                    .response_type = XCB_SELECTION_NOTIFY,
                    .time          = request->time,
                    .requestor     = request->requestor,
                    .selection     = request->selection,
                    .target        = request->target,
                    .property      = response_property_atom,
                };
                xcb_send_event(state->connection, false, request->requestor, 0, (const char *) &selection_notify);
                xcb_flush(state->connection);
            } break;
            default: {
                if (response_type == state->xkb_first_event) {
                    // NOTE(simon): HOW does this type not exist in the library headers???
                    typedef struct XKB_Event XKB_Event;
                    struct XKB_Event {
                        U8 repsone_type;
                        U8 type;
                        U16 sequence;
                        xcb_timestamp_t time;
                        U8 deviceID;
                    };
                    XKB_Event *xkb_event = (XKB_Event *) event_node->event;

                    if (xkb_event->deviceID == state->xkb_core_keyboard_id) {
                        switch (xkb_event->type) {
                            case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
                                xcb_xkb_new_keyboard_notify_event_t *keyboard_notify = (xcb_xkb_new_keyboard_notify_event_t *) xkb_event;
                                if (keyboard_notify->changed & XCB_XKB_NKN_DETAIL_KEYCODES) {
                                    xkb_keymap_unref(state->xkb_keymap);
                                    xkb_state_unref(state->xkb_state);
                                    state->xkb_keymap = xkb_x11_keymap_new_from_device(state->xkb_context, state->connection, state->xkb_core_keyboard_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
                                    state->xkb_state  = xkb_x11_state_new_from_device(state->xkb_keymap, state->connection, state->xkb_core_keyboard_id);
                                }
                            } break;
                            case XCB_XKB_MAP_NOTIFY: {
                                xkb_keymap_unref(state->xkb_keymap);
                                xkb_state_unref(state->xkb_state);
                                state->xkb_keymap = xkb_x11_keymap_new_from_device(state->xkb_context, state->connection, state->xkb_core_keyboard_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
                                state->xkb_state  = xkb_x11_state_new_from_device(state->xkb_keymap, state->connection, state->xkb_core_keyboard_id);
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
                }
            } break;
        }

        free(event_node->event);
    }

    state->first_event = 0;
    state->last_event = 0;
    arena_reset(state->event_arena);

    return events;
}

internal Void gfx_set_update_function(VoidFunction *update) {
    X11_State *state = &global_x11_state;
    state->update = update;
}



internal Void gfx_set_cursor(Gfx_Cursor cursor) {
    X11_State *state = &global_x11_state;
    state->cursor = cursor;
    x11_update_cursor();

}



// NOTE(simon): Windows.
internal B32 gfx_window_equal(Gfx_Window handle_a, Gfx_Window handle_b) {
    X11_Window *window_a = x11_window_from_handle(handle_a);
    X11_Window *window_b = x11_window_from_handle(handle_b);

    B32 result = window_a == window_b;
    return result;
}

internal Gfx_Window gfx_window_create(Str8 title, U32 width, U32 height) {
    X11_State *state = &global_x11_state;
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    // NOTE(simon): Allocate window.
    X11_Window *window = state->window_freelist;
    if (window) {
        sll_stack_pop(state->window_freelist);
        U64 generation = window->generation;
        memory_zero_struct(window);
        window->generation = generation;
    } else {
        window = arena_push_struct(state->permanent_arena, X11_Window);
    }
    dll_push_back(state->first_window, state->last_window, window);

    U32 value_mask = XCB_CW_EVENT_MASK;
    U32 value_list[] = {
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY,
    };

    window->window = xcb_generate_id(state->connection);
    xcb_create_window(
        state->connection,
        XCB_COPY_FROM_PARENT,
        window->window,
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
        window->window,
        XCB_ATOM_WM_NAME,
        state->utf8_string_atom,
        8,
        (U32) title.size,
        title.data
    );

    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        window->window,
        XCB_ATOM_WM_ICON_NAME,
        state->utf8_string_atom,
        8,
        (U32) title.size,
        title.data
    );

    U32 dnd_version = X11_Xdnd_Version;
    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        window->window,
        state->x_dnd_aware_atom,
        XCB_ATOM_ATOM,
        32,
        1,
        &dnd_version
    );

    X11_IcccmWmSizeHints wm_normal_hints = { 0 };
    wm_normal_hints.flags |= X11_IcccmWmSizeHint_MinSize;
    wm_normal_hints.min_width  = 50;
    wm_normal_hints.min_height = 50;
    wm_normal_hints.flags |= X11_IcccmWmSizeHint_WindowGravity;
    wm_normal_hints.window_gravity = XCB_GRAVITY_CENTER;

    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        window->window,
        XCB_ATOM_WM_NORMAL_HINTS,
        XCB_ATOM_WM_SIZE_HINTS,
        32,
        sizeof(X11_IcccmWmSizeHints) / sizeof(U32),
        &wm_normal_hints
    );

    X11_IcccmWmHints wm_hints = { 0 };
    wm_hints.flags |= X11_IcccmWmHint_Input;
    wm_hints.input = true;
    wm_hints.flags |= X11_IcccmWmHint_State;
    wm_hints.initial_state = X11_IcccmWmState_Normal;
    // NOTE(simon): We might want to use window_group if and when we have multiple windows.

    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        window->window,
        XCB_ATOM_WM_HINTS,
        XCB_ATOM_WM_HINTS,
        32,
        sizeof(X11_IcccmWmHints) / sizeof(U32),
        &wm_hints
    );

    // NOTE(simon): wm_instance does not follow POSIX convention, but that
    // should be fine.
    Str8 wm_instance = str8_format(scratch.arena, "%d", getpid());
    Str8 wm_class = title;
    U64 wm_class_size = wm_instance.size + 1 + wm_class.size + 1;
    U8 *wm_class_buffer = arena_push_array(scratch.arena, U8, wm_class_size);
    memory_copy(wm_class_buffer, wm_instance.data, wm_instance.size);
    wm_class_buffer[wm_instance.size] = 0;
    memory_copy(&wm_class_buffer[wm_instance.size + 1], wm_class.data, wm_class.size);
    wm_class_buffer[wm_instance.size + 1 + wm_class.size] = 0;
    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        window->window,
        XCB_ATOM_WM_CLASS,
        XCB_ATOM_STRING,
        8,
        (U32) wm_class_size,
        wm_class_buffer
    );

    // NOTE(simon): Create sync counter and attach it to the window.
    window->counter = xcb_generate_id(state->connection);
    xcb_sync_int64_t initial_counter_value = { 0 };
    xcb_sync_create_counter(state->connection, window->counter, initial_counter_value);
    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        window->window,
        state->net_wm_sync_request_counter_atom,
        XCB_ATOM_CARDINAL,
        32,
        1,
        &window->counter
    );

    // TODO(simon): Do we insert an empty WM_COLORMAP_WINDOWS property?

    xcb_atom_t wm_protocols[] = {
        state->wm_delete_window_atom,
        state->net_wm_sync_request_atom,
    };
    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        window->window,
        state->wm_protocols_atom,
        XCB_ATOM_ATOM,
        32,
        array_count(wm_protocols),
        wm_protocols
    );

    xcb_map_window(state->connection, window->window);

    xcb_flush(state->connection);

    arena_end_temporary(scratch);
    Gfx_Window result = x11_handle_from_window(window);
    return result;
}

internal Void gfx_window_close(Gfx_Window handle) {
    X11_State *state = &global_x11_state;
    X11_Window *window = x11_window_from_handle(handle);

    xcb_destroy_window(state->connection, window->window);
    xcb_sync_destroy_counter(state->connection, window->counter);

    ++window->generation;

    dll_remove(state->first_window, state->last_window, window);
    sll_stack_push(state->window_freelist, window);
}

internal V2U32 gfx_client_area_from_window(Gfx_Window handle) {
    X11_State *state = &global_x11_state;
    X11_Window *window = x11_window_from_handle(handle);
    V2U32 result = v2u32(window->width, window->height);
    return result;
}

// TODO(simon): This doesn't follow the specification if the mouse is outside
// of the window, it returns last mouse position that was inside the window.
internal V2F32 gfx_mouse_position_from_window(Gfx_Window handle) {
    X11_State *state = &global_x11_state;
    X11_Window *window = x11_window_from_handle(handle);

    xcb_query_pointer_cookie_t cookie = xcb_query_pointer(state->connection, window->window);
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

internal F32 gfx_dpi_from_window(Gfx_Window window) {
    X11_State *state = &global_x11_state;
    F32 dpi = state->dpi;
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
    if (error) {
        fprintf(stderr, "\x1B[1;31mERROR: %.*s\n", str8_expand(title));
        fprintf(stderr, "%.*s\x1B[0m\n", str8_expand(message));
    } else {
        fprintf(stderr, "INFO: %.*s\n", str8_expand(title));
        fprintf(stderr, "%.*s\n", str8_expand(message));
    }
}



// NOTE(simon): Clipboard
internal Void gfx_set_clipboard_text(Str8 text) {
    X11_State *state = &global_x11_state;

    arena_reset(state->copy_arena);
    state->copy_text = str8_copy(state->copy_arena, text);

    xcb_set_selection_owner(state->connection, state->clipboard_window, state->clipboard_atom, XCB_CURRENT_TIME);
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
    } else if (owner == state->clipboard_window) {
        // NOTE(simon): We own the clipboard selection! Perform a cheap internal copy.
        result = str8_copy(arena, state->copy_text);
    } else {
        // NOTE(simon): Someone else owns the clipboard selection, and we are
        // incredibly sad :( There is no good solution to how you get selection
        // contents.

        Arena_Temporary scratch = arena_get_scratch(&arena, 1);

        // NOTE(simon): Deleted so that the new contents can be put in the property.
        xcb_delete_property(state->connection, state->clipboard_window, state->clipboard_property_atom);

        xcb_convert_selection(
            state->connection,
            state->clipboard_window,
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

                    reply = x11_get_property(state->clipboard_window, state->clipboard_property_atom, state->utf8_string_atom);
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
                    
                    xcb_delete_property(state->connection, state->clipboard_window, state->clipboard_property_atom);
                    xcb_flush(state->connection);
                    free(reply);
                }
            } else if ((xcb_event->response_type & ~0x80) == XCB_PROPERTY_NOTIFY) {
                xcb_property_notify_event_t *notify = (xcb_property_notify_event_t *) xcb_event;

                xcb_get_property_reply_t *reply = 0;
                if (incremental && notify->atom == state->clipboard_property_atom && notify->state == XCB_PROPERTY_NEW_VALUE) {
                    consumed = true;

                    // TODO(simon): We might need to do this request multiple
                    // times if the X-server doesn't allow us to grab
                    // everything from the property in one go. The maximum size
                    // of a property might be larger than the maximum size of a
                    // response.
                    xcb_get_property_cookie_t cookie = xcb_get_property(
                        state->connection,
                        true,
                        state->clipboard_window,
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
                X11_EventNode *event_node = arena_push_struct(state->event_arena, X11_EventNode);
                event_node->event = xcb_event;
                sll_queue_push(state->first_event, state->last_event, event_node);
            }
        }

        result = str8_join(arena, copy_parts);
        arena_end_temporary(scratch);
    }

    return result;
}
