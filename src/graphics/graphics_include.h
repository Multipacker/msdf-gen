#ifndef GRAPHICS_INCLUDE_H
#define  GRAPHICS_INCLUDE_H

#if OS_LINUX
#  include "wayland/wayland_include.h"
#else
# error no backend for graphics_include.c on this operating system
#endif

typedef struct Gfx_Image Gfx_Image;
struct Gfx_Image {
    U32 *data;
    U32 width;
    U32 height;
};

typedef struct {
    Void *pointer;

    Arena *arena;
    Arena *frame_arena;

    V2F32 dpi;

    U32 width;
    U32 height;
    S32 x, y;
    B32 tab_pressed;
    B32 is_grabbed;
    B32 is_pressed;
    B32 is_released;
    B32 is_right;
    S32 scroll_amount;
    B32 exit_requested;
} GraphicsContext;

internal Void graphics_msdf(GraphicsContext *state, V2F32 position, V2F32 size, V3F32 color, V2F32 uv0, V2F32 uv1);

internal GraphicsContext *graphics_create(Str8 title, U32 width, U32 height, Gfx_Image font_atlas);
internal Void graphics_destroy(GraphicsContext *context);
internal Void graphics_begin_frame(GraphicsContext *context);
internal Void graphics_end_frame(GraphicsContext *context);
internal Void graphics_set_render_msdf(GraphicsContext *context, U32 render_msdf);

#endif // GRAPHICS_INCLUDE_H
