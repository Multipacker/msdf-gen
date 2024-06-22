#ifndef UI_CORE_H
#define UI_CORE_H

typedef enum {
    UI_Size_Pixels,
    UI_Size_ChildrenSum,
    UI_Size_ParentPercent,
} UI_SizeKind;

typedef struct UI_Size UI_Size;
struct UI_Size {
    UI_SizeKind kind;
    F32 value;
    F32 strictness;
};

typedef enum {
    UI_BoxFlags_DrawBackground = 1 << 0,
} UI_BoxFlags;

typedef struct UI_Box UI_Box;
struct UI_Box {
    UI_Box *parent;
    UI_Box *next;
    UI_Box *previous;
    UI_Box *first;
    UI_Box *last;

    UI_Size size[Axis2_COUNT];

    UI_BoxFlags flags;
    V4F32       color;
    Axis2       layout_axis;

    V2F32 calculated_size;
    V2F32 calculated_position;
    R2F32 calculated_rectangle;
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

ui_define_stack(Box,   box,   UI_Box *)
ui_define_stack(V4F32, v4f32, V4F32)
ui_define_stack(Size,  size,  UI_Size)
ui_define_stack(Axis,  axis,  Axis2)

typedef struct UI_Context UI_Context;
struct UI_Context {
    Arena *permanent_arena;
    Arena *frame_arena;

    UI_Box *root;

    UI_BoxStack   parent_stack;
    UI_V4F32Stack color_stack;
    UI_SizeStack  size_stacks[Axis2_COUNT];
    UI_AxisStack  layout_axis_stack;
};

internal UI_Size ui_size_pixels(F32 pixels, F32 strictness);
internal UI_Size ui_size_parent_percent(F32 percent, F32 strictness);
internal UI_Size ui_size_children_sum(F32 strictness);

internal UI_Context *ui_create(Void);

internal Void ui_begin(Gfx_Context *gfx, UI_Context *ui);
internal Void ui_end(UI_Context *ui);

internal UI_Box *ui_box_create(UI_Context *ui, UI_BoxFlags flags);

#define ui_parent_push(ui, parent) ui_box_stack_push(ui->frame_arena, &ui->parent_stack, parent, false)
#define ui_parent_pop(ui)          ui_box_stack_pop(&ui->parent_stack)
#define ui_parent(ui, parent)      defer_loop(ui_parent_push(ui, parent), ui_parent_pop(ui))
#define ui_parent_next(ui, parent) ui_box_stack_push(ui->frame_arena, &ui->parent_stack, parent, true)
#define ui_parent_auto_pop(ui)     ui_box_stack_auto_pop(&ui->parent_stack)
#define ui_parent_top(ui)          (ui->parent_stack.top->item)

#define ui_color_push(ui, color) ui_v4f32_stack_push(ui->frame_arena, &ui->color_stack, color, false)
#define ui_color_pop(ui)         ui_v4f32_stack_pop(&ui->color_stack)
#define ui_color(ui, color)      defer_loop(ui_color_push(ui, color), ui_color_pop(ui))
#define ui_color_next(ui, color) ui_v4f32_stack_push(ui->frame_arena, &ui->color_stack, color, true)
#define ui_color_auto_pop(ui)    ui_v4f32_stack_auto_pop(&ui->color_stack)
#define ui_color_top(ui)         (ui->color_stack.top->item)

#define ui_width_push(ui, size) ui_size_stack_push(ui->frame_arena, &ui->size_stacks[Axis2_X], size, false)
#define ui_width_pop(ui)        ui_size_stack_pop(&ui->size_stacks[Axis2_X])
#define ui_width(ui, size)      defer_loop(ui_width_push(ui, size), ui_width_pop(ui))
#define ui_width_next(ui, size) ui_size_stack_push(ui->frame_arena, &ui->size_stacks[Axis2_X], size, true)
#define ui_width_auto_pop(ui)   ui_size_stack_auto_pop(&ui->size_stacks[Axis2_X])
#define ui_width_top(ui)        (ui->size_stacks[Axis2_X].top->item)

#define ui_height_push(ui, size) ui_size_stack_push(ui->frame_arena, &ui->size_stacks[Axis2_Y], size, false)
#define ui_height_pop(ui)        ui_size_stack_pop(&ui->size_stacks[Axis2_Y])
#define ui_height(ui, size)      defer_loop(ui_height_push(ui, size), ui_height_pop(ui))
#define ui_height_next(ui, size) ui_size_stack_push(ui->frame_arena, &ui->size_stacks[Axis2_Y], size, true)
#define ui_height_auto_pop(ui)   ui_size_stack_auto_pop(&ui->size_stacks[Axis2_Y])
#define ui_height_top(ui)        (ui->size_stacks[Axis2_Y].top->item)

#define ui_size_push(ui, size, axis) ui_size_stack_push(ui->frame_arena, &ui->size_stacks[axis], size, false)
#define ui_size_pop(ui, axis)        ui_size_stack_pop(&ui->size_stacks[axis])
#define ui_size(ui, axis)            defer_loop(ui_size_push(ui, axis), ui_size_pop(ui))
#define ui_size_next(ui, size, axis) ui_size_stack_push(ui->frame_arena, &ui->size_stacks[axis], size, true)
#define ui_size_top(ui, axis)        (ui->size_stacks[axis].top->item)

#define ui_layout_axis_push(ui, axis) ui_axis_stack_push(ui->frame_arena, &ui->layout_axis_stack, axis, false)
#define ui_layout_axis_pop(ui)        ui_axis_stack_pop(&ui->layout_axis_stack)
#define ui_layout_axis(ui, axis)      defer_loop(ui_layout_axis_push(ui, axis), ui_layout_axis_pop(ui))
#define ui_layout_axis_next(ui, axis) ui_axis_stack_push(ui->frame_arena, &ui->layout_axis_stack, axis, true)
#define ui_layout_axis_auto_pop(ui)   ui_axis_stack_auto_pop(&ui->layout_axis_stack)
#define ui_layout_axis_top(ui)        (ui->layout_axis_stack.top->item)

#endif // UI_CORE_H
