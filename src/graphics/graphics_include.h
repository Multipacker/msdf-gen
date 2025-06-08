#ifndef GRAPHICS_INCLUDE_H
#define  GRAPHICS_INCLUDE_H

#include "graphics_core.h"

#if OS_LINUX
#  if LINUX_WAYLAND
#    include "wayland/wayland_include.h"
#  elif LINUX_X11
#    include "x11/x11_include.h"
#  endif
#elif OS_WINDOWS
#  include "win32/win32_include.h"
#else
# error no backend for graphics_include.h on this operating system
#endif

#endif // GRAPHICS_INCLUDE_H
