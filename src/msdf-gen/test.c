#include "src/base/base_include.h"
#include "src/graphics/graphics_include.h"
#include "src/render/render_include.h"
#include "src/draw/draw_include.h"

#include "src/base/base_include.c"
#include "src/graphics/graphics_include.c"
#include "src/render/render_include.c"
#include "src/draw/draw_include.c"

internal S32 os_run(Str8List arguments) {
    Arena *arena = arena_create();

    gfx_create(str8_literal("Test"), 1280, 720);
    render_init();
    render_create();

    B32 running = true;
    while (running) {
        Arena_Temporary scratch = arena_get_scratch(0, 0);

        Gfx_EventList events = gfx_get_events(scratch.arena, true);
        for (Gfx_Event *event = events.first; event; event = event->next) {
            Str8 kind = { 0 };
            switch (event->kind) {
                case Gfx_EventKind_Null:       { kind = str8_literal("Null");       } break;
                case Gfx_EventKind_Quit:       { kind = str8_literal("Quit");       } break;
                case Gfx_EventKind_KeyPress:   { kind = str8_literal("KeyPress");   } break;
                case Gfx_EventKind_KeyRelease: { kind = str8_literal("KeyRelease"); } break;
                case Gfx_EventKind_Text:       { kind = str8_literal("Text");       } break;
                case Gfx_EventKind_Scroll:     { kind = str8_literal("Scroll");     } break;
                case Gfx_EventKind_Resize:     { kind = str8_literal("Resize");     } break;
                case Gfx_EventKind_FileDrop:   { kind = str8_literal("FileDrop");   } break;
                case Gfx_EventKind_COUNT: {
                } break;
            }

            if (event->kind == Gfx_EventKind_Quit) {
                running = false;
            }

            if (event->kind == Gfx_EventKind_Text) {
                os_console_print(event->text);
                os_console_print(str8_literal("\n"));
            }

            if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_A) {
                os_console_print(str8_literal("Yup\n"));
            }

            if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_MouseLeft) {
                Str8 copy = gfx_get_clipboard_text(scratch.arena);
                os_console_print(copy);
            }
            if (event->kind == Gfx_EventKind_KeyPress && event->key == Gfx_Key_MouseRight) {
                gfx_set_clipboard_text(str8_literal("This is a test text"));
                os_console_print(str8_literal("Copied text\n"));
            }

            if (event->kind == Gfx_EventKind_Scroll) {
                os_console_print(str8_format(scratch.arena, "%f, %f\n", event->scroll.x, event->scroll.y));
            }

            os_console_print(kind);
            os_console_print(str8_literal("\n"));
        }

        V2U32 client_area = gfx_get_window_client_area();
        render_begin(client_area);
        draw_begin_frame();
        Draw_List *draw_list = draw_list_create();
        draw_list_push(draw_list);
        draw_rectangle(r2f32(100, 100, 200, 200), v4f32(1, 0, 0, 1), 0, 0, 0);
        draw_submit_list(draw_list);
        render_end();

        gfx_swap_buffers();
        arena_end_temporary(scratch);
    }

    arena_destroy(arena);
    return 0;
}
