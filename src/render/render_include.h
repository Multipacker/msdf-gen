#ifndef RENDER_INCLUDE_H
#define RENDER_INCLUDE_H

#include "render_core.h"

#if OS_WINDOWS
# include  "d3d11/d3d11_include.h"
#else
# include "opengl/opengl_include.h"
#endif

#endif // RENDER_INCLUDE_H
