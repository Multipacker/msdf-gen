#ifndef WAYLAND_INCLUDE_H
#define WAYLAND_INCLUDE_H

#undef global
#include <wayland-client.h>
#include "xdg-shell.h"
#define global static
#include "xdg-decoration-unstable-v1.h"

#include <xkbcommon/xkbcommon.h>

#define VK_USE_PLATFORM_WAYLAND_KHR
#include "../vulkan_include.h"

// TODO: These are very similar names for structs that keep different information. Improve them!
typedef struct WaylandOutput WaylandOutput;
struct WaylandOutput {
    WaylandOutput *next;
    WaylandOutput *previous;

    U32 name;
    struct wl_output *output;

    S32 scale;

    S32 pending_scale;
};

typedef struct WaylandSurfaceOutput WaylandSurfaceOutput;
struct WaylandSurfaceOutput {
    WaylandSurfaceOutput *next;
    WaylandSurfaceOutput *previous;

    struct wl_output *output;
};

typedef enum {
    WAYLAND_POINTER_EVENT_ENTER         = 1 << 0,
    WAYLAND_POINTER_EVENT_LEAVE         = 1 << 1,
    WAYLAND_POINTER_EVENT_MOTION        = 1 << 2,
    WAYLAND_POINTER_EVENT_BUTTON        = 1 << 3,
    WAYLAND_POINTER_EVENT_AXIS          = 1 << 4,
    WAYLAND_POINTER_EVENT_AXIS_SOURCE   = 1 << 5,
    WAYLAND_POINTER_EVENT_AXIS_STOP     = 1 << 6,
    WAYLAND_POINTER_EVENT_AXIS_DISCRETE = 1 << 7,
} WaylandPointerEventMask;

typedef struct {
    WaylandPointerEventMask mask;
    wl_fixed_t surface_x;
    wl_fixed_t surface_y;
    U32 button;
    enum wl_pointer_button_state button_state;
    U32 time;
    U32 serial;
    struct {
        B32 is_valid;
        wl_fixed_t value;
        S32 discrete;
    } axies[2]; // TODO: If wayland-client-protocol.h ever updates to use something like `WL_POINTER_AXIS_COUNT`, use that instead.
    enum wl_pointer_axis_source axis_source;
} WaylandPointerEvent;

typedef struct {
    VulkanState vulkan_state;

    Arena arenas[2];
    U32 arena_index;
    Arena permanent_arena;

    Arena *frame_arena;

    struct wl_display    *display;
    struct wl_registry   *registry;
    struct wl_compositor *compositor;
    struct wl_shm        *shm;
    struct xdg_wm_base   *xdg_wm_base;
    struct zxdg_decoration_manager_v1 *zxdg_decoration_manager;
    struct wl_seat       *seat;
    struct wl_pointer    *wl_pointer;
    WaylandPointerEvent   pending_pointer_event;
    struct wl_keyboard   *wl_keyboard;
    struct xkb_state     *xkb_state;
    struct xkb_context   *xkb_context;
    struct xkb_keymap    *xkb_keymap;

    S32 x, y;
    B32 is_grabbed;
    B32 is_pressed;
    B32 is_released;
    B32 is_right;
    S32 scroll_amount;

    struct wl_surface   *wl_surface;
    struct xdg_surface  *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct zxdg_toplevel_decoration_v1 *zxdg_toplevel_decoration;
    S32 width;
    S32 height;
    S32 surface_scale;
    B32 was_resized;
    B32 has_server_side_decoration; // TODO: Draw client side decorations properly.
    WaylandSurfaceOutput *surface_outputs_first;
    WaylandSurfaceOutput *surface_outputs_last;

    WaylandOutput *all_outputs_first;
    WaylandOutput *all_outputs_last;

    B32 can_draw;
    B32 exit_requested;
} WaylandState;

internal Void wayland_buffer_release(Void *data, struct wl_buffer *buffer);

internal Void wayland_zxdg_toplevel_decoration_handle_configure(Void *data, struct zxdg_toplevel_decoration_v1 *zxdg_toplevel_decoration, U32 mode);

internal Void wayland_xdg_surface_configure(Void *data, struct xdg_surface *xdg_surface, U32 serial);

internal Void wayland_xdg_wm_base_ping(Void *data, struct xdg_wm_base *xdg_wm_base, U32 serial);

internal Void wayland_surface_handle_enter(Void *data, struct wl_surface *wl_surface, struct wl_output *wl_output);
internal Void wayland_surface_handle_leave(Void *data, struct wl_surface *wl_surface, struct wl_output *wl_output);

internal Void wayland_surface_frame_done(Void *data, struct wl_callback *callback, U32 time);

internal Void wayland_xdg_toplevel_handle_configure(Void *data, struct xdg_toplevel *xdg_toplevel, S32 width, S32 height, struct wl_array *states);
internal Void wayland_xdg_toplevel_handle_close(Void *data, struct xdg_toplevel *xdg_toplevel);

internal Void wayland_output_handle_geometry(Void *data, struct wl_output *wl_output, S32 x, S32 y, S32 physical_width, S32 physical_height, S32 subpixel, const char *make, const char *model, S32 transform);
internal Void wayland_output_handle_mode(Void *data, struct wl_output *wl_output, U32 flags, S32 width, S32 height, S32 refresh);
internal Void wayland_output_handle_scale(Void *data, struct wl_output *wl_output, S32 scale);
internal Void wayland_output_handle_done(Void *data, struct wl_output *wl_output);

internal Void wayland_pointer_handle_enter(Void *data, struct wl_pointer *wl_pointer, U32 serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
internal Void wayland_pointer_handle_leave(Void *data, struct wl_pointer *wl_pointer, U32 serial, struct wl_surface *surface);
internal Void wayland_pointer_handle_motion(Void *data, struct wl_pointer *wl_pointer, U32 time, wl_fixed_t surface_x, wl_fixed_t surface_y);
internal Void wayland_pointer_handle_button(Void *data, struct wl_pointer *wl_pointer, U32 serial, U32 time, U32 button, U32 button_state);
internal Void wayland_pointer_handle_axis(Void *data, struct wl_pointer *wl_pointer, U32 time, U32 axis, wl_fixed_t value);
internal Void wayland_pointer_handle_frame(Void *data, struct wl_pointer *wl_pointer);
internal Void wayland_pointer_handle_axis_source(Void *data, struct wl_pointer *wl_pointer, U32 axis_source);
internal Void wayland_pointer_handle_axis_stop(Void *data, struct wl_pointer *wl_pointer, U32 time, U32 axis);
internal Void wayland_pointer_handle_axis_discrete(Void *data, struct wl_pointer *wl_pointer, U32 axis, S32 discrete);

internal Void wayland_keyboard_handle_keymap(Void *data, struct wl_keyboard *keyboard, U32 format, S32 fd, U32 size);
internal Void wayland_keyboard_handle_enter(Void *data, struct wl_keyboard *keyboard, U32 serial, struct wl_surface *surface, struct wl_array *keys);
internal Void wayland_keyboard_handle_leave(Void *data, struct wl_keyboard *keyboard, U32 serial, struct wl_surface *surface);
internal Void wayland_keyboard_handle_key(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 time, U32 key, U32 state);
internal Void wayland_keyboard_handle_modifiers(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 mods_depressed, U32 mods_latched, U32 mods_locked, U32 group);
internal Void wayland_keyboard_handle_repeat_info(Void *data, struct wl_keyboard *keyboard, S32 rate, S32 delay);

internal Void wayland_seat_handle_capabilities(Void *data, struct wl_seat *wl_seat, U32 capabilities);
internal Void wayland_seat_handle_name(Void *data, struct wl_seat *wl_seat, const char *name);

internal Void wayland_registry_handle_global(Void *data, struct wl_registry *registry, U32 name, const char *interface, U32 version);
internal Void wayland_registry_handle_global_remove(Void *data, struct wl_registry *registry, U32 name);

internal Void graphics_test(Void);
#endif // WAYLAND_INCLUDE_H
