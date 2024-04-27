#if OS_LINUX
#  include "wayland/wayland_include.c"
#  include "sdl/sdl_include.c"
#else
# error no backend for graphics_include.c on this operating system
#endif
