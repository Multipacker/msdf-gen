#include "src/base/base_include.h"
#include "src/graphics/graphics_include.h"

#include "src/base/base_include.c"
#include "src/graphics/graphics_include.c"

internal Void draw(GraphicsContext *state) {
    local S32 offset_x = 0;
    local S32 offset_y = 0;
    local S32 grab_x   = 0;
    local S32 grab_y   = 0;
    local F32 zoom     = 2.0f;

    if (state->is_grabbed) {
        offset_x += state->x - grab_x;
        offset_y += state->y - grab_y;
    }
    grab_x = state->x;
    grab_y = state->y;

    if (state->scroll_amount) {
        F32 old_zoom = zoom;

        zoom *= f32_pow(0.99, -state->scroll_amount);

        offset_x = state->x - old_zoom / zoom * (state->x - offset_x);
        offset_y = state->y - old_zoom / zoom * (state->y - offset_y);
    }

    graphics_msdf(state, v2f32(offset_x, offset_y), v2f32(100.0f / zoom, 100.0f / zoom), v3f32(1, 1, 1), v2f32(0, 0), v2f32(1, 1));
    //graphics_text(state, v2f32(offset_x, offset_y), 16 / zoom, str8_literal(u8"Simon Renhult\u00B5\nSimon Renhult\r_____________"));
}

internal S32 os_run(Str8List arguments) {
    FontDescription font_description = { 0 };
    //font_description.path = str8_literal("/usr/share/fonts/TTF/FiraMono-Regular.ttf");
    font_description.path = str8_literal("/usr/share/fonts/TTF/Inconsolata-Regular.ttf"),
    //font_description.path = str8_literal("/usr/share/fonts/ttf-linux-libertine/LinLibertine_Mah.ttf");
    //font_description.path = str8_literal("/usr/share/fonts/noto/NotoSerif-Regular.ttf"); // 2377
    font_description.codepoint_first = 0;
    font_description.codepoint_last  = 127;

    GraphicsContext *context = graphics_create(str8_literal("MSDF-gen"), 1280, 720, &font_description);

    B32 running = true;
    while (running) {
        graphics_begin_frame(context);

        draw(context);

        if (context->exit_requested) {
            running = false;
        }

        graphics_end_frame(context);
    }

    graphics_destroy(context);
    return 0;
}
