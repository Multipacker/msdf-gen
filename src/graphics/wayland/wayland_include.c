#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <linux/input-event-codes.h>

#include "xdg-shell.c"
#include "xdg-decoration-unstable-v1.c"

#include "../sdf_types.h"

#include "../font/geometry.c"
#include "../font/font.c"
#include "../font/msdf.c"
#include "../font/ttf.c"

#include "../vulkan_include.c"

global struct wl_buffer_listener wayland_buffer_listener = {
    .release = wayland_buffer_release,
};

global struct zxdg_toplevel_decoration_v1_listener wayland_zxdg_toplevel_decoration_listener = {
    .configure = wayland_zxdg_toplevel_decoration_handle_configure,
};

global struct xdg_surface_listener wayland_xdg_surface_listener = {
    .configure = wayland_xdg_surface_configure,
};

global struct xdg_wm_base_listener wayland_xdg_wm_base_listener = {
    .ping = wayland_xdg_wm_base_ping,
};

global struct wl_surface_listener wayland_surface_listener = {
    .enter = wayland_surface_handle_enter,
    .leave = wayland_surface_handle_leave,
};

global struct wl_callback_listener wayland_surface_frame_listener = {
    .done = wayland_surface_frame_done,
};

global struct xdg_toplevel_listener wayland_xdg_toplevel_listener = {
    .configure = wayland_xdg_toplevel_handle_configure,
    .close     = wayland_xdg_toplevel_handle_close,
};

global struct wl_output_listener wayland_output_listener = {
    .geometry = wayland_output_handle_geometry,
    .mode     = wayland_output_handle_mode,
    .scale    = wayland_output_handle_scale,
    .done     = wayland_output_handle_done,
};

global struct wl_pointer_listener wayland_pointer_listener = {
    .enter         = wayland_pointer_handle_enter,
    .leave         = wayland_pointer_handle_leave,
    .motion        = wayland_pointer_handle_motion,
    .button        = wayland_pointer_handle_button,
    .axis          = wayland_pointer_handle_axis,
    .frame         = wayland_pointer_handle_frame,
    .axis_source   = wayland_pointer_handle_axis_source,
    .axis_stop     = wayland_pointer_handle_axis_stop,
    .axis_discrete = wayland_pointer_handle_axis_discrete,
};

global struct wl_keyboard_listener wayland_keyboard_listener = {
    .keymap      = wayland_keyboard_handle_keymap,
    .enter       = wayland_keyboard_handle_enter,
    .leave       = wayland_keyboard_handle_leave,
    .key         = wayland_keyboard_handle_key,
    .modifiers   = wayland_keyboard_handle_modifiers,
    .repeat_info = wayland_keyboard_handle_repeat_info,
};

global struct wl_seat_listener wayland_seat_listener = {
    .capabilities = wayland_seat_handle_capabilities,
    .name         = wayland_seat_handle_name,
};

#undef global
static struct wl_registry_listener wayland_registry_listener = {
    .global        = wayland_registry_handle_global,
    .global_remove = wayland_registry_handle_global_remove,
};
#define global static

internal Void wayland_buffer_release(Void *data, struct wl_buffer *buffer) {
    wl_buffer_destroy(buffer);
}

internal Void wayland_zxdg_toplevel_decoration_handle_configure(Void *data, struct zxdg_toplevel_decoration_v1 *zxdg_toplevel_decoration, U32 mode) {
    WaylandState *state = data;
    state->has_server_side_decoration = (mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

internal Void wayland_update_buffer_scale(WaylandState *state) {
    if (state->wl_surface) {
        S32 scale = 1;

        for (WaylandSurfaceOutput *surface_output = state->surface_outputs_first; surface_output; surface_output = surface_output->next) {
            for (WaylandOutput *output = state->all_outputs_first; output; output = output->next) {
                if (surface_output->output == output->output) {
                    scale = s32_max(scale, output->scale);
                }
            }
        }

        state->was_resized = true;
        state->surface_scale = scale;
        wl_surface_set_buffer_scale(state->wl_surface, scale);
    }
}

internal Void wayland_xdg_surface_configure(Void *data, struct xdg_surface *xdg_surface, U32 serial) {
    WaylandState *state = data;
    xdg_surface_ack_configure(xdg_surface, serial);

    state->can_draw = true;
}

internal Void wayland_xdg_wm_base_ping(Void *data, struct xdg_wm_base *xdg_wm_base, U32 serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

internal Void wayland_surface_frame_done(Void *data, struct wl_callback *callback, U32 time) {
    wl_callback_destroy(callback);

    WaylandState *state = data;
    callback = wl_surface_frame(state->wl_surface);
    wl_callback_add_listener(callback, &wayland_surface_frame_listener, state);

    state->can_draw = true;
}

internal Void wayland_surface_handle_enter(Void *data, struct wl_surface *wl_surface, struct wl_output *wl_output) {
    WaylandState *state = data;

    WaylandSurfaceOutput *output = arena_push_struct_zero(state->frame_arena, WaylandSurfaceOutput);
    output->output = wl_output;
    dll_push_back(state->surface_outputs_first, state->surface_outputs_last, output);

    wayland_update_buffer_scale(state);
}

internal Void wayland_surface_handle_leave(Void *data, struct wl_surface *wl_surface, struct wl_output *wl_output) {
    WaylandState *state = data;

    for (WaylandSurfaceOutput *output = state->surface_outputs_first; output; output = output->next) {
        if (output->output == wl_output) {
            dll_remove(state->surface_outputs_first, state->surface_outputs_last, output);
            break;
        }
    }

    wayland_update_buffer_scale(state);
}

internal Void wayland_xdg_toplevel_handle_configure(Void *data, struct xdg_toplevel *xdg_toplevel, S32 width, S32 height, struct wl_array *states) {
    WaylandState *state = data;

    if (!(width == 0 || height == 0)) {
        state->width = width;
        state->height = height;
        wayland_update_buffer_scale(state);
    }
}

internal Void wayland_xdg_toplevel_handle_close(Void *data, struct xdg_toplevel *xdg_toplevel) {
    WaylandState *state = data;
    state->exit_requested = true;
}

internal Void wayland_output_handle_geometry(Void *data, struct wl_output *wl_output, S32 x, S32 y, S32 physical_width, S32 physical_height, S32 subpixel, const char *make, const char *model, S32 transform) {
}

internal Void wayland_output_handle_mode(Void *data, struct wl_output *wl_output, U32 flags, S32 width, S32 height, S32 refresh) {
}

internal Void wayland_output_handle_scale(Void *data, struct wl_output *wl_output, S32 scale) {
    WaylandState *state = data;

    for (WaylandOutput *output = state->all_outputs_first; output; output = output->next) {
        if (output->output == wl_output) {
            output->pending_scale = scale;
            break;
        }
    }
}

internal Void wayland_output_handle_done(Void *data, struct wl_output *wl_output) {
    WaylandState *state = data;

    for (WaylandOutput *output = state->all_outputs_first; output; output = output->next) {
        if (output->output == wl_output) {
            output->scale = output->pending_scale;
            break;
        }
    }

    wayland_update_buffer_scale(state);
}

internal Void wayland_pointer_handle_enter(Void *data, struct wl_pointer *wl_pointer, U32 serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    WaylandState *state = data;
    state->pending_pointer_event.mask     |= WAYLAND_POINTER_EVENT_ENTER;
    state->pending_pointer_event.serial    = serial;
    state->pending_pointer_event.surface_x = surface_x;
    state->pending_pointer_event.surface_y = surface_y;
}

internal Void wayland_pointer_handle_leave(Void *data, struct wl_pointer *wl_pointer, U32 serial, struct wl_surface *surface) {
    WaylandState *state = data;
    state->pending_pointer_event.mask  |= WAYLAND_POINTER_EVENT_LEAVE;
    state->pending_pointer_event.serial = serial;
}

internal Void wayland_pointer_handle_motion(Void *data, struct wl_pointer *wl_pointer, U32 time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    WaylandState *state = data;
    state->pending_pointer_event.mask     |= WAYLAND_POINTER_EVENT_MOTION;
    state->pending_pointer_event.time      = time;
    state->pending_pointer_event.surface_x = surface_x;
    state->pending_pointer_event.surface_y = surface_y;
}

internal Void wayland_pointer_handle_button(Void *data, struct wl_pointer *wl_pointer, U32 serial, U32 time, U32 button, U32 button_state) {
    WaylandState *state = data;
    state->pending_pointer_event.mask        |= WAYLAND_POINTER_EVENT_BUTTON;
    state->pending_pointer_event.time         = time;
    state->pending_pointer_event.serial       = serial;
    state->pending_pointer_event.button       = button;
    state->pending_pointer_event.button_state = button_state;
}

internal Void wayland_pointer_handle_axis(Void *data, struct wl_pointer *wl_pointer, U32 time, U32 axis, wl_fixed_t value) {
    WaylandState *state = data;

    assert(axis < array_count(state->pending_pointer_event.axies));

    state->pending_pointer_event.mask                |= WAYLAND_POINTER_EVENT_AXIS;
    state->pending_pointer_event.time                 = time;
    state->pending_pointer_event.axies[axis].is_valid = true;
    state->pending_pointer_event.axies[axis].value    = value;
}

internal Void wayland_pointer_handle_frame(Void *data, struct wl_pointer *wl_pointer) {
    WaylandState *state = data;

    if (state->pending_pointer_event.mask & (WAYLAND_POINTER_EVENT_ENTER | WAYLAND_POINTER_EVENT_MOTION)) {
        state->x = wl_fixed_to_double(state->pending_pointer_event.surface_x);
        state->y = wl_fixed_to_double(state->pending_pointer_event.surface_y);
    }

    if (state->pending_pointer_event.mask & WAYLAND_POINTER_EVENT_BUTTON) {
        if (state->pending_pointer_event.button_state == WL_POINTER_BUTTON_STATE_PRESSED) {
            state->is_grabbed |= (state->pending_pointer_event.button == BTN_LEFT);
            state->is_right   |= (state->pending_pointer_event.button == BTN_RIGHT);
            state->is_pressed |= (state->pending_pointer_event.button == BTN_LEFT);
        } else if (state->pending_pointer_event.button_state == WL_POINTER_BUTTON_STATE_RELEASED) {
            state->is_grabbed  &= !(state->pending_pointer_event.button == BTN_LEFT);
            state->is_right    &= !(state->pending_pointer_event.button == BTN_RIGHT);
            state->is_released |=  (state->pending_pointer_event.button == BTN_LEFT);
        }
    }

    if (state->pending_pointer_event.mask & WAYLAND_POINTER_EVENT_AXIS && state->pending_pointer_event.axies[WL_POINTER_AXIS_VERTICAL_SCROLL].is_valid) {
        state->scroll_amount += wl_fixed_to_double(state->pending_pointer_event.axies[WL_POINTER_AXIS_VERTICAL_SCROLL].value);
    }

    memory_zero_struct(&state->pending_pointer_event);
}

internal Void wayland_pointer_handle_axis_source(Void *data, struct wl_pointer *wl_pointer, U32 axis_source) {
    WaylandState *state = data;

    state->pending_pointer_event.mask       |= WAYLAND_POINTER_EVENT_AXIS_SOURCE;
    state->pending_pointer_event.axis_source = axis_source;
}

internal Void wayland_pointer_handle_axis_stop(Void *data, struct wl_pointer *wl_pointer, U32 time, U32 axis) {
    WaylandState *state = data;

    state->pending_pointer_event.mask                |= WAYLAND_POINTER_EVENT_AXIS_STOP;
    state->pending_pointer_event.time                 = time;
    state->pending_pointer_event.axies[axis].is_valid = true;
}

internal Void wayland_pointer_handle_axis_discrete(Void *data, struct wl_pointer *wl_pointer, U32 axis, S32 discrete) {
    WaylandState *state = data;

    state->pending_pointer_event.mask                |= WAYLAND_POINTER_EVENT_AXIS_DISCRETE;
    state->pending_pointer_event.axies[axis].is_valid = true;
    state->pending_pointer_event.axies[axis].discrete = discrete;
}

internal Void wayland_keyboard_handle_keymap(Void *data, struct wl_keyboard *keyboard, U32 format, S32 fd, U32 size) {
    WaylandState *state = data;
    assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

    CStr map_shm = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
    assert(map_shm != MAP_FAILED);

    struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_string(state->xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_shm, size);
    close(fd);

    struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);
    xkb_keymap_unref(state->xkb_keymap);
    xkb_state_unref(state->xkb_state);
    state->xkb_keymap = xkb_keymap;
    state->xkb_state  = xkb_state;
}

internal Void wayland_keyboard_handle_enter(Void *data, struct wl_keyboard *keyboard, U32 serial, struct wl_surface *surface, struct wl_array *keys) {
    WaylandState *state = data;

    Arena_Temporary scratch = arena_get_scratch(0, 0);

    U32 *key;
    wl_array_for_each(key, keys) {
        xkb_keysym_t sym = xkb_state_key_get_one_sym(state->xkb_state, *key + 8);
        U64 sym_size = xkb_keysym_get_name(sym, 0, 0) + 1; // Include the null-terminator.
        CStr sym_memory = arena_push_array(scratch.arena, char, sym_size);
        xkb_keysym_get_name(sym, sym_memory, sym_size);
        Str8 sym_name = str8_cstr(sym_memory);

        U64 key_size = xkb_state_key_get_utf8(state->xkb_state, *key + 8, 0, 0) + 1;
        CStr key_memory = arena_push_array(scratch.arena, char, key_size);
        xkb_state_key_get_utf8(state->xkb_state, *key + 8, key_memory, key_size);
        Str8 key_name = str8_cstr(key_memory);

        os_console_print(sym_name);
        os_console_print(str8_literal(", "));
        os_console_print(key_name);
        os_console_print(str8_literal("\n"));
    }

    arena_end_temporary(scratch);
}

internal Void wayland_keyboard_handle_leave(Void *data, struct wl_keyboard *keyboard, U32 serial, struct wl_surface *surface) {
}

internal Void wayland_keyboard_handle_key(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 time, U32 key, U32 key_state) {
    WaylandState *state = data;

    Arena_Temporary scratch = arena_get_scratch(0, 0);

    xkb_keysym_t sym = xkb_state_key_get_one_sym(state->xkb_state, key + 8);
    U64 sym_size = xkb_keysym_get_name(sym, 0, 0) + 1; // Include the null-terminator.
    CStr sym_memory = arena_push_array(scratch.arena, char, sym_size);
    xkb_keysym_get_name(sym, sym_memory, sym_size);
    Str8 sym_name = str8_cstr(sym_memory);

    U64 key_size = xkb_state_key_get_utf8(state->xkb_state, key + 8, 0, 0) + 1;
    CStr key_memory = arena_push_array(scratch.arena, char, key_size);
    xkb_state_key_get_utf8(state->xkb_state, key + 8, key_memory, key_size);
    Str8 key_name = str8_cstr(key_memory);

    os_console_print(key_state == WL_KEYBOARD_KEY_STATE_PRESSED ? str8_literal("pressed, ") : str8_literal("released, "));
    os_console_print(sym_name);
    os_console_print(str8_literal(", "));
    os_console_print(key_name);
    os_console_print(str8_literal("\n"));

    arena_end_temporary(scratch);
}

internal Void wayland_keyboard_handle_modifiers(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 mods_depressed, U32 mods_latched, U32 mods_locked, U32 group) {
    WaylandState *state = data;
    xkb_state_update_mask(state->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}

internal Void wayland_keyboard_handle_repeat_info(Void *data, struct wl_keyboard *keyboard, S32 rate, S32 delay) {
}

internal Void wayland_seat_handle_capabilities(Void *data, struct wl_seat *wl_seat, U32 capabilities) {
    WaylandState *state = data;

    B32 has_pointer  = capabilities & WL_SEAT_CAPABILITY_POINTER;
    B32 has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
    // TODO: Maybe add support for touch.

    if (has_pointer && !state->wl_pointer) {
        state->wl_pointer = wl_seat_get_pointer(wl_seat);
        wl_pointer_add_listener(state->wl_pointer, &wayland_pointer_listener, state);
    } else if (!has_pointer && state->wl_pointer) {
        wl_pointer_release(state->wl_pointer);
        state->wl_pointer = 0;
    }

    if (has_keyboard && !state->wl_keyboard) {
        state->wl_keyboard = wl_seat_get_keyboard(wl_seat);
        wl_keyboard_add_listener(state->wl_keyboard, &wayland_keyboard_listener, state);
    } else if (!has_keyboard && state->wl_keyboard) {
        wl_keyboard_release(state->wl_keyboard);
        state->wl_keyboard = 0;
    }
}

internal Void wayland_seat_handle_name(Void *data, struct wl_seat *wl_seat, const char *name) {
}

internal Void wayland_registry_handle_global(Void *data, struct wl_registry *registry, U32 name, const char *interface, U32 version) {
    WaylandState *state = data;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base, &wayland_xdg_wm_base_listener, state);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        WaylandOutput *output = arena_push_struct_zero(state->frame_arena, WaylandOutput);
        output->name = name;
        output->output = wl_registry_bind(registry, name, &wl_output_interface, 3);
        wl_output_add_listener(output->output, &wayland_output_listener, state);
        dll_push_back(state->all_outputs_first, state->all_outputs_last, output);
        wayland_update_buffer_scale(state);
    } else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
        state->zxdg_decoration_manager = wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        state->seat = wl_registry_bind(registry, name, &wl_seat_interface, 7);
        wl_seat_add_listener(state->seat, &wayland_seat_listener, state);
    }
}

internal Void wayland_registry_handle_global_remove(Void *data, struct wl_registry *registry, U32 name) {
    WaylandState *state = data;

    for (WaylandOutput *output = state->all_outputs_first; output; output = output->next) {
        if (output->name == name) {
            // TODO: What it the correct function? wl_output_destroy or wl_output_release? This affects the protocol version.
            wl_output_destroy(output->output);
            dll_remove(state->all_outputs_first, state->all_outputs_last, output);
            wayland_update_buffer_scale(state);
            break;
        }
    }
}

internal GraphicsContext *graphics_create(Str8 title, U32 width, U32 height, FontDescription *font_description) {
    Arena permanent_arena = arena_create();

    GraphicsContext *context = arena_push_struct_zero(&permanent_arena, GraphicsContext);
    WaylandState    *state   = arena_push_struct_zero(&permanent_arena, WaylandState);

    state->permanent_arena = permanent_arena;

    context->pointer = state;
    context->arena = &state->permanent_arena;

    for (U32 i = 0; i < array_count(state->arenas); ++i) {
        state->arenas[i] = arena_create();
    }

    context->frame_arena = &state->arenas[state->arena_index];
    state->frame_arena = &state->arenas[state->arena_index];
    state->vulkan_state.frame_arena = &state->arenas[state->arena_index];
    state->width = width;
    state->height = height;

    state->display = wl_display_connect(0);
    if (!state->display) {
        error_emit(str8_literal("ERROR(graphics/wayland): Failed to connect to display."));
        os_exit(1);
    }

    state->registry = wl_display_get_registry(state->display);
    if (!state->registry) {
        error_emit(str8_literal("ERROR(graphics/wayland): Failed to get registry."));
        os_exit(1);
    }

    state->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    wl_registry_add_listener(state->registry, &wayland_registry_listener, state);

    wl_display_roundtrip(state->display);

    state->wl_surface = wl_compositor_create_surface(state->compositor);
    state->xdg_surface = xdg_wm_base_get_xdg_surface(state->xdg_wm_base, state->wl_surface);
    wl_surface_add_listener(state->wl_surface, &wayland_surface_listener, state);
    xdg_surface_add_listener(state->xdg_surface, &wayland_xdg_surface_listener, state);
    state->xdg_toplevel = xdg_surface_get_toplevel(state->xdg_surface);
    xdg_toplevel_add_listener(state->xdg_toplevel, &wayland_xdg_toplevel_listener, state);

    {
        Arena_Temporary scratch = arena_get_scratch(0, 0);
        CStr cstr_title = cstr_from_str8(scratch.arena, title);
        xdg_toplevel_set_title(state->xdg_toplevel, cstr_title);
        arena_end_temporary(scratch);
    }

    if (state->zxdg_decoration_manager) {
        state->zxdg_toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(state->zxdg_decoration_manager, state->xdg_toplevel);
        zxdg_toplevel_decoration_v1_add_listener(state->zxdg_toplevel_decoration, &wayland_zxdg_toplevel_decoration_listener, state);
        zxdg_toplevel_decoration_v1_set_mode(state->zxdg_toplevel_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }
    wl_surface_commit(state->wl_surface);

    struct wl_callback *callback = wl_surface_frame(state->wl_surface);
    wl_callback_add_listener(callback, &wayland_surface_frame_listener, state);

    vulkan_create_instance(&state->vulkan_state);

    VkWaylandSurfaceCreateInfoKHR create_info = { 0 };
    create_info.sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    create_info.display = state->display;
    create_info.surface = state->wl_surface;

    VULKAN_RESULT_CHECK(vkCreateWaylandSurfaceKHR(state->vulkan_state.instance, &create_info, 0, &state->vulkan_state.surface));

    vulkan_initialize(context->arena, &state->vulkan_state, state->width, state->height, font_description);

    return context;
}

internal Void graphics_destroy(GraphicsContext *context) {
    WaylandState *state = (WaylandState *) context->pointer;

    vulkan_cleanup(&state->vulkan_state);

    wl_display_disconnect(state->display);

    for (U32 i = 0; i < array_count(state->arenas); ++i) {
        arena_destroy(&state->arenas[i]);
    }

    arena_destroy(&state->permanent_arena);
}

internal Void graphics_begin_frame(GraphicsContext *context) {
    WaylandState *state = (WaylandState *) context->pointer;

    state->arena_index = (state->arena_index + 1) % array_count(state->arenas);
    state->frame_arena = &state->arenas[state->arena_index];
    state->vulkan_state.frame_arena = &state->arenas[state->arena_index];
    arena_pop_to(state->frame_arena, 0);

    Vulkan_Frame *frame = &state->vulkan_state.frames[state->vulkan_state.current_frame_index];

    VULKAN_RESULT_CHECK(vkWaitForFences(state->vulkan_state.device, 1, &frame->in_flight_fence, VK_TRUE, U64_MAX));

    state->is_pressed = false;
    state->is_released = false;
    state->scroll_amount = 0;
    state->exit_requested = false;

    VkResult begin_frame_result = VK_SUCCESS;
    do {
        while (wl_display_dispatch(state->display) != -1 && !state->can_draw);

        if (begin_frame_result == VK_ERROR_OUT_OF_DATE_KHR || state->was_resized) {
            state->was_resized = false;
            state->vulkan_state.surface_scale = state->surface_scale;
            vulkan_recreate_swapchain(&state->vulkan_state, state->width * state->surface_scale, state->height * state->surface_scale);
        }

        // TODO: Assert on error
        begin_frame_result = vulkan_begin_frame(&state->vulkan_state);
    } while (begin_frame_result != VK_SUCCESS && begin_frame_result != VK_SUBOPTIMAL_KHR);

    state->can_draw = false;

    WaylandOutput *old_first = state->all_outputs_first;
    state->all_outputs_first = 0;
    state->all_outputs_last  = 0;
    for (WaylandOutput *old_output = old_first; old_output; old_output = old_output->next) {
        WaylandOutput *new_output = arena_push_struct_zero(state->frame_arena, WaylandOutput);
        new_output->name          = old_output->name;
        new_output->output        = old_output->output;
        new_output->scale         = old_output->scale;
        new_output->pending_scale = old_output->pending_scale;
        dll_push_back(state->all_outputs_first, state->all_outputs_last, new_output);
    }

    WaylandSurfaceOutput *old_surface_first = state->surface_outputs_first;
    state->surface_outputs_first = 0;
    state->surface_outputs_last  = 0;
    for (WaylandSurfaceOutput *old_output = old_surface_first; old_output; old_output = old_output->next) {
        WaylandSurfaceOutput *new_output = arena_push_struct_zero(state->frame_arena, WaylandSurfaceOutput);
        new_output->output = old_output->output;
        dll_push_back(state->surface_outputs_first, state->surface_outputs_last, new_output);
    }

    context->width = state->width;
    context->height = state->height;
    context->x = state->x;
    context->y = state->y;
    context->is_grabbed = state->is_grabbed;
    context->is_pressed = state->is_pressed;
    context->is_released = state->is_released;
    context->is_right = state->is_right;
    context->scroll_amount = state->scroll_amount;
    context->exit_requested = state->exit_requested;
}

internal Void graphics_end_frame(GraphicsContext *context) {
    WaylandState *state = (WaylandState *) context->pointer;

    vulkan_end_frame(&state->vulkan_state);
}
