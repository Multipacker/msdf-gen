#if OS_LINUX
//#  include "sdl/sdl_include.c"
//#  include "x11/x11_include.c"
#  include "wayland/wayland_include.c"
#elif OS_WINDOWS
#  include "win32/win32_include.c"
#else
# error no backend for graphics_include.c on this operating system
#endif
