#include "render_core.c"

#if OS_WINDOWS
# include  "d3d11/d3d11_include.c"
#else
# include "opengl/opengl_include.c"
#endif
