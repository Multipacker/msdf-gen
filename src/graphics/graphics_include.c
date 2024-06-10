#if OS_LINUX
#  include "sdl/sdl_include.c"
#elif OS_WINDOWS
#  include "win32/win32_include.c"
#  include "win32/win32_opengl.c"
#else
# error no backend for graphics_include.c on this operating system
#endif
