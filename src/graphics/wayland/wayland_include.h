#ifndef WAYLAND_INCLUDE_H
#define WAYLAND_INCLUDE_H

#undef global
#include <wayland-client.h>
#define global static
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>

#include "wayland_xdg_shell.generated.h"
#include "wayland_xdg_decoration.generated.h"

typedef struct Wayland_State Wayland_State;
struct Wayland_State {
    // NOTE(simon): Shared state.
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_data_device_manager *data_device_manager;
    struct wl_shm *shm;
    struct xdg_wm_base *xdg_wm_base;
    struct zxdg_decoration_manager_v1 *xdg_decoration_manager;
    struct xkb_context *xkb_context;
    struct wl_cursor_theme *cursor_theme;
    Arena *event_arena;
    // TODO(simon): Maybe have a shared internal arena for events while they
    // are being produced.
    Gfx_EventList events;

    // NOTE(simon): Per seat state.
    struct wl_seat *seat;
    U32 previous_capabilities;

    // NOTE(simon): Per seat pointer state.
    struct wl_pointer *pointer;
    V2F32 pointer_axis;
    V2F32 pointer_axis_discrete;
    V2F32 pointer_position;
    U32 pointer_enter_serial;
    Gfx_Cursor pointer_cursor;

    // NOTE(simon): Per seat keyboard state.
    struct wl_keyboard *keyboard;
    struct xkb_keymap  *xkb_keymap;
    struct xkb_state   *xkb_state;
    Gfx_KeyModifier     modifiers;

    // NOTE(simon): Per seat data device state.
    struct wl_data_device *data_device;
    struct wl_data_offer  *selection_offer;
    struct wl_data_source *selection_source;
    Arena *selection_source_arena;
    Str8   selection_source_str8;
    // TODO(simon): What if we don't have a serial yet? Do what SDL does and
    // wait for a serial and then send it.
    U32    selection_source_serial;
    struct wl_data_offer  *drag_and_drop_offer;

    // NOTE(simon): Per window state.
    // TODO(simon): Track configuration of windows
    S32 width;
    S32 height;
    struct wl_surface   *wl_surface;
    struct xdg_surface  *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration;
    VoidFunction *swap_buffers;
    VoidFunction *resize;
    VoidFunction *update;
};

// NOTE(simon): Forward declaration of all event listeners.

internal Void wayland_xdg_wm_base_ping(Void *data, struct xdg_wm_base *xdg_wm_base, U32 serial);

global const struct xdg_wm_base_listener wayland_xdg_wm_base_listener = {
    .ping = wayland_xdg_wm_base_ping,
};

internal Void wayland_pointer_enter(Void *data, struct wl_pointer *pointer, U32 serial, struct wl_surface *surface, wl_fixed_t suraface_x, wl_fixed_t surface_y);
internal Void wayland_pointer_leave(Void *data, struct wl_pointer *pointer, U32 serial, struct wl_surface *surface);
internal Void wayland_pointer_motion(Void *data, struct wl_pointer *pointer, U32 time, wl_fixed_t surface_x, wl_fixed_t surface_y);
internal Void wayland_pointer_button(Void *data, struct wl_pointer *pointer, U32 serial, U32 time, U32 button, U32 state);
internal Void wayland_pointer_axis(Void *data, struct wl_pointer *pointer, U32 time, U32 axis, wl_fixed_t value);
internal Void wayland_pointer_frame(Void *data, struct wl_pointer *pointer);
internal Void wayland_pointer_axis_source(Void *data, struct wl_pointer *pointer, U32 axis_source);
internal Void wayland_pointer_axis_stop(Void *data, struct wl_pointer *pointer, U32 time, U32 axis);
internal Void wayland_pointer_axis_discrete(Void *data, struct wl_pointer *pointer, U32 axis, S32 discrete);

global const struct wl_pointer_listener wayland_pointer_listener = {
    .enter         = wayland_pointer_enter,
    .leave         = wayland_pointer_leave,
    .motion        = wayland_pointer_motion,
    .button        = wayland_pointer_button,
    .axis          = wayland_pointer_axis,
    .frame         = wayland_pointer_frame,
    .axis_source   = wayland_pointer_axis_source,
    .axis_stop     = wayland_pointer_axis_stop,
    .axis_discrete = wayland_pointer_axis_discrete,
};

internal Void wayland_keyboard_keymap(Void *data, struct wl_keyboard *keyboard, U32 format, S32 fd, U32 size);
internal Void wayland_keyboard_enter(Void *data, struct wl_keyboard *keyboard, U32 serial, struct wl_surface *surface, struct wl_array *keys);
internal Void wayland_keyboard_leave(Void *data, struct wl_keyboard *keyboard, U32 serial, struct wl_surface *surface);
internal Void wayland_keyboard_key(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 time, U32 key, U32 key_state);
internal Void wayland_keyboard_modifiers(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 mods_depressed, U32 mods_latched, U32 mods_locked, U32 group);
internal Void wayland_keyboard_repeat_info(Void *data, struct wl_keyboard *keyboard, S32 rate, S32 delay);

global const struct wl_keyboard_listener wayland_keyboard_listener = {
    .keymap      = wayland_keyboard_keymap,
    .enter       = wayland_keyboard_enter,
    .leave       = wayland_keyboard_leave,
    .key         = wayland_keyboard_key,
    .modifiers   = wayland_keyboard_modifiers,
    .repeat_info = wayland_keyboard_repeat_info,
};

internal Void wayland_seat_capabilities(Void *data, struct wl_seat *seat, U32 capabilities);
internal Void wayland_seat_name(Void *data, struct wl_seat *seat, const char *name);

global const struct wl_seat_listener wayland_seat_listener = {
    .capabilities = wayland_seat_capabilities,
    .name         = wayland_seat_name,
};

internal Void wayland_data_device_data_offer(Void *data, struct wl_data_device *data_device, struct wl_data_offer *id);
internal Void wayland_data_device_enter(Void *data, struct wl_data_device *data_device, U32 serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *id);
internal Void wayland_data_device_leave(Void *data, struct wl_data_device *data_device);
internal Void wayland_data_device_motion(Void *data, struct wl_data_device *data_device, U32 time, wl_fixed_t x, wl_fixed_t y);
internal Void wayland_data_device_drop(Void *data, struct wl_data_device *data_device);
internal Void wayland_data_device_selection(Void *data, struct wl_data_device *data_device, struct wl_data_offer *id);

global const struct wl_data_device_listener wayland_data_device_listener = {
    .data_offer = wayland_data_device_data_offer,
    .enter      = wayland_data_device_enter,
    .leave      = wayland_data_device_leave,
    .motion     = wayland_data_device_motion,
    .drop       = wayland_data_device_drop,
    .selection  = wayland_data_device_selection,
};

internal Void wayland_register_global(Void *data, struct wl_registry *registry, U32 name, const char *interface, U32 version);
internal Void wayland_register_global_remove(Void *data, struct wl_registry *registry, U32 name);

global const struct wl_registry_listener wayland_registry_listener = {
#undef global
    .global = wayland_register_global,
#define global static
    .global_remove = wayland_register_global_remove,
};

internal Void wayland_xdg_surface_configure(Void *data, struct xdg_surface *xdg_surface, U32 serial);

global const struct xdg_surface_listener wayland_xdg_surface_listener = {
    .configure = wayland_xdg_surface_configure,
};

internal Void wayland_xdg_toplevel_configure(Void *data, struct xdg_toplevel *xgd_toplevel, S32 width, S32 height, struct wl_array *states);
internal Void wayland_xdg_toplevel_close(Void *data, struct xdg_toplevel *xdg_toplevel);

global const struct xdg_toplevel_listener wayland_xdg_toplevel_listener = {
    .configure = wayland_xdg_toplevel_configure,
    .close     = wayland_xdg_toplevel_close,
};

internal Void wayland_buffer_release(Void *data, struct wl_buffer *buffer);

global const struct wl_buffer_listener wayland_buffer_listener = {
    .release = wayland_buffer_release,
};

internal Void wayland_data_offer_offer(Void *data, struct wl_data_offer *wl_data_offer, const char *mime_type);
internal Void wayland_data_offer_source_actions(Void *data, struct wl_data_offer *wl_data_offer, U32 source_actions);
internal Void wayland_data_offer_action(Void *data, struct wl_data_offer *wl_data_offer, U32 dnd_action);

global const struct wl_data_offer_listener wayland_data_offer_listener = {
    .offer          = wayland_data_offer_offer,
    .source_actions = wayland_data_offer_source_actions,
    .action         = wayland_data_offer_action,
};

internal Void wayland_data_source_target(Void *data, struct wl_data_source *data_source, const char *mime_type);
internal Void wayland_data_source_send(Void *data, struct wl_data_source *data_source, const char *mime_type, S32 fd);
internal Void wayland_data_source_cancelled(Void *data, struct wl_data_source *data_source);
internal Void wayland_data_source_dnd_drop_performed(Void *data, struct wl_data_source *data_source);
internal Void wayland_data_source_dnd_finished(Void *data, struct wl_data_source *data_source);
internal Void wayland_data_source_action(Void *data, struct wl_data_source *data_source, U32 dnd_action);

global const struct wl_data_source_listener wayland_data_source_listener = {
    .target             = wayland_data_source_target,
    .send               = wayland_data_source_send,
    .cancelled          = wayland_data_source_cancelled,
    .dnd_drop_performed = wayland_data_source_dnd_drop_performed,
    .dnd_finished       = wayland_data_source_dnd_finished,
    .action             = wayland_data_source_action,
};

internal Void wayland_xdg_toplevel_decoration_configure(Void *data, struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration, U32 mode);

global const struct zxdg_toplevel_decoration_v1_listener wayland_xdg_toplevel_decoration_listener = {
    .configure = wayland_xdg_toplevel_decoration_configure,
};

#endif // WAYLAND_INCLUDE_H
