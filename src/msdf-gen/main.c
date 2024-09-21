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

/*
 * TODO:
 * Clipboard
 * Focus and keyboard navigation / interaction
 * More input information
 */
/* NOTE(simon): The old zoom equations, for reference.
 * F32 old_zoom = zoom;
 * zoom *= f32_pow(0.97f, event->scroll.y);
 * offset = v2f32_subtract(mouse, v2f32_scale(v2f32_subtract(mouse, offset), old_zoom / zoom));
*/

/*
 * TODO before next release:
 * * Bake the UI font into the executable
 * * Add support for tabs that can be moved around
 * * Align outlines and the MSDF correctly in the glyph view
 *
 * TODO long term
 * * Allow multiple codepoints to map to the same glyph, alternatively allow
 *   marking glyphs as missing as that is the main use case
 */

typedef struct Theme Theme;
struct Theme {
    Str8 name;
    V4F32 background_color;
    V4F32 element_color;
    V4F32 border_color;
    V4F32 text_color;
};

global Theme global_themes[2];

typedef struct Glyph Glyph;
struct Glyph {
    Glyph *next;
    Glyph *previous;

    U32 codepoint;

    V2F32 min_pt;
    V2F32 max_pt;
    F32   advance_pt;
    V2F32 uv_min;
    V2F32 uv_max;
};

global Glyph global_glyph_null = { 0 };

typedef struct GlyphList GlyphList;
struct GlyphList {
    Glyph *first;
    Glyph *last;
};

typedef struct Font Font;
struct Font {
    Arena *arena;

    U32 glyph_size;
    U32 glyphs_per_row;
    U32 atlas_size;

    Render_Texture atlas;
    GlyphList glyph_lists[2048];
    TTF_Font *ttf;

    U32 next_glyph_index;
};

typedef struct Panel Panel;

#define PANEL_BUILD_FUNCTION(name) Void name(Panel *panel, Theme *theme, R2F32 panel_rectangle)
typedef PANEL_BUILD_FUNCTION(PanelBuildFunction);

struct Panel {
    Panel *next;
    Panel *previous;
    Panel *first;
    Panel *last;
    Panel *parent;
    F32    percentage_of_parent;
    Axis2  split_axis;

    Arena *arena;
    Void  *view_state;
    PanelBuildFunction *build_view;
};

typedef struct PanelIterator PanelIterator;
struct PanelIterator {
    Panel *next;
    U32 push_count;
    U32 pop_count;
};

typedef struct State State;
struct State {
    Arena *arena;

    UI_Context *ui;

    Panel *panel_root;
    Panel *panel_freelist;

    Font *font;
    TTF_Font *ttf_font;
    U32 selected_codepoint;
    B32 running;
};

global State *global_state;

internal Font *font_create(Str8 font_path, U32 glyph_size, U32 glyphs_per_row) {
    Arena *arena = arena_create();
    Font *result = arena_push_struct_zero(arena, Font);

    result->glyph_size     = glyph_size;
    result->glyphs_per_row = glyphs_per_row;
    result->atlas_size     = glyph_size * glyphs_per_row;

    result->arena = arena;
    result->atlas = render_texture_create(v2u32(result->atlas_size, result->atlas_size), Render_TextureFormat_RGBA8, 0);
    result->ttf   = ttf_load(arena, font_path);

    if (result->ttf->errors.node_count != 0) {
        os_console_print(str8_join(arena, &result->ttf->errors));
    }

    return result;
}

internal Glyph *font_get_glyph(Font *font, U32 codepoint) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    U64 hash = u64_hash(codepoint);
    GlyphList *glyphs = &font->glyph_lists[hash % array_count(font->glyph_lists)];

    Glyph *result = &global_glyph_null;

    for (Glyph *glyph = glyphs->first; glyph; glyph = glyph->next) {
        if (glyph->codepoint == codepoint) {
            result = glyph;
            break;
        }
    }

    if (result == &global_glyph_null && font->next_glyph_index < font->glyphs_per_row * font->glyphs_per_row) {
        result = arena_push_struct_zero(font->arena, Glyph);
        result->codepoint = codepoint;

        MSDF_RasterResult raster_result = msdf_generate(scratch.arena, font->ttf, codepoint, font->glyph_size);

        V2U32 atlas_position = v2u32(
            font->glyph_size * (font->next_glyph_index % font->glyphs_per_row),
            font->glyph_size * (font->next_glyph_index / font->glyphs_per_row)
        );
        ++font->next_glyph_index;
        render_texture_update(font->atlas, atlas_position, v2u32(font->glyph_size, font->glyph_size), raster_result.data);

        // This adjustment increases the size of glyphs to acount for the
        // UVs needing to include a 1/2 texel border for rendering. This
        // makes sure that the glyphs have the same visual size.
        F32 scale = ((F32) font->glyph_size - 1.0f) / ((F32) font->glyph_size - 2.0f) - 1.0f;
        V2F32 adjustment = v2f32_scale(v2f32_subtract(raster_result.max, raster_result.min), 0.5f * scale);

        result->advance_pt = raster_result.advance_width;
        result->min_pt = v2f32_subtract(raster_result.min, adjustment);
        result->max_pt = v2f32_add(raster_result.max, adjustment);
        result->uv_min = v2f32(
            ((F32) atlas_position.x + 0.5f) / (F32) font->atlas_size,
            ((F32) atlas_position.y + 0.5f) / (F32) font->atlas_size
        );
        result->uv_max = v2f32(
            ((F32) atlas_position.x + (F32) font->glyph_size - 0.5f) / (F32) font->atlas_size,
            ((F32) atlas_position.y + (F32) font->glyph_size - 0.5f) / (F32) font->atlas_size
        );

        dll_push_back(glyphs->first, glyphs->last, result);
    }

    arena_end_temporary(scratch);
    return result;
}

internal Void draw_text_msdf(Font *font, V2F32 position, F32 point_size, Str8 text) {
    V2F32 text_point = position;
    for (U8 *ptr = text.data, *opl = text.data + text.size; ptr < opl; ) {
        StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
        ptr += decode.size;

        Glyph *glyph = font_get_glyph(font, decode.codepoint);

        draw_msdf(
            r2f32(
                text_point.x + glyph->min_pt.x * point_size,
                text_point.y + glyph->min_pt.y * point_size,
                text_point.x + glyph->max_pt.x * point_size,
                text_point.y + glyph->max_pt.y * point_size
            ),
            (R2F32) { glyph->uv_min, glyph->uv_max, },
            font->atlas,
            v4f32(1.0f, 1.0f, 1.0f, 1.0f)
        );

        text_point.x += glyph->advance_pt * point_size;
    }
}

internal Void draw_text(FontCache_Font *font, V2F32 origin, Str8 string, U32 size) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    FontCache_Text text = font_cache_text(scratch.arena, font, string, size);

    F32 advance = 0.0f;
    for (U64 i = 0; i < text.letter_count; ++i) {
        FontCache_Letter *letter = &text.letters[i];
        draw_glyph(
            r2f32(
                origin.x + letter->offset.x + advance,
                origin.y + letter->offset.y,
                origin.x + letter->offset.x + advance + letter->size.x,
                origin.y + letter->offset.y + letter->size.y
            ),
            letter->uvs,
            letter->texture,
            v4f32(1.0f, 1.0f, 1.0f, 1.0f)
        );
        advance += letter->advance;
    }

    arena_end_temporary(scratch);
}

internal Void draw_ui(UI_Box *root) {
    prof_function_begin();

    for (UI_Box *box = root; box != &global_ui_null_box;) {
        if (box->flags & UI_BoxFlag_DrawBackground) {
            draw_rectangle(box->calculated_rectangle, box->color, 0.0f, 0.0f, 0.0f);

            if (box->flags & UI_BoxFlag_DrawHot && box->hot_t > 0.0f) {
                Render_Shape *rect = draw_rectangle(box->calculated_rectangle, v4f32(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.0f);
                rect->colors[0] = v4f32(1.0f, 1.0f, 1.0f, 0.5f * box->hot_t);
                rect->colors[1] = v4f32(1.0f, 1.0f, 1.0f, 0.5f * box->hot_t);
            }

            if (box->flags & UI_BoxFlag_DrawActive && box->active_t > 0.0f) {
                Render_Shape *rect = draw_rectangle(box->calculated_rectangle, v4f32(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.0f);
                rect->colors[2] = v4f32(0.0f, 0.0f, 0.0f, 0.5f * box->active_t);
                rect->colors[3] = v4f32(0.0f, 0.0f, 0.0f, 0.5f * box->active_t);
            }
        }

        if (box->flags & UI_BoxFlag_DrawText) {
            V2F32 origin = ui_box_text_location(box);
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
                    letter->uvs,
                    letter->texture,
                    box->text_color
                );
                advance += letter->advance;
            }
        }

        if (box->draw_function) {
            box->draw_function(box, box->draw_data);
        }

        if (box->flags & UI_BoxFlag_Clip) {
            R2F32 top_clip = draw_clip_top();
            R2F32 new_clip = r2f32_intersect(top_clip, box->calculated_rectangle);
            draw_clip_push(new_clip);
        }

        UI_BoxIterator iterator = ui_box_iterator_depth_first_post_order(box);

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

            if (box->flags & UI_BoxFlag_DrawBorder) {
                draw_rectangle(box->calculated_rectangle, box->border_color, 0.0f, 1.0f, 1.0f);
            }

            if (box->flags & UI_BoxFlag_Disabled) {
                draw_rectangle(box->calculated_rectangle, v4f32(0.2f, 0.2f, 0.2f, 0.75f), 0.0f, 0.0f, 0.0f);
            }
        }

        box = iterator.next;
    }

    prof_function_end();
}

#include "panels.c"
#include "views.c"

internal Void update(Void) {
    // NOTE(simon): Themes
    {
        global_themes[0].name = str8_literal("Light theme");
        global_themes[0].background_color = color_from_srgba_u32(0xF8F9FAFF); // OC Gray 0
        global_themes[0].element_color    = color_from_srgba_u32(0xE9ECEFFF); // OC Gray 2
        global_themes[0].border_color     = color_from_srgba_u32(0xCED4DAFF); // OC Gray 4
        global_themes[0].text_color       = color_from_srgba_u32(0x495057FF); // OC Gray 7

        global_themes[1].name = str8_literal("Dark theme"),
        global_themes[1].background_color = color_from_srgba_u32(0x212529FF); // OC Gray 9
        global_themes[1].element_color    = color_from_srgba_u32(0x343A40FF); // OC Gray 8
        global_themes[1].border_color     = color_from_srgba_u32(0x495057FF); // OC Gray 7
        global_themes[1].text_color       = color_from_srgba_u32(0xF8F9FAFF); // OC Gray 0
    }

    State *state = global_state;

    Arena_Temporary scratch = arena_get_scratch(0, 0);
    Gfx_EventList events = gfx_get_events(scratch.arena);

    V2U32 client_area = gfx_get_window_client_area();
    render_begin(client_area);
    draw_begin_frame();
    ui_select_state(state->ui);
    ui_begin(&events, 1.0f / 60.0f);

    Theme *theme = &global_themes[1];

    // NOTE(simon): 14 pts * 96 pixels per inch / 72 points per inch
    ui_font_size_push((U32) (14.0f * 96.0f / 72.0f));

    R2F32 root_rectangle = r2f32(0.0f, 0.0f, (F32) client_area.x, (F32) client_area.y);
    F32 panel_pad = 2.0f;

    // NOTE(simon): Build non-leaf panel UI.
    for (Panel *panel = state->panel_root; panel; panel = panel_iterator_depth_first_pre_order(panel).next) {
        R2F32 panel_rectangle = rectangle_from_panel(panel, root_rectangle);
        V2F32 panel_rectangle_size = r2f32_size(panel_rectangle);

        for (Panel *child = panel->first; child && child->next; child = child->next) {
            R2F32 child_rectangle = rectangle_from_child_panel_parent_rectangle(child, panel_rectangle);
            R2F32 boundary_rectangle = child_rectangle;
            boundary_rectangle.min.values[panel->split_axis] = boundary_rectangle.max.values[panel->split_axis];
            boundary_rectangle.min.values[panel->split_axis] -= panel_pad;
            boundary_rectangle.max.values[panel->split_axis] += panel_pad;

            ui_fixed_position_next(boundary_rectangle.min);
            ui_width_next(ui_size_pixels(r2f32_size(boundary_rectangle).width, 1.0f));
            ui_height_next(ui_size_pixels(r2f32_size(boundary_rectangle).height, 1.0f));
            ui_hover_cursor_next(panel->split_axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
            UI_Box *boundary_box = ui_create_box_from_string_format(
                UI_BoxFlag_Clickable | UI_BoxFlag_FloatingPosition,
                "###panel_boundary_%p", child
            );
            UI_Input input = ui_input_from_box(boundary_box);

            if (input.input_flags & UI_InputFlag_LeftDragging) {
                Panel *min_child = child;
                Panel *max_child = child->next;

                local V2F32 drag_data = { 0 };
                if (input.input_flags & UI_InputFlag_LeftPressed) {
                    drag_data = v2f32(min_child->percentage_of_parent, max_child->percentage_of_parent);
                }

                F32 min_child_percentage_pre_drag = drag_data.x;
                F32 max_child_percentage_pre_drag = drag_data.y;
                F32 min_child_pixels_pre_drag = min_child_percentage_pre_drag * panel_rectangle_size.values[panel->split_axis];
                F32 max_child_pixels_pre_drag = max_child_percentage_pre_drag * panel_rectangle_size.values[panel->split_axis];

                // TODO(simon): This doesn't work if we have a big window, make
                // one of the panels 0 width, and then make the window smaller.
                V2F32 both_drag_delta = ui_drag_delta();
                F32 drag_delta = both_drag_delta.values[panel->split_axis];
                F32 clamped_drag_delta = drag_delta;
                if (drag_delta < 0.0f) {
                    clamped_drag_delta = -f32_min(-drag_delta, min_child_pixels_pre_drag - 2.0f * panel_pad);
                } else {
                    clamped_drag_delta = f32_min(drag_delta, max_child_pixels_pre_drag - 2.0f * panel_pad);
                }

                F32 min_child_pixels_post_drag = min_child_pixels_pre_drag + clamped_drag_delta;
                F32 max_child_pixels_post_drag = max_child_pixels_pre_drag - clamped_drag_delta;
                F32 min_child_percentage_post_drag = min_child_pixels_post_drag / panel_rectangle_size.values[panel->split_axis];
                F32 max_child_percentage_post_drag = max_child_pixels_post_drag / panel_rectangle_size.values[panel->split_axis];
                min_child->percentage_of_parent = min_child_percentage_post_drag;
                max_child->percentage_of_parent = max_child_percentage_post_drag;
            }
        }
    }

    // NOTE(simon): Build leaf panel UI.
    ui_color(theme->background_color)
    ui_border_color(theme->border_color)
    ui_layout_axis(Axis2_Y)
    for (Panel *panel = state->panel_root; panel; panel = panel_iterator_depth_first_pre_order(panel).next) {
        R2F32 panel_rectangle = r2f32_pad(rectangle_from_panel(panel, root_rectangle), -panel_pad);

        if (!panel->first) {
            ui_fixed_position_next(panel_rectangle.min);
            ui_width_next(ui_size_pixels(r2f32_size(panel_rectangle).width, 1.0f));
            ui_height_next(ui_size_pixels(r2f32_size(panel_rectangle).height, 1.0f));
            UI_Box *panel_box = ui_create_box_from_string_format(
                UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable | UI_BoxFlag_FloatingPosition | UI_BoxFlag_Clip,
                "###panel_box_%p", panel
            );

            ui_parent(panel_box) {
                if (panel->build_view) {
                    panel->build_view(panel, theme, panel_rectangle);
                } else {
                    // TODO(simon): Empty panel UI.
                }
            }
        }
    }

    ui_font_size_pop();
    ui_end();
    draw_clip(r2f32(0.0f, 0.0f, (F32) client_area.width, (F32) client_area.height)) {
        draw_ui(state->ui->root);
    }
    draw_submit();
    render_end();

    for (Gfx_Event *event = events.first, *next; event; event = next) {
        next = event->next;
        B32 consumed = false;

        if (event->kind == Gfx_EventKind_Quit) {
            state->running = false;
            consumed = true;
        }

        if (consumed) {
            dll_remove(events.first, events.last, event);
        }
    }

    arena_end_temporary(scratch);
    prof_frame_done();
}

internal S32 os_run(Str8List arguments) {
    if (!arguments.first->next) {
        os_console_print(str8_literal("You have to pass a file\n"));
        os_exit(1);
    }

    Arena *arena = arena_create();
    State *state = arena_push_struct(arena, State);
    state->arena = arena;
    state->ui = ui_create();
    state->panel_root = arena_push_struct_zero(state->arena, Panel);
    state->panel_root->percentage_of_parent = 1.0f;
    state->panel_root->split_axis = Axis2_X;
    {
        Panel *left = panel_create(state, view_glyph_list);
        Panel *right = arena_push_struct_zero(state->arena, Panel);
        right->split_axis = Axis2_Y;
        left->percentage_of_parent = 0.75f;
        right->percentage_of_parent = 0.25f;
        left->parent = right->parent = state->panel_root;
        dll_push_back(state->panel_root->first, state->panel_root->last, left);
        dll_push_back(state->panel_root->first, state->panel_root->last, right);

        Panel *top    = panel_create(state, view_glyph);
        Panel *bottom = panel_create(state, view_stats);
        top->percentage_of_parent = 0.75f;
        bottom->percentage_of_parent = 0.75f;
        top->parent = bottom->parent = right;
        dll_push_back(right->first, right->last, top);
        dll_push_back(right->first, right->last, bottom);
    }
    state->running = true;
    global_state = state;

    gfx_create(str8_literal("MSDF-gen"), 1280, 720);
    render_init();
    render_create();
    font_cache_create();

    state->font = font_create(arguments.first->next->string, 32, 64);
    state->ttf_font = ttf_load(arena, arguments.first->next->string);

    while (state->running) {
        update();
    }

    return 0;
}
