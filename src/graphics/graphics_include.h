#ifndef GRAPHICS_INCLUDE_H
#define  GRAPHICS_INCLUDE_H

#if OS_LINUX
#  include "sdl/sdl_include.h"
#else
# error no backend for graphics_include.c on this operating system
#endif

#endif // GRAPHICS_INCLUDE_H
