#if OS_LINUX
#  include "wayland/wayland_include.c"
#else
# error no backend for graphics_include.c on this operating system
#endif

internal Void graphics_quadratic_bezier(GraphicsContext *state, V2F32 p0, V2F32 p1, V2F32 p2, F32 width, V3F32 color) {
    if (
        (p0.x < 0.0f && p1.x < 0.0f && p2.x < 0.0f) ||
        (p0.y < 0.0f && p1.y < 0.0f && p2.y < 0.0f) ||
        (p0.x >= state->width && p1.x >= state->width && p2.x >= state->width) ||
        (p0.y >= state->height && p1.y >= state->height && p2.y >= state->height)
    ) {
        return;
    }

    // TODO: Maybe this should the a configurable variable.
    F32 error = 0.01f;
    F32 lx = 2.0f * f32_abs(p2.x - 2.0f * p1.x + p0.x);
    F32 ly = 2.0f * f32_abs(p2.y - 2.0f * p1.y + p0.y);
    U32 r = f32_max(0.0f, 0.25f * f32_log2(lx * lx + ly * ly) + 4.0f * error);

    if (r == 0) {
        graphics_line(state, p0, p2, width, color);
    } else {
        V2F32 new_points[6] = { 0 };
        quadratic_bezier_split(p0, p1, p2, 0.5f, new_points);
        graphics_quadratic_bezier(state, new_points[0], new_points[1], new_points[2], width, color);
        graphics_quadratic_bezier(state, new_points[3], new_points[4], new_points[5], width, color);
    }
}
