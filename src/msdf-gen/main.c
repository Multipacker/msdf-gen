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

/*
 * FIXME:
 * * UI active and hot keys should be per mouse button, not one for the whole UI
 * * Releasing mouse butttons outside of the window on windows doesn't generate
 *   release events.
 * * Releasing mouse butttons outside of the window on linux doesn't generate
 *   release events until you return the mouse pointer to the window.
 * * Pressing right or middle click during a panel resize creates black boxes.
 * * Window doens't repaint while resizing the window on windows.
 * * Scroll position of glyph view jumps occasionaly while switching tabs
 *   (probably because the box keys are tied to the panel???)
 * * Tabs and panels should be referenced through handles and not pointers to
 *   avoid crashes.
 * * Visualization of active and hot elements runs even when you right click on
 *   a button.
 * * Middle clicking on resizing borders causes panels to disappear.
 * * Tabbars are not clipped to the panels section of the screen, so they can
 *   overlap other panels.
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
    R2F32 uv;
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
typedef struct Tab Tab;

#define PANEL_BUILD_FUNCTION(name) Void name(Tab *tab, Theme *theme, R2F32 panel_rectangle)
typedef PANEL_BUILD_FUNCTION(PanelBuildFunction);

typedef struct {
    Str8 name;
    Str8 display_name;
    PanelBuildFunction *build;
} TabSpecification;

typedef enum {
    Command_FocusPanel,
    Command_ClosePanel,
    Command_OpenTab,
    Command_CloseTab,
    Command_PreviousTab,
    Command_NextTab,
    Command_MoveTab,
} CommandKind;

typedef struct {
    CommandKind kind;
    // NOTE(simon): What are we acting on?
    Tab   *tab;
    Panel *panel;
    // NOTE(simon): Where are we goind?
    Panel *destination_panel;
    Tab   *previous_tab;

    Str8   tab_specification;
} Command;

typedef struct CommandNode CommandNode;
struct CommandNode {
    CommandNode *next;
    Command command;
};

typedef struct {
    CommandNode *first;
    CommandNode *last;
} CommandList;

struct Tab {
    Tab *next;
    Tab *previous;

    Str8 name;
    Arena *arena;
    Void  *view_state;
    PanelBuildFunction *build_view;
};

struct Panel {
    Panel *next;
    Panel *previous;
    Panel *first;
    Panel *last;
    Panel *parent;
    F32    percentage_of_parent;
    Axis2  split_axis;
    U32    child_count;

    Tab *tab_first;
    Tab *tab_last;
    Tab *active_tab;
};

typedef struct PanelIterator PanelIterator;
struct PanelIterator {
    Panel *next;
    U32 push_count;
    U32 pop_count;
};

#include "views.h"

typedef enum {
    DragState_None,
    DragState_Dragging,
    DragState_Dropping,
} DragState;

typedef struct State State;
struct State {
    Arena *arena;

    UI_Context *ui;

    Panel *panel_root;
    Panel *panel_freelist;

    Tab *tab_freelist;

    Panel *active_panel;

    // NOTE(simon): Drag and drop state
    DragState drag_state;

    Font *font;
    TTF_Font *ttf_font;
    U32 selected_codepoint;
    B32 running;

    Arena *command_arena;
    CommandList commands;
};

global State *global_state;

#define push_command(_kind, ...) push_command_internal((Command) { .kind = _kind, __VA_ARGS__ })

internal Command *push_command_internal(Command command) {
    State *state = global_state;

    CommandNode *node = arena_push_struct_zero(state->command_arena, CommandNode);
    node->command = command;
    sll_queue_push(state->commands.first, state->commands.last, node);
    return &node->command;
}

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
        // source needing to include a 1/2 texel border for rendering. This
        // makes sure that the glyphs have the same visual size.
        F32 scale = ((F32) font->glyph_size - 1.0f) / ((F32) font->glyph_size - 2.0f) - 1.0f;
        V2F32 adjustment = v2f32_scale(v2f32_subtract(raster_result.max, raster_result.min), 0.5f * scale);

        result->advance_pt = raster_result.advance_width;
        result->min_pt = v2f32_subtract(raster_result.min, adjustment);
        result->max_pt = v2f32_add(raster_result.max, adjustment);
        result->uv = r2f32(
            (F32) atlas_position.x + 0.5f,
            (F32) atlas_position.y + 0.5f,
            (F32) atlas_position.x + (F32) font->glyph_size - 0.5f,
            (F32) atlas_position.y + (F32) font->glyph_size - 0.5f
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
            glyph->uv,
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
            letter->source,
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
                    letter->source,
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

            if (parent->flags & UI_BoxFlag_DrawBorder) {
                draw_rectangle(parent->calculated_rectangle, parent->border_color, 0.0f, 1.0f, 1.0f);
            }

            if (parent->flags & UI_BoxFlag_Disabled) {
                draw_rectangle(parent->calculated_rectangle, v4f32(0.2f, 0.2f, 0.2f, 0.75f), 0.0f, 0.0f, 0.0f);
            }
        }

        box = iterator.next;
    }

    prof_function_end();
}

#include "panels.c"
#include "views.c"

internal B32 drag_is_active(Void) {
    State *state = global_state;
    B32 result = (state->drag_state == DragState_Dragging || state->drag_state == DragState_Dropping);
    return result;
}

internal Void drag_begin(Void) {
    State *state = global_state;

    if (!drag_is_active()) {
        state->drag_state = DragState_Dragging;
    }
}

internal B32 drag_drop(Void) {
    State *state = global_state;

    B32 result = false;
    if (state->drag_state == DragState_Dropping) {
        result = true;
        state->drag_state = DragState_None;
    }

    return result;
}

internal Void update(Void) {
    State *state = global_state;

    Arena_Temporary scratch = arena_get_scratch(0, 0);
    Gfx_EventList events = gfx_get_events(scratch.arena);

    // NOTE(simon): Conseme events.
    for (Gfx_Event *event = events.first, *next; event; event = next) {
        next = event->next;
        B32 consume = false;

        if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_T && (event->key_modifiers & Gfx_KeyModifier_Control)) {
            consume = true;
            push_command(Command_OpenTab, .panel = state->active_panel, .tab_specification = str8_literal("RenderStats"));
        } else if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_W && (event->key_modifiers & Gfx_KeyModifier_Control)) {
            consume = true;
            push_command(Command_CloseTab, .panel = state->active_panel, .tab = state->active_panel->active_tab);
        } else if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_Tab && (event->key_modifiers & (Gfx_KeyModifier_Shift | Gfx_KeyModifier_Control)) == (Gfx_KeyModifier_Shift | Gfx_KeyModifier_Control)) {
            consume = true;
            push_command(Command_PreviousTab, .panel = state->active_panel);
        } else if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_Tab && (event->key_modifiers & Gfx_KeyModifier_Control)) {
            consume = true;
            push_command(Command_NextTab, .panel = state->active_panel);
        }

        if (drag_is_active() && event->kind == Gfx_EventKind_KeyRelease && event->key == Gfx_Key_MouseLeft) {
            state->drag_state = DragState_Dropping;
        }

        if (consume) {
            dll_remove(events.first, events.last, event);
        }
    }

    // NOTE(simon): Execute commands
    for (CommandNode *node = state->commands.first; node; node = node->next) {
        switch (node->command.kind) {
            case Command_FocusPanel: {
                state->active_panel = node->command.panel;
            } break;
            case Command_ClosePanel: {
                Panel *panel = node->command.panel;
                Panel *parent = panel->parent;

                if (parent) {
                    if (parent->child_count == 2) {
                        // NOTE(simon): Merge the panel that we keep with our grandparent.
                        Panel *discard_child = panel;
                        Panel *keep_child    = parent->first == discard_child ? parent->last : parent->first;
                        Panel *grandparent   = parent->parent;
                        Panel *previous      = parent->previous;
                        F32 parent_percentage = parent->percentage_of_parent;

                        panel_remove(parent, keep_child);

                        // NOTE(simon): Insert the panel we are keeping into the tree.
                        keep_child->percentage_of_parent = parent->percentage_of_parent;
                        if (grandparent) {
                            panel_remove(grandparent, parent);
                            panel_insert(grandparent, previous, keep_child);
                        } else {
                            state->panel_root = keep_child;
                        }

                        // NOTE(simon): Update active panel, recursing into children if needed.
                        if (state->active_panel == discard_child) {
                            state->active_panel = keep_child;
                            while (state->active_panel->first) {
                                state->active_panel = state->active_panel->first;
                            }
                        }

                        panel_free(state, discard_child);
                        panel_free(state, parent);

                        // NOTE(simon): If the split axis of keep child and grandparent are the same, merge their children.
                        if (grandparent && keep_child->first && grandparent->split_axis == keep_child->split_axis) {
                            Panel *child_previous = keep_child->previous;
                            panel_remove(grandparent, keep_child);

                            for (Panel *child = keep_child->first, *next; child; child = next) {
                                next = child->next;

                                panel_remove(keep_child, child);
                                panel_insert(grandparent, child_previous, child);
                                child_previous = child;
                                child->percentage_of_parent *= keep_child->percentage_of_parent;
                            }

                            panel_free(state, keep_child);
                        }
                    } else {
                        // NOTE(simon): Remove panel and adjust children to fill the empty space.
                        Panel *next = 0;
                        if (panel->next) {
                            next = panel->next;
                        } else if (panel->previous) {
                            next = panel->previous;
                        }
                        panel_remove(parent, panel);

                        for (Panel *child = parent->first; child; child = child->next) {
                            child->percentage_of_parent /= 1.0f - panel->percentage_of_parent;
                        }

                        // NOTE(simon): Update active panel, recursing into children if needed.
                        if (state->active_panel == panel) {
                            state->active_panel = next;
                            while (state->active_panel->first) {
                                state->active_panel = state->active_panel->first;
                            }
                        }

                        panel_free(state, panel);
                    }
                }
            } break;
            case Command_OpenTab: {
                TabSpecification *tab_spec = tab_specification_from_string(node->command.tab_specification);
                Tab *tab = tab_create(state, tab_spec->display_name);
                tab->build_view = tab_spec->build;

                Panel *panel = node->command.panel;
                panel_insert_tab(panel, panel->tab_last, tab);
            } break;
            case Command_CloseTab: {
                if (node->command.tab) {
                    panel_remove_tab(node->command.panel, node->command.tab);
                    tab_free(state, node->command.tab);
                }
            } break;
            case Command_PreviousTab: {
                Panel *panel = node->command.panel;
                Tab *next_tab = panel->active_tab;
                if (panel->active_tab->previous) {
                    next_tab = panel->active_tab->previous;
                } else if (panel->tab_last) {
                    next_tab = panel->tab_last;
                }

                panel->active_tab = next_tab;
            } break;
            case Command_NextTab: {
                Panel *panel = node->command.panel;
                Tab *next_tab = panel->active_tab;
                if (panel->active_tab->next) {
                    next_tab = panel->active_tab->next;
                } else if (panel->tab_first) {
                    next_tab = panel->tab_first;
                }

                panel->active_tab = next_tab;
            } break;
            case Command_MoveTab: {
                Panel *panel = node->command.panel;
                Tab   *tab   = node->command.tab;
                Panel *destination_panel = node->command.destination_panel;
                Tab   *previous_tab      = node->command.previous_tab;

                if (panel && destination_panel && tab != previous_tab) {
                    panel_remove_tab(panel, tab);
                    panel_insert_tab(destination_panel, previous_tab, tab);
                    state->active_panel = destination_panel;

                    if (!panel->tab_first && panel != state->panel_root) {
                        push_command(Command_ClosePanel, .panel = panel);
                    }
                }

            } break;
        }
    }
    arena_reset(state->command_arena);
    state->commands.first = 0;
    state->commands.last  = 0;

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

    V2U32 client_area = gfx_get_window_client_area();
    render_begin(client_area);
    draw_begin_frame();
    ui_select_state(state->ui);
    ui_begin(&events, 1.0f / 60.0f);

    Theme *theme = &global_themes[1];

    // NOTE(simon): 14 pts * 96 pixels per inch / 72 points per inch
    ui_font_size_push((U32) (11.0f * 96.0f / 72.0f));

    R2F32 root_rectangle = r2f32(0.0f, 0.0f, (F32) client_area.x, (F32) client_area.y);

    typedef struct DragTabData DragTabData;
    struct DragTabData {
        Panel *panel;
        Tab   *tab;
    };

    if (drag_is_active()) {
        ui_tooltip() {
            ui_width_next(ui_size_ems(60.0f, 1.0f));
            ui_height_next(ui_size_ems(40.0f, 1.0f));
            ui_layout_axis_next(Axis2_Y);
            ui_color_next(theme->element_color);
            UI_Box *container = ui_create_box_from_string(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clip, str8_literal("###drag_preview"));
            ui_parent(container) {
                DragTabData *data = ui_get_drag_data(DragTabData);
                if (data->tab && data->tab->build_view) {
                    data->tab->build_view(data->tab, theme, container->calculated_rectangle);
                }
            }
        }
    }

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
            Tab *next_active_tab = panel->active_tab;

            UI_Size tab_height = ui_size_ems(1.5f, 1.0f);
            R2F32 tab_bar_rectangle = r2f32(panel_rectangle.min.x, panel_rectangle.min.y, panel_rectangle.max.x, panel_rectangle.min.y + tab_height.value);
            R2F32 content_rectangle = r2f32(panel_rectangle.min.x, panel_rectangle.min.y + tab_height.value, panel_rectangle.max.x, panel_rectangle.max.y);

            ui_fixed_position_next(tab_bar_rectangle.min);
            ui_width_next(ui_size_pixels(r2f32_size(tab_bar_rectangle).width, 1.0f));
            ui_height_next(ui_size_pixels(r2f32_size(tab_bar_rectangle).height, 1.0f));
            ui_layout_axis_next(Axis2_X);
            UI_Box *tab_bar_box = ui_create_box_from_string_format(
                UI_BoxFlag_DrawBackground | UI_BoxFlag_Clickable | UI_BoxFlag_FloatingPosition | UI_BoxFlag_OverflowX | UI_BoxFlag_Clip,
                "###tab_bar_box_%p", panel
            );

            ui_color(theme->element_color)
            ui_border_color(theme->border_color)
            ui_width(ui_size_children_sum(1.0f))
            ui_height(tab_height)
            ui_layout_axis(Axis2_X)
            ui_parent(tab_bar_box) {
                for (Tab *tab = panel->tab_first; tab; tab = tab->next) {
                    if (tab == panel->active_tab) {
                        ui_border_color_next(color_from_srgba_u32(0x40C057FF));
                    }

                    ui_hover_cursor_next(Gfx_Cursor_Hand);
                    UI_Box *tab_box = ui_create_box_from_string_format(
                        UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive | UI_BoxFlag_AnimateX,
                        "###tab_%p", tab
                    );

                    ui_parent(tab_box) {
                        ui_text_align_next(UI_TextAlign_Left);
                        ui_width_next(ui_size_text_content(5.0f, 1.0f));
                        ui_label(tab->name);

                        ui_width_next(ui_size_ems(1.5f, 1.0f));
                        ui_text_align_next(UI_TextAlign_Center);
                        ui_hover_cursor_next(Gfx_Cursor_Hand);
                        UI_Box *close_box = ui_create_box_from_string_format(
                            UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText |
                            UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
                            UI_BoxFlag_Clickable,
                            "X##_tab_%p", tab
                        );
                        UI_Input close_input = ui_input_from_box(close_box);
                        if (close_input.input_flags & UI_InputFlag_LeftClicked) {
                            push_command(Command_CloseTab, .tab = tab, .panel = panel);
                        }
                    }

                    UI_Input input = ui_input_from_box(tab_box);

                    if (input.input_flags & UI_InputFlag_Hovering) {
                        ui_tooltip() {
                            ui_color_next(theme->border_color);
                            ui_extra_box_flags_next(UI_BoxFlag_DrawBackground);
                            ui_width_next(ui_size_text_content(5.0f, 1.0f));
                            ui_height_next(ui_size_text_content(0.0f, 1.0f));
                            ui_label(tab->name);
                        }
                    }

                    if (input.input_flags & UI_InputFlag_LeftPressed) {
                        push_command(Command_FocusPanel, .panel = panel);
                        next_active_tab = tab;
                    }

                    if (tab->next) {
                        ui_spacer_sized(ui_size_pixels(5.0f, 1.0f));
                    }

                    if (input.input_flags & UI_InputFlag_LeftDragging && !drag_is_active()) {
                        DragTabData data = {
                            .panel = panel,
                            .tab = tab,
                        };
                        ui_set_drag_data(&data);
                        drag_begin();
                    }
                }
            }

            if (panel == state->active_panel) {
                ui_border_color_next(color_from_srgba_u32(0x40C057FF));
            }
            ui_fixed_position_next(content_rectangle.min);
            ui_width_next(ui_size_pixels(r2f32_size(content_rectangle).width, 1.0f));
            ui_height_next(ui_size_pixels(r2f32_size(content_rectangle).height, 1.0f));
            UI_Box *content_box = ui_create_box_from_string_format(
                UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable | UI_BoxFlag_DropTarget | UI_BoxFlag_FloatingPosition | UI_BoxFlag_Clip,
                "###panel_box_%p", panel
            );

            ui_parent(content_box) {
                if (panel->active_tab && panel->active_tab->build_view) {
                    panel->active_tab->build_view(panel->active_tab, theme, content_rectangle);
                } else {
                    ui_width(ui_size_parent_percent(1.0f, 1.0f))
                    ui_height(ui_size_parent_percent(1.0f, 1.0f))
                    ui_column() {
                        ui_spacer_sized(ui_size_fill());
                        ui_height(ui_size_children_sum(1.0f))
                        ui_row() {
                            ui_spacer_sized(ui_size_fill());
                            ui_width_next(ui_size_text_content(5.0f, 1.0f));
                            ui_height_next(ui_size_text_content(0.0f, 1.0f));
                            UI_Input close_input = ui_button_format("Close panel###%p", panel);
                            if (close_input.input_flags & UI_InputFlag_LeftClicked) {
                                push_command(Command_ClosePanel, .panel = panel);
                            }

                            ui_spacer_sized(ui_size_fill());
                        }
                        ui_spacer_sized(ui_size_fill());
                    }
                }
            }

            // NOTE(simon): Consume fallthrough events.
            UI_Input content_input = ui_input_from_box(content_box);
            if (content_input.input_flags & UI_InputFlag_LeftClicked) {
                push_command(Command_FocusPanel, .panel = panel);
            }
            if (ui_drop_hot_key() == content_box->key && drag_drop()) {
                DragTabData *data = ui_get_drag_data(DragTabData);
                push_command(
                    Command_MoveTab,
                    .panel = data->panel,
                    .tab   = data->tab,
                    .destination_panel = panel,
                    .previous_tab      = panel->active_tab,
                );
            }

            panel->active_tab = next_active_tab;
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
    global_state = state;

    state->command_arena = arena_create();

    state->ui = ui_create();
    state->panel_root = arena_push_struct_zero(state->arena, Panel);
    state->panel_root->percentage_of_parent = 1.0f;
    state->panel_root->split_axis = Axis2_X;
    {
        Panel *left = panel_create(state);
        push_command(Command_OpenTab, .panel = left, .tab_specification = str8_literal("GlyphList"));

        Panel *right = panel_create(state);
        Panel *far_right = panel_create(state);
        right->split_axis = Axis2_Y;
        left->percentage_of_parent = 0.65f;
        right->percentage_of_parent = 0.25f;
        far_right->percentage_of_parent = 0.1f;
        panel_insert(state->panel_root, 0, left);
        panel_insert(state->panel_root, left, right);
        panel_insert(state->panel_root, right, far_right);

        // TODO(simon): This should be updated when the user navigates the interface
        state->active_panel = left;

        Panel *top = panel_create(state);
        push_command(Command_OpenTab, .panel = top, .tab_specification = str8_literal("GlyphView"));

        Panel *middle = panel_create(state);
        middle->split_axis = Axis2_X;
        {
            Panel *middle_left = panel_create(state);
            Panel *middle_right = panel_create(state);
            middle_left->percentage_of_parent = middle_right->percentage_of_parent = 0.5f;
            panel_insert(middle, 0, middle_left);
            panel_insert(middle, middle_left, middle_right);
        }

        Panel *bottom = panel_create(state);
        push_command(Command_OpenTab, .panel = bottom, .tab_specification = str8_literal("RenderStats"));

        top->percentage_of_parent = 0.65f;
        middle->percentage_of_parent = 0.1f;
        bottom->percentage_of_parent = 0.25f;
        panel_insert(right, 0, top);
        panel_insert(right, top, middle);
        panel_insert(right, middle, bottom);
    }
    state->running = true;

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
