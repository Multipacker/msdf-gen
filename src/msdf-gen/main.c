#include "src/base/base_include.h"
#include "src/graphics/graphics_include.h"
#include "src/render/render_include.h"
#include "src/font/font_include.h"
#include "src/font_cache/font_cache_include.h"
#include "src/ui/ui_include.h"
#include "src/c_lexer/c_lexer_include.h"

#include "src/base/base_include.c"
#include "src/graphics/graphics_include.c"
#include "src/render/render_include.c"
#include "src/font/font_include.c"
#include "src/font_cache/font_cache_include.c"
#include "src/ui/ui_include.c"
#include "src/c_lexer/c_lexer_include.c"

typedef struct {
    Str8 name;
    V4F32 background_color;
    V4F32 element_color;
    V4F32 border_color;
    V4F32 text_color;
} Theme;

global Theme global_themes[2];

typedef struct {
    V2F32 min_pt;
    V2F32 max_pt;
    F32   advance_pt;
    V2F32 uv_min;
    V2F32 uv_max;
} Glyph;

typedef struct {
    Render_Texture atlas;
    Glyph glyphs[256];
} Font;

internal Void load_font(Str8 font_path, Font *result) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    U32 glyph_size     = 32;
    U32 glyphs_per_row = 16;
    U32 atlas_size     = glyph_size * glyphs_per_row;
    result->atlas = render_texture_create(v2u32(atlas_size, atlas_size), Render_TextureFormat_RGBA8, 0);

    TTF_Font *font = ttf_load(scratch.arena, font_path);
    if (font->errors.node_count == 0) {
        // Generate glyphs
        for (U32 codepoint = 0; codepoint < 256; ++codepoint) {
            Arena_Temporary glyph_scratch = arena_get_scratch(&scratch.arena, 1);

            MSDF_RasterResult raster_result = msdf_generate(glyph_scratch.arena, font, codepoint, glyph_size);

            V2U32 atlas_position = v2u32(
                glyph_size * (codepoint % glyphs_per_row),
                glyph_size * (codepoint / glyphs_per_row)
            );
            render_texture_update(result->atlas, atlas_position, v2u32(glyph_size, glyph_size), raster_result.data);

            // This adjustment increases the size of glyphs to acount for the
            // UVs needing to include a 1/2 texel border for rendering. This
            // makes sure that the glyphs have the same visual size.
            F32 scale = ((F32) glyph_size - 1.0f) / ((F32) glyph_size - 2.0f) - 1.0f;
            F32 width_adjustment  = (raster_result.x_max - raster_result.x_min) * scale * 0.5f;
            F32 height_adjustment = (raster_result.y_max - raster_result.y_min) * scale * 0.5f;

            result->glyphs[codepoint].advance_pt = raster_result.advance_width;
            result->glyphs[codepoint].min_pt = v2f32(raster_result.x_min - width_adjustment, raster_result.y_min - height_adjustment);
            result->glyphs[codepoint].max_pt = v2f32(raster_result.x_max + width_adjustment, raster_result.y_max + height_adjustment);
            result->glyphs[codepoint].uv_min = v2f32(
                ((F32) atlas_position.x + 0.5f) / atlas_size,
                ((F32) atlas_position.y + 0.5f) / atlas_size
            );
            result->glyphs[codepoint].uv_max = v2f32(
                ((F32) atlas_position.x + glyph_size - 0.5f) / atlas_size,
                ((F32) atlas_position.y + glyph_size - 0.5f) / atlas_size
            );

            arena_end_temporary(glyph_scratch);
        }
    } else {
        os_console_print(str8_join(scratch.arena, &font->errors));
    }

    arena_end_temporary(scratch);
}

internal Void draw_text_msdf(Font *font, V2F32 position, F32 point_size, Str8 text) {
    V2F32 text_point = position;
    for (U8 *ptr = text.data, *opl = text.data + text.size; ptr < opl; ) {
        StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
        ptr += decode.size;

        Glyph *glyph = &font->glyphs[decode.codepoint];

        render_rectangle(
            v2f32_add(text_point, v2f32_scale(glyph->min_pt, point_size)), v2f32_add(text_point, v2f32_scale(glyph->max_pt, point_size)),
            .uv_min = glyph->uv_min, .uv_max = glyph->uv_max,
            .texture = font->atlas,
            .color = v4f32(1.0f, 1.0f, 1.0f, 1.0f),
            .flags = Render_RectangleFlags_MSDF
        );

        text_point.x += glyph->advance_pt * point_size;
    }
}

internal Void draw_text(FontCache_Font *font, V2F32 origin, Str8 string, U32 size) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    FontCache_Text text = font_cache_text(scratch.arena, font, string, size);

    F32 advance = 0.0f;
    for (U64 i = 0; i < text.letter_count; ++i) {
        FontCache_Letter *letter = &text.letters[i];
        render_rectangle(
            v2f32(
                origin.x + letter->offset.x + advance,
                origin.y + letter->offset.y
            ),
            v2f32(
                origin.x + letter->offset.x + advance + letter->size.x,
                origin.y + letter->offset.y + letter->size.y
            ),
            .uv_min = letter->uvs.min, .uv_max = letter->uvs.max,
            .texture = letter->texture,
            .flags = Render_RectangleFlags_AlphaMask
        );
        advance += letter->advance;
    }

    arena_end_temporary(scratch);
}

internal UI_BOX_DRAW_FUNCTION(draw_ui_msdf) {
    Font *font = (Font *) data;
    Glyph *glyph = &font->glyphs[box->string.data[0]];

    render_rectangle(
        box->calculated_rectangle.min,
        box->calculated_rectangle.max,
        .uv_min = glyph->uv_min, .uv_max = glyph->uv_max,
        .texture = font->atlas,
        .flags = Render_RectangleFlags_MSDF
    );
}

internal Void draw_ui(UI_Box *box) {
    if (box->flags & UI_BoxFlags_DrawBackground) {
        render_rectangle(
            box->calculated_rectangle.min, box->calculated_rectangle.max,
            .color = box->color,
        );

        if (box->flags & UI_BoxFlags_DrawHot && box->hot_t > 0.0f) {
            Render_Rectangle *rect = render_rectangle(
                box->calculated_rectangle.min, box->calculated_rectangle.max
            );
            rect->colors[0] = v4f32(1.0f, 1.0f, 1.0f, 0.5f * box->hot_t);
            rect->colors[1] = v4f32(1.0f, 1.0f, 1.0f, 0.5f * box->hot_t);
            rect->colors[2] = v4f32(0.0f, 0.0f, 0.0f, 0.0f);
            rect->colors[3] = v4f32(0.0f, 0.0f, 0.0f, 0.0f);
        }

        if (box->flags & UI_BoxFlags_DrawActive && box->active_t > 0.0f) {
            Render_Rectangle *rect = render_rectangle(
                box->calculated_rectangle.min, box->calculated_rectangle.max
            );
            rect->colors[0] = v4f32(0.0f, 0.0f, 0.0f, 0.0f);
            rect->colors[1] = v4f32(0.0f, 0.0f, 0.0f, 0.0f);
            rect->colors[2] = v4f32(0.0f, 0.0f, 0.0f, 0.5f * box->active_t);
            rect->colors[3] = v4f32(0.0f, 0.0f, 0.0f, 0.5f * box->active_t);
        }
    }

    if (box->flags & UI_BoxFlags_DrawText) {
        V2F32 origin = box->calculated_rectangle.min;
        F32 advance = 0.0f;
        for (U64 i = 0; i < box->text.letter_count; ++i) {
            FontCache_Letter *letter = &box->text.letters[i];
            render_rectangle(
                v2f32(
                    f32_floor(origin.x + letter->offset.x + advance),
                    f32_floor(origin.y + letter->offset.y)
                ),
                v2f32(
                    f32_floor(origin.x + letter->offset.x + advance + letter->size.x),
                    f32_floor(origin.y + letter->offset.y + letter->size.y)
                ),
                .color = box->text_color,
                .uv_min = letter->uvs.min, .uv_max = letter->uvs.max,
                .texture = letter->texture,
                .flags = Render_RectangleFlags_AlphaMask
            );
            advance += letter->advance;
        }
    }

    if (box->draw_function) {
        box->draw_function(box, box->draw_data);
    }

    if (box->flags & UI_BoxFlags_DrawBorder) {
        render_rectangle(
            box->calculated_rectangle.min, box->calculated_rectangle.max,
            .color = box->border_color,
            .thickness = 1.0f,
            .softness = 1.0f
        );
    }

    if (box->flags & UI_BoxFlags_Disabled) {
        Render_Rectangle *rect = render_rectangle(
            box->calculated_rectangle.min, box->calculated_rectangle.max,
            .color = v4f32(0.2f, 0.2f, 0.2f, 0.75f)
        );
    }

    for (UI_Box *child = box->last; child != &global_ui_null_box; child = child->previous) {
        draw_ui(child);
    }
}

internal S32 os_run(Str8List arguments) {
    Arena *arena = arena_create();

    Str8 source = { 0 };
    if (os_file_read(arena, str8_literal("src/base/base_core.h"), &source)) {
        CProc_LexerResult lexer_result = cproc_tokens_from_string(arena, source);
        for (U64 i = 0; i < lexer_result.tokens.count; ++i) {
            CProc_Token_Kind kind = lexer_result.tokens.tokens[i].kind;
            if (kind & CProc_Token_HeaderName)        { os_console_print(str8_literal("HeaderName"));        }
            if (kind & CProc_Token_Identifier)        { os_console_print(str8_literal("Identifier"));        }
            if (kind & CProc_Token_Number)            { os_console_print(str8_literal("Number"));            }
            if (kind & CProc_Token_CharacterConstant) { os_console_print(str8_literal("CharacterConstant")); }
            if (kind & CProc_Token_StringLiteral)     { os_console_print(str8_literal("StringLiteral"));     }
            if (kind & CProc_Token_Punctuator)        { os_console_print(str8_literal("Punctuator"));        }
            if (kind & CProc_Token_Whitespace)        { os_console_print(str8_literal("Whitespace"));        }
            if (kind & CProc_Token_Newline)           { os_console_print(str8_literal("Newline"));           }
            if (kind & CProc_Token_Comment)           { os_console_print(str8_literal("Comment"));           }
            if (kind & CProc_Token_Unknown)           { os_console_print(str8_literal("Unknown"));           }

            os_console_print(str8_literal(": "));
            os_console_print(lexer_result.tokens.tokens[i].source);
            os_console_print(str8_literal("\n"));
        }
        for (CProc_Error *error = lexer_result.errors.first; error; error = error->next) {
            Arena_Temporary scratch = arena_get_scratch(0, 0);
            os_console_print(str8_format(arena, "ERROR(%lu:%lu): %.*s\n", error->location.line, error->location.column, str8_expand(error->message)));
            arena_end_temporary(scratch);
        }
    }

    arena_destroy(arena);
    return 0;
#if 0

    if (!arguments.first->next) {
        os_console_print(str8_literal("You have to pass a file\n"));
        os_exit(1);
    }

    Arena *arena = arena_create();

    // NOTE(simon): Themes
    {
        global_themes[0].name = str8_literal("Light theme");
        global_themes[0].background_color = color_from_srgba_u32(0xF8F9FAFF); // OC Gray 0
        global_themes[0].element_color    = color_from_srgba_u32(0xE9ECEFFF); // OC Gray 2
        global_themes[0].border_color     = color_from_srgba_u32(0xCED4DAFF); // OC Gray 4
        global_themes[0].text_color       = color_from_srgba_u32(0x495057FF); // OC Gray 7

        global_themes[1].name = str8_literal("Dark theme"),
        global_themes[1].background_color = color_from_srgba_u32(0x212529FF); // OC Gray 9
        global_themes[1].element_color    = color_from_srgba_u32(0x343A40FF); // OC Gray 8
        global_themes[1].border_color     = color_from_srgba_u32(0x495057FF); // OC Gray 7
        global_themes[1].text_color       = color_from_srgba_u32(0xF8F9FAFF); // OC Gray 0
    }

    render_init();
    UI_Context *ui = ui_create();

    Gfx_Context *gfx = gfx_create(arena, str8_literal("MSDF-gen"), 1280, 720);
    if (gfx->errors.node_count) {
        os_console_print(str8_join(arena, &gfx->errors));
        return -1;
    }
    render_create(gfx);

    Font font = { 0 };
    load_font(arguments.first->next->string, &font);

    font_cache_create();

    B32 running = true;

    Arena *current_arena  = arena_create();
    Arena *previous_arena = arena_create();

    while (running) {
        Gfx_EventList events = gfx_get_events(current_arena, gfx);
        ui_begin(gfx, ui, &events, 1.0f / 60.0f);

        Theme *theme = &global_themes[1];
        local U32 selected_codepoint = 0;

        U32 codepoint_count = 128;
        ui_extra_box_flags_next(ui, UI_BoxFlags_OverflowY);
        ui_width_next(ui, ui_size_parent_percent(1.0f, 0.0f));
        ui_height_next(ui, ui_size_children_sum(1.0f));
        ui_column_string(ui, str8_literal("glyphs")) {
            UI_Box *container = ui_parent_top(ui);
            U32 column_count = (U32) (container->calculated_size.width / 200.0f);
            if (!column_count) {
                column_count = 10;
            }

            ui_color_push(ui, theme->element_color);
            ui_border_color_push(ui, theme->border_color);
            for (U32 codepoint = 0; codepoint < codepoint_count;) {
                ui_width_next(ui, ui_size_parent_percent(1.0f, 1.0f));
                ui_height_next(ui, ui_size_children_sum(1.0f));
                ui_row(ui) {
                    ui_width(ui, ui_size_fill())
                    ui_height(ui, ui_size_pixels(50.0f, 1.0f))
                    for (U32 column = 0; column < column_count && codepoint < codepoint_count; ++column, ++codepoint) {
                        U8 buffer[4] = { 0 };
                        U64 size = string_encode_utf8(buffer, codepoint);
                        Str8 string = str8(buffer, size);

                        ui_draw_function_next(ui, draw_ui_msdf);
                        ui_draw_data_next(ui, &font);
                        UI_Box *box = ui_create_box_from_string(
                            ui,
                            UI_BoxFlags_DrawBackground | UI_BoxFlags_DrawBorder |
                            UI_BoxFlags_DrawHot | UI_BoxFlags_DrawActive |
                            UI_BoxFlags_Clickable,
                            string
                        );
                        UI_Input input = ui_input_from_box(ui, box);

                        if (input.input_flags & UI_InputFlag_LeftClicked) {
                            selected_codepoint = codepoint;
                        }
                    }
                }
            }
            ui_border_color_pop(ui);
            ui_color_pop(ui);
        }

        ui_width_next(ui, ui_size_parent_percent(0.25f, 1.0f));
        ui_height_next(ui, ui_size_parent_percent(1.0f, 1.0f));
        ui_color_next(ui, theme->background_color);
        ui_extra_box_flags_next(ui, UI_BoxFlags_DrawBackground);
        ui_column(ui) {
            ui_width(ui, ui_size_text_content(5.0f, 1.0f))
            ui_height(ui, ui_size_text_content(5.0f, 1.0f))
            ui_color(ui, theme->element_color)
            ui_border_color(ui, theme->border_color) {
                ui_label_format(ui, "Selected glyph: U+%.6X", selected_codepoint);

                ui_width_next(ui, ui_size_parent_percent(1.0f, 0.0f));
                ui_height_next(ui, ui_size_parent_percent(1.0f, 0.0f));
                ui_draw_function_next(ui, draw_ui_msdf);
                ui_draw_data_next(ui, &font);
                U8 buffer[4] = { 0 };
                U64 size = string_encode_utf8(buffer, selected_codepoint);
                Str8 string = str8(buffer, size);
                ui_create_box_from_string(ui, 0, string);
            }
        }

        ui_end(gfx, ui);

        for (Gfx_Event *event = events.first, *next; event; event = next) {
            next = event->next;
            B32 consumed = false;

            if (event->kind == Gfx_EventKind_Quit) {
                running = false;
                consumed = true;
            }

            if (consumed) {
                dll_remove(events.first, events.last, event);
            }
        }

        V2U32 client_area = gfx_get_window_client_area(gfx);
        render_begin(client_area);
        draw_ui(ui->root);
        render_end();

        arena_reset(previous_arena);
        swap(current_arena, previous_arena, Arena *);
    }

    return 0;
#endif
}
