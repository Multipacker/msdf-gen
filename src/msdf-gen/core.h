#ifndef CORE_H
#define CORE_H

#define THEME_COLORS \
    X(Text,                  text,                    "Text")                    \
    X(Hover,                 hover,                   "Hover")                   \
    X(Cursor,                cursor,                  "Cursor")                  \
    X(Selection,             selection,               "Selection")               \
    X(Focus,                 focus,                   "Focus")                   \
    X(DropShadow,            drop_shadow,             "Drop shadow")             \
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

typedef struct Context Context;
struct Context {
    Context *next;

    // NOTE(simon): What are we acting on?
    Handle tab;
    Handle panel;

    // NOTE(simon): Where are we going?
    Handle destination_panel;
    Handle previous_tab;
    Direction2 direction;

    Str8 tab_specification;

    U32 codepoint;
};

#define context_top_values                                 \
    .tab               = top_context()->tab,               \
    .panel             = top_context()->panel,             \
    .destination_panel = top_context()->destination_panel, \
    .previous_tab      = top_context()->previous_tab,      \
    .direction         = top_context()->direction,         \
    .tab_specification = top_context()->tab_specification, \
    .codepoint         = top_context()->codepoint,

typedef enum {
    Command_FocusPanel,
    Command_ClosePanel,
    Command_SplitPanel,
    Command_OpenTab,
    Command_CloseTab,
    Command_PreviousTab,
    Command_NextTab,
    Command_MoveTab,
    Command_SaveProject,
    Command_SelectCodepoint,
} CommandKind;

typedef struct {
    CommandKind kind;
    Context *context;
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

typedef enum {
    DragState_None,
    DragState_Dragging,
    DragState_Dropping,
} DragState;

typedef struct State State;
struct State {
    Arena *arena;

    Arena *frame_arenas[2];
    U64 frame_index;

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
    F32 font_size;

    U32 frames_to_render;

    UI_Palette palettes[PaletteCode_COUNT];

    Context base_context;
    Context *context_stack;

    U64 previous_auto_save;
};

global State *global_state;

// NOTE(simon): Tabs
internal Tab   *tab_create(State *state, Str8 name);
internal Void   tab_free(State *state, Tab *tab);
internal Void  *tab_get_state(Tab *tab, U64 size);
internal Tab   *tab_from_handle(Handle handle);
internal Handle handle_from_tab(Tab *tab);

// NOTE(simon): Panels
internal Panel        *panel_create(State *state);
internal Void          panel_free(State *state, Panel *panel);
internal PanelIterator panel_iterator_depth_first_pre_order(Panel *panel);
internal R2F32         rectangle_from_child_panel_parent_rectangle(Panel *parent, Panel *child, R2F32 parent_rectangle);
internal R2F32         rectangle_from_panel(Panel *panel, R2F32 root_rectangle);
internal Void          panel_insert(Panel *parent, Panel *previous, Panel *child);
internal Void          panel_remove(Panel *parent, Panel *child);
internal Void          panel_remove_tab(Panel *panel, Tab *tab);
internal Void          panel_insert_tab(Panel *panel, Tab *previous_tab, Tab *tab);
internal Panel        *panel_from_handle(Handle handle);
internal Handle        handle_from_panel(Panel *panel);

// NOTE(simon): Context
internal Context *copy_context(Arena *arena, Context *context);
internal Void     push_context_internal(Context *context);
#define push_context(...) push_context_internal(&(Context) { context_top_values __VA_ARGS__ })
internal Void     pop_context(Void);
internal Context *top_context(Void);
#define context_scope(context) defer_loop(push_context(context), pop_context())

// NOTE(simon): Commands
internal Void push_command_internal(CommandKind kind, Context *context);
#define push_command(kind, ...) push_command_internal(kind, &(Context) { context_top_values __VA_ARGS__ })

// NOTE(simon): Drag-and-drop
internal B32  drag_is_active(Void);
internal Void drag_begin(Void);
internal B32  drag_drop(Void);
internal Void drag_cancel(Void);

// NOTE(simon): Themes
internal V4F32      color_from_theme(ThemeColor color);
internal UI_Palette palette_from_code(PaletteCode code);

// NOTE(simon): Frame related functions
internal Void request_frame(Void);
internal Void update(Void);
internal Arena *frame_arena(Void);

#endif // CORE_H
