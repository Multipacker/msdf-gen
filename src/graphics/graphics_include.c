#include "graphics_core.c"

#if OS_LINUX
#  if LINUX_WAYLAND
#    include "wayland/wayland_include.c"
#  elif LINUX_X11
#    include "x11/x11_include.c"
#  endif
#elif OS_WINDOWS
#  include "win32/win32_include.c"
#else
# error no backend for graphics_include.c on this operating system
#endif
