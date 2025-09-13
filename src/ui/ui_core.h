#ifndef UI_CORE_H
#define UI_CORE_H

typedef enum {
    UI_IconKind_Null,
    UI_IconKind_Minimize,
    UI_IconKind_Maximize,
    UI_IconKind_Close,
    UI_IconKind_Pin,
    UI_IconKind_Eye,
    UI_IconKind_NoEye,
    UI_IconKind_LeftArrow,
    UI_IconKind_RightArrow,
    UI_IconKind_UpArrow,
    UI_IconKind_DownArrow,
    UI_IconKind_LeftAngle,
    UI_IconKind_RightAngle,
    UI_IconKind_UpAngle,
    UI_IconKind_DownAngle,
    UI_IconKind_Check,
    UI_IconKind_File,
    UI_IconKind_Folder,
    UI_IconKind_COUNT,
} UI_IconKind;

typedef struct UI_IconInfo UI_IconInfo;
struct UI_IconInfo {
    FontCache_Font *icon_font;
    Str8            icon_kind_text[UI_IconKind_COUNT];
};

typedef enum {
    UI_Size_Pixels,
    UI_Size_ChildrenSum,
    UI_Size_ParentPercent,
    UI_Size_TextContent,
    UI_Size_AspectRatio,
} UI_SizeKind;

typedef struct UI_Size UI_Size;
struct UI_Size {
    UI_SizeKind kind;
    F32 value;
    F32 strictness;
};

typedef enum {
    UI_TextAlign_Left,
    UI_TextAlign_Center,
    UI_TextAlign_Right,
    UI_TextAlign_COUNT,
} UI_TextAlign;

typedef U64 UI_Key;

typedef enum {
    UI_MouseButton_Left,
    UI_MouseButton_Middle,
    UI_MouseButton_Right,
    UI_MouseButton_COUNT,
} UI_MouseButton;

typedef struct UI_Box UI_Box;

#define UI_BOX_DRAW_FUNCTION(name) Void name(UI_Box *box, Void *data)
typedef UI_BOX_DRAW_FUNCTION(UI_BoxDrawFunction);

typedef enum {
    UI_Focus_None,
    UI_Focus_Active,
    UI_Focus_Inactive,
    UI_Focus_Root,
    UI_Focus_COUNT,
} UI_Focus;

typedef enum {
    UI_Color_Background,
    UI_Color_Text,
    UI_Color_Border,
    UI_Color_Cursor,
    UI_Color_Selection,
    UI_Color_COUNT,
} UI_Color;

typedef union UI_Palette UI_Palette;
union UI_Palette {
    V4F32 colors[UI_Color_COUNT];
    struct {
        V4F32 background;
        V4F32 text;
        V4F32 border;
        V4F32 cursor;
        V4F32 selection;
    };
};

typedef enum {
    // NOTE(simon): Interaction
    UI_BoxFlag_Disabled              = 1 << 0,
    UI_BoxFlag_Clickable             = 1 << 1,
    UI_BoxFlag_Scrollable            = 1 << 2,
    UI_BoxFlag_DropTarget            = 1 << 3,
    UI_BoxFlag_KeyboardClickable     = 1 << 4,

    // NOTE(simon): Layout
    UI_BoxFlag_OverflowX             = 1 << 5,
    UI_BoxFlag_OverflowY             = 1 << 6,
    UI_BoxFlag_FloatingX             = 1 << 7,
    UI_BoxFlag_FloatingY             = 1 << 8,

    // NOTE(simon): Appearance
    UI_BoxFlag_AnimateX              = 1 << 9,
    UI_BoxFlag_AnimateY              = 1 << 10,
    UI_BoxFlag_DrawBackground        = 1 << 11,
    UI_BoxFlag_DrawBorder            = 1 << 12,
    UI_BoxFlag_DrawText              = 1 << 13,
    UI_BoxFlag_DrawHot               = 1 << 14,
    UI_BoxFlag_DrawActive            = 1 << 15,
    UI_BoxFlag_DrawDropShadow        = 1 << 16,
    UI_BoxFlag_DrawFuzzyMatches      = 1 << 17,
    UI_BoxFlag_Clip                  = 1 << 18,
    UI_BoxFlag_DisableFocusBorder    = 1 << 19,
    UI_BoxFlag_DisableFocusOverlay   = 1 << 20,

    UI_BoxFlag_FocusHot              = 1 << 21,
    UI_BoxFlag_FocusHotDisabled      = 1 << 22,
    UI_BoxFlag_FocusActive           = 1 << 23,
    UI_BoxFlag_FocusActiveDisabled   = 1 << 24,

    UI_BoxFlag_DefaultNavigation     = 1 << 25,
    UI_BoxFlag_ClickToFocus          = 1 << 26,
    UI_BoxFlag_DefaultNavigationSkip = 1 << 27,

    // NOTE(simon): Convenient combinations
    UI_BoxFlag_Overflow            = UI_BoxFlag_OverflowX | UI_BoxFlag_OverflowY,
    UI_BoxFlag_AnimatePosition     = UI_BoxFlag_AnimateX  | UI_BoxFlag_AnimateY,
    UI_BoxFlag_FloatingPosition    = UI_BoxFlag_FloatingX | UI_BoxFlag_FloatingY,
    UI_BoxFlag_DisableFocusEffects = UI_BoxFlag_DisableFocusBorder | UI_BoxFlag_DisableFocusOverlay,
} UI_BoxFlags;

struct UI_Box {
    UI_Box *parent;
    UI_Box *next;
    UI_Box *previous;
    UI_Box *first;
    UI_Box *last;

    UI_Box *hash_next;
    UI_Box *hash_previous;

    UI_Key key;

    // NOTE(simon): Per build state
    UI_Size             size[Axis2_COUNT];
    UI_BoxFlags         flags;
    UI_Palette          palette;
    Axis2               layout_axis;
    Str8                string;
    FontCache_Font     *font;
    U32                 font_size;
    UI_TextAlign        text_align;
    V2F32               text_padding;
    Gfx_Cursor          hover_cursor;
    Draw_List          *draw_list;
    UI_BoxDrawFunction *draw_function;
    Void               *draw_data;
    F32                 corner_radies[Corner_COUNT];

    FontCache_Text text;
    FuzzyMatchList fuzzy_matches;
    V2F32 calculated_size;
    V2F32 calculated_position;
    R2F32 calculated_rectangle;
    V2F32 view_offset;

    V2F32 animated_position;

    U64 create_index;
    U64 last_used_index;

    F32 hot_t;
    F32 active_t;
    F32 disabled_t;
    F32 focus_hot_t;
    F32 focus_active_t;
    F32 focus_active_disabled_t;

    UI_Key default_navigation_focus_hot_key;
    UI_Key default_navigation_focus_hot_key_next;
    UI_Key default_navigation_focus_active_key;
    UI_Key default_navigation_focus_active_key_next;
};

typedef struct UI_BoxIterator UI_BoxIterator;
struct UI_BoxIterator {
    UI_Box *next;
    U32 push_count;
    U32 pop_count;
};

#define ui_define_stack(type_name, variable_name, type)                                                     \
    typedef struct UI_##type_name##StackNode UI_##type_name##StackNode;                                     \
    struct UI_##type_name##StackNode {                                                                      \
        UI_##type_name##StackNode *next;                                                                    \
        type                       item;                                                                    \
    };                                                                                                      \
    typedef struct UI_##type_name##Stack UI_##type_name##Stack;                                             \
    struct UI_##type_name##Stack {                                                                          \
        UI_##type_name##StackNode *top;                                                                     \
        UI_##type_name##StackNode *freelist;                                                                \
        B32                        auto_pop;                                                                \
    };                                                                                                      \
    internal Void ui_##variable_name##_stack_push(UI_##type_name##Stack *stack, type value, B32 auto_pop) { \
        UI_##type_name##StackNode *node = 0;                                                                \
        if (stack->auto_pop) {                                                                              \
            node = stack->top;                                                                              \
            sll_stack_pop(stack->top);                                                                      \
        } else if (stack->freelist) {                                                                       \
            node = stack->freelist;                                                                         \
            sll_stack_pop(stack->freelist);                                                                 \
        } else {                                                                                            \
            node = arena_push_struct(ui_frame_arena(), UI_##type_name##StackNode);                          \
        }                                                                                                   \
        node->item = value;                                                                                 \
        sll_stack_push(stack->top, node);                                                                   \
        stack->auto_pop = auto_pop;                                                                         \
    }                                                                                                       \
    internal type ui_##variable_name##_stack_pop(UI_##type_name##Stack *stack) {                            \
        UI_##type_name##StackNode *node = stack->top;                                                       \
        if (node) {                                                                                         \
            sll_stack_pop(stack->top);                                                                      \
            sll_stack_push(stack->freelist, node);                                                          \
        }                                                                                                   \
        return node->item;                                                                                  \
    }                                                                                                       \
    internal Void ui_##variable_name##_stack_auto_pop(UI_##type_name##Stack *stack) {                       \
        if (stack->auto_pop) {                                                                              \
            stack->auto_pop = false;                                                                        \
            UI_##type_name##StackNode *node = stack->top;                                                   \
            sll_stack_pop(stack->top);                                                                      \
            sll_stack_push(stack->freelist, node);                                                          \
        }                                                                                                   \
    }

internal Arena *ui_frame_arena(Void);

ui_define_stack(Box,             box,               UI_Box *)
ui_define_stack(Palette,         palette,           UI_Palette)
ui_define_stack(Size,            size,              UI_Size)
ui_define_stack(Axis,            axis,              Axis2)
ui_define_stack(BoxFlags,        box_flags,         UI_BoxFlags)
ui_define_stack(F32,             f32,               F32)
ui_define_stack(U32,             u32,               U32)
ui_define_stack(Font,            font,              FontCache_Font *)
ui_define_stack(Cursor,          cursor,            Gfx_Cursor)
ui_define_stack(BoxDrawFunction, box_draw_function, UI_BoxDrawFunction *)
ui_define_stack(Pointer,         pointer,           Void *)
ui_define_stack(TextAlign,       text_align,        UI_TextAlign)
ui_define_stack(Focus,           focus,             UI_Focus)

typedef struct UI_BoxList UI_BoxList;
struct UI_BoxList {
    UI_Box *first;
    UI_Box *last;
};

typedef enum {
    // NOTE(simon): Pressed while hovering.
    UI_InputFlag_LeftPressed     = 1 << 0,
    UI_InputFlag_MiddlePressed   = 1 << 1,
    UI_InputFlag_RightPressed    = 1 << 2,

    // NOTE(simon): Previously pressed and now user released the button.
    UI_InputFlag_LeftReleased    = 1 << 3,
    UI_InputFlag_MiddleReleased  = 1 << 4,
    UI_InputFlag_RightReleased   = 1 << 5,

    // NOTE(simon): Previously pressed and released in bounds.
    UI_InputFlag_LeftClicked     = 1 << 6,
    UI_InputFlag_MiddleClicked   = 1 << 7,
    UI_InputFlag_RightClicked    = 1 << 8,

    // NOTE(simon): Pressed and holding in box.
    UI_InputFlag_LeftDragging     = 1 << 9,
    UI_InputFlag_MiddleDragging   = 1 << 10,
    UI_InputFlag_RightDragging    = 1 << 11,

    // NOTE(simon): Mouse is over this box.
    UI_InputFlag_Hovering        = 1 << 12,

    // NOTE(simon): Keyboard interaction with
    UI_InputFlag_KeyboardPressed = 1 << 13,
    UI_InputFlag_Commit          = 1 << 14,

    // NOTE(simon): Convenient combinations
    UI_InputFlag_Pressed  = UI_InputFlag_LeftPressed  | UI_InputFlag_KeyboardPressed,
    UI_InputFlag_Released = UI_InputFlag_LeftReleased,
    UI_InputFlag_Clicked  = UI_InputFlag_LeftClicked  | UI_InputFlag_KeyboardPressed,
    UI_InputFlag_Dragging = UI_InputFlag_LeftDragging,
} UI_InputFlag;

typedef struct UI_Input UI_Input;
struct UI_Input {
    UI_Box *box;
    UI_InputFlag flags;
    V2F32 scroll;
};

typedef enum {
    UI_EventKind_Null,
    UI_EventKind_KeyPress,
    UI_EventKind_KeyRelease,
    UI_EventKind_Text,
    UI_EventKind_Navigation,
    UI_EventKind_Edit,
    UI_EventKind_Scroll,
    UI_EventKind_Accept,
    UI_EventKind_Cancel,
    UI_EventKind_COUNT,
} UI_EventKind;

typedef enum {
    UI_EventFlag_KeepMark             = 1 << 0,
    UI_EventFlag_ZeroDeltaOnSelection = 1 << 1,
    UI_EventFlag_Delete               = 1 << 2,
    UI_EventFlag_PickSelectSide       = 1 << 3,
    UI_EventFlag_Copy                 = 1 << 4,
} UI_EventFlags;

typedef enum {
    UI_EventDeltaUnit_Null,
    UI_EventDeltaUnit_Character,
    UI_EventDeltaUnit_Word,
    UI_EventDeltaUnit_Line,
    UI_EventDeltaUnit_Page,
    UI_EventDeltaUnit_Whole,
    UI_EventDeltaUnit_COUNT,
} UI_EventDeltaUnit;

typedef struct UI_Event UI_Event;
struct UI_Event {
    UI_Event     *next;
    UI_Event     *previous;

    UI_EventKind      kind;
    V2S64             delta;
    UI_EventDeltaUnit unit;
    UI_EventFlags     flags;
    Str8              text;
    V2F32             position;
    Gfx_Key           key;
    Gfx_KeyModifier   modifiers;
    V2F32             scroll;
};

typedef struct UI_EventList UI_EventList;
struct UI_EventList {
    UI_Event *first;
    UI_Event *last;
};

#define UI_BOX_TABLE_SIZE (1 << 12)
#define UI_ANIMATION_TABLE_SIZE (1 << 12)

typedef struct UI_AnimationParameters UI_AnimationParameters;
struct UI_AnimationParameters {
    F32 initial;
    F32 target;
    F32 rate;
    F32 epsilon;
    B32 reset;
};

typedef struct UI_Animation UI_Animation;
struct UI_Animation {
    UI_Animation *next;
    UI_Animation *previous;

    U64 last_used_index;

    UI_Key key;
    UI_AnimationParameters parameters;
    F32 current;
};

typedef struct UI_AnimationList UI_AnimationList;
struct UI_AnimationList {
    UI_Animation *first;
    UI_Animation *last;
};

typedef struct UI_Context UI_Context;
struct UI_Context {
    Arena *permanent_arena;
    UI_BoxList *box_table;
    UI_Box *box_freelist;
    U64 box_count;

    UI_AnimationList *animation_table;
    UI_Animation *animation_freelist;

    Arena *frame_arenas[2];
    U64    frame_index;

    UI_Box *root;
    UI_Box *tooltip_root;
    UI_Box *context_menu_root;

    // NOTE(simon): Per frame input.
    UI_IconInfo   icon_info;
    UI_EventList *events;
    V2F32         mouse;
    F32 dt;
    F32 fast_rate;
    F32 slow_rate;
    F32 super_slow_rate;

    UI_Key hot_key;
    UI_Key active_key[UI_MouseButton_COUNT];
    UI_Key drop_hot_key;
    B32 is_animating;
    UI_Key default_navigation_root_key;

    // NOTE(simon): Drag data
    V2F32  drag_start;
    Arena *drag_arena;
    Str8   drag_data;

    // NOTE(simon): Tooltip state.
    UI_Key tooltip_anchor_key;
    F32 tooltip_t;
    B32 is_tooltip_active;

    // NOTE(simon): Context menu state.
    UI_Key context_menu_key;
    UI_Key context_menu_anchor_key;
    V2F32  context_menu_anchor_offset;
    UI_Key context_menu_key_next;
    UI_Key context_menu_anchor_key_next;
    V2F32  context_menu_anchor_offset_next;
    B32    context_menu_used_this_frame;

    // NOTE(simon): Style stacks.
    UI_BoxStack             parent_stack;
    UI_PaletteStack         palette_stack;
    UI_SizeStack            size_stacks[Axis2_COUNT];
    UI_AxisStack            layout_axis_stack;
    UI_BoxFlagsStack        extra_box_flags_stack;
    UI_F32Stack             fixed_x_stack;
    UI_F32Stack             fixed_y_stack;
    UI_FontStack            font_stack;
    UI_U32Stack             font_size_stack;
    UI_CursorStack          hover_cursor_stack;
    UI_BoxDrawFunctionStack draw_function_stack;
    UI_PointerStack         draw_data_stack;
    UI_TextAlignStack       text_align_stack;
    UI_F32Stack             text_x_padding_stack;
    UI_F32Stack             text_y_padding_stack;
    UI_F32Stack             corner_radius_stacks[Corner_COUNT];
    UI_FocusStack           focus_hot_stack;
    UI_FocusStack           focus_active_stack;
};

internal Void            ui_select_state(UI_Context *state);
internal UI_Key          ui_active_seed_key(Void);
internal V2F32           ui_mouse(Void);
internal FontCache_Font *ui_icon_font(Void);
internal Str8            ui_icon_string_from_kind(UI_IconKind icon);

// NOTE(simon): Event lists
internal Void ui_event_list_push_event(UI_EventList *list, UI_Event *event);
internal Void ui_event_list_consume_event(UI_EventList *list, UI_Event *event);

// NOTE(simon): Events
internal B32       ui_next_event(UI_Event **event);
internal Void      ui_consume_event(UI_Event *event);
internal UI_Event *ui_consume_event_kind(UI_EventKind kind);
internal UI_Event *ui_consume_key_press(Gfx_Key key, Gfx_KeyModifier modifiers);

// NOTE(simon): Extracting parts from UI strings.
internal Str8 ui_hash_part_from_string(Str8 string);
internal Str8 ui_display_part_from_string(Str8 string);

// NOTE(simon): Keys
internal B32    ui_keys_match(UI_Key a, UI_Key b);
internal B32    ui_key_is_null(UI_Key key);
internal UI_Key ui_key_from_string(UI_Key seed, Str8 string);
internal UI_Key ui_key_from_string_format(UI_Key seed, CStr format, ...);

// NOTE(simon): Focus
internal B32 ui_is_focus_hot(Void);
internal B32 ui_is_focus_active(Void);

// NOTE(simon): Sizes
internal UI_Size ui_size_pixels(F32 pixels, F32 strictness);
internal UI_Size ui_size_ems(F32 ems, F32 strictness);
internal UI_Size ui_size_parent_percent(F32 percent, F32 strictness);
internal UI_Size ui_size_children_sum(F32 strictness);
internal UI_Size ui_size_text_content(F32 padding, F32 strictness);
internal UI_Size ui_size_aspect_ratio(F32 ratio, F32 strictness);

internal UI_Context *ui_create(Void);

internal Void ui_begin(Gfx_Window window, UI_EventList *ui_events, UI_IconInfo *icon_info, F32 dt);
internal Void ui_end(Void);

internal UI_BoxIterator ui_box_iterator_depth_first_pre_order(UI_Box *box);

// NOTE(simon): Boxes
internal B32      ui_box_is_null(UI_Box *box);
internal UI_Box  *ui_box_from_key(UI_Key key);
internal UI_Box  *ui_create_box_from_key(UI_BoxFlags flags, UI_Key key);
internal UI_Box  *ui_create_box(UI_BoxFlags flags);
internal UI_Box  *ui_create_box_from_string(UI_BoxFlags flags, Str8 string);
internal UI_Box  *ui_create_box_from_string_format(UI_BoxFlags flags, CStr format, ...);

internal Void     ui_box_set_string(UI_Box *box, Str8 string);
internal Void     ui_box_set_fuzzy_match_list(UI_Box *box, FuzzyMatchList fuzzy_matches);
internal Void     ui_box_set_draw_list(UI_Box *box, Draw_List *list);
internal V2F32    ui_box_text_location(UI_Box *box);
internal UI_Input ui_input_from_box(UI_Box *box);

// NOTE(simon): Tooltips
internal Void ui_tooltip_begin(UI_Key anchor_key);
internal Void ui_tooltip_end(Void);
#define ui_tooltip(anchor_key) defer_loop(ui_tooltip_begin(anchor_key), ui_tooltip_end())

// NOTE(simon): Context menus
internal Void ui_context_menu_open(UI_Key context_key, UI_Key anchor_key, V2F32 anchor_offset);
internal Void ui_context_menu_close(Void);
internal B32  ui_context_menu_begin(UI_Key context_key);
internal Void ui_context_menu_end(Void);
internal B32 ui_context_menu_is_open(UI_Key context_key);
#define ui_context_menu(context_key)                                      \
    for (                                                                 \
        B32 glue(is_open, __LINE__) = ui_context_menu_begin(context_key); \
        glue(is_open, __LINE__) ? 1 : (ui_context_menu_end(), 0);         \
        glue(is_open, __LINE__) = false                                   \
    )



// NOTE(simon): Drag and drop
internal UI_Key ui_drop_hot_key(Void);
internal V2F32  ui_drag_delta(Void);
internal Str8   ui_get_drag_data_str8(U64 min_size);
internal Void   ui_set_drag_data_str8(Str8 data);
#define ui_get_drag_data(type) ((type *) ui_get_drag_data_str8(sizeof(type)).data)
#define ui_set_drag_data(ptr) ui_set_drag_data_str8(str8((U8 *) (ptr), sizeof(*(ptr))))



// NOTE(simon): Animation
internal F32 ui_animation_slow_rate(Void);
internal F32 ui_animation_fast_rate(Void);
internal B32 ui_is_animating_from_context(UI_Context *ui);
#define ui_animate(key, target_value, ...) ui_animate_internal(key, &(UI_AnimationParameters) {.target = target_value, .rate = global_ui_state->fast_rate, __VA_ARGS__ })
internal F32 ui_animate_internal(UI_Key key, UI_AnimationParameters *parameters);



#define ui_parent_push(parent) ui_box_stack_push(&global_ui_state->parent_stack, parent, false)
#define ui_parent_pop()        ui_box_stack_pop(&global_ui_state->parent_stack)
#define ui_parent(parent)      defer_loop(ui_parent_push(parent), ui_parent_pop())
#define ui_parent_next(parent) ui_box_stack_push(&global_ui_state->parent_stack, parent, true)
#define ui_parent_auto_pop()   ui_box_stack_auto_pop(&global_ui_state->parent_stack)
#define ui_parent_top()        (global_ui_state->parent_stack.top->item)

#define ui_palette_push(palette) ui_palette_stack_push(&global_ui_state->palette_stack, palette, false)
#define ui_palette_pop()       ui_palette_stack_pop(&global_ui_state->palette_stack)
#define ui_palette(palette)      defer_loop(ui_palette_push(palette), ui_palette_pop())
#define ui_palette_next(palette) ui_palette_stack_push(&global_ui_state->palette_stack, palette, true)
#define ui_palette_auto_pop()  ui_palette_stack_auto_pop(&global_ui_state->palette_stack)
#define ui_palette_top()       (global_ui_state->palette_stack.top->item)

#define ui_width_push(size) ui_size_stack_push(&global_ui_state->size_stacks[Axis2_X], size, false)
#define ui_width_pop()      ui_size_stack_pop(&global_ui_state->size_stacks[Axis2_X])
#define ui_width(size)      defer_loop(ui_width_push(size), ui_width_pop())
#define ui_width_next(size) ui_size_stack_push(&global_ui_state->size_stacks[Axis2_X], size, true)
#define ui_width_auto_pop() ui_size_stack_auto_pop(&global_ui_state->size_stacks[Axis2_X])
#define ui_width_top()      (global_ui_state->size_stacks[Axis2_X].top->item)

#define ui_height_push(size) ui_size_stack_push(&global_ui_state->size_stacks[Axis2_Y], size, false)
#define ui_height_pop()      ui_size_stack_pop(&global_ui_state->size_stacks[Axis2_Y])
#define ui_height(size)      defer_loop(ui_height_push(size), ui_height_pop())
#define ui_height_next(size) ui_size_stack_push(&global_ui_state->size_stacks[Axis2_Y], size, true)
#define ui_height_auto_pop() ui_size_stack_auto_pop(&global_ui_state->size_stacks[Axis2_Y])
#define ui_height_top()      (global_ui_state->size_stacks[Axis2_Y].top->item)

#define ui_size_push(size, axis) ui_size_stack_push(&global_ui_state->size_stacks[axis], size, false)
#define ui_size_pop(axis)        ui_size_stack_pop(&global_ui_state->size_stacks[axis])
#define ui_size(axis)            defer_loop(ui_size_push(axis), ui_size_pop())
#define ui_size_next(size, axis) ui_size_stack_push(&global_ui_state->size_stacks[axis], size, true)
#define ui_size_top(axis)        (global_ui_state->size_stacks[axis].top->item)

#define ui_layout_axis_push(axis) ui_axis_stack_push(&global_ui_state->layout_axis_stack, axis, false)
#define ui_layout_axis_pop()      ui_axis_stack_pop(&global_ui_state->layout_axis_stack)
#define ui_layout_axis(axis)      defer_loop(ui_layout_axis_push(axis), ui_layout_axis_pop())
#define ui_layout_axis_next(axis) ui_axis_stack_push(&global_ui_state->layout_axis_stack, axis, true)
#define ui_layout_axis_auto_pop() ui_axis_stack_auto_pop(&global_ui_state->layout_axis_stack)
#define ui_layout_axis_top()      (global_ui_state->layout_axis_stack.top->item)

#define ui_extra_box_flags_push(flags) ui_box_flags_stack_push(&global_ui_state->extra_box_flags_stack, flags, false)
#define ui_extra_box_flags_pop()       ui_box_flags_stack_pop(&global_ui_state->extra_box_flags_stack)
#define ui_extra_box_flags(flags)      defer_loop(ui_extra_box_flags_push(flags), ui_extra_box_flags_pop())
#define ui_extra_box_flags_next(flags) ui_box_flags_stack_push(&global_ui_state->extra_box_flags_stack, flags, true)
#define ui_extra_box_flags_auto_pop()  ui_box_flags_stack_auto_pop(&global_ui_state->extra_box_flags_stack)
#define ui_extra_box_flags_top()       (global_ui_state->extra_box_flags_stack.top->item)

#define ui_fixed_x_push(x)    ui_f32_stack_push(&global_ui_state->fixed_x_stack, x, false)
#define ui_fixed_x_pop()      ui_f32_stack_pop(&global_ui_state->fixed_x_stack)
#define ui_fixed_x(x)         defer_loop(ui_fixed_x_push(x), ui_fixed_x_pop())
#define ui_fixed_x_next(x)    ui_f32_stack_push(&global_ui_state->fixed_x_stack, x, true)
#define ui_fixed_x_auto_pop() ui_f32_stack_auto_pop(&global_ui_state->fixed_x_stack)
#define ui_fixed_x_top()      (global_ui_state->fixed_x_stack.top->item)

#define ui_fixed_y_push(y)    ui_f32_stack_push(&global_ui_state->fixed_y_stack, y, false)
#define ui_fixed_y_pop()      ui_f32_stack_pop(&global_ui_state->fixed_y_stack)
#define ui_fixed_y(y)         defer_loop(ui_fixed_y_push(y), ui_fixed_y_pop())
#define ui_fixed_y_next(y)    ui_f32_stack_push(&global_ui_state->fixed_y_stack, y, true)
#define ui_fixed_y_auto_pop() ui_f32_stack_auto_pop(&global_ui_state->fixed_y_stack)
#define ui_fixed_y_top()      (global_ui_state->fixed_y_stack.top->item)

#define ui_fixed_position_push(position) (ui_fixed_x_push(position.x), ui_fixed_y_push(position.y))
#define ui_fixed_position_pop()          (ui_fixed_x_pop(), ui_fixed_y_pop())
#define ui_fixed_position(position)      defer_loop(ui_fixed_position_push(position), ui_fixed_position_pop())
#define ui_fixed_position_next(position) (ui_fixed_x_next(position.x), ui_fixed_y_next(position.y))
#define ui_fixed_position_auto_pop()     (ui_fixed_x_auto_pop(), ui_fixed_y_auto_pop())
#define ui_fixed_position_top()          v2f32(ui_fixed_x_top(), ui_fixed_y_top())

#define ui_font_push(font) ui_font_stack_push(&global_ui_state->font_stack, font, false)
#define ui_font_pop()      ui_font_stack_pop(&global_ui_state->font_stack)
#define ui_font(font)      defer_loop(ui_font_push(font), ui_font_pop())
#define ui_font_next(font) ui_font_stack_push(&global_ui_state->font_stack, font, true)
#define ui_font_auto_pop() ui_font_stack_auto_pop(&global_ui_state->font_stack)
#define ui_font_top()      (global_ui_state->font_stack.top->item)

// NOTE(simon): These are in points.
#define ui_font_size_push(size) ui_u32_stack_push(&global_ui_state->font_size_stack, size, false)
#define ui_font_size_pop()      ui_u32_stack_pop(&global_ui_state->font_size_stack)
#define ui_font_size(size)      defer_loop(ui_font_size_push(size), ui_font_size_pop())
#define ui_font_size_next(size) ui_u32_stack_push(&global_ui_state->font_size_stack, size, true)
#define ui_font_size_auto_pop() ui_u32_stack_auto_pop(&global_ui_state->font_size_stack)
#define ui_font_size_top()      (global_ui_state->font_size_stack.top->item)

#define ui_text_align_push(align) ui_text_align_stack_push(&global_ui_state->text_align_stack, align, false)
#define ui_text_align_pop()       ui_text_align_stack_pop(&global_ui_state->text_align_stack)
#define ui_text_align(align)      defer_loop(ui_text_align_push(align), ui_text_align_pop())
#define ui_text_align_next(align) ui_text_align_stack_push(&global_ui_state->text_align_stack, align, true)
#define ui_text_align_auto_pop()  ui_text_align_stack_auto_pop(&global_ui_state->text_align_stack)
#define ui_text_align_top()       (global_ui_state->text_align_stack.top->item)

#define ui_hover_cursor_push(cursor) ui_cursor_stack_push(&global_ui_state->hover_cursor_stack, cursor, false)
#define ui_hover_cursor_pop()        ui_cursor_stack_pop(&global_ui_state->hover_cursor_stack)
#define ui_hover_cursor(cursor)      defer_loop(ui_hover_cursor_push(cursor), ui_hover_cursor_pop())
#define ui_hover_cursor_next(cursor) ui_cursor_stack_push(&global_ui_state->hover_cursor_stack, cursor, true)
#define ui_hover_cursor_auto_pop()   ui_cursor_stack_auto_pop(&global_ui_state->hover_cursor_stack)
#define ui_hover_cursor_top()        (global_ui_state->hover_cursor_stack.top->item)

#define ui_draw_function_push(function) ui_box_draw_function_stack_push(&global_ui_state->draw_function_stack, function, false)
#define ui_draw_function_pop()          ui_box_draw_function_stack_pop(&global_ui_state->draw_function_stack)
#define ui_draw_function(function)      defer_loop(ui_draw_function_push(function), ui_draw_function_pop())
#define ui_draw_function_next(function) ui_box_draw_function_stack_push(&global_ui_state->draw_function_stack, function, true)
#define ui_draw_function_auto_pop()     ui_box_draw_function_stack_auto_pop(&global_ui_state->draw_function_stack)
#define ui_draw_function_top()          (global_ui_state->draw_function_stack.top->item)

#define ui_draw_data_push(data) ui_pointer_stack_push(&global_ui_state->draw_data_stack, data, false)
#define ui_draw_data_pop()      ui_pointer_stack_pop(&global_ui_state->draw_data_stack)
#define ui_draw_data(data)      defer_loop(ui_draw_data_push(data), ui_draw_data_pop())
#define ui_draw_data_next(data) ui_pointer_stack_push(&global_ui_state->draw_data_stack, data, true)
#define ui_draw_data_auto_pop() ui_pointer_stack_auto_pop(&global_ui_state->draw_data_stack)
#define ui_draw_data_top()      (global_ui_state->draw_data_stack.top->item)

#define ui_text_x_padding_push(padding) ui_f32_stack_push(&global_ui_state->text_x_padding_stack, padding, false)
#define ui_text_x_padding_pop()         ui_f32_stack_pop(&global_ui_state->text_x_padding_stack)
#define ui_text_x_padding(padding)      defer_loop(ui_text_x_padding_push(padding), ui_text_x_padding_pop())
#define ui_text_x_padding_next(padding) ui_f32_stack_push(&global_ui_state->text_x_padding_stack, padding, true)
#define ui_text_x_padding_auto_pop()    ui_f32_stack_auto_pop(&global_ui_state->text_x_padding_stack)
#define ui_text_x_padding_top()         (global_ui_state->text_x_padding_stack.top->item)

#define ui_text_y_padding_push(padding) ui_f32_stack_push(&global_ui_state->text_y_padding_stack, padding, false)
#define ui_text_y_padding_pop()         ui_f32_stack_pop(&global_ui_state->text_y_padding_stack)
#define ui_text_y_padding(padding)      defer_loop(ui_text_y_padding_push(padding), ui_text_y_padding_pop())
#define ui_text_y_padding_next(padding) ui_f32_stack_push(&global_ui_state->text_y_padding_stack, padding, true)
#define ui_text_y_padding_auto_pop()    ui_f32_stack_auto_pop(&global_ui_state->text_y_padding_stack)
#define ui_text_y_padding_top()         (global_ui_state->text_y_padding_stack.top->item)

#define ui_text_padding_push(padding)     (ui_text_x_padding_push(padding.x), ui_text_y_padding_push(padding.y))
#define ui_text_padding_pop()             (ui_text_x_padding_pop(), ui_text_y_padding_pop())
#define ui_text_padding(padding)          defer_loop(ui_text_padding_push(padding), ui_text_padding_pop())
#define ui_text_padding_next(padding)     (ui_text_x_padding_next(padding.x), ui_text_y_padding_next(padding.y))
#define ui_text_padding_auto_pop(padding) (ui_text_x_padding_auto_pop(), ui_text_y_padding_auto_pop())
#define ui_text_padding_top()             v2f32(ui_text_x_padding_top(), ui_text_y_padding_top())

#define ui_corner_radius_00_push(radius) ui_f32_stack_push(&global_ui_state->corner_radius_stacks[Corner_00], radius, false)
#define ui_corner_radius_00_pop()        ui_f32_stack_pop(&global_ui_state->corner_radius_stacks[Corner_00])
#define ui_corner_radius_00(radius)      defer_loop(ui_corner_radius_00_push(radius), ui_corner_radius_00_pop())
#define ui_corner_radius_00_next(radius) ui_f32_stack_push(&global_ui_state->corner_radius_stacks[Corner_00], radius, true)
#define ui_corner_radius_00_auto_pop()   ui_f32_stack_auto_pop(&global_ui_state->corner_radius_stacks[Corner_00])
#define ui_corner_radius_00_top()        (global_ui_state->corner_radius_stacks[Corner_00].top->item)

#define ui_corner_radius_01_push(radius) ui_f32_stack_push(&global_ui_state->corner_radius_stacks[Corner_01], radius, false)
#define ui_corner_radius_01_pop()        ui_f32_stack_pop(&global_ui_state->corner_radius_stacks[Corner_01])
#define ui_corner_radius_01(radius)      defer_loop(ui_corner_radius_01_push(radius), ui_corner_radius_01_pop())
#define ui_corner_radius_01_next(radius) ui_f32_stack_push(&global_ui_state->corner_radius_stacks[Corner_01], radius, true)
#define ui_corner_radius_01_auto_pop()   ui_f32_stack_auto_pop(&global_ui_state->corner_radius_stacks[Corner_01])
#define ui_corner_radius_01_top()        (global_ui_state->corner_radius_stacks[Corner_01].top->item)

#define ui_corner_radius_10_push(radius) ui_f32_stack_push(&global_ui_state->corner_radius_stacks[Corner_10], radius, false)
#define ui_corner_radius_10_pop()        ui_f32_stack_pop(&global_ui_state->corner_radius_stacks[Corner_10])
#define ui_corner_radius_10(radius)      defer_loop(ui_corner_radius_10_push(radius), ui_corner_radius_10_pop())
#define ui_corner_radius_10_next(radius) ui_f32_stack_push(&global_ui_state->corner_radius_stacks[Corner_10], radius, true)
#define ui_corner_radius_10_auto_pop()   ui_f32_stack_auto_pop(&global_ui_state->corner_radius_stacks[Corner_10])
#define ui_corner_radius_10_top()        (global_ui_state->corner_radius_stacks[Corner_10].top->item)

#define ui_corner_radius_11_push(radius) ui_f32_stack_push(&global_ui_state->corner_radius_stacks[Corner_11], radius, false)
#define ui_corner_radius_11_pop()        ui_f32_stack_pop(&global_ui_state->corner_radius_stacks[Corner_11])
#define ui_corner_radius_11(radius)      defer_loop(ui_corner_radius_11_push(radius), ui_corner_radius_11_pop())
#define ui_corner_radius_11_next(radius) ui_f32_stack_push(&global_ui_state->corner_radius_stacks[Corner_11], radius, true)
#define ui_corner_radius_11_auto_pop()   ui_f32_stack_auto_pop(&global_ui_state->corner_radius_stacks[Corner_11])
#define ui_corner_radius_11_top()        (global_ui_state->corner_radius_stacks[Corner_11].top->item)

#define ui_corner_radius_push(radius) (ui_corner_radius_00_push(radius), ui_corner_radius_01_push(radius), ui_corner_radius_10_push(radius), ui_corner_radius_11_push(radius))
#define ui_corner_radius_pop()        (ui_corner_radius_00_pop(), ui_corner_radius_01_pop(), ui_corner_radius_10_pop(), ui_corner_radius_11_pop())
#define ui_corner_radius(radius)      defer_loop(ui_corner_radius_push(radius), ui_corner_radius_pop())
#define ui_corner_radius_next(radius) (ui_corner_radius_00_next(radius), ui_corner_radius_01_next(radius), ui_corner_radius_10_next(radius), ui_corner_radius_11_next(radius))

#define ui_focus_hot_push(focus) ui_focus_stack_push(&global_ui_state->focus_hot_stack, focus, false)
#define ui_focus_hot_pop()       ui_focus_stack_pop(&global_ui_state->focus_hot_stack)
#define ui_focus_hot(focus)      defer_loop(ui_focus_hot_push(focus), ui_focus_hot_pop())
#define ui_focus_hot_next(focus) ui_focus_stack_push(&global_ui_state->focus_hot_stack, focus, true)
#define ui_focus_hot_auto_pop()  ui_focus_stack_auto_pop(&global_ui_state->focus_hot_stack)
#define ui_focus_hot_top()       (global_ui_state->focus_hot_stack.top->item)

#define ui_focus_active_push(focus) ui_focus_stack_push(&global_ui_state->focus_active_stack, focus, false)
#define ui_focus_active_pop()       ui_focus_stack_pop(&global_ui_state->focus_active_stack)
#define ui_focus_active(focus)      defer_loop(ui_focus_active_push(focus), ui_focus_active_pop())
#define ui_focus_active_next(focus) ui_focus_stack_push(&global_ui_state->focus_active_stack, focus, true)
#define ui_focus_active_auto_pop()  ui_focus_stack_auto_pop(&global_ui_state->focus_active_stack)
#define ui_focus_active_top()       (global_ui_state->focus_active_stack.top->item)

#define ui_focus_push(focus) (ui_focus_hot_push(focus), ui_focus_active_push(focus))
#define ui_focus_pop()       (ui_focus_hot_pop(), ui_focus_active_pop())
#define ui_focus_next(focus) (ui_focus_hot_next(focus), ui_focus_active_next(focus))
#define ui_focus(focus)      defer_loop((ui_focus_hot_push(focus), ui_focus_active_push(focus)), (ui_focus_hot_pop(), ui_focus_active_pop()))

#endif // UI_CORE_H
