#ifndef DRAW_CORE_H
#define DRAW_CORE_H

#define draw_define_stack(type_name, variable_name, type)                                                       \
    typedef struct Draw_##type_name##StackNode Draw_##type_name##StackNode;                                     \
    struct Draw_##type_name##StackNode {                                                                        \
        Draw_##type_name##StackNode *next;                                                                      \
        type                       item;                                                                        \
    };                                                                                                          \
    typedef struct Draw_##type_name##Stack Draw_##type_name##Stack;                                             \
    struct Draw_##type_name##Stack {                                                                            \
        Draw_##type_name##StackNode *top;                                                                       \
        Draw_##type_name##StackNode *freelist;                                                                  \
    };                                                                                                          \
    internal Void draw_##variable_name##_stack_push(Arena *arena, Draw_##type_name##Stack *stack, type value) { \
        Draw_##type_name##StackNode *node = 0;                                                                  \
        if (stack->freelist) {                                                                                  \
            node = stack->freelist;                                                                             \
            sll_stack_pop(stack->freelist);                                                                     \
        } else {                                                                                                \
            node = arena_push_struct_zero(arena, Draw_##type_name##StackNode);                                  \
        }                                                                                                       \
        node->item = value;                                                                                     \
        sll_stack_push(stack->top, node);                                                                       \
    }                                                                                                           \
    internal type draw_##variable_name##_stack_pop(Draw_##type_name##Stack *stack) {                            \
        Draw_##type_name##StackNode *node = stack->top;                                                         \
        if (node) {                                                                                             \
            sll_stack_pop(stack->top);                                                                          \
            sll_stack_push(stack->freelist, node);                                                              \
        }                                                                                                       \
        return node->item;                                                                                      \
    }                                                                                                           \

draw_define_stack(R2F32, r2f32, R2F32)

typedef struct Draw_Context Draw_Context;
struct Draw_Context {
    Arena *arena;
    Draw_R2F32Stack clip_stack;

    Render_BatchList batches;
};

global Draw_Context global_draw_context;

internal Void draw_begin_frame(Void);
internal Void draw_submit(Void);

internal Render_Rectangle *draw_rectangle(R2F32 rectangle, V4F32 color, F32 radius, F32 thickness, F32 softness);
internal Render_Rectangle *draw_texture(R2F32 rectangle, R2F32 uvs, Render_Texture texture, V4F32 color, F32 radius, F32 thickness, F32 softness, Render_RectangleFlags flags);

internal Render_Rectangle *draw_image(R2F32 rectangle, R2F32 uvs, Render_Texture texture, V4F32 color, F32 radius, F32 thickness, F32 softness);
internal Render_Rectangle *draw_glyph(R2F32 rectangle, R2F32 uvs, Render_Texture atlas, V4F32 color);
internal Render_Rectangle *draw_msdf(R2F32 rectangle, R2F32 uvs, Render_Texture atlas, V4F32 color);

#define draw_clip_push(clip) draw_r2f32_stack_push(global_draw_context.arena, &global_draw_context.clip_stack, clip)
#define draw_clip_pop()      draw_r2f32_stack_pop(&global_draw_context.clip_stack)
#define draw_clip_top()      global_draw_context.clip_stack.top->item
#define draw_clip(clip)      defer_loop(draw_clip_push(clip), draw_clip_pop())

#endif // DRAW_CORE_H
