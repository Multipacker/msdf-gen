#ifndef CORE_H
#define CORE_H

#define THEME_COLORS \
    X(Text,                      text,                        "Text")                        \
    X(WeakText,                  weak_text,                   "Weak text")                   \
    X(Hover,                     hover,                       "Hover")                       \
    X(Cursor,                    cursor,                      "Cursor")                      \
    X(Selection,                 selection,                   "Selection")                   \
    X(Focus,                     focus,                       "Focus")                       \
    X(DropShadow,                drop_shadow,                 "Drop shadow")                 \
    X(DisabledOverlay,           disabled_overlay,            "Disabled overlay")            \
    X(DropSiteOverlay,           drop_site_overlay,           "Drop site overlay")           \
    X(InactivePanelOverlay,      inactive_panel_overlay,      "Inactive panel overlay")      \
    X(BaseBackground,            base_background,             "Base background")             \
    X(BaseBorder,                base_border,                 "Base border")                 \
    X(TitleBarBackground,        title_bar_background,        "Title bar background")        \
    X(TitleBarBorder,            title_bar_border,            "Title bar border")            \
    X(TabBackground,             tab_background,              "Tab background")              \
    X(TabBorder,                 tab_border,                  "Tab border")                  \
    X(InactiveTabBackground,     inactive_tab_background,     "Inactive tab background")     \
    X(InactiveTabBorder,         inactive_tab_border,         "Inactive tab border")         \
    X(ButtonBackground,          button_background,           "Button background")           \
    X(ButtonBorder,              button_border,               "Button border")               \
    X(SecondaryButtonBackground, secondary_button_background, "Secondary button background") \
    X(SecondaryButtonBorder,     secondary_button_border,     "Secondary button border")     \
    X(Outline,                   outline,                     "Outline")                     \
    X(OnCurve,                   on_curve,                    "On curve")                    \
    X(OffCurve,                  off_curve,                   "Off curve")

global Str8 icon_kind_text[] = {
    [UI_IconKind_Minimize]   = str8_literal_compile("\uF2D1"),
    [UI_IconKind_Maximize]   = str8_literal_compile("\uF2D0"),
    [UI_IconKind_Close]      = str8_literal_compile("\uE807"),
    [UI_IconKind_Pin]        = str8_literal_compile("\uE809"),
    [UI_IconKind_Eye]        = str8_literal_compile("\uE80A"),
    [UI_IconKind_NoEye]      = str8_literal_compile("\uE80C"),
    [UI_IconKind_LeftArrow]  = str8_literal_compile("\uE801"),
    [UI_IconKind_RightArrow] = str8_literal_compile("\uE800"),
    [UI_IconKind_UpArrow]    = str8_literal_compile("\uE802"),
    [UI_IconKind_DownArrow]  = str8_literal_compile("\uE80B"),
    [UI_IconKind_LeftAngle]  = str8_literal_compile("\uE804"),
    [UI_IconKind_RightAngle] = str8_literal_compile("\uE805"),
    [UI_IconKind_UpAngle]    = str8_literal_compile("\uE806"),
    [UI_IconKind_DownAngle]  = str8_literal_compile("\uE803"),
    [UI_IconKind_Check]      = str8_literal_compile("\uE808"),
};

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

global Theme global_themes[6];

typedef enum {
    PaletteCode_Base,
    PaletteCode_TitleBar,
    PaletteCode_Button,
    PaletteCode_SecondaryButton,
    PaletteCode_Tab,
    PaletteCode_InactiveTab,
    PaletteCode_DropSiteOverlay,
    PaletteCode_COUNT,
} PaletteCode;

typedef struct Panel Panel;
typedef struct Tab Tab;

#define PANEL_BUILD_FUNCTION(name) Void name(Tab *tab, R2F32 panel_rectangle)
typedef PANEL_BUILD_FUNCTION(PanelBuildFunction);

typedef struct TabSpecification TabSpecification;
struct TabSpecification {
    Str8 name;
    Str8 display_name;
    PanelBuildFunction *build;
};

#define TABS                                              \
    X(Null,        "Empty",             view_null)        \
    X(GlyphList,   "Glyph list",        view_glyph_list)  \
    X(GlyphView,   "Glyph view",        view_glyph)       \
    X(GlyphDebug,  "Glyph debug",       view_glyph_debug) \
    X(RenderStats, "Render statistics", view_stats)       \
    X(Theme,       "Theme",             view_theme)       \
    X(Preview,     "Preview",           view_preview)     \
    X(Test,        "Test",              view_test)

#define X(name, ...) Tab_##name,
typedef enum {
    TABS
    Tab_COUNT,
} TabKind;
#undef X

#define X(name, display_name, build) PANEL_BUILD_FUNCTION(build);
TABS
#undef X

#define X(name, display_name, build) { str8_literal_compile(#name), str8_literal_compile(display_name), build, },
global TabSpecification tab_specifications[] = {
    TABS
};
#undef X

TabKind tab_kind_from_string(Str8 string);
TabSpecification *tab_specification_from_string(Str8 string);

typedef struct Handle Handle;
struct Handle {
    Void *data;
    U64 generation;
};

typedef enum {
    ContextSlot_Null,
    ContextSlot_Codepoint,
    ContextSlot_LogNode,
    ContextSlot_COUNT,
} ContextSlot;

typedef struct Context Context;
struct Context {
    Context *next;

    // NOTE(simon): What are we acting on?
    Handle tab;
    Handle panel;
    Str8   path;

    // NOTE(simon): Where are we going?
    Handle destination_panel;
    Handle previous_tab;
    Direction2 direction;

    Str8 tab_specification;

    U32           codepoint;
    MSDF_LogNode *log_node;
};

#define context_top_values                                 \
    .tab               = top_context()->tab,               \
    .panel             = top_context()->panel,             \
    .destination_panel = top_context()->destination_panel, \
    .previous_tab      = top_context()->previous_tab,      \
    .direction         = top_context()->direction,         \
    .tab_specification = top_context()->tab_specification, \
    .codepoint         = top_context()->codepoint,

// NOTE(simon): Enum name, show in command lister, display name, description
#define COMMANDS                                                                                                                                  \
    X(Quit,                 true,  "Quit",                        "Quits the program")                                                            \
    X(FocusPanel,           false, "Focus panel",                 "Focuses a panel")                                                              \
    X(FocusPanelLeft,       true,  "Focus panel left",            "Focuses a panel to the left")                                                  \
    X(FocusPanelUp,         true,  "Focus panel up",              "Focuses a panel to up")                                                        \
    X(FocusPanelRight,      true,  "Focus panel right",           "Focuses a panel to the right")                                                 \
    X(FocusPanelDown,       true,  "Focus panel down",            "Focuses a panel to down")                                                      \
    X(ClosePanel,           true,  "Close panel",                 "Closes the current panel")                                                     \
    X(SplitPanel,           true,  "Split panel",                 "Splits a panel")                                                               \
    X(OpenTab,              false, "Open tab",                    "Opens a new tab")                                                              \
    X(CloseTab,             true,  "Close tab",                   "Closes the current tab")                                                       \
    X(NextTab,              true,  "Next tab",                    "Switches to the next tab")                                                     \
    X(PreviousTab,          true,  "Previous tab",                "Switches to the previous tab")                                                 \
    X(MoveTab,              false, "Move tab",                    "Moves a tab from one panel to another")                                        \
    X(SaveProject,          true,  "Save project",                "Saves settings from this run of the program")                                  \
    X(SelectCodepoint,      false, "Select codepoint",            "Selects a codepoint as the active one")                                        \
    X(NextTheme,            true,  "Next theme",                  "Switches to the next theme")                                                   \
    X(PreviousTheme,        true,  "Previous theme",              "Switches to the previous theme")                                               \
    X(SelectWordLeft,       true,  "Select word left",            "Extends the selection one word to the left")                                   \
    X(SelectWordUp,         true,  "Select word up",              "Extends the selection one word up")                                            \
    X(SelectWordRight,      true,  "Select word right",           "Extends the selection one word to the right")                                  \
    X(SelectWordDown,       true,  "Select word down",            "Extends the selection one word down")                                          \
    X(SelectCharacterLeft,  true,  "Select character left",       "Extends the selection one character to the left")                              \
    X(SelectCharacterUp,    true,  "Select character up",         "Extends the selection one character up")                                       \
    X(SelectCharacterRight, true,  "Select character right",      "Extends the selection one character to the right")                             \
    X(SelectCharacterDown,  true,  "Select character down",       "Extends the selection one character down")                                     \
    X(MoveWordLeft,         true,  "Move word left",              "Moves one word to the left")                                                   \
    X(MoveWordUp,           true,  "Move word up",                "Moves one word up")                                                            \
    X(MoveWordRight,        true,  "Move word right",             "Moves one word to the right")                                                  \
    X(MoveWordDown,         true,  "Move word down",              "Moves one word down")                                                          \
    X(MoveCharacterLeft,    true,  "Move character left",         "Moves one character to the left")                                              \
    X(MoveCharacterUp,      true,  "Move character up",           "Moves one character up")                                                       \
    X(MoveCharacterRight,   true,  "Move character right",        "Moves one character to the right")                                             \
    X(MoveCharacterDown,    true,  "Move character down",         "Moves one character down")                                                     \
    X(SelectHome,           true,  "Select home",                 "Extends the selection to the start of the line")                               \
    X(SelectEnd,            true,  "Select end",                  "Extends the selection to the end of the line")                                 \
    X(MoveHome,             true,  "Move home",                   "Moves to the start of the line")                                               \
    X(MoveEnd,              true,  "Move end",                    "Moves to the end of the line")                                                 \
    X(SelectPageUp,         true,  "Select page up",              "Extends the selection on page up")                                             \
    X(SelectPageDown,       true,  "Select page down",            "Extends the selection on page down")                                           \
    X(MovePageUp,           true,  "Move page up",                "Moves one page up")                                                            \
    X(MovePageDown,         true,  "Move page down",              "Moves one page down")                                                          \
    X(SelectWholeUp,        true,  "Select whole up",             "Extends the selection to the begining")                                        \
    X(SelectWholeDown,      true,  "Select whole down",           "Extends the selection to the start")                                           \
    X(MoveWholeUp,          true,  "Move whole up",               "Moves to the beginging")                                                       \
    X(MoveWholeDown,        true,  "Move whole down",             "Moves to the end")                                                             \
    X(RemoveWord,           true,  "Remove word",                 "Removes one word")                                                             \
    X(DeleteWord,           true,  "Delete word",                 "Deletes one word")                                                             \
    X(RemoveCharacter,      true,  "Remove character",            "Removes one character")                                                        \
    X(DeleteCharacter,      true,  "Delete character",            "Deletes one character")                                                        \
    X(SelectAll,            true,  "Select all",                  "Selects everything")                                                           \
    X(Copy,                 true,  "Copy",                        "Copies the current selection to the clipboard")                                \
    X(Paste,                true,  "Paste",                       "Pastes the current clipboard contents")                                        \
    X(Cut,                  true,  "Cut",                         "Copies the current selection to the clipboard and deletes it")                 \
    X(ToggleListView,       true,  "Toggle list view",            "Toggles the display mode of glyph lists between unicode mode and mapped mode") \
    X(OpenCommandLister,    true,  "Open command lister",         "Opens the command lister")                                                     \
    X(OpenGlyphListView,    true,  "Open glyph list view",        "Opens a new tab with a glyph list")                                            \
    X(OpenGlyphViewView,    true,  "Open glyph view",             "Opens a new tab with a glyph inspector")                                       \
    X(OpenGlyphDebugView,   true,  "Open glyph debug view",       "Opens a new tab with a glyph debug inspector")                                 \
    X(OpenPreviewView,      true,  "Open preview view",           "Opens a new tab with a preview")                                               \
    X(OpenRenderStatsView,  true,  "Open render statistics view", "Opens a new tab with render statistics")                                       \
    X(OpenThemeView,        true,  "Open theme view",             "Opens a new tab with theme settings")                                          \
    X(OpenTestView,         true,  "Open test view",              "Opens a new tab with a test view")                                             \
    X(Accept,               true,  "Accept",                      "Accepts the current action")                                                   \
    X(Cancel,               true,  "Cancel",                      "Cancles the current action")                                                   \
    X(UnloadFont,           true,  "Unload font",                 "Unloads the current font")                                                     \
    X(LoadFont,             false, "Load font",                   "Loads a new font")                                                             \
    X(IncreaseFontSize,     true,  "Increase font size",          "Increases the font size by one point")                                         \
    X(DecreaseFontSize,     true,  "Decrease font size",          "Decreases the font size by one point")

#define X(name, show_in_ui, display_string, description) Command_##name,
typedef enum {
    COMMANDS
    Command_COUNT,
} CommandKind;
#undef X

#define X(name, show_in_ui, display_string, description) str8_literal_compile(display_string),
global Str8 command_names[] = {
    COMMANDS
};
#undef X

#define X(name, show_in_ui, display_string, description) show_in_ui,
global B8 command_show_in_ui[] = {
    COMMANDS
};
#undef X

#define X(name, show_in_ui, display_string, description) str8_literal_compile(description),
global Str8 command_descriptions[] = {
    COMMANDS
};
#undef X

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

    Str8    name;
    Arena  *arena;
    Void   *view_state;
    TabKind kind;

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

    R2F32 animated_rectangle_percentage;

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

    Gfx_Window    window;
    Render_Window render;

    UI_Context *ui;

    Panel *panel_root;
    Panel *panel_freelist;

    Tab *tab_freelist;

    Handle active_panel;

    // NOTE(simon): Drag and drop state
    DragState drag_state;

    B32 all_of_unicode;
    Arena *ttf_arena;
    TTF_Font *ttf_font;
    U32 codepoint;
    B32 running;

    ContextSlot hover_context_slot;
    Context    *hover_context;
    ContextSlot hover_context_slot_next;
    Context    *hover_context_next;

    // TODO(simon): This feels hacky and messy, we should probably use some
    // stable key for this instead.
    MSDF_LogNode *pinned_log_node;
    MSDF_LogNode *pinned_log_node_next;

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

    B32 show_command_lister;
    F32 command_lister_t;

    Arena *popup_arena;
    Str8 popup_title;
    Str8List popup_message_lines;
    F32 popup_t;
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
internal PanelIterator panel_iterator_depth_first_pre_order(Panel *panel, Panel *root);
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
internal Arena *frame_arena(Void);

internal Void update(Void);

#endif // CORE_H
