#ifndef DRAW_CORE_H
#define DRAW_CORE_H

typedef struct Draw_List Draw_List;

internal Draw_List *draw_list_create(Void);
internal Void       draw_list_push(Draw_List *list);
internal Void       draw_list_pop(Void);
internal Draw_List *draw_list_top(Void);
#define draw_list_scope(list) defer_loop(draw_list_push(list), draw_list_pop())

#define draw_define_stack(type_name, variable_name, type)                   \
    typedef struct Draw_##type_name##StackNode Draw_##type_name##StackNode; \
    struct Draw_##type_name##StackNode {                                    \
        Draw_##type_name##StackNode *next;                                  \
        type                       item;                                    \
    };                                                                      \
    typedef struct Draw_##type_name##Stack Draw_##type_name##Stack;         \
    struct Draw_##type_name##Stack {                                        \
        Draw_##type_name##StackNode *top;                                   \
        Draw_##type_name##StackNode *freelist;                              \
    };

#define draw_define_stack_implementation(type_name, variable_name, type)                           \
    internal Void draw_##variable_name##_stack_push(Draw_##type_name##Stack *stack, type value) {  \
        Draw_##type_name##StackNode *node = 0;                                                     \
        if (stack->freelist) {                                                                     \
            node = stack->freelist;                                                                \
            sll_stack_pop(stack->freelist);                                                        \
        } else {                                                                                   \
            node = arena_push_struct(global_draw_context.arena, Draw_##type_name##StackNode);      \
        }                                                                                          \
        node->item = value;                                                                        \
        sll_stack_push(stack->top, node);                                                          \
        ++draw_list_top()->stack_generation;                                                       \
    }                                                                                              \
    internal type draw_##variable_name##_stack_pop(Draw_##type_name##Stack *stack) {               \
        Draw_##type_name##StackNode *node = stack->top;                                            \
        if (node) {                                                                                \
            ++draw_list_top()->stack_generation;                                                   \
            sll_stack_pop(stack->top);                                                             \
            sll_stack_push(stack->freelist, node);                                                 \
        }                                                                                          \
        return node->item;                                                                         \
    }                                                                                              \

draw_define_stack(R2F32,     r2f32,     R2F32)
draw_define_stack(M3F32,     m3f32,     M3F32)
draw_define_stack(Filtering, filtering, Render_Filtering)

typedef struct Draw_List Draw_List;
struct Draw_List {
    Draw_List *next;
    Draw_R2F32Stack clip_stack;
    Draw_M3F32Stack transform_stack;
    Draw_FilteringStack filtering_stack;
    U64 stack_generation;
    U64 batch_generation;
    Render_BatchList batches;
};

typedef struct Draw_Context Draw_Context;
struct Draw_Context {
    Arena *arena;
    Draw_List *list_stack;
};

global Draw_Context global_draw_context;

draw_define_stack_implementation(R2F32, r2f32, R2F32)
draw_define_stack_implementation(M3F32, m3f32, M3F32)
draw_define_stack_implementation(Filtering, filtering, Render_Filtering)

internal Void draw_begin_frame(Void);
internal Void draw_submit_list(Gfx_Window graphics_window, Render_Window render_window, Draw_List *list);

internal Render_Shape *draw_rectangle(R2F32 rectangle, V4F32 color, F32 radius, F32 thickness, F32 softness);
internal Render_Shape *draw_texture(R2F32 rectangle, R2F32 source, Render_Texture texture, V4F32 color, F32 radius, F32 thickness, F32 softness, Render_ShapeFlags flags);

internal Render_Shape *draw_image(R2F32 rectangle, R2F32 source, Render_Texture texture, V4F32 color, F32 radius, F32 thickness, F32 softness);
internal Render_Shape *draw_glyph(R2F32 rectangle, R2F32 source, Render_Texture atlas, V4F32 color);
internal Render_Shape *draw_msdf(R2F32 rectangle, R2F32 source, Render_Texture atlas, V4F32 color);

internal Render_Shape *draw_line(V2F32 p0, V2F32 p1, V4F32 color, F32 radius, F32 thickness, F32 softness);
internal Void          draw_bezier(V2F32 p0, V2F32 p1, V2F32 p2, V4F32 color, F32 radius, F32 thickness, F32 softness);

internal Void draw_sub_list(Draw_List *sub_list);

#define draw_clip_push(clip) draw_r2f32_stack_push(&draw_list_top()->clip_stack, clip)
#define draw_clip_pop()      draw_r2f32_stack_pop(&draw_list_top()->clip_stack)
#define draw_clip_top()      draw_list_top()->clip_stack.top->item
#define draw_clip(clip)      defer_loop(draw_clip_push(clip), draw_clip_pop())

#define draw_transform_push(transform) draw_m3f32_stack_push(&draw_list_top()->transform_stack, transform)
#define draw_transform_pop()           draw_m3f32_stack_pop(&draw_list_top()->transform_stack)
#define draw_transform_top()           draw_list_top()->transform_stack.top->item
#define draw_transform(transform)      defer_loop(draw_transform_push(transform), draw_transform_pop())

#define draw_filtering_push(filtering) draw_filtering_stack_push(&draw_list_top()->filtering_stack, filtering)
#define draw_filtering_pop()           draw_filtering_stack_pop(&draw_list_top()->filtering_stack)
#define draw_filtering_top()           draw_list_top()->filtering_stack.top->item
#define draw_filtering(filtering)      defer_loop(draw_filtering_push(filtering), draw_filtering_pop())

#endif // DRAW_CORE_H
