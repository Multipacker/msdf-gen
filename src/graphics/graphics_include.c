#if OS_LINUX
#  include "sdl/sdl_include.c"
#  include "opengl/opengl_include.c"
#else
# error no backend for graphics_include.c on this operating system
#endif
