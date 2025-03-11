#ifndef GRAPHICS_LINUX_INCLUDE_H
#define GRAPHICS_LINUX_INCLUDE_H

typedef struct Gfx_LinuxState Gfx_LinuxState;
struct Gfx_LinuxState {
    Arena *arena;
    Str8 font_paths[Gfx_Font_COUNT];
};

internal Void gfx_linux_init(Void);

#endif // GRAPHICS_LINUX_INCLUDE_H
