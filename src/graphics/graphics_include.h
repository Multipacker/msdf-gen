#ifndef GRAPHICS_INCLUDE_H
#define  GRAPHICS_INCLUDE_H

#include "graphics_core.h"

#if OS_LINUX
#  include "sdl/sdl_include.h"
#elif OS_WINDOWS
#  include "win32/win32_include.h"
#else
# error no backend for graphics_include.h on this operating system
#endif

#endif // GRAPHICS_INCLUDE_H
