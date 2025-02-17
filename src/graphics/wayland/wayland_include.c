#include <poll.h>
#include <string.h>

#include <linux/input-event-codes.h>

#include "wayland_xdg_shell.generated.c"

global Wayland_State global_wayland_state;

// NOTE(simon): XDG WM base events
internal Void wayland_xdg_wm_base_ping(Void *data, struct xdg_wm_base *xdg_wm_base, U32 serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}



// NOTE(simon): Pointer events
internal Void wayland_pointer_enter(Void *data, struct wl_pointer *pointer, U32 serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    Wayland_State *state = &global_wayland_state;
    state->selection_source_serial = serial;

    state->pointer_position = v2f32(
        (F32) wl_fixed_to_double(surface_x),
        (F32) wl_fixed_to_double(surface_y)
    );
}

internal Void wayland_pointer_leave(Void *data, struct wl_pointer *pointer, U32 serial, struct wl_surface *surface) {
    Wayland_State *state = &global_wayland_state;
    state->selection_source_serial = serial;
}

internal Void wayland_pointer_motion(Void *data, struct wl_pointer *pointer, U32 time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    Wayland_State *state = &global_wayland_state;

    state->pointer_position = v2f32(
        (F32) wl_fixed_to_double(surface_x),
        (F32) wl_fixed_to_double(surface_y)
    );
}

internal Void wayland_pointer_button(Void *data, struct wl_pointer *pointer, U32 serial, U32 time, U32 button, U32 button_state) {
    Wayland_State *state = &global_wayland_state;
    state->selection_source_serial = serial;

    Gfx_EventKind kind = Gfx_EventKind_Null;
    switch (button_state) {
        case WL_POINTER_BUTTON_STATE_PRESSED:  kind = Gfx_EventKind_KeyPress;   break;
        case WL_POINTER_BUTTON_STATE_RELEASED: kind = Gfx_EventKind_KeyRelease; break;
    }

    Gfx_Key key = Gfx_Key_Null;
    switch (button) {
        case BTN_LEFT:   key = Gfx_Key_MouseLeft;   break;
        case BTN_MIDDLE: key = Gfx_Key_MouseMiddle; break;
        case BTN_RIGHT:  key = Gfx_Key_MouseRight;  break;
    }

    if (kind != Gfx_EventKind_Null && key != Gfx_Key_Null) {
        Gfx_Event *event = arena_push_struct_zero(state->event_arena, Gfx_Event);
        event->kind     = kind;
        event->key      = key;
        event->position = state->pointer_position;
        dll_push_back(state->events.first, state->events.last, event);
    }
}

internal Void wayland_pointer_axis(Void *data, struct wl_pointer *pointer, U32 time, U32 axis, wl_fixed_t value) {
    Wayland_State *state = &global_wayland_state;

    switch (axis) {
        case WL_POINTER_AXIS_VERTICAL_SCROLL:   state->pointer_axis.y = (F32) -wl_fixed_to_double(value); break;
        case WL_POINTER_AXIS_HORIZONTAL_SCROLL: state->pointer_axis.x = (F32) -wl_fixed_to_double(value); break;
    }
}

internal Void wayland_pointer_frame(Void *data, struct wl_pointer *pointer) {
    Wayland_State *state = &global_wayland_state;

    // NOTE(simon): Prefer discrete events of continuous events for scrolling.
    // We will get both kinds of events within one input frame, and we don't
    // want to output two events for the same action.
    if (state->pointer_axis_discrete.x != 0.0f || state->pointer_axis_discrete.y != 0.0f) {
        Gfx_Event *event = arena_push_struct_zero(state->event_arena, Gfx_Event);
        event->kind     = Gfx_EventKind_Scroll;
        event->scroll   = state->pointer_axis_discrete;
        event->position = state->pointer_position;
        dll_push_back(state->events.first, state->events.last, event);

        memory_zero_struct(&state->pointer_axis);
        memory_zero_struct(&state->pointer_axis_discrete);
    }

    if (state->pointer_axis.x != 0.0f || state->pointer_axis.y != 0.0f) {
        Gfx_Event *event = arena_push_struct_zero(state->event_arena, Gfx_Event);
        event->kind     = Gfx_EventKind_Scroll;
        event->scroll   = state->pointer_axis;
        event->position = state->pointer_position;
        dll_push_back(state->events.first, state->events.last, event);

        memory_zero_struct(&state->pointer_axis);
        memory_zero_struct(&state->pointer_axis_discrete);
    }
}

internal Void wayland_pointer_axis_source(Void *data, struct wl_pointer *pointer, U32 axis_source) {
}

internal Void wayland_pointer_axis_stop(Void *data, struct wl_pointer *pointer, U32 time, U32 axis) {
}

internal Void wayland_pointer_axis_discrete(Void *data, struct wl_pointer *pointer, U32 axis, S32 discrete) {
    Wayland_State *state = &global_wayland_state;
    switch (axis) {
        case WL_POINTER_AXIS_VERTICAL_SCROLL:   state->pointer_axis_discrete.y = (F32) -discrete; break;
        case WL_POINTER_AXIS_HORIZONTAL_SCROLL: state->pointer_axis_discrete.x = (F32) -discrete; break;
    }
}



// NOTE(simon): Keyboard events
internal Void wayland_keyboard_keymap(Void *data, struct wl_keyboard *keyboard, U32 format, S32 fd, U32 size) {
    Wayland_State *state = &global_wayland_state;

    xkb_keymap_unref(state->xkb_keymap);
    xkb_state_unref(state->xkb_state);
    state->xkb_keymap = 0;
    state->xkb_state = 0;

    if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        char *map_shm = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);

        if (map_shm != MAP_FAILED) {
            state->xkb_keymap = xkb_keymap_new_from_string(
                state->xkb_context,
                map_shm,
                XKB_KEYMAP_FORMAT_TEXT_V1,
                XKB_KEYMAP_COMPILE_NO_FLAGS
            );
            munmap(map_shm, size);
        }
    }

    if (state->xkb_keymap) {
        state->xkb_state = xkb_state_new(state->xkb_keymap);
    }

    close(fd);
}

internal Void wayland_keyboard_enter(Void *data, struct wl_keyboard *keyboard, U32 serial, struct wl_surface *surface, struct wl_array *keys) {
    Wayland_State *state = &global_wayland_state;
    state->selection_source_serial = serial;
}

internal Void wayland_keyboard_leave(Void *data, struct wl_keyboard *keyboard, U32 serial, struct wl_surface *surface) {
    Wayland_State *state = &global_wayland_state;
    state->selection_source_serial = serial;
}

internal Void wayland_keyboard_key(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 time, U32 key, U32 key_state) {
    Wayland_State *state = &global_wayland_state;
    state->selection_source_serial = serial;

    U32 xkb_key = 8 + key;

    if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        int required_length = xkb_state_key_get_utf8(state->xkb_state, xkb_key, 0, 0) + 1;
        if (required_length > 1) {
            CStr buffer = arena_push_array_zero(state->event_arena, char, (U64) required_length);
            int length = xkb_state_key_get_utf8(state->xkb_state, xkb_key, buffer, (size_t) required_length);

            Gfx_Event *event = arena_push_struct_zero(state->event_arena, Gfx_Event);
            event->kind = Gfx_EventKind_Text;
            event->text = str8((U8 *) buffer, (U64) length);
            dll_push_back(state->events.first, state->events.last, event);
        }
    }

    xkb_keysym_t *keysyms = 0;
    int keysym_count = xkb_keymap_key_get_syms_by_level(state->xkb_keymap, xkb_key, 0, 0, (const xkb_keysym_t **) &keysyms);
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
        }

        if (event_key != Gfx_Key_Null) {
            Gfx_Event *event = arena_push_struct_zero(state->event_arena, Gfx_Event);
            event->kind = (key_state == WL_KEYBOARD_KEY_STATE_PRESSED ? Gfx_EventKind_KeyPress : Gfx_EventKind_KeyRelease);
            event->key  = event_key;
            event->key_modifiers = state->modifiers;
            dll_push_back(state->events.first, state->events.last, event);
        }

        // NOTE(simon): Update modifiers.
        {
            Gfx_KeyModifier modifier = 0;
            if (event_key == Gfx_Key_Control ) {
                modifier = Gfx_KeyModifier_Control;
            }
            if (event_key == Gfx_Key_Shift ) {
                modifier = Gfx_KeyModifier_Shift;
            }

            if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED) {
                state->modifiers |= modifier;
            } else {
                state->modifiers &= ~modifier;
            }
        }
    }
}

internal Void wayland_keyboard_modifiers(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 mods_depressed, U32 mods_latched, U32 mods_locked, U32 group) {
    Wayland_State *state = &global_wayland_state;
    state->selection_source_serial = serial;

    if (state->xkb_state) {
        xkb_state_update_mask(state->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    }
}

internal Void wayland_keyboard_repeat_info(Void *data, struct wl_keyboard *keyboard, S32 rate, S32 delay) {
    // TODO(simon): Handle repeat info
}



// NOTE(simon): Seat events
internal Void wayland_seat_capabilities(Void *data, struct wl_seat *seat, U32 capabilities) {
    Wayland_State *state = &global_wayland_state;

    U32 changed_capabilities = state->previous_capabilities ^ capabilities;
    U32 removed_capabilities = changed_capabilities & ~capabilities;
    U32 added_capabilities   = changed_capabilities &  capabilities;

    state->previous_capabilities = capabilities;

    if (removed_capabilities & WL_SEAT_CAPABILITY_POINTER) {
        wl_pointer_release(state->pointer);
        state->pointer = 0;
    }

    if (removed_capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        wl_keyboard_release(state->keyboard);
        xkb_keymap_unref(state->xkb_keymap);
        xkb_state_unref(state->xkb_state);
        state->keyboard = 0;
        state->xkb_keymap = 0;
        state->xkb_state = 0;
    }

    if (added_capabilities & WL_SEAT_CAPABILITY_POINTER) {
        state->pointer = wl_seat_get_pointer(state->seat);
        wl_pointer_add_listener(state->pointer, &wayland_pointer_listener, 0);
    }

    if (added_capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        state->keyboard = wl_seat_get_keyboard(state->seat);
        wl_keyboard_add_listener(state->keyboard, &wayland_keyboard_listener, 0);
    }
}

internal Void wayland_seat_name(Void *data, struct wl_seat *seat, const char *name) {
}



// NOTE(simon): Data offer events.
internal Void wayland_data_offer_offer(Void *data, struct wl_data_offer *wl_data_offer, const char *mime_type) {
}

internal Void wayland_data_offer_source_actions(Void *data, struct wl_data_offer *wl_data_offer, U32 source_actions) {
}

internal Void wayland_data_offer_action(Void *data, struct wl_data_offer *wl_data_offer, U32 dnd_action) {
}



// NOTE(simon): Data device events.
internal Void wayland_data_device_data_offer(Void *data, struct wl_data_device *data_device, struct wl_data_offer *id) {
    Wayland_State *state = &global_wayland_state;
    wl_data_offer_add_listener(id, &wayland_data_offer_listener, 0);
}

internal Void wayland_data_device_enter(Void *data, struct wl_data_device *data_device, U32 serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *id) {
    Wayland_State *state = &global_wayland_state;

    state->drag_and_drop_offer = id;
}

internal Void wayland_data_device_leave(Void *data, struct wl_data_device *data_device) {
    Wayland_State *state = &global_wayland_state;

    wl_data_offer_destroy(state->drag_and_drop_offer);
    state->drag_and_drop_offer = 0;
}

internal Void wayland_data_device_motion(Void *data, struct wl_data_device *data_device, U32 time, wl_fixed_t x, wl_fixed_t y) {
}

internal Void wayland_data_device_drop(Void *data, struct wl_data_device *data_device) {
    Wayland_State *state = &global_wayland_state;
    wl_data_offer_destroy(state->drag_and_drop_offer);
    state->drag_and_drop_offer = 0;
}

internal Void wayland_data_device_selection(Void *data, struct wl_data_device *data_device, struct wl_data_offer *id) {
    Wayland_State *state = &global_wayland_state;

    if (state->selection_offer) {
        wl_data_offer_destroy(state->selection_offer);
        state->selection_offer = 0;
    }

    state->selection_offer = id;
}



// NOTE(simon): Data source events.
internal Void wayland_data_source_target(Void *data, struct wl_data_source *data_source, const char *mime_type) {
    // NOTE(simon): Only used for drag-and-drop.
}

internal Void wayland_data_source_send(Void *data, struct wl_data_source *data_source, const char *mime_type, S32 fd) {
    Wayland_State *state = &global_wayland_state;

    if (data_source == state->selection_source) {
        // TODO(simon): Look at availible mime types.
        if (
            strcmp(mime_type, "text/plain;charset=utf-8") == 0 ||
            strcmp(mime_type, "UTF8_STRING") == 0
        ) {
            U8 *ptr = state->selection_source_str8.data;
            U8 *opl = state->selection_source_str8.data + state->selection_source_str8.size;

            while (ptr < opl) {
                size_t bytes_to_write = (size_t) s64_min((opl - ptr), (S64) SSIZE_MAX);
                ssize_t bytes_written = write(fd, ptr, bytes_to_write);

                if (bytes_written >= 0) {
                    ptr += bytes_written;
                } else if (errno != EINTR) {
                    break;
                }
            }
        }
    }

    close(fd);
}

internal Void wayland_data_source_cancelled(Void *data, struct wl_data_source *data_source) {
    Wayland_State *state = &global_wayland_state;

    if (data_source == state->selection_source) {
        arena_reset(state->selection_source_arena);
        memory_zero_struct(&state->selection_source_str8);
        state->selection_source = 0;
    }

    wl_data_source_destroy(data_source);
}

internal Void wayland_data_source_dnd_drop_performed(Void *data, struct wl_data_source *data_source) {
    // NOTE(simon): Only used for drag-and-drop.
}

internal Void wayland_data_source_dnd_finished(Void *data, struct wl_data_source *data_source) {
    // NOTE(simon): Only used for drag-and-drop.
}

internal Void wayland_data_source_action(Void *data, struct wl_data_source *data_source, U32 dnd_action) {
    // NOTE(simon): Only used for drag-and-drop.
}




// NOTE(simon): XDG surface events.
internal Void wayland_xdg_surface_configure(Void *data, struct xdg_surface *xdg_surface, U32 serial) {
    Wayland_State *state = &global_wayland_state;
    if (state->resize) {
        state->resize();
    }
    if (state->update) {
        state->update();
    }
    xdg_surface_ack_configure(xdg_surface, serial);
}



// NOTE(simon): XDG toplevel events
internal Void wayland_xdg_toplevel_configure(Void *data, struct xdg_toplevel *xgd_toplevel, S32 width, S32 height, struct wl_array *states) {
    Wayland_State *state = &global_wayland_state;
    state->width = width;
    state->height = height;
}

internal Void wayland_xdg_toplevel_close(Void *data, struct xdg_toplevel *xdg_toplevel) {
    Wayland_State *state = &global_wayland_state;
    Gfx_Event *event = arena_push_struct_zero(state->event_arena, Gfx_Event);
    event->kind = Gfx_EventKind_Quit;
    dll_push_back(state->events.first, state->events.last, event);
}



// NOTE(simon): Registry events
internal Void wayland_register_global(Void *data, struct wl_registry *registry, U32 name, const char *interface, U32 version) {
    Wayland_State *state = &global_wayland_state;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        // TODO(simon): Handle multiple seats
        state->seat = wl_registry_bind(registry, name, &wl_seat_interface, 7);
        wl_seat_add_listener(state->seat, &wayland_seat_listener, 0);
    } else if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
        state->data_device_manager = wl_registry_bind(registry, name, &wl_data_device_manager_interface, 3);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 3);
        xdg_wm_base_add_listener(state->xdg_wm_base, &wayland_xdg_wm_base_listener, 0);
    }
}

internal Void wayland_register_global_remove(Void *data, struct wl_registry *registry, U32 name) {
    Wayland_State *state = &global_wayland_state;
}



internal Void gfx_create(Str8 title, U32 width, U32 height) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    Wayland_State *state = &global_wayland_state;
    state->event_arena = arena_create();
    state->display = wl_display_connect(0);
    struct wl_registry *registry = wl_display_get_registry(state->display);
    wl_registry_add_listener(registry, &wayland_registry_listener, 0);

    state->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    wl_display_roundtrip(state->display);

    state->selection_source_arena = arena_create();

    if (state->data_device_manager && state->seat) {
        state->data_device = wl_data_device_manager_get_data_device(state->data_device_manager, state->seat);
        wl_data_device_add_listener(state->data_device, &wayland_data_device_listener, 0);
    }

    state->width = (S32) width;
    state->height = (S32) height;
    state->wl_surface = wl_compositor_create_surface(state->compositor);
    state->xdg_surface = xdg_wm_base_get_xdg_surface(state->xdg_wm_base, state->wl_surface);
    xdg_surface_add_listener(state->xdg_surface, &wayland_xdg_surface_listener, 0);
    state->xdg_toplevel = xdg_surface_get_toplevel(state->xdg_surface);
    xdg_toplevel_add_listener(state->xdg_toplevel, &wayland_xdg_toplevel_listener, 0);
    CStr title_cstr = cstr_from_str8(scratch.arena, title);
    xdg_toplevel_set_title(state->xdg_toplevel, title_cstr);
    wl_surface_commit(state->wl_surface);

    arena_end_temporary(scratch);
}

internal V2U32 gfx_get_window_client_area(Void) {
    Wayland_State *state = &global_wayland_state;
    V2U32 result = v2u32((U32) state->width, (U32) state->height);
    return result;
}

internal Void gfx_send_wakeup_event(Void) {
    Wayland_State *state = &global_wayland_state;
    wl_display_sync(state->display);
}

internal Gfx_EventList gfx_get_events(Arena *arena, B32 wait) {
    Wayland_State *state = &global_wayland_state;

    // TODO(simon): Error handling
    if (wait && !state->events.first) {
        for (;;) {
            int dispatched_events = 0;
            while (wl_display_prepare_read(state->display) != 0) {
                dispatched_events += wl_display_dispatch_pending(state->display);
            }
            wl_display_flush(state->display);

            struct pollfd fd = { 0 };
            fd.fd = wl_display_get_fd(state->display);
            fd.events = POLLIN;
            poll(&fd, 1, -1);

            wl_display_read_events(state->display);
            dispatched_events += wl_display_dispatch_pending(state->display);
            if (dispatched_events > 0) {
                break;
            }
        }
    } else {
        while (wl_display_prepare_read(state->display) != 0) {
            wl_display_dispatch_pending(state->display);
        }
        wl_display_flush(state->display);

        wl_display_read_events(state->display);
        wl_display_dispatch_pending(state->display);
    }

    Gfx_EventList events = { 0 };

    // NOTE(simon): Copy events to the provided arena.
    for (Gfx_Event *event = state->events.first; event; event = event->next) {
        Gfx_Event *new_event = arena_push_struct_zero(arena, Gfx_Event);
        *new_event = *event;
        new_event->text = str8_copy(arena, new_event->text);
        new_event->path = str8_copy(arena, new_event->path);
        dll_push_back(events.first, events.last, new_event);
    }

    // NOTE(simon): Reset event state.
    arena_reset(state->event_arena);
    memory_zero_struct(&state->events);

    return events;
}

internal V2F32 gfx_get_mouse_position(Void) {
    Wayland_State *state = &global_wayland_state;
    V2F32 result = state->pointer_position;
    return result;
}

internal Void gfx_swap_buffers(Void) {
    Wayland_State *state = &global_wayland_state;
    if (state->swap_buffers) {
        state->swap_buffers();
    }
}

internal Void gfx_set_cursor(Gfx_Cursor cursor) {
    Wayland_State *state = &global_wayland_state;
}

internal Void gfx_set_update_function(VoidFunction *update) {
    Wayland_State *state = &global_wayland_state;
    state->update = update;
}


// NOTE(simon): Clipboard
internal Void gfx_set_clipboard_text(Str8 text) {
    Wayland_State *state = &global_wayland_state;

    arena_reset(state->selection_source_arena);
    state->selection_source_str8 = str8_copy(state->selection_source_arena, text);

    state->selection_source = wl_data_device_manager_create_data_source(state->data_device_manager);
    wl_data_source_add_listener(state->selection_source, &wayland_data_source_listener, 0);
    // TODO(simon): Look at availible mime types.
    wl_data_source_offer(state->selection_source, "text/plain;charset=utf-8");
    wl_data_source_offer(state->selection_source, "UTF8_STRING");
    wl_data_device_set_selection(state->data_device, state->selection_source, state->selection_source_serial);
}

internal Str8 gfx_get_clipboard_text(Arena *arena) {
    Wayland_State *state = &global_wayland_state;
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    Str8List segments = { 0 };

    if (state->selection_offer) {
        if (state->selection_source) {
            // NOTE(simon): We own the selection! Perform an internal copy to
            // avoid having to both read and write to a pipe in the same
            // process.
            str8_list_push(scratch.arena, &segments, state->selection_source_str8);
        } else {
            int file_descriptors[2] = { 0 };
            if (pipe(file_descriptors) != -1) {
                // TODO(simon): Look at offered mime types.
                wl_data_offer_receive(state->selection_offer, "UTF8_STRING", file_descriptors[1]);
                wl_display_flush(state->display);

                // NOTE(simon): Close the write file descriptor as we are done with
                // it on our side.
                close(file_descriptors[1]);

                size_t buffer_capacity = (size_t) s64_min(1 << 16, (S64) SSIZE_MAX);

                for (;;) {
                    U8 *buffer = arena_push_array_zero(scratch.arena, U8, buffer_capacity);
                    U64 buffer_size = 0;

                    for (;;) {
                        ssize_t bytes_read = read(file_descriptors[0], buffer, buffer_capacity);

                        if (bytes_read >= 0) {
                            buffer_size = (U64) bytes_read;
                            break;
                        } else if (errno != EINTR) {
                            // NOTE(simon): Unrecoverable error, abort the copy.
                            break;
                        }
                    }

                    if (buffer_size != 0) {
                        str8_list_push(scratch.arena, &segments, str8(buffer, buffer_size));
                    } else {
                        break;
                    }
                }

                close(file_descriptors[0]);
            }
        }
    }

    Str8 result = str8_join(arena, &segments);

    arena_end_temporary(scratch);
    return result;
}
