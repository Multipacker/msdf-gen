#ifndef GRAPHICS_INCLUDE_H
#define  GRAPHICS_INCLUDE_H

#if OS_LINUX
#  include "wayland/wayland_include.h"
#else
# error no backend for graphics_include.c on this operating system
#endif

typedef struct {
    Void *pointer;

    Arena *arena;
    Arena *frame_arena;

    V2F32 dpi;

    U32 width;
    U32 height;
    S32 x, y;
    B32 is_grabbed;
    B32 is_pressed;
    B32 is_released;
    B32 is_right;
    S32 scroll_amount;
    B32 exit_requested;
} GraphicsContext;

// Primitive draw commands.
internal Void graphics_push_scissor(GraphicsContext *state, V2F32 position, V2F32 size, B32 should_inherit);
internal Void graphics_pop_scissor(GraphicsContext *state);
internal Void graphics_rectangle(GraphicsContext *state, V2F32 position, V2F32 size, V3F32 color);
internal Void graphics_rectangle_texture(GraphicsContext *state, V2F32 position, V2F32 size, V3F32 color, U32 texture_index, V2F32 uv0, V2F32 uv1);
internal Void graphics_circle(GraphicsContext *state, V2F32 position, F32 radius, V3F32 color);
internal Void graphics_circle_texture(GraphicsContext *state, V2F32 position, F32 radius, V3F32 color, U32 texture_index, V2F32 uv0, V2F32 uv1);
internal Void graphics_line(GraphicsContext *state, V2F32 p0, V2F32 p1, F32 width, V3F32 color);
internal Void graphics_msdf(GraphicsContext *state, V2F32 position, V2F32 size, V3F32 color, V2F32 uv0, V2F32 uv1);
internal Void graphics_text(GraphicsContext *state, V2F32 position, F32 point_size, Str8 text);

// Composite draw commands.
internal Void graphics_quadratic_bezier(GraphicsContext *state, V2F32 p0, V2F32 p1, V2F32 p2, F32 width, V3F32 color);

internal GraphicsContext *graphics_create(Str8 title, U32 width, U32 height, FontDescription *font_description);
internal Void graphics_destroy(GraphicsContext *context);
internal Void graphics_begin_frame(GraphicsContext *context);
internal Void graphics_end_frame(GraphicsContext *context);

#endif // GRAPHICS_INCLUDE_H
