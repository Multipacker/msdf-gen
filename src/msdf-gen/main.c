#include "src/base/base_include.h"
#include "src/graphics/graphics_include.h"
#include "src/render/render_include.h"
#include "src/font/font_include.h"
#include "src/font_cache/font_cache_include.h"
#include "src/draw/draw_include.h"
#include "src/ui/ui_include.h"

#include "src/base/base_include.c"
#include "src/graphics/graphics_include.c"
#include "src/render/render_include.c"
#include "src/font/font_include.c"
#include "src/font_cache/font_cache_include.c"
#include "src/draw/draw_include.c"
#include "src/ui/ui_include.c"

/*
 * TODO:
 * Clipboard
 * Focus and keyboard navigation / interaction
 * More input information
 */

/*
 * TODO before next release:
 * * Bake the UI font into the executable
 * * Offload MSDF generation to a background thread so that the main thread and
 *   UI don't hang because we are generating glyphs.
 * * Add drop-shadows to make it easier to distinguish foreground elements from
 *   background ones.
 * * Improve the look of the preview when dragging tabs
 *
 * TODO long term
 * * Allow multiple codepoints to map to the same glyph, alternatively allow
 *   marking glyphs as missing as that is the main use case
 */

/*
 * FIXME:
 * * Tabbars are not clipped to the panels section of the screen, so they can
 *   overlap other panels.
 */

typedef struct Glyph Glyph;
struct Glyph {
    Glyph *next;
    Glyph *previous;

    U32 codepoint;

    R2F32 rectangle_pt;
    F32   advance_pt;
    R2F32 uv;
    Render_Texture texture;

    MSDF_Log log;
};

global Glyph global_glyph_null = { 0 };

typedef struct GlyphList GlyphList;
struct GlyphList {
    Glyph *first;
    Glyph *last;
};

// NOTE(simon): Must be a power of 2
#define ATLAS_GLYPHS_PER_SIDE (1 << 6)
typedef struct Atlas Atlas;
struct Atlas {
    Atlas *next;
    Atlas *previous;

    Render_Texture texture;
    U64 occupancy[(ATLAS_GLYPHS_PER_SIDE * ATLAS_GLYPHS_PER_SIDE + 63) / 64];
};

typedef struct Font Font;
struct Font {
    Arena *arena;

    U32 glyph_size;
    U32 glyphs_per_side;

    Atlas *first_atlas;
    Atlas *last_atlas;

    GlyphList glyph_lists[2048];
    TTF_Font *ttf;

    U32 next_glyph_index;
};

internal Font *font_create(Str8 font_path, U32 glyph_size) {
    Arena *arena = arena_create();
    Font *result = arena_push_struct_zero(arena, Font);

    result->glyph_size = glyph_size;

    result->arena = arena;
    result->ttf   = ttf_load(arena, font_path);

    if (result->ttf->errors.node_count != 0) {
        Arena_Temporary scratch = arena_get_scratch(0, 0);
        os_console_print(str8_join(scratch.arena, &result->ttf->errors));
        arena_end_temporary(scratch);
    }

    return result;
}

internal Glyph *font_get_glyph(Font *font, U32 codepoint) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    U64 hash = u64_hash(codepoint);
    GlyphList *glyphs = &font->glyph_lists[hash % array_count(font->glyph_lists)];

    Glyph *result = &global_glyph_null;

    for (Glyph *glyph = glyphs->first; glyph; glyph = glyph->next) {
        if (glyph->codepoint == codepoint) {
            result = glyph;
            break;
        }
    }

    // NOTE(simon): Generate the glyph if it doesn't exist yet.
    if (result == &global_glyph_null) {
        result = arena_push_struct_zero(font->arena, Glyph);
        result->codepoint = codepoint;

        MSDF_RasterResult raster_result = msdf_generate(scratch.arena, font->ttf, codepoint, font->glyph_size);

        // NOTE(simon): Select glyph atlas.
        Atlas *selected_atlas = 0;
        for (Atlas *atlas = font->first_atlas; atlas && !selected_atlas; atlas = atlas->next) {
            for (U64 i = 0; i < array_count(atlas->occupancy); ++i) {
                if (~atlas->occupancy[i] != 0) {
                    selected_atlas = atlas;
                    break;
                }
            }
        }

        // NOTE(simon): Allocate a new atlas if we couldn't find one with space in it.
        if (!selected_atlas) {
            V2U32 size = v2u32(font->glyph_size * ATLAS_GLYPHS_PER_SIDE, font->glyph_size * ATLAS_GLYPHS_PER_SIDE);
            selected_atlas = arena_push_struct_zero(font->arena, Atlas);
            selected_atlas->texture = render_texture_create(size, Render_TextureFormat_RGBA8, 0);
            dll_push_back(font->first_atlas, font->last_atlas, selected_atlas);
        }

        // NOTE(simon): Allocate atlas region.
        U32 glyph_index = 0;
        while (~selected_atlas->occupancy[glyph_index / 64] == 0) {
            glyph_index += 64;
        }
        while ((selected_atlas->occupancy[glyph_index / 64] & (U64) (1 << glyph_index % 64)) != 0) {
            ++glyph_index;
        }
        selected_atlas->occupancy[glyph_index / 64] |= (U64) (1 << glyph_index % 64);

        V2U32 atlas_position = v2u32(
            font->glyph_size * (glyph_index % ATLAS_GLYPHS_PER_SIDE),
            font->glyph_size * (glyph_index / ATLAS_GLYPHS_PER_SIDE)
        );

        render_texture_update(
            selected_atlas->texture,
            atlas_position,
            v2u32(font->glyph_size, font->glyph_size),
            raster_result.data
        );

        // This adjustment increases the size of glyphs to acount for the
        // source needing to include a 1/2 texel border for rendering. This
        // makes sure that the glyphs have the same visual size.
        F32 scale = ((F32) font->glyph_size - 1.0f) / ((F32) font->glyph_size - 2.0f) - 1.0f;
        V2F32 adjustment = v2f32_scale(v2f32_subtract(raster_result.max, raster_result.min), 0.5f * scale);

        result->advance_pt = raster_result.advance_width;
        result->rectangle_pt = r2f32(
            raster_result.min.x - adjustment.x,
            raster_result.min.y - adjustment.y,
            raster_result.max.x + adjustment.x,
            raster_result.max.y + adjustment.y
        );

        result->uv = r2f32(
            (F32) atlas_position.x + 0.5f,
            (F32) atlas_position.y + 0.5f,
            (F32) atlas_position.x + (F32) font->glyph_size - 0.5f,
            (F32) atlas_position.y + (F32) font->glyph_size - 0.5f
        );
        result->texture = selected_atlas->texture;
        for (MSDF_LogEntry *src_entry = raster_result.log.first; src_entry; src_entry = src_entry->next) {
            MSDF_LogEntry *entry = arena_push_struct_zero(font->arena, MSDF_LogEntry);
            entry->description = str8_copy(font->arena, src_entry->description);
            for (MSDF_LogGroup *src_group = src_entry->first_group; src_group; src_group = src_group->next) {
                MSDF_LogGroup *group = arena_push_struct_zero(font->arena, MSDF_LogGroup);
                for (MSDF_LogGeometry *src_geometry = src_group->first_geometry; src_geometry; src_geometry = src_geometry->next) {
                    MSDF_LogGeometry *geometry = arena_push_struct_zero(font->arena, MSDF_LogGeometry);
                    memory_copy(geometry, src_geometry, sizeof(*geometry));
                    dll_push_back(group->first_geometry, group->last_geometry, geometry);
                }
                dll_push_back(entry->first_group, entry->last_group, group);
                ++entry->group_count;
            }
            dll_push_back(result->log.first, result->log.last, entry);
            ++result->log.count;
        }

        dll_push_back(glyphs->first, glyphs->last, result);
    }

    arena_end_temporary(scratch);
    return result;
}

internal Void draw_text_msdf(Font *font, V2F32 position, F32 point_size, Str8 text) {
    V2F32 text_point = position;
    for (U8 *ptr = text.data, *opl = text.data + text.size; ptr < opl; ) {
        StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
        ptr += decode.size;

        Glyph *glyph = font_get_glyph(font, decode.codepoint);

        draw_msdf(
            r2f32(
                text_point.x + glyph->rectangle_pt.min.x * point_size,
                text_point.y + glyph->rectangle_pt.min.y * point_size,
                text_point.x + glyph->rectangle_pt.max.x * point_size,
                text_point.y + glyph->rectangle_pt.max.y * point_size
            ),
            glyph->uv,
            glyph->texture,
            v4f32(1.0f, 1.0f, 1.0f, 1.0f)
        );

        text_point.x += glyph->advance_pt * point_size;
    }
}

#include "core.h"
#include "views.h"

#include "core.c"
#include "views.c"

internal S32 os_run(Str8List arguments) {
    if (!arguments.first->next) {
        os_console_print(str8_literal("You have to pass a file\n"));
        os_exit(1);
    }

    Arena *arena = arena_create();
    State *state = arena_push_struct(arena, State);
    state->arena = arena;
    global_state = state;

    for (U64 i = 0; i < array_count(state->frame_arenas); ++i) {
        state->frame_arenas[i] = arena_create();
    }

    state->context_stack = &state->base_context;

    state->command_arena = arena_create();

    // NOTE(simon): Themes
    {
        {
            Theme *theme = &global_themes[0];
            theme->name = str8_literal("Catppuccin Latte");

            V4F32 rosewater = color_from_srgba_u32(0xDC8A78FF);
            V4F32 flamingo  = color_from_srgba_u32(0xDD7878FF);
            V4F32 pink      = color_from_srgba_u32(0xEA76CBFF);
            V4F32 mauve     = color_from_srgba_u32(0x8839EFFF);
            V4F32 red       = color_from_srgba_u32(0xD20F39FF);
            V4F32 maroon    = color_from_srgba_u32(0xE64553FF);
            V4F32 peach     = color_from_srgba_u32(0xFE640BFF);
            V4F32 yellow    = color_from_srgba_u32(0xDF8E1DFF);
            V4F32 green     = color_from_srgba_u32(0x40A02BFF);
            V4F32 teal      = color_from_srgba_u32(0x179299FF);
            V4F32 sky       = color_from_srgba_u32(0x04A5E5FF);
            V4F32 sapphire  = color_from_srgba_u32(0x209FB5FF);
            V4F32 blue      = color_from_srgba_u32(0x1E66F5FF);
            V4F32 lavender  = color_from_srgba_u32(0x7287FDFF);
            V4F32 text      = color_from_srgba_u32(0x4C4F69FF);
            V4F32 subtext1  = color_from_srgba_u32(0x5C5F77FF);
            V4F32 subtext0  = color_from_srgba_u32(0x6C6F85FF);
            V4F32 overlay2  = color_from_srgba_u32(0x7C7F93FF);
            V4F32 overlay1  = color_from_srgba_u32(0x8C8FA1FF);
            V4F32 overlay0  = color_from_srgba_u32(0x9CA0B0FF);
            V4F32 surface2  = color_from_srgba_u32(0xACB0BEFF);
            V4F32 surface1  = color_from_srgba_u32(0xBCC0CCFF);
            V4F32 surface0  = color_from_srgba_u32(0xCCD0DAFF);
            V4F32 base      = color_from_srgba_u32(0xEFF1F5FF);
            V4F32 mantle    = color_from_srgba_u32(0xE6E9EFFF);
            V4F32 crust     = color_from_srgba_u32(0xDCE0E8FF);

            theme->text  = text;
            theme->hover = overlay2;
            theme->cursor = rosewater;
            theme->selection = overlay2;
            theme->focus = rosewater;

            theme->disabled_overlay         = overlay0;
            theme->disabled_overlay.a       = 0.5f;
            theme->drop_site_overlay        = overlay1;
            theme->drop_site_overlay.a      = 0.5f;
            theme->inactive_panel_overlay   = crust;
            theme->inactive_panel_overlay.a = 0.2f;
            theme->drop_shadow              = v4f32(0.0f, 0.0f, 0.0f, 0.8f);

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface0;

            theme->outline   = overlay0;
            theme->on_curve  = green;
            theme->off_curve = red;
        }

        {
            Theme *theme = &global_themes[1];
            theme->name = str8_literal("Catppuccin Frappé");

            V4F32 rosewater = color_from_srgba_u32(0xF2D5CFFF);
            V4F32 flamingo  = color_from_srgba_u32(0xEEBEBEFF);
            V4F32 pink      = color_from_srgba_u32(0xF4B8E4FF);
            V4F32 mauve     = color_from_srgba_u32(0xCA9EE6FF);
            V4F32 red       = color_from_srgba_u32(0xE78284FF);
            V4F32 maroon    = color_from_srgba_u32(0xEA999CFF);
            V4F32 peach     = color_from_srgba_u32(0xEF9F76FF);
            V4F32 yellow    = color_from_srgba_u32(0xE5C890FF);
            V4F32 green     = color_from_srgba_u32(0xA6D189FF);
            V4F32 teal      = color_from_srgba_u32(0x81C8BEFF);
            V4F32 sky       = color_from_srgba_u32(0x99D1DBFF);
            V4F32 sapphire  = color_from_srgba_u32(0x85C1DCFF);
            V4F32 blue      = color_from_srgba_u32(0x8CAAEEFF);
            V4F32 lavender  = color_from_srgba_u32(0xBABBF1FF);
            V4F32 text      = color_from_srgba_u32(0xC6D0F5FF);
            V4F32 subtext1  = color_from_srgba_u32(0xB5BFE2FF);
            V4F32 subtext0  = color_from_srgba_u32(0xA5ADCEFF);
            V4F32 overlay2  = color_from_srgba_u32(0x949CBBFF);
            V4F32 overlay1  = color_from_srgba_u32(0x838BA7FF);
            V4F32 overlay0  = color_from_srgba_u32(0x737994FF);
            V4F32 surface2  = color_from_srgba_u32(0x626880FF);
            V4F32 surface1  = color_from_srgba_u32(0x51576DFF);
            V4F32 surface0  = color_from_srgba_u32(0x414559FF);
            V4F32 base      = color_from_srgba_u32(0x303446FF);
            V4F32 mantle    = color_from_srgba_u32(0x292C3CFF);
            V4F32 crust     = color_from_srgba_u32(0x232634FF);

            theme->text  = text;
            theme->hover = overlay2;
            theme->cursor = rosewater;
            theme->selection = overlay2;
            theme->focus = rosewater;

            theme->disabled_overlay         = overlay0;
            theme->disabled_overlay.a       = 0.5f;
            theme->drop_site_overlay        = overlay1;
            theme->drop_site_overlay.a      = 0.5f;
            theme->inactive_panel_overlay   = crust;
            theme->inactive_panel_overlay.a = 0.5f;
            theme->drop_shadow              = v4f32(0.0f, 0.0f, 0.0f, 0.8f);

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface0;

            theme->outline   = overlay0;
            theme->on_curve  = green;
            theme->off_curve = red;
        }

        {
            Theme *theme = &global_themes[2];
            theme->name = str8_literal("Catppuccin Macchiato");

            V4F32 rosewater = color_from_srgba_u32(0xF4DBD6FF);
            V4F32 flamingo  = color_from_srgba_u32(0xF0C6C6FF);
            V4F32 pink      = color_from_srgba_u32(0xF5BDE6FF);
            V4F32 mauve     = color_from_srgba_u32(0xC6A0F6FF);
            V4F32 red       = color_from_srgba_u32(0xED8796FF);
            V4F32 maroon    = color_from_srgba_u32(0xEE99A0FF);
            V4F32 peach     = color_from_srgba_u32(0xF5A97FFF);
            V4F32 yellow    = color_from_srgba_u32(0xEED49FFF);
            V4F32 green     = color_from_srgba_u32(0xA6DA95FF);
            V4F32 teal      = color_from_srgba_u32(0x8BD5CAFF);
            V4F32 sky       = color_from_srgba_u32(0x91D7E3FF);
            V4F32 sapphire  = color_from_srgba_u32(0x7DC4E4FF);
            V4F32 blue      = color_from_srgba_u32(0x8AADF4FF);
            V4F32 lavender  = color_from_srgba_u32(0xB7BDF8FF);
            V4F32 text      = color_from_srgba_u32(0xCAD3F5FF);
            V4F32 subtext1  = color_from_srgba_u32(0xB8C0E0FF);
            V4F32 subtext0  = color_from_srgba_u32(0xA5ADCBFF);
            V4F32 overlay2  = color_from_srgba_u32(0x939AB7FF);
            V4F32 overlay1  = color_from_srgba_u32(0x8087A2FF);
            V4F32 overlay0  = color_from_srgba_u32(0x6E738DFF);
            V4F32 surface2  = color_from_srgba_u32(0x5B6078FF);
            V4F32 surface1  = color_from_srgba_u32(0x494D64FF);
            V4F32 surface0  = color_from_srgba_u32(0x363A4FFF);
            V4F32 base      = color_from_srgba_u32(0x24273AFF);
            V4F32 mantle    = color_from_srgba_u32(0x1E2030FF);
            V4F32 crust     = color_from_srgba_u32(0x181926FF);

            theme->text  = text;
            theme->hover = overlay2;
            theme->cursor = rosewater;
            theme->selection = overlay2;
            theme->focus = rosewater;

            theme->disabled_overlay         = overlay0;
            theme->disabled_overlay.a       = 0.5f;
            theme->drop_site_overlay        = overlay1;
            theme->drop_site_overlay.a      = 0.5f;
            theme->inactive_panel_overlay   = crust;
            theme->inactive_panel_overlay.a = 0.5f;
            theme->drop_shadow              = v4f32(0.0f, 0.0f, 0.0f, 0.8f);

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface0;

            theme->outline   = overlay0;
            theme->on_curve  = green;
            theme->off_curve = red;
        }

        {
            Theme *theme = &global_themes[3];
            theme->name = str8_literal("Catppuccin Mocha");

            V4F32 rosewater = color_from_srgba_u32(0xF5E0DCFF);
            V4F32 flamingo  = color_from_srgba_u32(0xF2CDCDFF);
            V4F32 pink      = color_from_srgba_u32(0xF5C2E7FF);
            V4F32 mauve     = color_from_srgba_u32(0xCBA6F7FF);
            V4F32 red       = color_from_srgba_u32(0xF38BA8FF);
            V4F32 maroon    = color_from_srgba_u32(0xEBA0ACFF);
            V4F32 peach     = color_from_srgba_u32(0xFAB387FF);
            V4F32 yellow    = color_from_srgba_u32(0xF9E2AFFF);
            V4F32 green     = color_from_srgba_u32(0xA6E3A1FF);
            V4F32 teal      = color_from_srgba_u32(0x94E2D5FF);
            V4F32 sky       = color_from_srgba_u32(0x89DCEBFF);
            V4F32 sapphire  = color_from_srgba_u32(0x74C7ECFF);
            V4F32 blue      = color_from_srgba_u32(0x89B4FAFF);
            V4F32 lavender  = color_from_srgba_u32(0xB4BEFEFF);
            V4F32 text      = color_from_srgba_u32(0xCDD6F4FF);
            V4F32 subtext1  = color_from_srgba_u32(0xBAC2DEFF);
            V4F32 subtext0  = color_from_srgba_u32(0xA6ADC8FF);
            V4F32 overlay2  = color_from_srgba_u32(0x9399B2FF);
            V4F32 overlay1  = color_from_srgba_u32(0x7F849CFF);
            V4F32 overlay0  = color_from_srgba_u32(0x6C7086FF);
            V4F32 surface2  = color_from_srgba_u32(0x585B70FF);
            V4F32 surface1  = color_from_srgba_u32(0x45475AFF);
            V4F32 surface0  = color_from_srgba_u32(0x313244FF);
            V4F32 base      = color_from_srgba_u32(0x1E1E2EFF);
            V4F32 mantle    = color_from_srgba_u32(0x181825FF);
            V4F32 crust     = color_from_srgba_u32(0x11111BFF);

            theme->text  = text;
            theme->hover = overlay2;
            theme->cursor = rosewater;
            theme->selection = overlay2;
            theme->focus = rosewater;

            theme->disabled_overlay         = overlay0;
            theme->disabled_overlay.a       = 0.5f;
            theme->drop_site_overlay        = overlay1;
            theme->drop_site_overlay.a      = 0.5f;
            theme->inactive_panel_overlay   = crust;
            theme->inactive_panel_overlay.a = 0.5f;
            theme->drop_shadow              = v4f32(0.0f, 0.0f, 0.0f, 0.8f);

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface0;

            theme->outline   = overlay0;
            theme->on_curve  = green;
            theme->off_curve = red;
        }
    }

    state->ui = ui_create();
    state->panel_root = arena_push_struct_zero(state->arena, Panel);
    state->panel_root->percentage_of_parent = 1.0f;
    state->panel_root->split_axis = Axis2_X;
    {
        Panel *left = panel_create(state);
        push_command(Command_OpenTab, .panel = handle_from_panel(left), .tab_specification = str8_literal("Theme"));
        push_command(Command_OpenTab, .panel = handle_from_panel(left), .tab_specification = str8_literal("GlyphList"));
        push_command(Command_OpenTab, .panel = handle_from_panel(left), .tab_specification = str8_literal("Test"));

        Panel *right = panel_create(state);
        right->split_axis = Axis2_Y;
        left->percentage_of_parent = 0.65f;
        right->percentage_of_parent = 0.35f;
        panel_insert(state->panel_root, 0, left);
        panel_insert(state->panel_root, left, right);

        // TODO(simon): This should be updated when the user navigates the interface
        state->active_panel = handle_from_panel(left);

        Panel *top = panel_create(state);
        push_command(Command_OpenTab, .panel = handle_from_panel(top), .tab_specification = str8_literal("GlyphView"));

        Panel *bottom = panel_create(state);
        push_command(Command_OpenTab, .panel = handle_from_panel(bottom), .tab_specification = str8_literal("RenderStats"));

        top->percentage_of_parent = 0.65f;
        bottom->percentage_of_parent = 0.35f;
        panel_insert(right, 0, top);
        panel_insert(right, top, bottom);
    }

    state->running = true;

    state->theme_index = 3;
    state->theme = global_themes[state->theme_index];
    state->target_theme = global_themes[state->theme_index];

    state->frames_to_render = 4;

    gfx_create(str8_literal("MSDF-gen"), 1280, 720);
    render_init();
    render_create();
    font_cache_create();
    gfx_set_update_function(update);

    state->font = font_create(arguments.first->next->string, 32);
    state->ttf_font = ttf_load(arena, arguments.first->next->string);

    while (state->running) {
        update();
    }

    return 0;
}
