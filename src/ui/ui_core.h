#ifndef UI_CORE_H
#define UI_CORE_H

typedef enum {
    UI_MouseButtonKind_Left,
    UI_MouseButtonKind_Middle,
    UI_MouseButtonKind_Right,
    UI_MouseButtonKind_COUNT,
} UI_MouseButtonKind;

typedef enum {
    UI_Size_Pixels,
    UI_Size_ChildrenSum,
    UI_Size_ParentPercent,
    UI_Size_TextContent,
} UI_SizeKind;

typedef struct UI_Size UI_Size;
struct UI_Size {
    UI_SizeKind kind;
    F32 value;
    F32 strictness;
};

typedef U64 UI_Key;

typedef enum {
    // NOTE(simon): Interaction
    UI_BoxFlags_Disabled       = 1 << 0,
    UI_BoxFlags_Clickable      = 1 << 1,
    UI_BoxFlags_Scrollable     = 1 << 2,

    // NOTE(simon): Layout
    UI_BoxFlags_OverflowX      = 1 << 3,
    UI_BoxFlags_OverflowY      = 1 << 4,
    UI_BoxFlags_FloatingX      = 1 << 5,
    UI_BoxFlags_FloatingY      = 1 << 6,

    // NOTE(simon): Appearance
    UI_BoxFlags_AnimateX       = 1 << 7,
    UI_BoxFlags_AnimateY       = 1 << 8,
    UI_BoxFlags_DrawBackground = 1 << 9,
    UI_BoxFlags_DrawBorder     = 1 << 10,
    UI_BoxFlags_DrawText       = 1 << 11,
    UI_BoxFlags_DrawHot        = 1 << 12,
    UI_BoxFlags_DrawActive     = 1 << 13,

    // NOTE(simon): Convenient combinations
    UI_BoxFlags_Overflow         = UI_BoxFlags_OverflowX | UI_BoxFlags_OverflowY,
    UI_BoxFlags_AnimatePosition  = UI_BoxFlags_AnimateX | UI_BoxFlags_AnimateY,
    UI_BoxFlags_FloatingPosition = UI_BoxFlags_FloatingX | UI_BoxFlags_FloatingY,
} UI_BoxFlags;

typedef struct UI_Box UI_Box;
struct UI_Box {
    UI_Box *parent;
    UI_Box *next;
    UI_Box *previous;
    UI_Box *first;
    UI_Box *last;

    UI_Box *hash_next;
    UI_Box *hash_previous;

    UI_Key key;

    UI_Size size[Axis2_COUNT];

    UI_BoxFlags     flags;
    V4F32           color;
    V4F32           border_color;
    V4F32           text_color;
    Axis2           layout_axis;
    Str8            string;
    FontCache_Font *font;
    U32             font_size;
    F32             text_padding;

    FontCache_Text text;
    V2F32 calculated_size;
    V2F32 calculated_position;
    R2F32 calculated_rectangle;

    V2F32 animated_position;

    U64 create_index;
    U64 last_used_index;

    F32 hot_t;
    F32 active_t;
    F32 disabled_t;
};

#define ui_define_stack(type_name, variable_name, type)                                                                   \
    typedef struct UI_##type_name##StackNode UI_##type_name##StackNode;                                                   \
    struct UI_##type_name##StackNode {                                                                                    \
        UI_##type_name##StackNode *next;                                                                                  \
        type                       item;                                                                                  \
    };                                                                                                                    \
    typedef struct UI_##type_name##Stack UI_##type_name##Stack;                                                           \
    struct UI_##type_name##Stack {                                                                                        \
        UI_##type_name##StackNode *top;                                                                                   \
        UI_##type_name##StackNode *freelist;                                                                              \
        B32                        auto_pop;                                                                              \
    };                                                                                                                    \
    internal Void ui_##variable_name##_stack_push(Arena *arena, UI_##type_name##Stack *stack, type value, B32 auto_pop) { \
        UI_##type_name##StackNode *node = 0;                                                                              \
        if (stack->auto_pop) {                                                                                            \
            node = stack->top;                                                                                            \
            sll_stack_pop(stack->top);                                                                                    \
        } else if (stack->freelist) {                                                                                     \
            node = stack->freelist;                                                                                       \
            sll_stack_pop(stack->freelist);                                                                               \
        } else {                                                                                                          \
            node = arena_push_struct_zero(arena, UI_##type_name##StackNode);                                              \
        }                                                                                                                 \
        node->item = value;                                                                                               \
        sll_stack_push(stack->top, node);                                                                                 \
        stack->auto_pop = auto_pop;                                                                                       \
    }                                                                                                                     \
    internal type ui_##variable_name##_stack_pop(UI_##type_name##Stack *stack) {                                          \
        UI_##type_name##StackNode *node = stack->top;                                                                     \
        if (node) {                                                                                                       \
            sll_stack_pop(stack->top);                                                                                    \
            sll_stack_push(stack->freelist, node);                                                                        \
        }                                                                                                                 \
        return node->item;                                                                                                \
    }                                                                                                                     \
    internal Void ui_##variable_name##_stack_auto_pop(UI_##type_name##Stack *stack) {                                     \
        if (stack->auto_pop) {                                                                                            \
            stack->auto_pop = false;                                                                                      \
            UI_##type_name##StackNode *node = stack->top;                                                                 \
            sll_stack_pop(stack->top);                                                                                    \
            sll_stack_push(stack->freelist, node);                                                                        \
        }                                                                                                                 \
    }

ui_define_stack(Box,      box,       UI_Box *)
ui_define_stack(V4F32,    v4f32,     V4F32)
ui_define_stack(Size,     size,      UI_Size)
ui_define_stack(Axis,     axis,      Axis2)
ui_define_stack(BoxFlags, box_flags, UI_BoxFlags)
ui_define_stack(F32,      f32,       F32)
ui_define_stack(U32,      u32,       U32)
ui_define_stack(Str8,     str8,      Str8)

typedef struct UI_BoxList UI_BoxList;
struct UI_BoxList {
    UI_Box *first;
    UI_Box *last;
};

typedef enum {
    // NOTE(simon): Pressed while hovering.
    UI_InputFlag_LeftPressed    = 1 << 0,
    UI_InputFlag_MiddlePressed  = 1 << 1,
    UI_InputFlag_RightPressed   = 1 << 2,

    // NOTE(simon): Previously pressed and now user released the button.
    UI_InputFlag_LeftReleased   = 1 << 3,
    UI_InputFlag_MiddleReleased = 1 << 4,
    UI_InputFlag_RightReleased  = 1 << 5,

    // NOTE(simon): Previously pressed and released in bounds.
    UI_InputFlag_LeftClicked    = 1 << 6,
    UI_InputFlag_MiddleClicked  = 1 << 7,
    UI_InputFlag_RightClicked   = 1 << 8,

    // NOTE(simon): Mouse is over this box.
    UI_InputFlag_Hovering       = 1 << 9,

    // NOTE(simon): Convenient combinations
    UI_InputFlag_Pressed  = UI_InputFlag_LeftPressed | UI_InputFlag_MiddlePressed | UI_InputFlag_RightPressed,
    UI_InputFlag_Released = UI_InputFlag_LeftReleased | UI_InputFlag_MiddleReleased | UI_InputFlag_RightReleased,
    UI_InputFlag_Clicked  = UI_InputFlag_LeftClicked | UI_InputFlag_MiddleClicked | UI_InputFlag_RightClicked,
} UI_InputFlag;

typedef struct UI_Input UI_Input;
struct UI_Input {
    UI_Box *box;
    UI_InputFlag input_flags;
};

#define UI_BOX_TABLE_SIZE (1 << 12)

typedef struct UI_Context UI_Context;
struct UI_Context {
    Arena *permanent_arena;
    UI_BoxList *box_table;
    UI_Box *box_freelist;

    Arena *frame_arenas[2];
    U64    frame_index;

    UI_Box *root;

    Gfx_EventList *events;
    V2F32          mouse;

    F32 dt;
    UI_Key hot_key;
    UI_Key active_key;

    UI_BoxStack      parent_stack;
    UI_V4F32Stack    color_stack;
    UI_V4F32Stack    border_color_stack;
    UI_V4F32Stack    text_color_stack;
    UI_SizeStack     size_stacks[Axis2_COUNT];
    UI_AxisStack     layout_axis_stack;
    UI_BoxFlagsStack extra_box_flags_stack;
    UI_F32Stack      fixed_x_stack;
    UI_F32Stack      fixed_y_stack;
    UI_Str8Stack     font_stack;
    UI_U32Stack      font_size_stack;
};

internal Arena *ui_frame_arena(UI_Context *ui);

internal UI_Key ui_key_from_string(Str8 string);

internal UI_Size ui_size_pixels(F32 pixels, F32 strictness);
internal UI_Size ui_size_parent_percent(F32 percent, F32 strictness);
internal UI_Size ui_size_children_sum(F32 strictness);
internal UI_Size ui_size_text_content(F32 padding, F32 strictness);

internal UI_Context *ui_create(Void);

internal Void ui_begin(Gfx_Context *gfx, UI_Context *ui, Gfx_EventList *events, F32 dt);
internal Void ui_end(UI_Context *ui);

internal UI_Box **ui_box_reference_from_key(UI_Context *ui, UI_Key key);
internal UI_Box *ui_box_from_key(UI_Context *ui, UI_Key key);

internal UI_Box *ui_create_box_from_key(UI_Context *ui, UI_BoxFlags flags, UI_Key key);
internal UI_Box *ui_create_box(UI_Context *ui, UI_BoxFlags flags);
internal UI_Box *ui_create_box_from_string(UI_Context *ui, UI_BoxFlags flags, Str8 string);
internal UI_Box *ui_create_box_from_string_format(UI_Context *ui, UI_Key key, CStr format, ...);

internal Void     ui_box_set_string(UI_Context *ui, UI_Box *box, Str8 string);
internal UI_Input ui_input_from_box(UI_Context *ui, UI_Box *box);

#define ui_parent_push(ui, parent) ui_box_stack_push(ui_frame_arena(ui), &ui->parent_stack, parent, false)
#define ui_parent_pop(ui)          ui_box_stack_pop(&ui->parent_stack)
#define ui_parent(ui, parent)      defer_loop(ui_parent_push(ui, parent), ui_parent_pop(ui))
#define ui_parent_next(ui, parent) ui_box_stack_push(ui_frame_arena(ui), &ui->parent_stack, parent, true)
#define ui_parent_auto_pop(ui)     ui_box_stack_auto_pop(&ui->parent_stack)
#define ui_parent_top(ui)          (ui->parent_stack.top->item)

#define ui_color_push(ui, color) ui_v4f32_stack_push(ui_frame_arena(ui), &ui->color_stack, color, false)
#define ui_color_pop(ui)         ui_v4f32_stack_pop(&ui->color_stack)
#define ui_color(ui, color)      defer_loop(ui_color_push(ui, color), ui_color_pop(ui))
#define ui_color_next(ui, color) ui_v4f32_stack_push(ui_frame_arena(ui), &ui->color_stack, color, true)
#define ui_color_auto_pop(ui)    ui_v4f32_stack_auto_pop(&ui->color_stack)
#define ui_color_top(ui)         (ui->color_stack.top->item)

#define ui_border_color_push(ui, color) ui_v4f32_stack_push(ui_frame_arena(ui), &ui->border_color_stack, color, false)
#define ui_border_color_pop(ui)         ui_v4f32_stack_pop(&ui->border_color_stack)
#define ui_border_color(ui, color)      defer_loop(ui_border_color_push(ui, color), ui_border_color_pop(ui))
#define ui_border_color_next(ui, color) ui_v4f32_stack_push(ui_frame_arena(ui), &ui->border_color_stack, color, true)
#define ui_border_color_auto_pop(ui)    ui_v4f32_stack_auto_pop(&ui->border_color_stack)
#define ui_border_color_top(ui)         (ui->border_color_stack.top->item)

#define ui_text_color_push(ui, color) ui_v4f32_stack_push(ui_frame_arena(ui), &ui->text_color_stack, color, false)
#define ui_text_color_pop(ui)         ui_v4f32_stack_pop(&ui->text_color_stack)
#define ui_text_color(ui, color)      defer_loop(ui_text_color_push(ui, color), ui_text_color_pop(ui))
#define ui_text_color_next(ui, color) ui_v4f32_stack_push(ui_frame_arena(ui), &ui->text_color_stack, color, true)
#define ui_text_color_auto_pop(ui)    ui_v4f32_stack_auto_pop(&ui->text_color_stack)
#define ui_text_color_top(ui)         (ui->text_color_stack.top->item)

#define ui_width_push(ui, size) ui_size_stack_push(ui_frame_arena(ui), &ui->size_stacks[Axis2_X], size, false)
#define ui_width_pop(ui)        ui_size_stack_pop(&ui->size_stacks[Axis2_X])
#define ui_width(ui, size)      defer_loop(ui_width_push(ui, size), ui_width_pop(ui))
#define ui_width_next(ui, size) ui_size_stack_push(ui_frame_arena(ui), &ui->size_stacks[Axis2_X], size, true)
#define ui_width_auto_pop(ui)   ui_size_stack_auto_pop(&ui->size_stacks[Axis2_X])
#define ui_width_top(ui)        (ui->size_stacks[Axis2_X].top->item)

#define ui_height_push(ui, size) ui_size_stack_push(ui_frame_arena(ui), &ui->size_stacks[Axis2_Y], size, false)
#define ui_height_pop(ui)        ui_size_stack_pop(&ui->size_stacks[Axis2_Y])
#define ui_height(ui, size)      defer_loop(ui_height_push(ui, size), ui_height_pop(ui))
#define ui_height_next(ui, size) ui_size_stack_push(ui_frame_arena(ui), &ui->size_stacks[Axis2_Y], size, true)
#define ui_height_auto_pop(ui)   ui_size_stack_auto_pop(&ui->size_stacks[Axis2_Y])
#define ui_height_top(ui)        (ui->size_stacks[Axis2_Y].top->item)

#define ui_size_push(ui, size, axis) ui_size_stack_push(ui_frame_arena(ui), &ui->size_stacks[axis], size, false)
#define ui_size_pop(ui, axis)        ui_size_stack_pop(&ui->size_stacks[axis])
#define ui_size(ui, axis)            defer_loop(ui_size_push(ui, axis), ui_size_pop(ui))
#define ui_size_next(ui, size, axis) ui_size_stack_push(ui_frame_arena(ui), &ui->size_stacks[axis], size, true)
#define ui_size_top(ui, axis)        (ui->size_stacks[axis].top->item)

#define ui_layout_axis_push(ui, axis) ui_axis_stack_push(ui_frame_arena(ui), &ui->layout_axis_stack, axis, false)
#define ui_layout_axis_pop(ui)        ui_axis_stack_pop(&ui->layout_axis_stack)
#define ui_layout_axis(ui, axis)      defer_loop(ui_layout_axis_push(ui, axis), ui_layout_axis_pop(ui))
#define ui_layout_axis_next(ui, axis) ui_axis_stack_push(ui_frame_arena(ui), &ui->layout_axis_stack, axis, true)
#define ui_layout_axis_auto_pop(ui)   ui_axis_stack_auto_pop(&ui->layout_axis_stack)
#define ui_layout_axis_top(ui)        (ui->layout_axis_stack.top->item)

#define ui_extra_box_flags_push(ui, flags) ui_box_flags_stack_push(ui_frame_arena(ui), &ui->extra_box_flags_stack, flags, false)
#define ui_extra_box_flags_pop(ui)         ui_box_flags_stack_pop(&ui->extra_box_flags_stack)
#define ui_extra_box_flags(ui, flags)      defer_loop(ui_extra_box_flags_push(ui, flags), ui_extra_box_flags_pop(ui))
#define ui_extra_box_flags_next(ui, flags) ui_box_flags_stack_push(ui_frame_arena(ui), &ui->extra_box_flags_stack, flags, true)
#define ui_extra_box_flags_auto_pop(ui)    ui_box_flags_stack_auto_pop(&ui->extra_box_flags_stack)
#define ui_extra_box_flags_top(ui)         (ui->extra_box_flags_stack.top->item)

#define ui_fixed_x_push(ui, x)  ui_f32_stack_push(ui_frame_arena(ui), &ui->fixed_x_stack, x, false)
#define ui_fixed_x_pop(ui)      ui_f32_stack_pop(&ui->fixed_x_stack)
#define ui_fixed_x(ui, x)       defer_loop(ui_fixed_x_push(ui, x), ui_fixed_x_pop(ui))
#define ui_fixed_x_next(ui, x)  ui_f32_stack_push(ui_frame_arena(ui), &ui->fixed_x_stack, x, true)
#define ui_fixed_x_auto_pop(ui) ui_f32_stack_auto_pop(&ui->fixed_x_stack)
#define ui_fixed_x_top(ui)      (ui->fixed_x_stack.top->item)

#define ui_fixed_y_push(ui, y)  ui_f32_stack_push(ui_frame_arena(ui), &ui->fixed_y_stack, y, false)
#define ui_fixed_y_pop(ui)      ui_f32_stack_pop(&ui->fixed_y_stack)
#define ui_fixed_y(ui, y)       defer_loop(ui_fixed_y_push(ui, y), ui_fixed_y_pop(ui))
#define ui_fixed_y_next(ui, y)  ui_f32_stack_push(ui_frame_arena(ui), &ui->fixed_y_stack, y, true)
#define ui_fixed_y_auto_pop(ui) ui_f32_stack_auto_pop(&ui->fixed_y_stack)
#define ui_fixed_y_top(ui)      (ui->fixed_y_stack.top->item)

#define ui_fixed_position_push(ui, position) (ui_fixed_x_push(ui, position.x), ui_fixed_y_push(ui, position.y))
#define ui_fixed_position_pop(ui)            (ui_fixed_x_pop(ui), ui_fixed_y_pop(ui))
#define ui_fixed_position(ui, position)      defer_loop(ui_fixed_position_push(ui, position), ui_fixed_position_pop(ui))
#define ui_fixed_position_next(ui, position) (ui_fixed_x_next(ui, position.x), ui_fixed_y_next(ui, position.y))
#define ui_fixed_position_auto_pop(ui)       (ui_fixed_x_auto_pop(ui), ui_fixed_y_auto_pop(ui))
#define ui_fixed_position_top(ui)            v2f32(ui_fixed_x_top(ui), ui_fixed_y_top(ui))

#define ui_font_push(ui, font) ui_str8_stack_push(ui_frame_arena(ui), &ui->font_stack, font, false)
#define ui_font_pop(ui)        ui_str8_stack_pop(&ui->font_stack)
#define ui_font(ui, font)      defer_loop(ui_font_push(ui, flags), ui_font_pop(ui))
#define ui_font_next(ui, font) ui_str8_stack_push(ui_frame_arena(ui), &ui->font_stack, font, true)
#define ui_font_auto_pop(ui)   ui_str8_stack_auto_pop(&ui->font_stack)
#define ui_font_top(ui)        (ui->font_stack.top->item)

// NOTE(simon): These are in points.
#define ui_font_size_push(ui, size) ui_u32_stack_push(ui_frame_arena(ui), &ui->font_size_stack, size, false)
#define ui_font_size_pop(ui)        ui_u32_stack_pop(&ui->font_size_stack)
#define ui_font_size(ui, size)      defer_loop(ui_font_size_push(ui, size), ui_font_size_pop(ui))
#define ui_font_size_next(ui, size) ui_u32_stack_push(ui_frame_arena(ui), &ui->font_size_stack, size, true)
#define ui_font_size_auto_pop(ui)   ui_u32_stack_auto_pop(&ui->font_size_stack)
#define ui_font_size_top(ui)        (ui->font_size_stack.top->item)

#endif // UI_CORE_H
