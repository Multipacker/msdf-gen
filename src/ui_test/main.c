#include "src/base/base_include.h"
#include "src/graphics/graphics_include.h"
#include "src/render/render_include.h"
#include "src/font/font_include.h"
#include "src/font_cache/font_cache_include.h"
#include "src/draw/draw_include.h"
#include "src/ui/ui_include.h"

#include "src/base/base_include.c"
#include "src/graphics/graphics_include.c"
#include "src/render/render_include.c"
#include "src/font/font_include.c"
#include "src/font_cache/font_cache_include.c"
#include "src/draw/draw_include.c"
#include "src/ui/ui_include.c"

#define COLORINGS(X) \
    X(Static,     "Static")      \
    X(ByIndex,    "By index")    \
    X(ByLocation, "By location")

#define X(name, display) Coloring_##name,
typedef enum {
    COLORINGS(X)
    Coloring_COUNT,
} Coloring;
#undef X

#define X(name, display) [Coloring_##name] = str8_literal(display),
global Str8 coloring_names[] = {
    COLORINGS(X)
};
#undef X
#undef COLORS

typedef struct State State;
struct State {
    Arena *arena;
    B32 running;

    Arena *frame_arenas[2];
    U64 frame_index;
    U64 frames_to_render;

    UI_Context *ui;

    F32 point_radius;
    F32 line_width;
    F32 point_scale;
    F32 target_point_scale;
    U8 buffer[10];
    U64 buffer_size;
    U64 cursor;
    U64 mark;
    Coloring coloring;
    V4F32 point_color;

    B32 generate;
};

global State *state;

internal Arena *frame_arena(Void) {
    Arena *result = state->frame_arenas[state->frame_index % array_count(state->frame_arenas)];
    return result;
}

internal Void request_frame(Void) {
    state->frames_to_render = 4;
}

internal Void update(Void) {
    arena_reset(frame_arena());

    Gfx_EventList events = { 0 };

    local U32 depth = 0;
    if (depth == 0) {
        ++depth;
        events = gfx_get_events(frame_arena(), state->frames_to_render == 0);
        --depth;
    }

    UI_EventList ui_events = { 0 };

    for (Gfx_Event *event = events.first, *next = 0; event; event = next) {
        next = event->next;
        B32 consume = false;
        UI_Event *ui_event = 0;

        if (event->kind == Gfx_EventKind_Quit) {
            consume = true;
            state->running = false;
        } else if (event->kind == Gfx_EventKind_KeyPress || event->kind == Gfx_EventKind_KeyRelease || event->kind == Gfx_EventKind_Text || event->kind == Gfx_EventKind_Scroll) {
            consume = true;
            UI_EventKind kind = UI_EventKind_Null;
            switch (event->kind) {
                case Gfx_EventKind_Null:       kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_Quit:       kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_KeyPress:   kind = UI_EventKind_KeyPress;   break;
                case Gfx_EventKind_KeyRelease: kind = UI_EventKind_KeyRelease; break;
                case Gfx_EventKind_MouseMove:  kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_Text:       kind = UI_EventKind_Text;       break;
                case Gfx_EventKind_Scroll:     kind = UI_EventKind_Scroll;     break;
                case Gfx_EventKind_Resize:     kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_FileDrop:   kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_Wakeup:     kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_COUNT:      kind = UI_EventKind_Null;       break;
            }
            ui_event = arena_push_struct(frame_arena(), UI_Event);
            ui_event->kind      = kind;
            ui_event->text      = event->text;
            ui_event->position  = event->position;
            ui_event->scroll    = event->scroll;
            ui_event->key       = event->key;
            ui_event->modifiers = event->key_modifiers;
        }

        if (ui_event && ui_event->kind != UI_EventKind_Null) {
            ui_event_list_push_event(&ui_events, ui_event);
        }

        if (consume) {
            dll_remove(events.first, events.last, event);
        }
    }

    // NOTE(simon): If there are UI events, they will potentially trigger UI
    // changes, so render more frames.
    if (ui_events.first) {
        request_frame();
    }

    V2U32 client_area = gfx_get_window_client_area();
    render_begin(client_area);
    draw_begin_frame();
    Draw_List *draw_list = draw_list_create();
    draw_list_push(draw_list);
    ui_select_state(state->ui);
    ui_begin(&ui_events, 1.0f / 60.0f);
    ui_font_size_push((U32) (11.0f * gfx_dpi() / 72.0f));

    UI_Palette palette = {
        .background = color_from_srgba(v4f32(0.31f, 0.3f, 0.3f, 1.0f)),
        .text       = color_from_srgba(v4f32(0.9f, 0.9f, 0.9f, 1.0f)),
        .border     = color_from_srgba(v4f32(0.41f, 0.4f, 0.4f, 1.0f)),
        .cursor     = color_from_srgba(v4f32(0.7f, 0.6f, 0.0f, 1.0f)),
        .selection  = color_from_srgba(v4f32(0.5f, 0.5f, 1.0f, 0.4f)),
    };
    ui_palette_push(palette);

    ui_width(ui_size_fill())
    ui_height(ui_size_fill())
    ui_text_x_padding(ui_size_ems(0.5f, 1.0f).value) {
        ui_layout_axis_next(Axis2_X);
        UI_Box *panel = ui_create_box_from_string(
            UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawDropShadow |
            UI_BoxFlag_Clickable | UI_BoxFlag_Scrollable | UI_BoxFlag_DefaultNavigation,
            str8_literal("##panel")
        );
        ui_parent(panel)
        ui_padding(ui_size_ems(0.5f, 1.0f))
        ui_column()
        ui_padding(ui_size_ems(0.5f, 1.0f))
        ui_width(ui_size_text_content(0.0f, 1.0f))
        ui_height(ui_size_ems(1.5f, 1.0f))
        ui_corner_radius(5.0f) {
            ui_label(str8_literal("Settings"));

            ui_spacer_sized(ui_size_ems(0.5f, 1.0f));

            ui_width(ui_size_children_sum(1.0f))
            ui_height(ui_size_children_sum(1.0f))
            ui_row() {
                ui_column()
                ui_width(ui_size_text_content(0.0f, 1.0f))
                ui_height(ui_size_ems(1.5f, 1.0f)) {
                    ui_label(str8_literal("Radius"));
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    ui_label(str8_literal("Width"));
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    ui_label(str8_literal("Scale"));
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    ui_label(str8_literal("Points"));
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    ui_label(str8_literal("Coloring"));
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    ui_label(str8_literal("Static color"));
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    UI_Input generate_input = ui_button(str8_literal("Generate"));
                    if (generate_input.flags & UI_InputFlag_Clicked) {
                        state->generate = true;
                    }
                }

                ui_column()
                ui_width(ui_size_ems(10.0f, 1.0f))
                ui_height(ui_size_ems(1.5f, 1.0f)) {
                    ui_slider(1.0f, &state->point_radius, 20.0f, ui_key_from_string(ui_active_seed_key(), str8_literal("##radius_slider")));
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    ui_slider(1.0f, &state->line_width, 20.0f, ui_key_from_string(ui_active_seed_key(), str8_literal("##width_slider")));
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    ui_slider(0.01f, &state->target_point_scale, 10.0f, ui_key_from_string(ui_active_seed_key(), str8_literal("##scale_slider")));
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    {
                        UI_Key line_key = ui_key_from_string(ui_active_seed_key(), str8_literal("##point_count"));
                        ui_set_auto_focus_hot_key(line_key);
                        ui_set_auto_focus_active_key(line_key);
                        ui_line_edit(state->buffer, &state->buffer_size, array_count(state->buffer), &state->cursor, &state->mark, line_key);
                    }
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    {
                        UI_Key dropdown_key = ui_key_from_string(ui_active_seed_key(), str8_literal("##color_dropdown"));

                        ui_context_menu(dropdown_key) {
                            ui_height_next(ui_size_children_sum(1.0f));
                            ui_extra_box_flags_next(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawDropShadow);
                            ui_column()
                            ui_height(ui_size_ems(1.5f, 1.0f))
                            ui_hover_cursor(Gfx_Cursor_Hand)
                            for (Coloring i = 0; i < Coloring_COUNT; ++i) {
                                UI_Box *color_box = ui_create_box_from_string(
                                    UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText |
                                    UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
                                    UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable,
                                    coloring_names[i]
                                );
                                UI_Input color_input = ui_input_from_box(color_box);
                                if (color_input.flags & UI_InputFlag_Clicked) {
                                    state->coloring = i;
                                    ui_context_menu_close();
                                }
                            }
                        }

                        ui_layout_axis_next(Axis2_X);
                        ui_hover_cursor_next(Gfx_Cursor_Hand);
                        UI_Box *combo_box = ui_create_box_from_string(
                            UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder |
                            UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
                            UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable,
                            str8_literal("##coloring_selector")
                        );
                        ui_parent(combo_box) {
                            ui_width_next(ui_size_fill());
                            ui_label(coloring_names[state->coloring]);
                            ui_width_next(ui_size_pixels(combo_box->calculated_size.height, 1.0f));
                            ui_create_box_from_string(UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawText, ui_context_menu_is_open(dropdown_key) ? str8_literal("v") : str8_literal("<"));
                        }

                        UI_Input combo_input = ui_input_from_box(combo_box);
                        if (combo_input.flags & UI_InputFlag_Clicked) {
                            ui_context_menu_open(dropdown_key, combo_box->key, v2f32(0.0f, 0.0f));
                        }
                    }
                    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));
                    {
                        UI_Key color_picker_key = ui_key_from_string(ui_active_seed_key(), str8_literal("##color_picker"));

                        ui_context_menu(color_picker_key) {
                            ui_extra_box_flags_next(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawDropShadow);
                            ui_height(ui_size_children_sum(1.0f))
                            ui_column()
                            ui_padding(ui_size_ems(0.5f, 1.0f)) {
                                ui_row()
                                ui_padding(ui_size_ems(0.5f, 1.0f)) {
                                    ui_color_picker(&state->point_color, ui_size_ems(6.0f, 1.0f), ui_size_ems(1.0f, 1.0f), ui_size_ems(0.5f, 1.0f));
                                }
                            }
                        }

                        UI_Palette color_palette = ui_palette_top();
                        color_palette.background = state->point_color;
                        ui_palette_next(color_palette);
                        ui_hover_cursor_next(Gfx_Cursor_Hand);
                        UI_Box *color_box = ui_create_box_from_string(
                            UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder |
                            UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
                            UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable | (state->coloring != Coloring_Static ? UI_BoxFlag_Disabled : 0),
                            str8_literal("##point_color")
                        );

                        UI_Input color_input = ui_input_from_box(color_box);
                        if (color_input.flags & UI_InputFlag_Clicked) {
                            ui_context_menu_open(color_picker_key, color_box->key, v2f32(0.0f, 0.0f));
                        }
                    }

                    for (U32 i = 0; i < 10; ++i) {
                        ui_spacer_sized(ui_size_ems(0.5f, 1.0f));

                        UI_Input input = ui_button_format("Button %u", i);
                        if (input.flags & UI_InputFlag_Clicked) {
                            os_console_print(str8_format(ui_frame_arena(), "Button %u\n", i));
                        }
                    }
                }
            }
        }

        UI_Input panel_input = ui_input_from_box(panel);
    }

    for (UI_Event *event = 0; ui_next_event(&event);) {
        if (event->kind == UI_EventKind_Scroll) {
            state->target_point_scale *= f32_pow(0.97f, -event->scroll.y);
            state->target_point_scale = f32_clamp(state->target_point_scale, 0.01f, 10.0f);
            ui_consume_event(event);
        }
    }

    state->point_scale += (state->target_point_scale - state->point_scale) * ui_animation_slow_rate();
    if (f32_abs(1.0f - state->point_scale / state->target_point_scale) < 0.001f) {
        state->point_scale = state->target_point_scale;
    } else {
        request_frame();
    }

    ui_end();

    // NOTE(simon): Draw
    F32 scale = state->point_scale * 0.5f * f32_min((F32) client_area.width - 2.0f * state->point_radius, (F32) client_area.height - 2.0f * state->point_radius);
    M3F32 scale_matrix = m3f32_scale(v2f32(scale, scale));
    M3F32 translation_matrix = m3f32_translation(v2f32_scale(v2f32((F32) client_area.width, (F32) client_area.height), 0.5f));
    M3F32 transform = m3f32_multiply_m3f32(translation_matrix, scale_matrix);

    for (UI_Box *box = state->ui->root; box != &global_ui_null_box;) {
        if (box->flags & UI_BoxFlag_DrawDropShadow) {
            draw_rectangle(
                r2f32(
                    box->calculated_rectangle.min.x - 4.0f,
                    box->calculated_rectangle.min.y - 4.0f,
                    box->calculated_rectangle.max.x + 12.0f,
                    box->calculated_rectangle.max.y + 12.0f
                ),
                color_from_srgba(v4f32(0.0f, 0.0f, 0.0f, 0.5f)),
                0.8f, 0.0f, 8.0f
            );
        }

        if (box->flags & UI_BoxFlag_DrawBackground) {
            {
                Render_Shape *shape = draw_rectangle(box->calculated_rectangle, box->palette.background, 0.0f, 0.0f, 1.0f);
                memory_copy(shape->radies, box->corner_radies, sizeof(shape->radies));
            }

            if (box->flags & UI_BoxFlag_DrawHot && box->hot_t > 0.0f) {
                F32 active_t = box->active_t;
                if (!(box->flags & UI_BoxFlag_DrawActive)) {
                    active_t = 0.0f;
                }
                V4F32 color = color_from_srgba(v4f32(0.5f, 0.5f, 0.5f, 1.0f));
                color.a *= 0.2f * (box->hot_t - active_t);

                Render_Shape *rect = draw_rectangle(box->calculated_rectangle, color, 0.0f, 0.0f, 1.0f);
                memory_copy(rect->radies, box->corner_radies, sizeof(rect->radies));
            }

            if (box->flags & UI_BoxFlag_DrawActive && box->active_t > 0.0f) {
                Render_Shape *rect = draw_rectangle(box->calculated_rectangle, v4f32(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 1.0f);
                V4F32 color = color_from_srgba(v4f32(0.5f, 0.5f, 0.5f, 1.0f));
                color.r *= 0.3f;
                color.g *= 0.3f;
                color.b *= 0.3f;
                color.a *= 0.5f * box->active_t;
                rect->colors[Corner_10] = color;
                rect->colors[Corner_11] = color;
                memory_copy(rect->radies, box->corner_radies, sizeof(rect->radies));
            }
        }

        if (box->flags & UI_BoxFlag_DrawText) {
            V2F32 origin = ui_box_text_location(box);

            {
                F32 advance = 0.0f;
                for (U64 i = 0; i < box->text.letter_count; ++i) {
                    FontCache_Letter *letter = &box->text.letters[i];
                    draw_glyph(
                        r2f32(
                            f32_floor(origin.x + letter->offset.x + advance),
                            f32_floor(origin.y + letter->offset.y),
                            f32_floor(origin.x + letter->offset.x + advance + letter->size.x),
                            f32_floor(origin.y + letter->offset.y + letter->size.y)
                        ),
                        letter->source,
                        letter->texture,
                        box->palette.text
                    );
                    advance += letter->advance;
                }
            }

            if (box->flags & UI_BoxFlag_DrawFuzzyMatches) {
                F32 ascent = box->text.ascent;
                F32 descent = box->text.descent;
                for (FuzzyMatch *match = box->fuzzy_matches.first; match; match = match->next) {
                    F32 pixel_min =  f32_infinity();
                    F32 pixel_max = -f32_infinity();
                    U64 byte_offset = 0;
                    F32 advance = 0.0f;
                    for (U64 i = 0; i < box->text.letter_count; ++i) {
                        FontCache_Letter *letter = &box->text.letters[i];

                        if (match->min <= byte_offset && byte_offset < match->max) {
                            F32 pre_offset  = advance + letter->offset.x;
                            F32 post_offset = advance + letter->advance;
                            pixel_min = f32_min(pre_offset,  pixel_min);
                            pixel_max = f32_max(post_offset, pixel_max);
                        }

                        advance += letter->advance;
                        byte_offset += letter->decode_size;
                    }
                    V4F32 color = color_from_srgba(v4f32(1.0, 0.5f, 0.0f, 1.0f));
                    color.a *= 0.2f;
                    draw_rectangle(
                        r2f32(
                            f32_floor(origin.x + pixel_min),
                            f32_floor(origin.y - ascent),
                            f32_floor(origin.x + pixel_max),
                            f32_floor(origin.y - descent)
                        ),
                        color,
                        0,
                        0,
                        0
                    );
                }
            }
        }

        if (box->flags & UI_BoxFlag_Clip) {
            R2F32 top_clip = draw_clip_top();
            R2F32 new_clip = r2f32_intersect(top_clip, box->calculated_rectangle);
            draw_clip_push(new_clip);
        }

        if (box->draw_list) {
            draw_transform(m3f32_translation(box->calculated_rectangle.min)) {
                draw_sub_list(box->draw_list);
            }
        }

        if (box->draw_function) {
            box->draw_function(box, box->draw_data);
        }

        UI_BoxIterator iterator = ui_box_iterator_depth_first_pre_order(box);

        // NOTE(simon): We use `<=` because we need to pop our state when
        // moving to our siblings. Traversing siblings sets both `push_count`
        // and `pop_count` to 0.
        U32 pop_index = 0;
        for (UI_Box *parent = box; pop_index <= iterator.pop_count; parent = parent->parent, ++pop_index) {
            if (parent == box && iterator.push_count) {
                continue;
            }

            if (parent->flags & UI_BoxFlag_Clip) {
                draw_clip_pop();
            }

            if (parent->flags & UI_BoxFlag_DrawBorder) {
                Render_Shape *shape = draw_rectangle(r2f32_pad(parent->calculated_rectangle, 1.0f), parent->palette.border, 0.0f, 1.0f, 1.0f);
                memory_copy(shape->radies, parent->corner_radies, sizeof(shape->radies));

                if (parent->flags & UI_BoxFlag_DrawHot && parent->hot_t > 0.0f) {
                    V4F32 color = color_from_srgba(v4f32(0.5f, 0.5f, 0.5f, 1.0f));
                    color.a *= parent->hot_t;

                    Render_Shape *rect = draw_rectangle(r2f32_pad(parent->calculated_rectangle, 1.0f), color, 0.0f, 1.0f, 1.0f);
                    memory_copy(rect->radies, parent->corner_radies, sizeof(rect->radies));
                }
            }

            if (parent->flags & UI_BoxFlag_Clickable && parent->flags & UI_BoxFlag_FocusActive) {
                V4F32 color = color_from_srgba(v4f32(0.7f, 0.6f, 0.0f, 1.0f));
                color.a *= 0.2f * box->focus_active_t;
                Render_Shape *shape = draw_rectangle(parent->calculated_rectangle, color, 0.0f, 0.0f, 0.0f);
                memory_copy(shape->radies, parent->corner_radies, sizeof(shape->radies));
            }

            if (parent->flags & UI_BoxFlag_Clickable && parent->flags & UI_BoxFlag_FocusActive) {
                V4F32 color = color_from_srgba(v4f32(0.7f, 0.6f, 0.0f, 1.0f));
                color.a *= box->focus_active_t;
                Render_Shape *shape = draw_rectangle(r2f32_pad(parent->calculated_rectangle, 1.0f), color, 0.0f, 1.0f, 1.0f);
                memory_copy(shape->radies, parent->corner_radies, sizeof(shape->radies));
            }

            if (parent->flags & UI_BoxFlag_Disabled) {
                V4F32 color = color_from_srgba(v4f32(0.5f, 0.5f, 0.5f, 0.5f));
                color.a *= box->disabled_t;
                Render_Shape *shape = draw_rectangle(parent->calculated_rectangle, color, 0.0f, 0.0f, 1.0f);
                memory_copy(shape->radies, box->corner_radies, sizeof(shape->radies));
            }
        }

        box = iterator.next;
    }

    draw_submit_list(draw_list);
    render_end();

    if (state->frames_to_render > 0) {
        --state->frames_to_render;
    }

    if (ui_is_animating_from_context(state->ui)) {
        request_frame();
    }

    if (PROFILE_BUILD) {
        request_frame();
    }

    ++state->frame_index;
}

internal S32 os_run(Str8List arguments) {
    Arena *arena = arena_create();
    state = arena_push_struct(arena, State);
    state->arena = arena;
    state->running = true;

    for (U64 i = 0; i < array_count(state->frame_arenas); ++i) {
        state->frame_arenas[i] = arena_create();
    }

    gfx_create(str8_literal("Points"), 1280, 720);
    render_init();
    render_create();
    font_cache_create();
    gfx_set_update_function(update);

    state->ui = ui_create();

    state->frames_to_render = 4;

    state->point_radius = 10.0f;
    state->line_width = 2.0f;
    state->point_color = v4f32(0.9f, 0.9f, 0.9f, 1.0f);
    state->buffer[0] = '1';
    state->buffer[1] = '0';
    state->buffer[2] = '0';
    state->buffer_size = 3;

    while (state->running) {
        update();
    }

    return 0;
}
