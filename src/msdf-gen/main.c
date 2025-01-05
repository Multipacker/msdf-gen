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
 * * Offload MSDF generation to a background thread so that the main thread and
 *   UI don't hang because we are generating glyphs.
 * * Add drop-shadows to make it easier to distinguish foreground elements from
 *   background ones.
 * * Improve the look of the preview when dragging tabs
 *
 * TODO long term
 * * Allow multiple codepoints to map to the same glyph, alternatively allow
 *   marking glyphs as missing as that is the main use case
 */

/*
 * FIXME:
 * * Tabbars are not clipped to the panels section of the screen, so they can
 *   overlap other panels.
 * * Releasing a panel drag near the edge of the window causes a one frame
 *   visual bug
 */

#define THEME_COLORS \
    X(Text,                  text,                    "Text")                    \
    X(Hover,                 hover,                   "Hover")                   \
    X(DisabledOverlay,       disabled_overlay,        "Disabled overlay")        \
    X(DropSiteOverlay,       drop_site_overlay,       "Drop site overlay")       \
    X(InactivePanelOverlay,  inactive_panel_overlay,  "Inactive panel overlay")  \
    X(BaseBackground,        base_background,         "Base background")         \
    X(BaseBorder,            base_border,             "Base border")             \
    X(TabBackground,         tab_background,          "Tab background")          \
    X(TabBorder,             tab_border,              "Tab border")              \
    X(InactiveTabBackground, inactive_tab_background, "Inactive tab background") \
    X(InactiveTabBorder,     inactive_tab_border,     "Inactive tab border")     \
    X(ButtonBackground,      button_background,       "Button background")       \
    X(ButtonBorder,          button_border,           "Button border")           \
    X(Outline,               outline,                 "Outline")                 \
    X(OnCurve,               on_curve,                "On curve")                \
    X(OffCurve,              off_curve,               "Off curve")

#define X(name, snake_name, display_name) ThemeColor_##name,
typedef enum {
    THEME_COLORS
    ThemeColor_COUNT,
} ThemeColor;
#undef X

#define X(name, snake_name, display_name) str8_literal_compile(display_name),
global Str8 theme_color_names[] = {
    THEME_COLORS
};
#undef X

#define X(name, snake_name, display_name) V4F32 snake_name;
typedef struct Theme Theme;
struct Theme {
    Str8 name;
    union {
        V4F32 colors[ThemeColor_COUNT];
        struct {
            THEME_COLORS
        };
    };
};
#undef X

global Theme global_themes[4];

typedef enum {
    PaletteCode_Base,
    PaletteCode_Button,
    PaletteCode_Tab,
    PaletteCode_InactiveTab,
    PaletteCode_DropSiteOverlay,
    PaletteCode_COUNT,
} PaletteCode;

typedef struct Glyph Glyph;
struct Glyph {
    Glyph *next;
    Glyph *previous;

    U32 codepoint;

    R2F32 rectangle_pt;
    F32   advance_pt;
    R2F32 uv;
    Render_Texture texture;
};

global Glyph global_glyph_null = { 0 };

typedef struct GlyphList GlyphList;
struct GlyphList {
    Glyph *first;
    Glyph *last;
};

// NOTE(simon): Must be a power of 2
#define ATLAS_GLYPHS_PER_SIDE (1 << 6)
typedef struct Atlas Atlas;
struct Atlas {
    Atlas *next;
    Atlas *previous;

    Render_Texture texture;
    U64 occupancy[(ATLAS_GLYPHS_PER_SIDE * ATLAS_GLYPHS_PER_SIDE + 63) / 64];
};

typedef struct Font Font;
struct Font {
    Arena *arena;

    U32 glyph_size;
    U32 glyphs_per_side;

    Atlas *first_atlas;
    Atlas *last_atlas;

    GlyphList glyph_lists[2048];
    TTF_Font *ttf;

    U32 next_glyph_index;
};

typedef struct Panel Panel;
typedef struct Tab Tab;

#define PANEL_BUILD_FUNCTION(name) Void name(Tab *tab, R2F32 panel_rectangle)
typedef PANEL_BUILD_FUNCTION(PanelBuildFunction);

typedef struct {
    Str8 name;
    Str8 display_name;
    PanelBuildFunction *build;
} TabSpecification;

typedef struct Handle Handle;
struct Handle {
    Void *data;
    U64 generation;
};

typedef enum {
    Command_FocusPanel,
    Command_ClosePanel,
    Command_SplitPanel,
    Command_OpenTab,
    Command_CloseTab,
    Command_PreviousTab,
    Command_NextTab,
    Command_MoveTab,
} CommandKind;

typedef struct {
    CommandKind kind;
    // NOTE(simon): What are we acting on?
    Handle tab;
    Handle panel;
    // NOTE(simon): Where are we going?
    Handle destination_panel;
    Handle previous_tab;
    Direction2 direction;

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

    U64 generation;
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

    Handle active_tab;

    U64 generation;
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

    Handle active_panel;

    // NOTE(simon): Drag and drop state
    DragState drag_state;

    Font *font;
    TTF_Font *ttf_font;
    U32 selected_codepoint;
    B32 running;

    Arena *command_arena;
    CommandList commands;

    Theme theme;
    Theme target_theme;
    U32 theme_index;

    U32 frames_to_render;

    UI_Palette palettes[PaletteCode_COUNT];
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

internal Tab *tab_from_handle(Handle handle) {
    Tab *result = (Tab *) handle.data;

    if (result && handle.generation != result->generation) {
        result = 0;
    }

    return result;
}

internal Handle handle_from_tab(Tab *tab) {
    Handle result = { 0 };

    if (tab) {
        result.data = tab;
        result.generation = tab->generation;
    }

    return result;
}

internal Panel *panel_from_handle(Handle handle) {
    Panel *result = (Panel *) handle.data;

    if (result && handle.generation != result->generation) {
        result = 0;
    }
    return result;
}

internal Handle handle_from_panel(Panel *panel) {
    Handle result = { 0 };

    if (panel) {
        result.data = panel;
        result.generation = panel->generation;
    }

    return result;
}

internal Void request_frame(Void) {
    State *state = global_state;
    state->frames_to_render = 4;
}

internal Font *font_create(Str8 font_path, U32 glyph_size) {
    Arena *arena = arena_create();
    Font *result = arena_push_struct_zero(arena, Font);

    result->glyph_size = glyph_size;

    result->arena = arena;
    result->ttf   = ttf_load(arena, font_path);

    if (result->ttf->errors.node_count != 0) {
        Arena_Temporary scratch = arena_get_scratch(0, 0);
        os_console_print(str8_join(scratch.arena, &result->ttf->errors));
        arena_end_temporary(scratch);
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

    // NOTE(simon): Generate the glyph if it doesn't exist yet.
    if (result == &global_glyph_null) {
        result = arena_push_struct_zero(font->arena, Glyph);
        result->codepoint = codepoint;

        MSDF_RasterResult raster_result = msdf_generate(scratch.arena, font->ttf, codepoint, font->glyph_size);

        // NOTE(simon): Select glyph atlas.
        Atlas *selected_atlas = 0;
        for (Atlas *atlas = font->first_atlas; atlas && !selected_atlas; atlas = atlas->next) {
            for (U64 i = 0; i < array_count(atlas->occupancy); ++i) {
                if (~atlas->occupancy[i] != 0) {
                    selected_atlas = atlas;
                    break;
                }
            }
        }

        // NOTE(simon): Allocate a new atlas if we couldn't find one with space in it.
        if (!selected_atlas) {
            V2U32 size = v2u32(font->glyph_size * ATLAS_GLYPHS_PER_SIDE, font->glyph_size * ATLAS_GLYPHS_PER_SIDE);
            selected_atlas = arena_push_struct_zero(font->arena, Atlas);
            selected_atlas->texture = render_texture_create(size, Render_TextureFormat_RGBA8, 0);
            dll_push_back(font->first_atlas, font->last_atlas, selected_atlas);
        }

        // NOTE(simon): Allocate atlas region.
        U32 glyph_index = 0;
        while (~selected_atlas->occupancy[glyph_index / 64] == 0) {
            glyph_index += 64;
        }
        while ((selected_atlas->occupancy[glyph_index / 64] & (U64) (1 << glyph_index % 64)) != 0) {
            ++glyph_index;
        }
        selected_atlas->occupancy[glyph_index / 64] |= (U64) (1 << glyph_index % 64);

        V2U32 atlas_position = v2u32(
            font->glyph_size * (glyph_index % ATLAS_GLYPHS_PER_SIDE),
            font->glyph_size * (glyph_index / ATLAS_GLYPHS_PER_SIDE)
        );

        render_texture_update(selected_atlas->texture, atlas_position, v2u32(font->glyph_size, font->glyph_size), raster_result.data);

        // This adjustment increases the size of glyphs to acount for the
        // source needing to include a 1/2 texel border for rendering. This
        // makes sure that the glyphs have the same visual size.
        F32 scale = ((F32) font->glyph_size - 1.0f) / ((F32) font->glyph_size - 2.0f) - 1.0f;
        V2F32 adjustment = v2f32_scale(v2f32_subtract(raster_result.max, raster_result.min), 0.5f * scale);

        result->advance_pt = raster_result.advance_width;
        result->rectangle_pt = r2f32(
            raster_result.min.x - adjustment.x,
            raster_result.min.y - adjustment.y,
            raster_result.max.x + adjustment.x,
            raster_result.max.y + adjustment.y
        );

        result->uv = r2f32(
            (F32) atlas_position.x + 0.5f,
            (F32) atlas_position.y + 0.5f,
            (F32) atlas_position.x + (F32) font->glyph_size - 0.5f,
            (F32) atlas_position.y + (F32) font->glyph_size - 0.5f
        );
        result->texture = selected_atlas->texture;

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
                text_point.x + glyph->rectangle_pt.min.x * point_size,
                text_point.y + glyph->rectangle_pt.min.y * point_size,
                text_point.x + glyph->rectangle_pt.max.x * point_size,
                text_point.y + glyph->rectangle_pt.max.y * point_size
            ),
            glyph->uv,
            glyph->texture,
            v4f32(1.0f, 1.0f, 1.0f, 1.0f)
        );

        text_point.x += glyph->advance_pt * point_size;
    }
}

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

internal Void drag_cancel(Void) {
    State *state = global_state;

    if (drag_is_active()) {
        state->drag_state = DragState_None;
    }
}

internal V4F32 color_from_theme(ThemeColor color) {
    State *state = global_state;
    V4F32 result = state->theme.colors[color];
    return result;
}

internal UI_Palette palette_from_code(PaletteCode code) {
    State *state = global_state;
    UI_Palette result = state->palettes[code];
    return result;
}

#include "panels.c"
#include "views.c"

internal Void update(Void) {
    State *state = global_state;

    Arena_Temporary scratch = arena_get_scratch(0, 0);
    Gfx_EventList events = { 0 };

    local U32 depth = 0;
    if (depth == 0) {
        ++depth;
        events = gfx_get_events(scratch.arena, state->frames_to_render == 0);
        --depth;
    }

    // NOTE(simon): Consume events.
    for (Gfx_Event *event = events.first, *next; event; event = next) {
        next = event->next;
        B32 consume = false;

        if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_T && event->key_modifiers == Gfx_KeyModifier_Control) {
            consume = true;
            push_command(Command_OpenTab, .panel = state->active_panel, .tab_specification = str8_literal("RenderStats"));
        } else if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_W && event->key_modifiers == Gfx_KeyModifier_Control) {
            consume = true;
            push_command(Command_CloseTab, .panel = state->active_panel, .tab = panel_from_handle(state->active_panel)->active_tab);
        } else if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_Tab && event->key_modifiers == (Gfx_KeyModifier_Shift | Gfx_KeyModifier_Control)) {
            consume = true;
            push_command(Command_PreviousTab, .panel = state->active_panel);
        } else if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_Tab && event->key_modifiers == Gfx_KeyModifier_Control) {
            consume = true;
            push_command(Command_NextTab, .panel = state->active_panel);
        } else if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_N && event->key_modifiers == Gfx_KeyModifier_Control) {
            consume = true;
            request_frame();
            state->theme_index = (state->theme_index + 1) % array_count(global_themes);
            state->target_theme = global_themes[state->theme_index];
        } else if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_P && event->key_modifiers == Gfx_KeyModifier_Control) {
            consume = true;
            request_frame();
            state->theme_index = (state->theme_index + array_count(global_themes) - 1) % array_count(global_themes);
            state->target_theme = global_themes[state->theme_index];
        }

        if (drag_is_active() && event->kind == Gfx_EventKind_KeyRelease && event->key == Gfx_Key_MouseLeft) {
            state->drag_state = DragState_Dropping;
        }

        if (consume) {
            dll_remove(events.first, events.last, event);
        }
    }

    // NOTE(simon): If there are events left, they will go to the UI and
    // potentially trigger UI changes, so render more frames.
    if (events.first) {
        request_frame();
    }

    // NOTE(simon): Execute commands
    if (depth == 0) {
        for (CommandNode *node = state->commands.first; node; node = node->next) {
            request_frame();
            switch (node->command.kind) {
                case Command_FocusPanel: {
                    state->active_panel = node->command.panel;
                } break;
                case Command_ClosePanel: {
                    Panel *panel = panel_from_handle(node->command.panel);

                    if (panel && panel->parent) {
                        Panel *parent = panel->parent;
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
                            if (panel_from_handle(state->active_panel) == discard_child) {
                                Panel *next_panel = keep_child;
                                while (next_panel->first) {
                                    next_panel = next_panel->first;
                                }
                                state->active_panel = handle_from_panel(next_panel);
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
                            if (panel_from_handle(state->active_panel) == panel) {
                                Panel *next_panel = next;
                                while (next_panel->first) {
                                    next_panel = next_panel->first;
                                }
                                state->active_panel = handle_from_panel(next_panel);
                            }

                            panel_free(state, panel);
                        }
                    }
                } break;
                case Command_SplitPanel: {
                    Side side = side_from_direction2(node->command.direction);
                    Axis2 axis = axis2_from_direction2(node->command.direction);

                    Panel *split_panel = panel_from_handle(node->command.destination_panel);
                    if (split_panel && node->command.direction != Direction2_Invalid) {
                        Panel *parent = split_panel->parent;

                        Panel *new_panel = 0;
                        if (parent && axis == parent->split_axis) {
                            Panel *next = panel_create(state);
                            panel_insert(parent, side == Side_Max ? split_panel : split_panel->previous, next);
                            next->percentage_of_parent = 1.0f / (F32) parent->child_count;
                            for (Panel *child = parent->first; child; child = child->next) {
                                if (child != next) {
                                    child->percentage_of_parent *= (F32) (parent->child_count - 1) / (F32) parent->child_count;
                                }
                            }
                            state->active_panel = handle_from_panel(next);
                            new_panel = next;
                        } else {
                            Panel *previous_previous = split_panel->previous;
                            Panel *previous_parent = parent;
                            Panel *new_parent = panel_create(state);
                            new_parent->percentage_of_parent = split_panel->percentage_of_parent;
                            if (previous_parent) {
                                panel_remove(previous_parent, split_panel);
                                panel_insert(previous_parent, previous_previous, new_parent);
                            } else {
                                state->panel_root = new_parent;
                            }
                            Panel *left = split_panel;
                            Panel *right = panel_create(state);
                            new_panel = right;
                            if (side == Side_Min) {
                                swap(left, right, Panel *);
                            }

                            panel_insert(new_parent, 0, left);
                            panel_insert(new_parent, left, right);
                            new_parent->split_axis = axis;
                            left->percentage_of_parent = 0.5f;
                            right->percentage_of_parent = 0.5f;
                            state->active_panel = handle_from_panel(new_panel);
                        }

                        Panel *move_panel = panel_from_handle(node->command.panel);
                        Tab *move_tab = tab_from_handle(node->command.tab);

                        if (new_panel && move_panel && move_tab) {
                            panel_remove_tab(move_panel, move_tab);
                            panel_insert_tab(new_panel, new_panel->tab_last, move_tab);
                            new_panel->active_tab = handle_from_tab(move_tab);

                            if (!move_panel->tab_first && move_panel != state->panel_root && move_panel != new_panel->next && move_panel != new_panel->previous) {
                                push_command(Command_ClosePanel, .panel = handle_from_panel(move_panel));
                            }
                        }
                    }
                } break;
                case Command_OpenTab: {
                    Panel *panel = panel_from_handle(node->command.panel);
                    if (panel) {
                        TabSpecification *tab_spec = tab_specification_from_string(node->command.tab_specification);
                        Tab *tab = tab_create(state, tab_spec->display_name);
                        tab->build_view = tab_spec->build;
                        panel_insert_tab(panel, panel->tab_last, tab);
                    }
                } break;
                case Command_CloseTab: {
                    Panel *panel = panel_from_handle(node->command.panel);
                    Tab *tab = tab_from_handle(node->command.tab);
                    if (panel && tab) {
                        panel_remove_tab(panel, tab);
                        tab_free(state, tab);
                    }
                } break;
                case Command_PreviousTab: {
                    Panel *panel = panel_from_handle(node->command.panel);
                    if (panel) {
                        Tab *next_tab = tab_from_handle(panel->active_tab);
                        if (next_tab->previous) {
                            next_tab = next_tab->previous;
                        } else if (panel->tab_last) {
                            next_tab = panel->tab_last;
                        }

                        panel->active_tab = handle_from_tab(next_tab);
                    }
                } break;
                case Command_NextTab: {
                    Panel *panel = panel_from_handle(node->command.panel);
                    if (panel) {
                        Tab *next_tab = tab_from_handle(panel->active_tab);
                        if (next_tab->next) {
                            next_tab = next_tab->next;
                        } else if (panel->tab_first) {
                            next_tab = panel->tab_first;
                        }

                        panel->active_tab = handle_from_tab(next_tab);
                    }
                } break;
                case Command_MoveTab: {
                    Panel *panel = panel_from_handle(node->command.panel);
                    Tab   *tab   = tab_from_handle(node->command.tab);
                    Panel *destination_panel = panel_from_handle(node->command.destination_panel);
                    Tab   *previous_tab      = tab_from_handle(node->command.previous_tab);

                    if (panel && destination_panel && tab && tab != previous_tab) {
                        panel_remove_tab(panel, tab);
                        panel_insert_tab(destination_panel, previous_tab, tab);
                        state->active_panel = handle_from_panel(destination_panel);

                        if (!panel->tab_first && panel != state->panel_root) {
                            push_command(Command_ClosePanel, .panel = node->command.panel);
                        }
                    }

                } break;
            }
        }
        arena_reset(state->command_arena);
        state->commands.first = 0;
        state->commands.last  = 0;
    }

    // NOTE(simon): Build palettes
    state->palettes[PaletteCode_Base].background = state->theme.base_background;
    state->palettes[PaletteCode_Base].border     = state->theme.base_border;
    state->palettes[PaletteCode_Base].text       = state->theme.text;
    state->palettes[PaletteCode_Button].background = state->theme.button_background;
    state->palettes[PaletteCode_Button].border     = state->theme.button_border;
    state->palettes[PaletteCode_Button].text       = state->theme.text;
    state->palettes[PaletteCode_Tab].background = state->theme.tab_background;
    state->palettes[PaletteCode_Tab].border     = state->theme.tab_border;
    state->palettes[PaletteCode_Tab].text       = state->theme.text;
    state->palettes[PaletteCode_InactiveTab].background = state->theme.inactive_tab_background;
    state->palettes[PaletteCode_InactiveTab].border     = state->theme.inactive_tab_border;
    state->palettes[PaletteCode_InactiveTab].text       = state->theme.text;
    state->palettes[PaletteCode_DropSiteOverlay].background = state->theme.drop_site_overlay;
    state->palettes[PaletteCode_DropSiteOverlay].border     = state->theme.drop_site_overlay;
    state->palettes[PaletteCode_DropSiteOverlay].text       = state->theme.text;

    V2U32 client_area = gfx_get_window_client_area();
    render_begin(client_area);
    draw_begin_frame();
    {
        prof_zone_begin(prof_ui_build, "ui build");
        ui_select_state(state->ui);
        ui_begin(&events, 1.0f / 60.0f);

        ui_palette_push(palette_from_code(PaletteCode_Base));

        // NOTE(simon): 11 pts * 96 pixels per inch / 72 points per inch
        ui_font_size_push((U32) (11.0f * 96.0f / 72.0f));

        R2F32 root_rectangle = r2f32(0.0f, 0.0f, (F32) client_area.x, (F32) client_area.y);

        typedef struct DragTabData DragTabData;
        struct DragTabData {
            Handle panel;
            Handle tab;
        };

        if (drag_is_active()) {
            DragTabData *data = ui_get_drag_data(DragTabData);
            Tab *tab = tab_from_handle(data->tab);
            if (tab && tab->build_view) {
                ui_tooltip() {
                    ui_width_next(ui_size_ems(60.0f, 1.0f));
                    ui_height_next(ui_size_ems(40.0f, 1.0f));
                    ui_corner_radius_next(10.0f);
                    ui_layout_axis_next(Axis2_Y);
                    UI_Box *preview_box = ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder);
                    ui_parent(preview_box) {
                        ui_corner_radius_00_next(10.0f);
                        ui_corner_radius_01_next(10.0f);
                        ui_text_align_next(UI_TextAlign_Left);
                        ui_width_next(ui_size_text_content(5.0f, 1.0f));
                        ui_height_next(ui_size_text_content(0.0f, 1.0f));
                        ui_palette_next(palette_from_code(PaletteCode_Tab));
                        UI_Box *tab_box = ui_create_box_from_string(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawText, tab->name);

                        ui_spacer_sized(ui_size_pixels(10.0f, 1.0f));

                        ui_width_next(ui_size_fill());
                        ui_height_next(ui_size_fill());
                        UI_Box *content_box = ui_create_box_from_string(UI_BoxFlag_Clip, str8_literal("###drag_preview"));

                        ui_parent(content_box) {
                            tab->build_view(tab, content_box->calculated_rectangle);
                        }

                        ui_spacer_sized(ui_size_pixels(10.0f, 1.0f));
                    }
                }
            } else {
                drag_cancel();
            }
        }

        F32 panel_pad = 2.0f;

        // NOTE(simon): Build non-leaf panel UI.
        for (Panel *panel = state->panel_root; panel; panel = panel_iterator_depth_first_pre_order(panel).next) {
            if (!panel->first) {
                continue;
            }

            R2F32 panel_rectangle = rectangle_from_panel(panel, root_rectangle);
            V2F32 panel_rectangle_size = r2f32_size(panel_rectangle);

            if (drag_is_active()) {
                F32 drop_major_half_size = 5.0f * (F32) ui_font_size_top();
                F32 drop_minor_half_size = 3.0f * (F32) ui_font_size_top();
                F32 corner_radius = 0.5f * (F32) ui_font_size_top();
                F32 padding = 0.5f * (F32) ui_font_size_top();

                Axis2 split_axis = panel->split_axis;
                if (panel == state->panel_root) {
                    ui_corner_radius(corner_radius)
                    for (Side side = 0; side < Side_COUNT; ++side) {
                        V2F32 panel_center = r2f32_center(panel_rectangle);

                        UI_Key key = ui_key_from_string_format(global_ui_null_key, "root_extra_split_%i", side);
                        R2F32 drop_rectangle = { 0 };
                        drop_rectangle.min.values[axis2_flip(split_axis)] = panel_rectangle.values[side].values[axis2_flip(split_axis)] - drop_minor_half_size;
                        drop_rectangle.max.values[axis2_flip(split_axis)] = panel_rectangle.values[side].values[axis2_flip(split_axis)] + drop_minor_half_size;
                        drop_rectangle.min.values[split_axis] = panel_center.values[split_axis] - drop_major_half_size;
                        drop_rectangle.max.values[split_axis] = panel_center.values[split_axis] + drop_major_half_size;

                        ui_fixed_position_next(drop_rectangle.min);
                        ui_width_next(ui_size_pixels(r2f32_size(drop_rectangle).width, 1.0f));
                        ui_height_next(ui_size_pixels(r2f32_size(drop_rectangle).height, 1.0f));

                        ui_layout_axis_next(Axis2_Y);
                        UI_Box *drop_site = ui_create_box_from_key(UI_BoxFlag_FloatingPosition | UI_BoxFlag_DropTarget, key);
                        ui_input_from_box(drop_site);

                        ui_parent(drop_site)
                        ui_width(ui_size_fill())
                        ui_height(ui_size_fill())
                        ui_palette(palette_from_code(PaletteCode_DropSiteOverlay))
                        ui_padding(ui_size_pixels(padding, 1.0f))
                        ui_row()
                        ui_padding(ui_size_pixels(padding, 1.0f)) {
                            ui_layout_axis_next(split_axis);

                            if (ui_keys_match(key, ui_drop_hot_key())) {
                                UI_Palette overlay = ui_palette_top();
                                overlay.border = color_from_theme(ThemeColor_Hover);
                                ui_palette_next(overlay);
                            }
                            UI_Box *visualization = ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder);
                            ui_parent(visualization)
                            ui_padding(ui_size_pixels(padding, 1.0f))
                            {
                                ui_layout_axis_next(axis2_flip(split_axis));
                                UI_Box *row_or_column = ui_create_box(0);
                                ui_parent(row_or_column)
                                ui_padding(ui_size_pixels(padding, 1.0f)) {
                                    ui_create_box(UI_BoxFlag_DrawBorder);
                                    ui_spacer_sized(ui_size_pixels(padding, 1.0f));
                                    ui_create_box(UI_BoxFlag_DrawBorder);
                                }
                            }
                        }

                        if (ui_keys_match(key, ui_drop_hot_key())) {
                            R2F32 future_split_rectangle = drop_rectangle;
                            future_split_rectangle.min.values[axis2_flip(split_axis)] -= drop_major_half_size;
                            future_split_rectangle.max.values[axis2_flip(split_axis)] += drop_major_half_size;
                            future_split_rectangle.min.values[split_axis] = panel_rectangle.min.values[split_axis];
                            future_split_rectangle.max.values[split_axis] = panel_rectangle.max.values[split_axis];

                            ui_palette_next(palette_from_code(PaletteCode_DropSiteOverlay));
                            ui_fixed_position_next(future_split_rectangle.min);
                            ui_width_next(ui_size_pixels(r2f32_size(future_split_rectangle).width, 1.0f));
                            ui_height_next(ui_size_pixels(r2f32_size(future_split_rectangle).height, 1.0f));
                            ui_create_box(UI_BoxFlag_FloatingPosition | UI_BoxFlag_DrawBackground);
                        }

                        if (ui_keys_match(key, ui_drop_hot_key()) && drag_drop()) {
                            Direction2 direction = axis2_flip(split_axis) == Axis2_X ? Direction2_Left : Direction2_Up;
                            if (side == Side_Max) {
                                direction = axis2_flip(split_axis) == Axis2_X ? Direction2_Right : Direction2_Down;
                            }

                            DragTabData *data = ui_get_drag_data(DragTabData);
                            push_command(
                                Command_SplitPanel,
                                .panel = data->panel,
                                .tab   = data->tab,
                                .destination_panel = handle_from_panel(panel),
                                .direction = direction,
                            );
                        }
                    }
                }

                ui_corner_radius(corner_radius)
                for (Panel *child = panel->first;; child = child->next) {
                    R2F32 child_rectangle = rectangle_from_child_panel_parent_rectangle(panel, child, panel_rectangle);
                    V2F32 child_center = r2f32_center(child_rectangle);

                    UI_Key key = ui_key_from_string_format(global_ui_null_key, "drop_boundary_%p_%p", panel, child);
                    R2F32 drop_rectangle = { 0 };
                    drop_rectangle.min.values[split_axis] = child_rectangle.min.values[split_axis] - drop_minor_half_size;
                    drop_rectangle.max.values[split_axis] = child_rectangle.min.values[split_axis] + drop_minor_half_size;
                    drop_rectangle.min.values[axis2_flip(split_axis)] = child_center.values[axis2_flip(split_axis)] - drop_major_half_size;
                    drop_rectangle.max.values[axis2_flip(split_axis)] = child_center.values[axis2_flip(split_axis)] + drop_major_half_size;

                    ui_fixed_position_next(drop_rectangle.min);
                    ui_width_next(ui_size_pixels(r2f32_size(drop_rectangle).width, 1.0f));
                    ui_height_next(ui_size_pixels(r2f32_size(drop_rectangle).height, 1.0f));

                    ui_layout_axis_next(Axis2_Y);
                    UI_Box *drop_site = ui_create_box_from_key(UI_BoxFlag_FloatingPosition | UI_BoxFlag_DropTarget, key);
                    ui_input_from_box(drop_site);

                    ui_parent(drop_site)
                    ui_width(ui_size_fill())
                    ui_height(ui_size_fill())
                    ui_palette(palette_from_code(PaletteCode_DropSiteOverlay))
                    ui_padding(ui_size_pixels(padding, 1.0f))
                    ui_row()
                    ui_padding(ui_size_pixels(padding, 1.0f)) {
                        ui_layout_axis_next(axis2_flip(split_axis));
                            if (ui_keys_match(key, ui_drop_hot_key())) {
                                UI_Palette overlay = ui_palette_top();
                                overlay.border = color_from_theme(ThemeColor_Hover);
                                ui_palette_next(overlay);
                            }
                        UI_Box *visualization = ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder);
                        ui_parent(visualization)
                        ui_padding(ui_size_pixels(padding, 1.0f))
                        {
                            ui_layout_axis_next(split_axis);
                            UI_Box *row_or_column = ui_create_box(0);
                            ui_parent(row_or_column)
                            ui_padding(ui_size_pixels(padding, 1.0f)) {
                                ui_create_box(UI_BoxFlag_DrawBorder);
                                ui_spacer_sized(ui_size_pixels(padding, 1.0f));
                                ui_create_box(UI_BoxFlag_DrawBorder);
                            }
                        }
                    }

                    if (ui_keys_match(key, ui_drop_hot_key())) {
                        R2F32 future_split_rectangle = drop_rectangle;
                        future_split_rectangle.min.values[split_axis] -= drop_major_half_size;
                        future_split_rectangle.max.values[split_axis] += drop_major_half_size;
                        future_split_rectangle.min.values[axis2_flip(split_axis)] = child_rectangle.min.values[axis2_flip(split_axis)];
                        future_split_rectangle.max.values[axis2_flip(split_axis)] = child_rectangle.max.values[axis2_flip(split_axis)];

                        ui_palette_next(palette_from_code(PaletteCode_DropSiteOverlay));
                        ui_fixed_position_next(future_split_rectangle.min);
                        ui_width_next(ui_size_pixels(r2f32_size(future_split_rectangle).width, 1.0f));
                        ui_height_next(ui_size_pixels(r2f32_size(future_split_rectangle).height, 1.0f));
                        ui_create_box(UI_BoxFlag_FloatingPosition | UI_BoxFlag_DrawBackground);
                    }

                    if (ui_keys_match(key, ui_drop_hot_key()) && drag_drop()) {
                        Direction2 direction = split_axis == Axis2_X ? Direction2_Left : Direction2_Up;
                        Panel *split_panel = child;
                        if (!split_panel) {
                            split_panel = panel->last;
                            direction = split_axis == Axis2_X ? Direction2_Right : Direction2_Down;
                        }

                        DragTabData *data = ui_get_drag_data(DragTabData);
                        push_command(
                            Command_SplitPanel,
                            .panel = data->panel,
                            .tab   = data->tab,
                            .destination_panel = handle_from_panel(split_panel),
                            .direction = direction,
                        );
                    }

                    if (!child) {
                        break;
                    }
                }
            }

            for (Panel *child = panel->first; child && child->next; child = child->next) {
                R2F32 child_rectangle = rectangle_from_child_panel_parent_rectangle(panel, child, panel_rectangle);
                R2F32 boundary_rectangle = child_rectangle;
                boundary_rectangle.min.values[panel->split_axis] = boundary_rectangle.max.values[panel->split_axis];
                boundary_rectangle.min.values[panel->split_axis] -= panel_pad;
                boundary_rectangle.max.values[panel->split_axis] += panel_pad;

                ui_fixed_position_next(boundary_rectangle.min);
                ui_width_next(ui_size_pixels(r2f32_size(boundary_rectangle).width, 1.0f));
                ui_height_next(ui_size_pixels(r2f32_size(boundary_rectangle).height, 1.0f));
                ui_hover_cursor_next(panel->split_axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
                UI_Box *boundary_box = ui_create_box_from_string_format(UI_BoxFlag_Clickable | UI_BoxFlag_FloatingPosition, "###panel_boundary_%p", child);
                UI_Input input = ui_input_from_box(boundary_box);

                if (input.input_flags & UI_InputFlag_LeftDragging) {
                    Panel *min_child = child;
                    Panel *max_child = child->next;

                    if (input.input_flags & UI_InputFlag_LeftPressed) {
                        V2F32 drag_data = v2f32(min_child->percentage_of_parent, max_child->percentage_of_parent);
                        ui_set_drag_data(&drag_data);
                    }

                    V2F32 drag_data = *ui_get_drag_data(V2F32);

                    F32 min_child_percentage_pre_drag = drag_data.x;
                    F32 max_child_percentage_pre_drag = drag_data.y;
                    F32 min_child_pixels_pre_drag = min_child_percentage_pre_drag * panel_rectangle_size.values[panel->split_axis];
                    F32 max_child_pixels_pre_drag = max_child_percentage_pre_drag * panel_rectangle_size.values[panel->split_axis];

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
        ui_layout_axis(Axis2_Y)
        for (Panel *panel = state->panel_root; panel; panel = panel_iterator_depth_first_pre_order(panel).next) {
            if (panel->first) {
                continue;
            }

            R2F32 panel_rectangle = r2f32_pad(rectangle_from_panel(panel, root_rectangle), -panel_pad);

            if (drag_is_active() && r2f32_contains_v2f32(panel_rectangle, ui_mouse())) {
                V2F32 center = r2f32_center(panel_rectangle);
                F32 drop_half_size = 3.0f * (F32) ui_font_size_top();
                F32 corner_radius = 0.5f * (F32) ui_font_size_top();
                F32 padding = 0.5f * (F32) ui_font_size_top();
                typedef struct DropTarget DropTarget;
                struct DropTarget {
                    UI_Key key;
                    Direction2 direction;
                    R2F32 rectangle;
                };
                DropTarget targets[] = {
                    {
                        ui_key_from_string_format(global_ui_null_key, "drop_center_%p", panel),
                        Direction2_Invalid,
                        r2f32(
                            center.x - drop_half_size, center.y - drop_half_size,
                            center.x + drop_half_size, center.y + drop_half_size
                        ),
                    },
                    {
                        ui_key_from_string_format(global_ui_null_key, "drop_left_%p", panel),
                        Direction2_Left,
                        r2f32(
                            center.x - drop_half_size - 2.0f * drop_half_size, center.y - drop_half_size,
                            center.x + drop_half_size - 2.0f * drop_half_size, center.y + drop_half_size
                        ),
                    },
                    {
                        ui_key_from_string_format(global_ui_null_key, "drop_up_%p", panel),
                        Direction2_Up,
                        r2f32(
                            center.x - drop_half_size, center.y - drop_half_size - 2.0f * drop_half_size,
                            center.x + drop_half_size, center.y + drop_half_size - 2.0f * drop_half_size
                        ),
                    },
                    {
                        ui_key_from_string_format(global_ui_null_key, "drop_right_%p", panel),
                        Direction2_Right,
                        r2f32(
                            center.x - drop_half_size + 2.0f * drop_half_size, center.y - drop_half_size,
                            center.x + drop_half_size + 2.0f * drop_half_size, center.y + drop_half_size
                        ),
                    },
                    {
                        ui_key_from_string_format(global_ui_null_key, "drop_down_%p", panel),
                        Direction2_Down,
                        r2f32(
                            center.x - drop_half_size, center.y - drop_half_size + 2.0f * drop_half_size,
                            center.x + drop_half_size, center.y + drop_half_size + 2.0f * drop_half_size
                        ),
                    },
                };

                ui_corner_radius(corner_radius)
                for (U32 i = 0; i < array_count(targets); ++i) {
                    Axis2 axis = axis2_from_direction2(targets[i].direction);
                    if (targets[i].direction != Direction2_Invalid && panel->parent && axis == panel->parent->split_axis) {
                        continue;
                    }

                    ui_fixed_position_next(targets[i].rectangle.min);
                    ui_width_next(ui_size_pixels(r2f32_size(targets[i].rectangle).width, 1.0f));
                    ui_height_next(ui_size_pixels(r2f32_size(targets[i].rectangle).height, 1.0f));

                    UI_Box *drop_site = ui_create_box_from_key(UI_BoxFlag_FloatingPosition | UI_BoxFlag_DropTarget, targets[i].key);
                    ui_input_from_box(drop_site);

                    ui_parent(drop_site)
                    ui_width(ui_size_fill())
                    ui_height(ui_size_fill())
                    ui_palette(palette_from_code(PaletteCode_DropSiteOverlay))
                    ui_padding(ui_size_pixels(padding, 1.0f))
                    ui_row()
                    ui_padding(ui_size_pixels(padding, 1.0f)) {
                        if (ui_keys_match(targets[i].key, ui_drop_hot_key())) {
                            UI_Palette overlay = ui_palette_top();
                            overlay.border = color_from_theme(ThemeColor_Hover);
                            ui_palette_next(overlay);
                        }
                        ui_layout_axis_next(axis2_flip(axis));
                        UI_Box *visualization = ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder);
                        ui_parent(visualization)
                        ui_width(ui_size_fill())
                        ui_height(ui_size_fill())
                        ui_padding(ui_size_pixels(padding, 1.0f)) {
                            if (targets[i].direction != Direction2_Invalid) {
                                ui_layout_axis_next(axis);
                                UI_Box *row_or_column = ui_create_box(0);
                                ui_parent(row_or_column)
                                ui_padding(ui_size_pixels(padding, 1.0f)) {
                                    ui_create_box(UI_BoxFlag_DrawBorder);
                                    ui_spacer_sized(ui_size_pixels(padding, 1.0f));
                                    ui_create_box(UI_BoxFlag_DrawBorder);
                                }
                            } else {
                                ui_layout_axis_next(axis);
                                UI_Box *row_or_column = ui_create_box(0);
                                ui_parent(row_or_column)
                                ui_padding(ui_size_pixels(padding, 1.0f)) {
                                    ui_create_box(UI_BoxFlag_DrawBorder);
                                }
                            }
                        }
                    }

                    if (ui_keys_match(targets[i].key, ui_drop_hot_key()) && drag_drop()) {
                        if (targets[i].direction == Direction2_Invalid) {
                            DragTabData *data = ui_get_drag_data(DragTabData);
                            push_command(
                                Command_MoveTab,
                                .panel = data->panel,
                                .tab   = data->tab,
                                .destination_panel = handle_from_panel(panel),
                                .previous_tab      = panel->active_tab,
                            );
                        } else {
                            DragTabData *data = ui_get_drag_data(DragTabData);
                            push_command(
                                Command_SplitPanel,
                                .panel = data->panel,
                                .tab   = data->tab,
                                .destination_panel = handle_from_panel(panel),
                                .direction = targets[i].direction,
                            );
                        }
                    }

                    if (ui_keys_match(targets[i].key, ui_drop_hot_key())) {
                        Axis2 split_axis = axis2_from_direction2(targets[i].direction);
                        Side split_side = side_from_direction2(targets[i].direction);
                        R2F32 future_split_rectangle = panel_rectangle;
                        if (targets[i].direction != Direction2_Invalid) {
                            V2F32 panel_center = r2f32_center(panel_rectangle);
                            future_split_rectangle.values[side_flip(split_side)].values[split_axis] = panel_center.values[split_axis];
                        }

                        ui_palette_next(palette_from_code(PaletteCode_DropSiteOverlay));
                        ui_fixed_position_next(future_split_rectangle.min);
                        ui_width_next(ui_size_pixels(r2f32_size(future_split_rectangle).width, 1.0f));
                        ui_height_next(ui_size_pixels(r2f32_size(future_split_rectangle).height, 1.0f));
                        ui_create_box(UI_BoxFlag_DrawBackground);
                    }
                }
            }

            Tab *next_active_tab = tab_from_handle(panel->active_tab);

            UI_Size tab_height = ui_size_ems(2.0f, 1.0f);
            R2F32 tab_bar_rectangle = r2f32(panel_rectangle.min.x, panel_rectangle.min.y, panel_rectangle.max.x, panel_rectangle.min.y + tab_height.value);
            R2F32 content_rectangle = r2f32(panel_rectangle.min.x, panel_rectangle.min.y + tab_height.value, panel_rectangle.max.x, panel_rectangle.max.y);

            if (panel != panel_from_handle(state->active_panel)) {
                UI_Palette overlay = ui_palette_top();
                overlay.background = color_from_theme(ThemeColor_InactivePanelOverlay);
                ui_palette_next(overlay);
                ui_fixed_position_next(panel_rectangle.min);
                ui_width_next(ui_size_pixels(r2f32_size(panel_rectangle).width, 1.0f));
                ui_height_next(ui_size_pixels(r2f32_size(panel_rectangle).height, 1.0f));
                ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_FloatingPosition);
            }

            ui_fixed_position_next(tab_bar_rectangle.min);
            ui_width_next(ui_size_pixels(r2f32_size(tab_bar_rectangle).width, 1.0f));
            ui_height_next(ui_size_pixels(r2f32_size(tab_bar_rectangle).height, 1.0f));
            ui_layout_axis_next(Axis2_X);
            UI_Box *tab_bar_box = ui_create_box_from_string_format(
                UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable | UI_BoxFlag_FloatingPosition | UI_BoxFlag_OverflowX | UI_BoxFlag_Clip,
                "###tab_bar_box_%p", panel
            );

            ui_palette(palette_from_code(PaletteCode_InactiveTab))
            ui_width(ui_size_children_sum(1.0f))
            ui_height(tab_height)
            ui_layout_axis(Axis2_X)
            ui_parent(tab_bar_box)
            ui_corner_radius_00(10.0f)
            ui_corner_radius_01(10.0f) {
                for (Tab *tab = panel->tab_first; tab; tab = tab->next) {
                    if (tab == tab_from_handle(panel->active_tab)) {
                        ui_palette_push(palette_from_code(PaletteCode_Tab));
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
                            push_command(Command_CloseTab, .tab = handle_from_tab(tab), .panel = handle_from_panel(panel));
                        }
                    }

                    UI_Input input = ui_input_from_box(tab_box);

                    if (input.input_flags & UI_InputFlag_LeftPressed) {
                        push_command(Command_FocusPanel, .panel = handle_from_panel(panel));
                        next_active_tab = tab;
                    }

                    if (tab->next) {
                        ui_spacer_sized(ui_size_ems(0.3f, 1.0f));
                    }

                    if (input.input_flags & UI_InputFlag_LeftDragging && !drag_is_active() && v2f32_length(ui_drag_delta()) > 10.0f) {
                        DragTabData data = {
                            .panel = handle_from_panel(panel),
                            .tab = handle_from_tab(tab),
                        };
                        ui_set_drag_data(&data);
                        drag_begin();
                    }

                    if (tab == tab_from_handle(panel->active_tab)) {
                        ui_palette_pop();
                    }
                }
            }

            ui_fixed_position_next(content_rectangle.min);
            ui_width_next(ui_size_pixels(r2f32_size(content_rectangle).width, 1.0f));
            ui_height_next(ui_size_pixels(r2f32_size(content_rectangle).height, 1.0f));
            UI_Box *content_box = ui_create_box_from_string_format(
                UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable | UI_BoxFlag_DropTarget | UI_BoxFlag_FloatingPosition | UI_BoxFlag_Clip,
                "###panel_box_%p", panel
            );

            ui_parent(content_box) {
                Tab *tab = tab_from_handle(panel->active_tab);
                if (tab && tab->build_view) {
                    tab->build_view(tab, content_rectangle);
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
                            ui_palette_next(palette_from_code(PaletteCode_Button));
                            ui_corner_radius_next(5.0f);
                            UI_Input close_input = ui_button_format("Close panel###%p", panel);
                            if (close_input.input_flags & UI_InputFlag_LeftClicked) {
                                push_command(Command_ClosePanel, .panel = handle_from_panel(panel));
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
                push_command(Command_FocusPanel, .panel = handle_from_panel(panel));
            }
            if (ui_drop_hot_key() == content_box->key && drag_drop()) {
                DragTabData *data = ui_get_drag_data(DragTabData);
                push_command(
                    Command_MoveTab,
                    .panel = data->panel,
                    .tab   = data->tab,
                    .destination_panel = handle_from_panel(panel),
                    .previous_tab      = panel->active_tab,
                );
            }

            panel->active_tab = handle_from_tab(next_active_tab);
        }

        ui_font_size_pop();
        ui_palette_pop();
        ui_end();
        prof_zone_end(prof_ui_build);
    }
    draw_clip(r2f32(0.0f, 0.0f, (F32) client_area.width, (F32) client_area.height)) {
        prof_zone_begin(prof_draw_ui, "draw ui");

        for (UI_Box *box = state->ui->root; box != &global_ui_null_box;) {
            if (box->flags & UI_BoxFlag_DrawBackground) {
                {
                    Render_Shape *shape = draw_rectangle(box->calculated_rectangle, box->palette.background, 0.0f, 0.0f, 1.0f);
                    memory_copy(shape->radies, box->corner_radies, sizeof(shape->radies));
                }

                if (box->flags & UI_BoxFlag_DrawHot && box->hot_t > 0.0f) {
                    Render_Shape *rect = draw_rectangle(box->calculated_rectangle, v4f32(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 1.0f);
                    rect->colors[0] = color_from_theme(ThemeColor_Hover);
                    rect->colors[1] = color_from_theme(ThemeColor_Hover);
                    rect->colors[0].a *= box->hot_t;
                    rect->colors[1].a *= box->hot_t;
                    memory_copy(rect->radies, box->corner_radies, sizeof(rect->radies));
                }

                if (box->flags & UI_BoxFlag_DrawActive && box->active_t > 0.0f) {
                    Render_Shape *rect = draw_rectangle(box->calculated_rectangle, v4f32(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 1.0f);
                    rect->colors[2] = v4f32(0.0f, 0.0f, 0.0f, 0.5f * box->active_t);
                    rect->colors[3] = v4f32(0.0f, 0.0f, 0.0f, 0.5f * box->active_t);
                    memory_copy(rect->radies, box->corner_radies, sizeof(rect->radies));
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
                        box->palette.text
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
                    Render_Shape *shape = draw_rectangle(parent->calculated_rectangle, parent->palette.border, 0.0f, 1.0f, 1.0f);
                    memory_copy(shape->radies, parent->corner_radies, sizeof(shape->radies));
                }

                if (parent->flags & UI_BoxFlag_Disabled) {
                    Render_Shape *shape = draw_rectangle(parent->calculated_rectangle, color_from_theme(ThemeColor_DisabledOverlay), 0.0f, 0.0f, 1.0f);
                    memory_copy(shape->radies, box->corner_radies, sizeof(shape->radies));
                }
            }

            box = iterator.next;
        }

        prof_zone_end(prof_draw_ui);
    }
    draw_submit();
    render_end();

    // NOTE(simon): Animate theme
    {
        B32 is_animating = false;
        for (ThemeColor color_index = 0; color_index < ThemeColor_COUNT; ++color_index) {
            V4F32 *color = &state->theme.colors[color_index];
            V4F32 *target_color = &state->target_theme.colors[color_index];

            is_animating |= f32_abs(target_color->r - color->r) > 0.001f;
            is_animating |= f32_abs(target_color->g - color->g) > 0.001f;
            is_animating |= f32_abs(target_color->b - color->b) > 0.001f;
            is_animating |= f32_abs(target_color->a - color->a) > 0.001f;

            color->r += (target_color->r - color->r) * ui_animation_slow_rate();
            color->g += (target_color->g - color->g) * ui_animation_slow_rate();
            color->b += (target_color->b - color->b) * ui_animation_slow_rate();
            color->a += (target_color->a - color->a) * ui_animation_slow_rate();
        }

        if (is_animating) {
            request_frame();
        }
    }

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

    // NOTE(simon): Cancel drag and drop if nothing caught it.
    if (state->drag_state == DragState_Dropping) {
        drag_cancel();
    }

    if (state->frames_to_render > 0) {
        --state->frames_to_render;
    }

    if (ui_is_animating_from_context(state->ui)) {
        request_frame();
    }

    if (PROFILE_BUILD) {
        request_frame();
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

    // NOTE(simon): Themes
    {
        {
            Theme *theme = &global_themes[0];
            theme->name = str8_literal("Catppuccin Latte");

            V4F32 rosewater = color_from_srgba_u32(0xDC8A78FF);
            V4F32 flamingo  = color_from_srgba_u32(0xDD7878FF);
            V4F32 pink      = color_from_srgba_u32(0xEA76CBFF);
            V4F32 mauve     = color_from_srgba_u32(0x8839EFFF);
            V4F32 red       = color_from_srgba_u32(0xD20F39FF);
            V4F32 maroon    = color_from_srgba_u32(0xE64553FF);
            V4F32 peach     = color_from_srgba_u32(0xFE640BFF);
            V4F32 yellow    = color_from_srgba_u32(0xDF8E1DFF);
            V4F32 green     = color_from_srgba_u32(0x40A02BFF);
            V4F32 teal      = color_from_srgba_u32(0x179299FF);
            V4F32 sky       = color_from_srgba_u32(0x04A5E5FF);
            V4F32 sapphire  = color_from_srgba_u32(0x209FB5FF);
            V4F32 blue      = color_from_srgba_u32(0x1E66F5FF);
            V4F32 lavender  = color_from_srgba_u32(0x7287FDFF);
            V4F32 text      = color_from_srgba_u32(0x4C4F69FF);
            V4F32 subtext1  = color_from_srgba_u32(0x5C5F77FF);
            V4F32 subtext0  = color_from_srgba_u32(0x6C6F85FF);
            V4F32 overlay2  = color_from_srgba_u32(0x7C7F93FF);
            V4F32 overlay1  = color_from_srgba_u32(0x8C8FA1FF);
            V4F32 overlay0  = color_from_srgba_u32(0x9CA0B0FF);
            V4F32 surface2  = color_from_srgba_u32(0xACB0BEFF);
            V4F32 surface1  = color_from_srgba_u32(0xBCC0CCFF);
            V4F32 surface0  = color_from_srgba_u32(0xCCD0DAFF);
            V4F32 base      = color_from_srgba_u32(0xEFF1F5FF);
            V4F32 mantle    = color_from_srgba_u32(0xE6E9EFFF);
            V4F32 crust     = color_from_srgba_u32(0xDCE0E8FF);

            theme->text  = text;
            theme->hover = overlay2;

            theme->disabled_overlay       = overlay0;
            theme->disabled_overlay.a = 0.5f;
            theme->drop_site_overlay      = overlay1;
            theme->drop_site_overlay.a = 0.5f;
            theme->inactive_panel_overlay = crust;
            theme->inactive_panel_overlay.a = 0.2f;

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface0;

            theme->outline   = overlay0;
            theme->on_curve  = green;
            theme->off_curve = red;
        }

        {
            Theme *theme = &global_themes[1];
            theme->name = str8_literal("Catppuccin Frappé");

            V4F32 rosewater = color_from_srgba_u32(0xF2D5CFFF);
            V4F32 flamingo  = color_from_srgba_u32(0xEEBEBEFF);
            V4F32 pink      = color_from_srgba_u32(0xF4B8E4FF);
            V4F32 mauve     = color_from_srgba_u32(0xCA9EE6FF);
            V4F32 red       = color_from_srgba_u32(0xE78284FF);
            V4F32 maroon    = color_from_srgba_u32(0xEA999CFF);
            V4F32 peach     = color_from_srgba_u32(0xEF9F76FF);
            V4F32 yellow    = color_from_srgba_u32(0xE5C890FF);
            V4F32 green     = color_from_srgba_u32(0xA6D189FF);
            V4F32 teal      = color_from_srgba_u32(0x81C8BEFF);
            V4F32 sky       = color_from_srgba_u32(0x99D1DBFF);
            V4F32 sapphire  = color_from_srgba_u32(0x85C1DCFF);
            V4F32 blue      = color_from_srgba_u32(0x8CAAEEFF);
            V4F32 lavender  = color_from_srgba_u32(0xBABBF1FF);
            V4F32 text      = color_from_srgba_u32(0xC6D0F5FF);
            V4F32 subtext1  = color_from_srgba_u32(0xB5BFE2FF);
            V4F32 subtext0  = color_from_srgba_u32(0xA5ADCEFF);
            V4F32 overlay2  = color_from_srgba_u32(0x949CBBFF);
            V4F32 overlay1  = color_from_srgba_u32(0x838BA7FF);
            V4F32 overlay0  = color_from_srgba_u32(0x737994FF);
            V4F32 surface2  = color_from_srgba_u32(0x626880FF);
            V4F32 surface1  = color_from_srgba_u32(0x51576DFF);
            V4F32 surface0  = color_from_srgba_u32(0x414559FF);
            V4F32 base      = color_from_srgba_u32(0x303446FF);
            V4F32 mantle    = color_from_srgba_u32(0x292C3CFF);
            V4F32 crust     = color_from_srgba_u32(0x232634FF);

            theme->text  = text;
            theme->hover = overlay2;

            theme->disabled_overlay       = overlay0;
            theme->disabled_overlay.a = 0.5f;
            theme->drop_site_overlay      = overlay1;
            theme->drop_site_overlay.a = 0.5f;
            theme->inactive_panel_overlay = crust;
            theme->inactive_panel_overlay.a = 0.5f;

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface0;

            theme->outline   = overlay0;
            theme->on_curve  = green;
            theme->off_curve = red;
        }

        {
            Theme *theme = &global_themes[2];
            theme->name = str8_literal("Catppuccin Macchiato");

            V4F32 rosewater = color_from_srgba_u32(0xF4DBD6FF);
            V4F32 flamingo  = color_from_srgba_u32(0xF0C6C6FF);
            V4F32 pink      = color_from_srgba_u32(0xF5BDE6FF);
            V4F32 mauve     = color_from_srgba_u32(0xC6A0F6FF);
            V4F32 red       = color_from_srgba_u32(0xED8796FF);
            V4F32 maroon    = color_from_srgba_u32(0xEE99A0FF);
            V4F32 peach     = color_from_srgba_u32(0xF5A97FFF);
            V4F32 yellow    = color_from_srgba_u32(0xEED49FFF);
            V4F32 green     = color_from_srgba_u32(0xA6DA95FF);
            V4F32 teal      = color_from_srgba_u32(0x8BD5CAFF);
            V4F32 sky       = color_from_srgba_u32(0x91D7E3FF);
            V4F32 sapphire  = color_from_srgba_u32(0x7DC4E4FF);
            V4F32 blue      = color_from_srgba_u32(0x8AADF4FF);
            V4F32 lavender  = color_from_srgba_u32(0xB7BDF8FF);
            V4F32 text      = color_from_srgba_u32(0xCAD3F5FF);
            V4F32 subtext1  = color_from_srgba_u32(0xB8C0E0FF);
            V4F32 subtext0  = color_from_srgba_u32(0xA5ADCBFF);
            V4F32 overlay2  = color_from_srgba_u32(0x939AB7FF);
            V4F32 overlay1  = color_from_srgba_u32(0x8087A2FF);
            V4F32 overlay0  = color_from_srgba_u32(0x6E738DFF);
            V4F32 surface2  = color_from_srgba_u32(0x5B6078FF);
            V4F32 surface1  = color_from_srgba_u32(0x494D64FF);
            V4F32 surface0  = color_from_srgba_u32(0x363A4FFF);
            V4F32 base      = color_from_srgba_u32(0x24273AFF);
            V4F32 mantle    = color_from_srgba_u32(0x1E2030FF);
            V4F32 crust     = color_from_srgba_u32(0x181926FF);

            theme->text  = text;
            theme->hover = overlay2;

            theme->disabled_overlay       = overlay0;
            theme->disabled_overlay.a = 0.5f;
            theme->drop_site_overlay      = overlay1;
            theme->drop_site_overlay.a = 0.5f;
            theme->inactive_panel_overlay = crust;
            theme->inactive_panel_overlay.a = 0.5f;

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface0;

            theme->outline   = overlay0;
            theme->on_curve  = green;
            theme->off_curve = red;
        }

        {
            Theme *theme = &global_themes[3];
            theme->name = str8_literal("Catppuccin Mocha");

            V4F32 rosewater = color_from_srgba_u32(0xF5E0DCFF);
            V4F32 flamingo  = color_from_srgba_u32(0xF2CDCDFF);
            V4F32 pink      = color_from_srgba_u32(0xF5C2E7FF);
            V4F32 mauve     = color_from_srgba_u32(0xCBA6F7FF);
            V4F32 red       = color_from_srgba_u32(0xF38BA8FF);
            V4F32 maroon    = color_from_srgba_u32(0xEBA0ACFF);
            V4F32 peach     = color_from_srgba_u32(0xFAB387FF);
            V4F32 yellow    = color_from_srgba_u32(0xF9E2AFFF);
            V4F32 green     = color_from_srgba_u32(0xA6E3A1FF);
            V4F32 teal      = color_from_srgba_u32(0x94E2D5FF);
            V4F32 sky       = color_from_srgba_u32(0x89DCEBFF);
            V4F32 sapphire  = color_from_srgba_u32(0x74C7ECFF);
            V4F32 blue      = color_from_srgba_u32(0x89B4FAFF);
            V4F32 lavender  = color_from_srgba_u32(0xB4BEFEFF);
            V4F32 text      = color_from_srgba_u32(0xCDD6F4FF);
            V4F32 subtext1  = color_from_srgba_u32(0xBAC2DEFF);
            V4F32 subtext0  = color_from_srgba_u32(0xA6ADC8FF);
            V4F32 overlay2  = color_from_srgba_u32(0x9399B2FF);
            V4F32 overlay1  = color_from_srgba_u32(0x7F849CFF);
            V4F32 overlay0  = color_from_srgba_u32(0x6C7086FF);
            V4F32 surface2  = color_from_srgba_u32(0x585B70FF);
            V4F32 surface1  = color_from_srgba_u32(0x45475AFF);
            V4F32 surface0  = color_from_srgba_u32(0x313244FF);
            V4F32 base      = color_from_srgba_u32(0x1E1E2EFF);
            V4F32 mantle    = color_from_srgba_u32(0x181825FF);
            V4F32 crust     = color_from_srgba_u32(0x11111BFF);

            theme->text  = text;
            theme->hover = overlay2;

            theme->disabled_overlay       = overlay0;
            theme->disabled_overlay.a = 0.5f;
            theme->drop_site_overlay      = overlay1;
            theme->drop_site_overlay.a = 0.5f;
            theme->inactive_panel_overlay = crust;
            theme->inactive_panel_overlay.a = 0.5f;

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface0;

            theme->outline   = overlay0;
            theme->on_curve  = green;
            theme->off_curve = red;
        }
    }

    state->ui = ui_create();
    state->panel_root = arena_push_struct_zero(state->arena, Panel);
    state->panel_root->percentage_of_parent = 1.0f;
    state->panel_root->split_axis = Axis2_X;
    {
        Panel *left = panel_create(state);
        push_command(Command_OpenTab, .panel = handle_from_panel(left), .tab_specification = str8_literal("Theme"));
        push_command(Command_OpenTab, .panel = handle_from_panel(left), .tab_specification = str8_literal("GlyphList"));

        Panel *right = panel_create(state);
        right->split_axis = Axis2_Y;
        left->percentage_of_parent = 0.65f;
        right->percentage_of_parent = 0.35f;
        panel_insert(state->panel_root, 0, left);
        panel_insert(state->panel_root, left, right);

        // TODO(simon): This should be updated when the user navigates the interface
        state->active_panel = handle_from_panel(left);

        Panel *top = panel_create(state);
        push_command(Command_OpenTab, .panel = handle_from_panel(top), .tab_specification = str8_literal("GlyphView"));

        Panel *bottom = panel_create(state);
        push_command(Command_OpenTab, .panel = handle_from_panel(bottom), .tab_specification = str8_literal("RenderStats"));

        top->percentage_of_parent = 0.65f;
        bottom->percentage_of_parent = 0.35f;
        panel_insert(right, 0, top);
        panel_insert(right, top, bottom);
    }

    state->running = true;

    state->theme_index = 3;
    state->theme = global_themes[state->theme_index];
    state->target_theme = global_themes[state->theme_index];

    state->frames_to_render = 4;

    gfx_create(str8_literal("MSDF-gen"), 1280, 720);
    render_init();
    render_create();
    font_cache_create();
    gfx_set_update_function(update);

    state->font = font_create(arguments.first->next->string, 32);
    state->ttf_font = ttf_load(arena, arguments.first->next->string);

    while (state->running) {
        update();
    }

    return 0;
}
