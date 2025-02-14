#include <string.h>

#include "wayland_xdg_shell.generated.c"

global Wayland_State global_wayland_state;

// NOTE(simon): XDG WM base events
internal Void wayland_xdg_wm_base_ping(Void *data, struct xdg_wm_base *xdg_wm_base, U32 serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}



// NOTE(simon): Pointer events
internal Void wayland_pointer_enter(Void *data, struct wl_pointer *pointer, U32 serial, struct wl_surface *surface, wl_fixed_t suraface_x, wl_fixed_t surface_y) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_pointer_leave(Void *data, struct wl_pointer *pointer, U32 serial, struct wl_surface *surface) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_pointer_motion(Void *data, struct wl_pointer *pointer, U32 time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_pointer_button(Void *data, struct wl_pointer *pointer, U32 serial, U32 time, U32 button, U32 state) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_pointer_axis(Void *data, struct wl_pointer *pointer, U32 time, U32 axis, wl_fixed_t value) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_pointer_frame(Void *data, struct wl_pointer *pointer) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_pointer_axis_source(Void *data, struct wl_pointer *pointer, U32 axis_source) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_pointer_axis_stop(Void *data, struct wl_pointer *pointer, U32 time, U32 axis) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_pointer_axis_discrete(Void *data, struct wl_pointer *pointer, U32 axis, S32 discrete) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}



// NOTE(simon): Keyboard events
internal Void wayland_keyboard_keymap(Void *data, struct wl_keyboard *keyboard, U32 format, S32 fd, U32 size) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
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
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_keyboard_leave(Void *data, struct wl_keyboard *keyboard, U32 serial, struct wl_surface *surface) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_keyboard_key(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 time, U32 key, U32 key_state) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
    Wayland_State *state = &global_wayland_state;
}

internal Void wayland_keyboard_modifiers(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 mods_depressed, U32 mods_latched, U32 mods_locked, U32 group) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
    Wayland_State *state = &global_wayland_state;

    if (state->xkb_state) {
        xkb_state_update_mask(state->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    }
}

internal Void wayland_keyboard_repeat_info(Void *data, struct wl_keyboard *keyboard, S32 rate, S32 delay) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
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



// NOTE(simon): Data device events
internal Void wayland_data_device_data_offer(Void *data, struct wl_data_device *data_device, struct wl_data_offer *id) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_data_device_enter(Void *data, struct wl_data_device *data_device, U32 serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *id) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_data_device_leave(Void *data, struct wl_data_device *data_device) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_data_device_motion(Void *data, struct wl_data_device *data_device, U32 time, wl_fixed_t x, wl_fixed_t y) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_data_device_drop(Void *data, struct wl_data_device *data_device) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}

internal Void wayland_data_device_selection(Void *data, struct wl_data_device *data_device, struct wl_data_offer *id) {
    os_console_print(str8_literal(__FUNCTION__));
    os_console_print(str8_literal("\n"));
}



// NOTE(simon): XDG surface events
internal Void wayland_xdg_surface_configure(Void *data, struct xdg_surface *xdg_surface, U32 serial) {
    Wayland_State *state = &global_wayland_state;
    if (state->resize) {
        state->resize();
    }
    if (state->update) {
        state->update();
    }
    xdg_surface_ack_configure(xdg_surface, serial);
    gfx_swap_buffers();
}



// NOTE(simon): XDG toplevel events
internal Void wayland_xdg_toplevel_configure(Void *data, struct xdg_toplevel *xgd_toplevel, S32 width, S32 height, struct wl_array *states) {
    Wayland_State *state = &global_wayland_state;
    state->width = width;
    state->height = height;
}

internal Void wayland_xdg_toplevel_close(Void *data, struct xdg_toplevel *xdg_toplevel) {
}



// NOTE(simon): Registry events
internal Void wayland_register_global(Void *data, struct wl_registry *registry, U32 name, const char *interface, U32 version) {
    Wayland_State *state = &global_wayland_state;

    os_console_print(str8_cstr((CStr) interface));
    os_console_print(str8_literal("\n"));

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
    state->display = wl_display_connect(0);
    struct wl_registry *registry = wl_display_get_registry(state->display);
    wl_registry_add_listener(registry, &wayland_registry_listener, 0);

    state->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    wl_display_roundtrip(state->display);

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
}

internal Gfx_EventList gfx_get_events(Arena *arena, B32 wait) {
    Wayland_State *state = &global_wayland_state;
    Gfx_EventList events = { 0 };

    // TODO(simon): Respect wait
    // TODO(simon): Error handling
    while (wl_display_prepare_read(state->display) != 0) {
        wl_display_dispatch_pending(state->display);
    }
    wl_display_flush(state->display);
    wl_display_read_events(state->display);
    wl_display_dispatch_pending(state->display);

    return events;
}

internal V2F32 gfx_get_mouse_position(Void) {
    Wayland_State *state = &global_wayland_state;
    V2F32 result = { 0 };
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
}

internal Str8 gfx_get_clipboard_text(Arena *arena) {
    Wayland_State *state = &global_wayland_state;
    Str8 result = { 0 };
    return result;
}
