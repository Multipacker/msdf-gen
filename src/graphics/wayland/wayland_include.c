#include <poll.h>
#include <string.h>

#include <sys/timerfd.h>

#include <linux/input-event-codes.h>

#include "wayland_xdg_shell.generated.c"
#include "wayland_viewporter.generated.c"
#include "wayland_fractional_scale.generated.c"
#include "wayland_xdg_decoration.generated.c"

global Wayland_State global_wayland_state;

typedef enum {
    UriFlag_HasAuthority = 1 << 0,
    UriFlag_HasQuery     = 1 << 1,
    UriFlag_HasFragment  = 1 << 2,
} UriFlags;

typedef struct Uri Uri;
struct Uri {
    UriFlags flags;

    Str8 scheme;
    Str8 authority;
    Str8 path;
    Str8 query;
    Str8 fragment;
};

// TODO(simon): There is a lot more to do here, but this will do for a *very*
// basic implementation.
internal Uri uri_from_string(Str8 string) {
    Uri uri = { 0 };

    // NOTE(simon): Scheme
    {
        U64 colon_index = str8_first_index_of(string, ':');
        uri.scheme = str8_prefix(string, colon_index);
        string = str8_skip(string, colon_index + 1);
    }

    // TODO(simon): Validation of shceme according to https://datatracker.ietf.org/doc/html/rfc3986#section-3.1

    // NOTE(simon): Hier-part

    // NOTE(simon): Authority
    if (string.size >= 2 && string.data[0] == '/' && string.data[1] == '/') {
        string = str8_skip(string, 2);
        uri.flags |= UriFlag_HasAuthority;

        U64 slash_index         = str8_first_index_of(string, '/');
        U64 question_mark_index = str8_first_index_of(string, '?');
        U64 number_sign_index   = str8_first_index_of(string, '#');
        U64 authority_end       = u64_min(u64_min(slash_index, question_mark_index), number_sign_index);

        uri.authority = str8_prefix(string, authority_end);
        string = str8_skip(string, authority_end);

        // TODO(simon): Validate according to https://datatracker.ietf.org/doc/html/rfc3986#section-3.2
    }

    // NOTE(simon): Path
    {
        U64 question_mark_index = str8_first_index_of(string, '?');
        U64 number_sign_index   = str8_first_index_of(string, '#');
        U64 path_end            = u64_min(question_mark_index, number_sign_index);

        uri.path = str8_prefix(string, path_end);
        string = str8_skip(string, path_end);

        // TODO(simon): Validation according to https://datatracker.ietf.org/doc/html/rfc3986#section-3.3
    }

    // NOTE(simon): Query
    if (string.size >= 1 && string.data[0] == '?') {
        string = str8_skip(string, 1);
        uri.flags |= UriFlag_HasQuery;

        U64 number_sign_index = str8_first_index_of(string, '#');
        uri.query = str8_prefix(string, number_sign_index);
        string = str8_skip(string, number_sign_index);

        // TODO(simon): Validation according to https://datatracker.ietf.org/doc/html/rfc3986#section-3.4
    }

    // NOTE(simon): Fragment
    if (string.size >= 1 && string.data[0] == '#') {
        string = str8_skip(string, 1);
        uri.flags |= UriFlag_HasFragment;

        uri.fragment = string;

        // TODO(simon): Validation according to https://datatracker.ietf.org/doc/html/rfc3986#section-3.5
    }

    return uri;
}



internal Void wayland_update_cursor(Void) {
    Wayland_State *state = &global_wayland_state;

    if (!state->pointer_surface) {
        return;
    }

    Wayland_CursorTheme *theme = state->first_cursor_theme;
    while (theme && theme->scale != state->pointer_surface->scale) {
        theme = theme->next;
    }

    if (!theme) {
        theme = arena_push_struct(state->arena, Wayland_CursorTheme);
        theme->scale = state->pointer_surface->scale;
        theme->theme = wl_cursor_theme_load(state->cursor_theme_name, (S32) f64_ceil(state->pointer_surface->scale * (F64) state->cursor_theme_size), state->shm);
        dll_push_back(state->first_cursor_theme, state->last_cursor_theme, theme);
    }

    if (!theme->cursors[state->pointer_cursor]) {
        CStr names[] = {
            [Gfx_Cursor_Pointer]  = "default",
            [Gfx_Cursor_Hand]     = "pointer",
            [Gfx_Cursor_Beam]     = "text",
            [Gfx_Cursor_SizeNWSE] = "nwse-resize",
            [Gfx_Cursor_SizeNESW] = "nesw-resize",
            [Gfx_Cursor_SizeWE]   = "ew-resize",
            [Gfx_Cursor_SizeNS]   = "ns-resize",
            [Gfx_Cursor_SizeAll]  = "all-scroll",
            [Gfx_Cursor_Disabled] = "not-allowd",
        };

        struct wl_cursor *theme_cursor = wl_cursor_theme_get_cursor(theme->theme, names[state->pointer_cursor]);
        if (theme_cursor && theme_cursor->image_count > 0) {
            struct wl_cursor_image *image = theme_cursor->images[0];
            theme->cursors[state->pointer_cursor]  = wl_cursor_image_get_buffer(image);
            theme->hotspots[state->pointer_cursor] = v2s32((S32) f64_ceil((F64) image->hotspot_x / theme->scale), (S32) f64_ceil((F64) image->hotspot_y / theme->scale));
            theme->sizes[state->pointer_cursor]    = v2s32((S32) image->width, (S32) image->height);
        }
    }

    if (theme->cursors[state->pointer_cursor]) {
        wl_surface_attach(state->pointer_surface->surface, theme->cursors[state->pointer_cursor], 0, 0);
        wl_surface_damage_buffer(state->pointer_surface->surface, 0, 0, S32_MAX, S32_MAX);

        if (state->pointer_surface->viewport && state->pointer_surface->fractional_scale) {
            wp_viewport_set_source(
                state->pointer_surface->viewport,
                wl_fixed_from_int(0),
                wl_fixed_from_int(0),
                wl_fixed_from_double((F64) theme->sizes[state->pointer_cursor].width),
                wl_fixed_from_double((F64) theme->sizes[state->pointer_cursor].height)
            );
            wp_viewport_set_destination(
                state->pointer_surface->viewport,
                (S32) f64_ceil(theme->sizes[state->pointer_cursor].width  / state->pointer_surface->scale),
                (S32) f64_ceil(theme->sizes[state->pointer_cursor].height / state->pointer_surface->scale)
            );
            wl_surface_set_buffer_scale(state->pointer_surface->surface, 1);
        } else {
            wl_surface_set_buffer_scale(state->pointer_surface->surface, (S32) f64_ceil(state->pointer_surface->scale));
        }

        wl_surface_commit(state->pointer_surface->surface);
        wl_pointer_set_cursor(
            state->pointer,
            state->pointer_enter_serial,
            state->pointer_surface->surface,
            theme->hotspots[state->pointer_cursor].x,
            theme->hotspots[state->pointer_cursor].y
        );
    }
}

internal Void wayland_set_selection(Void) {
    Wayland_State *state = &global_wayland_state;

    if (state->selection_source_serial) {
        state->selection_source = wl_data_device_manager_create_data_source(state->data_device_manager);
        wl_data_source_add_listener(state->selection_source, &wayland_data_source_listener, 0);
        // TODO(simon): Look at availible mime types.
        wl_data_source_offer(state->selection_source, "text/plain;charset=utf-8");
        wl_data_source_offer(state->selection_source, "UTF8_STRING");
        wl_data_device_set_selection(state->data_device, state->selection_source, state->selection_source_serial);
    }
}

internal Void wayland_update_selection_serial(U32 serial) {
    Wayland_State *state = &global_wayland_state;

    state->selection_source_serial = serial;

    if (!state->selection_source && state->selection_source_str8.size) {
        wayland_set_selection();
    }
}

internal Void wayland_handle_key(U32 key, U32 key_state) {
    Wayland_State *state = &global_wayland_state;

    if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        U32 codepoint = xkb_state_key_get_utf32(state->xkb_state, key);

        B32 is_c0_control = codepoint <= 0x1F || codepoint == 0x7F;
        B32 is_c1_control = (0x80 <= codepoint && codepoint <= 0x9F);

        if (!is_c0_control && !is_c1_control) {
            Gfx_Event *event = arena_push_struct(state->event_arena, Gfx_Event);
            event->kind = Gfx_EventKind_Text;
            event->text.data = arena_push_array_no_zero(state->event_arena, U8, 4);
            event->text.size = string_encode_utf8(event->text.data, codepoint);
            dll_push_back(state->events.first, state->events.last, event);
        }
    }

    xkb_keysym_t *keysyms = 0;
    int keysym_count = xkb_keymap_key_get_syms_by_level(state->xkb_keymap, key, 0, 0, (const xkb_keysym_t **) &keysyms);
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
            Gfx_Event *event = arena_push_struct(state->event_arena, Gfx_Event);
            event->kind = (key_state == WL_KEYBOARD_KEY_STATE_PRESSED ? Gfx_EventKind_KeyPress : Gfx_EventKind_KeyRelease);
            event->key  = event_key;
            event->key_modifiers |= (xkb_state_mod_name_is_active(state->xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0 ? Gfx_KeyModifier_Shift   : 0);
            event->key_modifiers |= (xkb_state_mod_name_is_active(state->xkb_state, XKB_MOD_NAME_CTRL,  XKB_STATE_MODS_EFFECTIVE) > 0 ? Gfx_KeyModifier_Control : 0);
            dll_push_back(state->events.first, state->events.last, event);
        }
    }
}

internal Void wayland_update_surface_scale(Wayland_Surface *surface) {
    S32 scale = 1;
    for (Wayland_OutputNode *node = surface->first_output; node; node = node->next) {
        scale = s32_max(scale, node->output->scale);
    }

    if (wl_surface_get_version(surface->surface) != 6 && !(surface->viewport && surface->fractional_scale)) {
        surface->scale = (F64) scale;
    }
}

internal Wayland_Surface *wayland_surface_create(Void) {
    Wayland_State *state = &global_wayland_state;

    Wayland_Surface *surface = state->surface_freelist;
    if (surface) {
        sll_stack_pop(state->surface_freelist);
        memory_zero_struct(surface);
    } else {
        surface = arena_push_struct(state->arena, Wayland_Surface);
    }

    surface->surface = wl_compositor_create_surface(state->compositor);
    wl_surface_add_listener(surface->surface, &wayland_surface_listener, surface);
    surface->scale = 1.0;

    if (state->viewporter && state->fractional_scale_manager) {
        surface->viewport         = wp_viewporter_get_viewport(state->viewporter, surface->surface);
        surface->fractional_scale = wp_fractional_scale_manager_v1_get_fractional_scale(state->fractional_scale_manager, surface->surface);
        wp_fractional_scale_v1_add_listener(surface->fractional_scale, &wayland_fractional_scale_listener, surface);
    }

    dll_push_back(state->first_surface, state->last_surface, surface);

    return surface;
}

internal Void wayland_surface_destroy(Wayland_Surface *surface) {
    Wayland_State *state = &global_wayland_state;

    for (Wayland_OutputNode *node = surface->first_output, *next = 0; node; node = next) {
        next = node->next;
        dll_remove(surface->first_output, surface->last_output, node);
        sll_stack_push(state->output_node_freelist, node);
    }

    wl_surface_destroy(surface->surface);
    dll_remove(state->first_surface, state->last_surface, surface);
    sll_stack_push(state->surface_freelist, surface);
}

internal Str8 wayland_data_offer_receive(Arena *arena, Wayland_DataOffer *data_offer, CStr mime_type) {
    Wayland_State *state = &global_wayland_state;
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    Str8List segments = { 0 };
    int file_descriptors[2] = { 0 };

    if (pipe(file_descriptors) != -1) {
        wl_data_offer_receive(data_offer->data_offer, mime_type, file_descriptors[1]);
        wl_display_flush(state->display);

        // NOTE(simon): Close the write file descriptor as we are done with
        // it on our side.
        close(file_descriptors[1]);

        size_t buffer_capacity = (size_t) s64_min(1 << 16, (S64) SSIZE_MAX);

        for (;;) {
            U8 *buffer = arena_push_array(scratch.arena, U8, buffer_capacity);
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

    Str8 result = str8_join(arena, &segments);

    arena_end_temporary(scratch);
    return result;
}

internal Void wayland_data_offer_destroy(Wayland_DataOffer *data_offer) {
    Wayland_State *state = &global_wayland_state;

    wl_data_offer_destroy(data_offer->data_offer);
    dll_remove(state->first_data_offer, state->last_data_offer, data_offer);
    sll_stack_push(state->data_offer_freelist, data_offer);
}



// NOTE(simon): Wakeup callback events
internal Void wayland_wakeup_callback_done(Void *data, struct wl_callback *wl_callback, U32 callback_data) {
    Wayland_State *state = &global_wayland_state;

    Gfx_Event *event = arena_push_struct(state->event_arena, Gfx_Event);
    event->kind = Gfx_EventKind_Wakeup;
    dll_push_back(state->events.first, state->events.last, event);
}



// NOTE(simon): XDG WM base events
internal Void wayland_xdg_wm_base_ping(Void *data, struct xdg_wm_base *xdg_wm_base, U32 serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}



// NOTE(simon): Pointer events
internal Void wayland_pointer_enter(Void *data, struct wl_pointer *pointer, U32 serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    Wayland_State *state = &global_wayland_state;
    state->pointer_enter_serial = serial;

    state->pointer_position = v2f32(
        (F32) (wl_fixed_to_double(surface_x) * state->surface->scale),
        (F32) (wl_fixed_to_double(surface_y) * state->surface->scale)
    );

    wayland_update_cursor();
    wayland_update_selection_serial(serial);
}

internal Void wayland_pointer_leave(Void *data, struct wl_pointer *pointer, U32 serial, struct wl_surface *surface) {
    Wayland_State *state = &global_wayland_state;
    wayland_update_selection_serial(serial);
}

internal Void wayland_pointer_motion(Void *data, struct wl_pointer *pointer, U32 time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    Wayland_State *state = &global_wayland_state;

    state->pointer_position = v2f32(
        (F32) (wl_fixed_to_double(surface_x) * state->surface->scale),
        (F32) (wl_fixed_to_double(surface_y) * state->surface->scale)
    );

    Gfx_Event *event = arena_push_struct(state->event_arena, Gfx_Event);
    event->kind = Gfx_EventKind_MouseMove;
    event->position = state->pointer_position;
    dll_push_back(state->events.first, state->events.last, event);
}

internal Void wayland_pointer_button(Void *data, struct wl_pointer *pointer, U32 serial, U32 time, U32 button, U32 button_state) {
    Wayland_State *state = &global_wayland_state;
    wayland_update_selection_serial(serial);

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
        Gfx_Event *event = arena_push_struct(state->event_arena, Gfx_Event);
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
        Gfx_Event *event = arena_push_struct(state->event_arena, Gfx_Event);
        event->kind     = Gfx_EventKind_Scroll;
        event->scroll   = state->pointer_axis_discrete;
        event->position = state->pointer_position;
        dll_push_back(state->events.first, state->events.last, event);

        memory_zero_struct(&state->pointer_axis);
        memory_zero_struct(&state->pointer_axis_discrete);
    }

    if (state->pointer_axis.x != 0.0f || state->pointer_axis.y != 0.0f) {
        Gfx_Event *event = arena_push_struct(state->event_arena, Gfx_Event);
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
    wayland_update_selection_serial(serial);
}

internal Void wayland_keyboard_leave(Void *data, struct wl_keyboard *keyboard, U32 serial, struct wl_surface *surface) {
    Wayland_State *state = &global_wayland_state;
    wayland_update_selection_serial(serial);
    state->last_key = 0;
    struct itimerspec timer = { 0 };
    timerfd_settime(state->key_repeat_fd, 0, &timer, 0);
}

internal Void wayland_keyboard_key(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 time, U32 key, U32 key_state) {
    Wayland_State *state = &global_wayland_state;
    wayland_update_selection_serial(serial);

    struct itimerspec timer = { 0 };

    U32 xkb_key = 8 + key;
    if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        state->last_key = xkb_key;

        if (state->key_repeat_rate) {
            if (state->key_repeat_rate > 1) {
                timer.it_interval.tv_nsec = 1000000000 / state->key_repeat_rate;
            } else {
                timer.it_interval.tv_sec = 1;
            }

            timer.it_value.tv_sec  = state->key_repeat_delay / 1000;
            timer.it_value.tv_nsec = (state->key_repeat_delay % 1000) * 1000000;
        }
    } else if (key_state == WL_KEYBOARD_KEY_STATE_RELEASED) {
        if (state->last_key == xkb_key) {
            state->last_key = 0;
        }
    }

    timerfd_settime(state->key_repeat_fd, 0, &timer, 0);
    wayland_handle_key(xkb_key, key_state);
}

internal Void wayland_keyboard_modifiers(Void *data, struct wl_keyboard *keyboard, U32 serial, U32 mods_depressed, U32 mods_latched, U32 mods_locked, U32 group) {
    Wayland_State *state = &global_wayland_state;
    wayland_update_selection_serial(serial);

    if (state->xkb_state) {
        xkb_state_update_mask(state->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    }
}

internal Void wayland_keyboard_repeat_info(Void *data, struct wl_keyboard *keyboard, S32 rate, S32 delay) {
    Wayland_State *state = &global_wayland_state;
    state->key_repeat_rate  = (U64) rate;
    state->key_repeat_delay = (U64) delay;
}



// NOTE(simon): Seat events
internal Void wayland_seat_capabilities(Void *data, struct wl_seat *seat, U32 capabilities) {
    Wayland_State *state = &global_wayland_state;

    U32 changed_capabilities = state->previous_capabilities ^ capabilities;
    U32 removed_capabilities = changed_capabilities & ~capabilities;
    U32 added_capabilities   = changed_capabilities &  capabilities;

    state->previous_capabilities = capabilities;

    if (removed_capabilities & WL_SEAT_CAPABILITY_POINTER) {
        wayland_surface_destroy(state->pointer_surface);
        wl_pointer_release(state->pointer);
        state->pointer_surface = 0;
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
        state->pointer_surface = wayland_surface_create();
    }

    if (added_capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        state->keyboard = wl_seat_get_keyboard(state->seat);
        wl_keyboard_add_listener(state->keyboard, &wayland_keyboard_listener, 0);
        state->key_repeat_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    }
}

internal Void wayland_seat_name(Void *data, struct wl_seat *seat, const char *name) {
}



// NOTE(simon): Data offer events.
internal Void wayland_data_offer_offer(Void *data, struct wl_data_offer *wl_data_offer, const char *mime_type) {
    Wayland_DataOffer *data_offer = (Wayland_DataOffer *) data;

    if (strcmp(mime_type, "text/uri-list") == 0) {
        data_offer->mime_types |= Wayland_MimeType_TextUriList;
    }
    if (strcmp(mime_type, "text/plain;charset=utf-8") == 0) {
        data_offer->mime_types |= Wayland_MimeType_TextPlainUtf8;
    }
    if (strcmp(mime_type, "UTF8_STRING") == 0) {
        data_offer->mime_types |= Wayland_MimeType_Utf8String;
    }
}

internal Void wayland_data_offer_source_actions(Void *data, struct wl_data_offer *wl_data_offer, U32 source_actions) {
    Wayland_DataOffer *data_offer = (Wayland_DataOffer *) data;
    data_offer->source_actions = source_actions;
}

internal Void wayland_data_offer_action(Void *data, struct wl_data_offer *wl_data_offer, U32 dnd_action) {
    Wayland_DataOffer *data_offer = (Wayland_DataOffer *) data;
    data_offer->action = dnd_action;
}



// NOTE(simon): Data device events.
internal Void wayland_data_device_data_offer(Void *data, struct wl_data_device *data_device, struct wl_data_offer *id) {
    Wayland_State *state = &global_wayland_state;

    Wayland_DataOffer *data_offer = state->data_offer_freelist;
    if (data_offer) {
        sll_stack_pop(state->data_offer_freelist);
        memory_zero_struct(data_offer);
    } else {
        data_offer = arena_push_struct(state->arena, Wayland_DataOffer);
    }

    data_offer->data_offer = id;
    wl_data_offer_add_listener(id, &wayland_data_offer_listener, data_offer);
    dll_push_back(state->first_data_offer, state->last_data_offer, data_offer);
}

internal Void wayland_data_device_enter(Void *data, struct wl_data_device *data_device, U32 serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *id) {
    Wayland_State *state = &global_wayland_state;

    // NOTE(simon): Replace any previous drag-and-drop operation.
    if (state->drag_and_drop_offer) {
        wayland_data_offer_destroy(state->drag_and_drop_offer);
        state->drag_and_drop_offer = 0;
    }

    // NOTE(simon): Find the state for the new drag-and-drop offer.
    for (Wayland_DataOffer *data_offer = state->first_data_offer; data_offer; data_offer = data_offer->next) {
        if (data_offer->data_offer == id) {
            state->drag_and_drop_offer = data_offer;
            break;
        }
    }

    // NOTE(simon): Drag-and-drop data offers are introduced through wayland_data_device_data_offer.
    assert(state->drag_and_drop_offer);

    if (state->drag_and_drop_offer) {
        wl_data_offer_set_actions(id, WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY, WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
        wl_data_offer_accept(id, serial, "text/uri-list");
        state->drag_and_drop_position = v2f32(
            (F32) (wl_fixed_to_double(x) * state->surface->scale),
            (F32) (wl_fixed_to_double(y) * state->surface->scale)
        );
    }
}

internal Void wayland_data_device_leave(Void *data, struct wl_data_device *data_device) {
    Wayland_State *state = &global_wayland_state;

    if (state->drag_and_drop_offer) {
        wayland_data_offer_destroy(state->drag_and_drop_offer);
        state->drag_and_drop_offer = 0;
    }
}

internal Void wayland_data_device_motion(Void *data, struct wl_data_device *data_device, U32 time, wl_fixed_t x, wl_fixed_t y) {
    Wayland_State *state = &global_wayland_state;

    state->drag_and_drop_position = v2f32(
        (F32) (wl_fixed_to_double(x) * state->surface->scale),
        (F32) (wl_fixed_to_double(y) * state->surface->scale)
    );
}

internal Void wayland_data_device_drop(Void *data, struct wl_data_device *data_device) {
    Wayland_State *state = &global_wayland_state;
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    if (state->drag_and_drop_offer && state->drag_and_drop_offer->mime_types & Wayland_MimeType_TextUriList) {
        Str8 drag_and_drop = wayland_data_offer_receive(scratch.arena, state->drag_and_drop_offer, "text/uri-list");

        // NOTE(simon): Implementation of https://datatracker.ietf.org/doc/html/rfc2483#section-5
        // NOTE(simon): Iterate through lines.
        for (U64 index = 0; index < drag_and_drop.size;) {
            U64 next_index = str8_find(index, str8_literal("\r\n"), drag_and_drop);
            Str8 line = str8_substring(drag_and_drop, index, next_index - index);
            index = next_index + 2;

            // NOTE(simon): Lines starting with a '#' are comments.
            if (line.size >= 1 && line.data[0] == '#') {
                continue;
            }

            Uri uri = uri_from_string(line);

            // NOTE(simon): We only accept files from localhost.
            if (str8_equal(uri.scheme, str8_literal("file")) && uri.authority.size == 0) {
                Gfx_Event *event = arena_push_struct(state->event_arena, Gfx_Event);
                event->kind     = Gfx_EventKind_FileDrop;
                event->position = state->drag_and_drop_position;
                event->path     = str8_copy(state->event_arena, uri.path);
                dll_push_back(state->events.first, state->events.last, event);
            }
        }

        wl_data_offer_finish(state->drag_and_drop_offer->data_offer);
    }

    arena_end_temporary(scratch);
}

internal Void wayland_data_device_selection(Void *data, struct wl_data_device *data_device, struct wl_data_offer *id) {
    Wayland_State *state = &global_wayland_state;

    if (state->selection_offer) {
        wayland_data_offer_destroy(state->selection_offer);
        state->selection_offer = 0;
    }

    // NOTE(simon): Find the state for the new selection offer.
    for (Wayland_DataOffer *data_offer = state->first_data_offer; data_offer; data_offer = data_offer->next) {
        if (data_offer->data_offer == id) {
            state->selection_offer = data_offer;
            break;
        }
    }
}



// NOTE(simon): Data source events.
internal Void wayland_data_source_target(Void *data, struct wl_data_source *data_source, const char *mime_type) {
    // NOTE(simon): Only used for drag-and-drop.
}

internal Void wayland_data_source_send(Void *data, struct wl_data_source *data_source, const char *mime_type, S32 fd) {
    Wayland_State *state = &global_wayland_state;

    if (data_source == state->selection_source) {
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

    state->xdg_surface_configure_serial = serial;
    if (state->update) {
        state->update();
    }
}



// NOTE(simon): XDG toplevel events.
internal Void wayland_xdg_toplevel_configure(Void *data, struct xdg_toplevel *xgd_toplevel, S32 width, S32 height, struct wl_array *states) {
    Wayland_State *state = &global_wayland_state;

    if (width != 0 && height != 0) {
        state->width = width;
        state->height = height;
    }
}

internal Void wayland_xdg_toplevel_close(Void *data, struct xdg_toplevel *xdg_toplevel) {
    Wayland_State *state = &global_wayland_state;
    Gfx_Event *event = arena_push_struct(state->event_arena, Gfx_Event);
    event->kind = Gfx_EventKind_Quit;
    dll_push_back(state->events.first, state->events.last, event);
}


// NOTE(simon): XDG toplevel decoration events.
internal Void wayland_xdg_toplevel_decoration_configure(Void *data, struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration, U32 mode) {
}



// NOTE(simon): Output events.
internal Void wayland_output_geometry(Void *data, struct wl_output *wl_output, S32 x, S32 y, S32 physical_width, S32 physical_height, S32 subpixel, const char *make, const char *model, S32 transform) {
    Wayland_Output *output = (Wayland_Output *) data;
}

internal Void wayland_output_mode(Void *data, struct wl_output *wl_output, U32 flags, S32 width, S32 height, S32 refresh) {
    Wayland_Output *output = (Wayland_Output *) data;
}

internal Void wayland_output_done(Void *data, struct wl_output *wl_output) {
    Wayland_State *state = &global_wayland_state;
    Wayland_Output *output = (Wayland_Output *) data;
    output->scale = output->pending_scale;

    for (Wayland_Surface *surface = state->first_surface; surface; surface = surface->next) {
        wayland_update_surface_scale(surface);
    }

    wayland_update_cursor();
    gfx_send_wakeup_event();
}

internal Void wayland_output_scale(Void *data, struct wl_output *wl_output, S32 factor) {
    Wayland_Output *output = (Wayland_Output *) data;
    output->pending_scale = factor;
}

internal Void wayland_output_name(Void *data, struct wl_output *wl_output, const char *name) {
}

internal Void wayland_output_description(Void *data, struct wl_output *wl_output, const char *description) {
}



// NOTE(simon): Surface events.
internal Void wayland_surface_enter(Void *data, struct wl_surface *wl_surface, struct wl_output *wl_output) {
    Wayland_State *state = &global_wayland_state;
    Wayland_Surface *surface = (Wayland_Surface *) data;

    // NOTE(simon): Find output.
    Wayland_Output *output = state->first_output;
    while (output && output->output != wl_output) {
        output = output->next;
    }

    if (output) {
        Wayland_OutputNode *node = state->output_node_freelist;
        if (node) {
            sll_stack_pop(state->output_node_freelist);
        } else {
            node = arena_push_struct_no_zero(state->arena, Wayland_OutputNode);
        }

        node->output = output;
        dll_push_back(surface->first_output, surface->last_output, node);
    }

    wayland_update_surface_scale(surface);
    wayland_update_cursor();
    gfx_send_wakeup_event();
}

internal Void wayland_surface_leave(Void *data, struct wl_surface *wl_surface, struct wl_output *output) {
    Wayland_State *state = &global_wayland_state;
    Wayland_Surface *surface = (Wayland_Surface *) data;

    for (Wayland_OutputNode *node = surface->first_output, *next = 0; node; node = next) {
        next = node->next;

        if (node->output->output == output) {
            dll_remove(surface->first_output, surface->last_output, node);
            sll_stack_push(state->output_node_freelist, node);
            break;
        }
    }

    wayland_update_surface_scale(surface);
    wayland_update_cursor();
    gfx_send_wakeup_event();
}

internal Void wayland_surface_preferred_buffer_scale(Void *data, struct wl_surface *wl_surface, S32 factor) {
    Wayland_Surface *surface = (Wayland_Surface *) data;

    if (!(surface->viewport && surface->fractional_scale)) {
        surface->scale = (S32) factor;
    }

    wayland_update_surface_scale(surface);
    wayland_update_cursor();
    gfx_send_wakeup_event();
}

internal Void wayland_surface_preferred_buffer_transform(Void *data, struct wl_surface *wl_surface, U32 transform) {
}



// NOTE(simon): Fractional scale events.
internal Void wayland_fractional_scale_preferred_scale(Void *data, struct wp_fractional_scale_v1 *fractional_scale, U32 scale) {
    Wayland_Surface *surface = (Wayland_Surface *) data;
    surface->scale = (F64) scale / 120.0;

    wayland_update_cursor();
    gfx_send_wakeup_event();
}



// NOTE(simon): Registry events.
internal Void wayland_registry_global(Void *data, struct wl_registry *registry, U32 name, const char *interface, U32 version) {
    Wayland_State *state = &global_wayland_state;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        if (version >= 6) {
            state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 6);
        } else {
            state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
        }
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        Wayland_Output *output = state->output_freelist;
        if (output) {
            sll_stack_pop(state->output_freelist);
            memory_zero_struct(output);
        } else {
            output = arena_push_struct(state->arena, Wayland_Output);
        }

        output->name = name;
        output->output = wl_registry_bind(registry, name, &wl_output_interface, 4);

        output->pending_scale = 1;
        output->scale = 1;

        wl_output_add_listener(output->output, &wayland_output_listener, output);
        dll_push_back(state->first_output, state->last_output, output);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        // TODO(simon): Handle multiple seats
        state->seat = wl_registry_bind(registry, name, &wl_seat_interface, 7);
        wl_seat_add_listener(state->seat, &wayland_seat_listener, 0);
    } else if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
        state->data_device_manager = wl_registry_bind(registry, name, &wl_data_device_manager_interface, 3);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 3);
        xdg_wm_base_add_listener(state->xdg_wm_base, &wayland_xdg_wm_base_listener, 0);
    } else if (strcmp(interface, wp_viewporter_interface.name) == 0) {
        state->viewporter = wl_registry_bind(registry, name, &wp_viewporter_interface, 1);
    } else if (strcmp(interface, wp_fractional_scale_manager_v1_interface.name) == 0) {
        state->fractional_scale_manager = wl_registry_bind(registry, name, &wp_fractional_scale_manager_v1_interface, 1);
    } else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
        state->xdg_decoration_manager = wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1);
    }
}

internal Void wayland_registry_global_remove(Void *data, struct wl_registry *registry, U32 name) {
    Wayland_State *state = &global_wayland_state;

    for (Wayland_Output *output = state->first_output, *next = 0; output; output = next) {
        next = output->next;

        if (output->name == name) {
            wl_output_release(output->output);
            dll_remove(state->first_output, state->last_output, output);
            sll_stack_push(state->output_freelist, output);
            break;
        }
    }
}



internal Void gfx_create(Str8 title, U32 width, U32 height) {
    gfx_linux_init();

    Arena_Temporary scratch = arena_get_scratch(0, 0);
    Wayland_State *state = &global_wayland_state;
    state->arena = arena_create();
    state->selection_source_arena = arena_create();

    state->display = wl_display_connect(0);
    struct wl_registry *registry = wl_display_get_registry(state->display);
    wl_registry_add_listener(registry, &wayland_registry_listener, 0);

    state->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    wl_display_roundtrip(state->display);

    if (state->data_device_manager && state->seat) {
        state->data_device = wl_data_device_manager_get_data_device(state->data_device_manager, state->seat);
        wl_data_device_add_listener(state->data_device, &wayland_data_device_listener, 0);
    }

    state->cursor_theme_name = getenv("XCURSOR_THEME");
    if (!state->cursor_theme_name) {
        state->cursor_theme_name = "default";
    }

    char *cursor_theme_size_string = getenv("XCURSOR_SIZE");
    if (!cursor_theme_size_string) {
        cursor_theme_size_string = "24";
    }
    state->cursor_theme_size = u64_from_str8(str8_cstr(cursor_theme_size_string)).value;

    state->width = (S32) width;
    state->height = (S32) height;
    state->surface = wayland_surface_create();
    state->event_arena = arena_create();
    state->xdg_surface = xdg_wm_base_get_xdg_surface(state->xdg_wm_base, state->surface->surface);
    xdg_surface_add_listener(state->xdg_surface, &wayland_xdg_surface_listener, 0);
    state->xdg_toplevel = xdg_surface_get_toplevel(state->xdg_surface);
    xdg_toplevel_add_listener(state->xdg_toplevel, &wayland_xdg_toplevel_listener, 0);
    CStr title_cstr = cstr_from_str8(scratch.arena, title);
    xdg_toplevel_set_title(state->xdg_toplevel, title_cstr);
    if (state->xdg_decoration_manager) {
        state->xdg_toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(state->xdg_decoration_manager, state->xdg_toplevel);
        zxdg_toplevel_decoration_v1_add_listener(state->xdg_toplevel_decoration, &wayland_xdg_toplevel_decoration_listener, 0);
        zxdg_toplevel_decoration_v1_set_mode(state->xdg_toplevel_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }
    wl_surface_commit(state->surface->surface);

    arena_end_temporary(scratch);
}

internal V2U32 gfx_get_window_client_area(Void) {
    Wayland_State *state = &global_wayland_state;

    V2U32 result = v2u32(
        (U32) f64_ceil((F64) state->width  * state->surface->scale),
        (U32) f64_ceil((F64) state->height * state->surface->scale)
    );
    return result;
}

internal Void gfx_send_wakeup_event(Void) {
    Wayland_State *state = &global_wayland_state;
    struct wl_callback *wakeup_callback = wl_display_sync(state->display);
    wl_callback_add_listener(wakeup_callback, &wayland_wakeup_callback_listener, 0);
    wl_display_flush(state->display);
}

internal Gfx_EventList gfx_get_events(Arena *arena, B32 wait) {
    prof_function_begin();
    Wayland_State *state = &global_wayland_state;

    // TODO(simon): Error handling
    do {
        while (wl_display_prepare_read(state->display) != 0) {
            wl_display_dispatch_pending(state->display);
        }

        wl_display_flush(state->display);

        struct pollfd fds[2] = { 0 };
        fds[0].fd     = wl_display_get_fd(state->display);
        fds[0].events = POLLIN;
        fds[1].fd     = state->key_repeat_fd;
        fds[1].events = POLLIN;
        poll(fds, array_count(fds), wait ? -1 : 0);

        // NOTE(simon): Handle display events.
        if (fds[0].revents & POLLIN) {
            wl_display_read_events(state->display);
            wl_display_dispatch_pending(state->display);
        } else {
            wl_display_cancel_read(state->display);
        }

        // NOTE(simon): Handle key repeats.
        if (fds[1].revents & POLLIN) {
            U64 key_repeats = 0;
            ssize_t bytes_read = 0;
            do {
                bytes_read = read(state->key_repeat_fd, &key_repeats, sizeof(key_repeats));
            } while (bytes_read == -1 && errno == EINTR);

            if (bytes_read == sizeof(key_repeats)) {
                for (U64 i = 0; i < key_repeats; ++i) {
                    wayland_handle_key(state->last_key, WL_KEYBOARD_KEY_STATE_PRESSED);
                }
            }
        }
    } while (wait && !state->events.first);

    // NOTE(simon): Copy events to the provided arena.
    Gfx_EventList events = { 0 };
    for (Gfx_Event *event = state->events.first; event; event = event->next) {
        Gfx_Event *new_event = arena_push_struct(arena, Gfx_Event);
        *new_event = *event;
        new_event->text = str8_copy(arena, new_event->text);
        new_event->path = str8_copy(arena, new_event->path);
        dll_push_back(events.first, events.last, new_event);
    }

    // NOTE(simon): Reset event state.
    arena_reset(state->event_arena);
    memory_zero_struct(&state->events);

    prof_function_end();
    return events;
}

internal V2F32 gfx_get_mouse_position(Void) {
    Wayland_State *state = &global_wayland_state;
    V2F32 result = state->pointer_position;
    return result;
}

internal Void gfx_swap_buffers(Void) {
    Wayland_State *state = &global_wayland_state;

    if (state->surface->viewport && state->surface->fractional_scale) {
        wp_viewport_set_source(
            state->surface->viewport,
            wl_fixed_from_int(0),
            wl_fixed_from_int(0),
            wl_fixed_from_double((F64) state->width  * state->surface->scale),
            wl_fixed_from_double((F64) state->height * state->surface->scale)
        );
        wp_viewport_set_destination(state->surface->viewport, state->width, state->height);
        wl_surface_set_buffer_scale(state->surface->surface, 1);
    } else {
        wl_surface_set_buffer_scale(state->surface->surface, (S32) f64_ceil(state->surface->scale));
    }

    if (state->xdg_surface_configure_serial != state->xdg_surface_last_configure_serial) {
        xdg_surface_ack_configure(state->xdg_surface, state->xdg_surface_configure_serial);
        state->xdg_surface_last_configure_serial = state->xdg_surface_configure_serial;
    }

    if (state->swap_buffers) {
        state->swap_buffers();
    }
}

internal Void gfx_set_cursor(Gfx_Cursor cursor) {
    Wayland_State *state = &global_wayland_state;

    if (cursor != state->pointer_cursor) {
        state->pointer_cursor = cursor;
        wayland_update_cursor();
    }
}

internal Void gfx_set_update_function(VoidFunction *update) {
    Wayland_State *state = &global_wayland_state;
    state->update = update;
}

internal F32 gfx_dpi(Void) {
    Wayland_State *state = &global_wayland_state;
    F32 dpi = (F32) (96.0 * state->surface->scale);
    return dpi;
}


// NOTE(simon): Clipboard
internal Void gfx_set_clipboard_text(Str8 text) {
    Wayland_State *state = &global_wayland_state;

    arena_reset(state->selection_source_arena);
    state->selection_source_str8 = str8_copy(state->selection_source_arena, text);
    wayland_set_selection();
}

internal Str8 gfx_get_clipboard_text(Arena *arena) {
    Wayland_State *state = &global_wayland_state;

    Str8 result = { 0 };

    if (state->selection_offer) {
        if (state->selection_source) {
            // NOTE(simon): We own the selection! Perform an internal copy to
            // avoid having to both read and write to a pipe in the same
            // process.
            result = str8_copy(arena, state->selection_source_str8);
        } else {
            CStr mime_type = 0;
            if (state->selection_offer->mime_types & Wayland_MimeType_Utf8String) {
                mime_type = "UTF8_STRING";
            } else if (state->selection_offer->mime_types & Wayland_MimeType_TextPlainUtf8) {
                mime_type = "text/plain;charset=utf-8";
            }

            if (mime_type) {
                result = wayland_data_offer_receive(arena, state->selection_offer, mime_type);
            }
        }
    }

    return result;
}
