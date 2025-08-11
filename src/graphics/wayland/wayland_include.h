#ifndef WAYLAND_INCLUDE_H
#define WAYLAND_INCLUDE_H

#undef global
#include <wayland-client.h>
#define global static
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>

#include "wayland_xdg_shell.generated.h"
#include "wayland_viewporter.generated.h"
#include "wayland_fractional_scale.generated.h"
#include "wayland_xdg_decoration.generated.h"

typedef struct Wayland_Output Wayland_Output;
struct Wayland_Output {
    Wayland_Output *next;
    Wayland_Output *previous;

    U32 name;
    struct wl_output *output;

    // NOTE(simon): Pending state.
    S32 pending_scale;

    // NOTE(simon): Active state.
    S32 scale;
};

typedef struct Wayland_OutputNode Wayland_OutputNode;
struct Wayland_OutputNode {
    Wayland_OutputNode *next;
    Wayland_OutputNode *previous;
    Wayland_Output     *output;
};

typedef struct Wayland_CursorTheme Wayland_CursorTheme;
struct Wayland_CursorTheme {
    Wayland_CursorTheme *next;
    Wayland_CursorTheme *previous;

    F64 scale;

    struct wl_cursor_theme *theme;
    struct wl_buffer *cursors[Gfx_Cursor_COUNT];
    V2S32 hotspots[Gfx_Cursor_COUNT];
    V2S32 sizes[Gfx_Cursor_COUNT];
};

typedef struct Wayland_Surface Wayland_Surface;
struct Wayland_Surface {
    Wayland_Surface *next;
    Wayland_Surface *previous;

    struct wl_surface  *surface;
    struct wp_viewport *viewport;
    struct wp_fractional_scale_v1 *fractional_scale;

    S32 width;
    S32 height;
    F64 scale;
    Wayland_OutputNode *first_output;
    Wayland_OutputNode *last_output;
};

typedef enum {
    Wayland_MimeType_TextUriList   = 1 << 0,
    Wayland_MimeType_TextPlainUtf8 = 1 << 1,
    Wayland_MimeType_Utf8String    = 1 << 2,
} Wayland_MimeType;

typedef struct Wayland_DataOffer Wayland_DataOffer;
struct Wayland_DataOffer {
    Wayland_DataOffer *next;
    Wayland_DataOffer *previous;

    struct wl_data_offer *data_offer;
    enum   wl_data_device_manager_dnd_action source_actions;
    enum   wl_data_device_manager_dnd_action action;
    Wayland_MimeType mime_types;
};

typedef struct Wayland_TitleBarClientArea Wayland_TitleBarClientArea;
struct Wayland_TitleBarClientArea {
    Wayland_TitleBarClientArea *next;
    R2F32 rectangle;
};

typedef struct Wayland_Window Wayland_Window;
struct Wayland_Window {
    Wayland_Window *next;
    Wayland_Window *previous;

    Wayland_Surface     *surface;
    struct xdg_surface  *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration;
    B32 is_maximized;
    B32 is_configured;

    // NOTE(simon): Custom title bars.
    Arena *title_bar_arena;
    B32 has_server_side_decorations;
    F32 title_bar_height;
    F32 border_width;
    Wayland_TitleBarClientArea *first_client_area;
    Wayland_TitleBarClientArea *last_client_area;

    U64 generation;
};

typedef struct Wayland_State Wayland_State;
struct Wayland_State {
    // NOTE(simon): Shared state.
    Arena *arena;
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_data_device_manager *data_device_manager;
    struct wl_shm *shm;
    struct xdg_wm_base *xdg_wm_base;
    struct wp_viewporter *viewporter;
    struct wp_fractional_scale_manager_v1 *fractional_scale_manager;
    struct zxdg_decoration_manager_v1 *xdg_decoration_manager;
    struct xkb_context *xkb_context;
    Wayland_CursorTheme *first_cursor_theme;
    Wayland_CursorTheme *last_cursor_theme;
    Wayland_Surface *surface_freelist;
    Wayland_Surface *first_surface;
    Wayland_Surface *last_surface;
    CStr cursor_theme_name;
    U64 cursor_theme_size;

    // NOTE(simon): Per seat state.
    struct wl_seat *seat;
    U32 previous_capabilities;

    // NOTE(simon): Per seat pointer state.
    struct wl_pointer *pointer;
    Wayland_Surface *pointer_surface;
    V2F32 pointer_axis;
    V2F32 pointer_axis_discrete;
    V2F32 pointer_position;
    U32 pointer_enter_serial;
    Gfx_Cursor pointer_cursor;
    Wayland_Window *pointer_window;
    Wayland_Window *keyboard_window;

    // NOTE(simon): Per seat keyboard state.
    struct wl_keyboard *keyboard;
    struct xkb_keymap  *xkb_keymap;
    struct xkb_state   *xkb_state;
    U32 last_key;
    U64 key_repeat_delay;
    U64 key_repeat_rate;
    S32 key_repeat_fd;

    // NOTE(simon): Per seat data device state.
    struct wl_data_device *data_device;
    Wayland_DataOffer *first_data_offer;
    Wayland_DataOffer *last_data_offer;
    Wayland_DataOffer *data_offer_freelist;
    Wayland_DataOffer *selection_offer;
    struct wl_data_source *selection_source;
    Arena *selection_source_arena;
    Str8   selection_source_str8;
    U32    selection_source_serial;
    Wayland_DataOffer *drag_and_drop_offer;
    V2F32              drag_and_drop_position;
    Wayland_Window *data_device_window;

    // NOTE(simon): Outputs.
    Wayland_Output *output_freelist;
    Wayland_Output *first_output;
    Wayland_Output *last_output;
    Wayland_OutputNode *output_node_freelist;

    // NOTE(simon): Windows.
    Wayland_Window *first_window;
    Wayland_Window *last_window;
    Wayland_Window *window_freelist;
    VoidFunction *update;

    // NOTE(simon): Events.
    Arena        *event_arena;
    Gfx_EventList events;
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

internal Void wayland_registry_global(Void *data, struct wl_registry *registry, U32 name, const char *interface, U32 version);
internal Void wayland_registry_global_remove(Void *data, struct wl_registry *registry, U32 name);

global const struct wl_registry_listener wayland_registry_listener = {
#undef global
    .global = wayland_registry_global,
#define global static
    .global_remove = wayland_registry_global_remove,
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

internal Void wayland_output_geometry(Void *data, struct wl_output *wl_output, S32 x, S32 y, S32 physical_width, S32 physical_height, S32 subpixel, const char *make, const char *model, S32 transform);
internal Void wayland_output_mode(Void *data, struct wl_output *wl_output, U32 flags, S32 width, S32 height, S32 refresh);
internal Void wayland_output_done(Void *data, struct wl_output *wl_output);
internal Void wayland_output_scale(Void *data, struct wl_output *wl_output, S32 factor);
internal Void wayland_output_name(Void *data, struct wl_output *wl_output, const char *name);
internal Void wayland_output_description(Void *data, struct wl_output *wl_output, const char *description);

global const struct wl_output_listener wayland_output_listener = {
    .geometry    = wayland_output_geometry,
    .mode        = wayland_output_mode,
    .done        = wayland_output_done,
    .scale       = wayland_output_scale,
    .name        = wayland_output_name,
    .description = wayland_output_description,
};

internal Void wayland_surface_enter(Void *data, struct wl_surface *wl_surface, struct wl_output *output);
internal Void wayland_surface_leave(Void *data, struct wl_surface *wl_surface, struct wl_output *output);
internal Void wayland_surface_preferred_buffer_scale(Void *data, struct wl_surface *wl_surface, S32 factor);
internal Void wayland_surface_preferred_buffer_transform(Void *data, struct wl_surface *wl_surface, U32 transform);

global const struct wl_surface_listener wayland_surface_listener = {
    .enter = wayland_surface_enter,
    .leave = wayland_surface_leave,
    .preferred_buffer_scale = wayland_surface_preferred_buffer_scale,
    .preferred_buffer_transform = wayland_surface_preferred_buffer_transform,
};

internal Void wayland_fractional_scale_preferred_scale(Void *data, struct wp_fractional_scale_v1 *fractional_scale, U32 scale);

global const struct wp_fractional_scale_v1_listener wayland_fractional_scale_listener = {
    .preferred_scale = wayland_fractional_scale_preferred_scale,
};

internal Void wayland_wakeup_callback_done(Void *data, struct wl_callback *wl_callback, U32 callback_data);

global const struct wl_callback_listener wayland_wakeup_callback_listener = {
    .done = wayland_wakeup_callback_done,
};

#endif // WAYLAND_INCLUDE_H
