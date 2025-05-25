#include "src/base/base_include.h"
#include "src/graphics/graphics_include.h"
#include "src/render/render_include.h"
#include "src/font/font_include.h"
#include "src/font_cache/font_cache_include.h"
#include "src/draw/draw_include.h"
#include "src/ui/ui_include.h"
#include "src/msdf_cache/msdf_cache_include.h"
#include "src/unicode/unicode_include.h"

#include "src/base/base_include.c"
#include "src/graphics/graphics_include.c"
#include "src/render/render_include.c"
#include "src/font/font_include.c"
#include "src/font_cache/font_cache_include.c"
#include "src/draw/draw_include.c"
#include "src/ui/ui_include.c"
#include "src/msdf_cache/msdf_cache_include.c"
#include "src/unicode/unicode_include.c"

/*
 * TODO before next release:
 * * Improve the look of the preview when dragging tabs
 * * More carefully think about how filtering in the command lister works
 *
 * TODO long term
 * * Focus and keyboard navigation / interaction
 * * More input information
 * * Complete client side decorations for wayland
 * * Client side decorations for Windows
 * * Client side decorations for Xorg
 */

/*
 * FIXME:
 * * Tabbars are not clipped to the panels section of the screen, so they can
 *   overlap other panels.
 */

#include "core.h"
#include "views.h"

#include "core.c"
#include "views.c"

internal S32 os_run(Str8List arguments) {
    Arena *arena = arena_create();
    State *state = arena_push_struct(arena, State);
    state->arena = arena;
    global_state = state;

    for (U64 i = 0; i < array_count(state->frame_arenas); ++i) {
        state->frame_arenas[i] = arena_create();
    }

    Log *log = log_create();
    log_select(log);

    state->context_stack = &state->base_context;

    state->command_arena = arena_create();
    state->popup_arena = arena_create();

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
            theme->weak_text = subtext0;
            theme->hover = overlay1;
            theme->cursor = rosewater;
            theme->selection = overlay2;
            theme->selection.a = 0.3f;
            theme->focus = lavender;

            theme->disabled_overlay         = overlay0;
            theme->disabled_overlay.a       = 0.5f;
            theme->drop_site_overlay        = overlay1;
            theme->drop_site_overlay.a      = 0.5f;
            theme->inactive_panel_overlay   = crust;
            theme->inactive_panel_overlay.a = 0.2f;
            theme->drop_shadow              = v4f32(0.0f, 0.0f, 0.0f, 0.8f);

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->title_bar_background    = surface1;
            theme->title_bar_border        = surface2;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface1;
            theme->secondary_button_background = surface1;
            theme->secondary_button_border     = surface1;

            theme->outline   = surface2;
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
            theme->weak_text = subtext0;
            theme->hover = overlay1;
            theme->cursor = rosewater;
            theme->selection = overlay2;
            theme->selection.a = 0.3f;
            theme->focus = lavender;

            theme->disabled_overlay         = overlay0;
            theme->disabled_overlay.a       = 0.5f;
            theme->drop_site_overlay        = overlay1;
            theme->drop_site_overlay.a      = 0.5f;
            theme->inactive_panel_overlay   = crust;
            theme->inactive_panel_overlay.a = 0.5f;
            theme->drop_shadow              = v4f32(0.0f, 0.0f, 0.0f, 0.8f);

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->title_bar_background    = surface1;
            theme->title_bar_border        = surface2;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface1;
            theme->secondary_button_background = surface1;
            theme->secondary_button_border     = surface1;

            theme->outline   = surface2;
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
            theme->weak_text = subtext0;
            theme->hover = overlay1;
            theme->cursor = rosewater;
            theme->selection = overlay2;
            theme->selection.a = 0.3f;
            theme->focus = lavender;

            theme->disabled_overlay         = overlay0;
            theme->disabled_overlay.a       = 0.5f;
            theme->drop_site_overlay        = overlay1;
            theme->drop_site_overlay.a      = 0.5f;
            theme->inactive_panel_overlay   = crust;
            theme->inactive_panel_overlay.a = 0.5f;
            theme->drop_shadow              = v4f32(0.0f, 0.0f, 0.0f, 0.8f);

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->title_bar_background    = surface1;
            theme->title_bar_border        = surface2;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface1;
            theme->secondary_button_background = surface1;
            theme->secondary_button_border     = surface1;

            theme->outline   = surface2;
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
            theme->weak_text = subtext0;
            theme->hover = overlay1;
            theme->cursor = rosewater;
            theme->selection = overlay2;
            theme->selection.a = 0.3f;
            theme->focus = lavender;

            theme->disabled_overlay         = overlay0;
            theme->disabled_overlay.a       = 0.5f;
            theme->drop_site_overlay        = overlay1;
            theme->drop_site_overlay.a      = 0.5f;
            theme->inactive_panel_overlay   = crust;
            theme->inactive_panel_overlay.a = 0.5f;
            theme->drop_shadow              = v4f32(0.0f, 0.0f, 0.0f, 0.8f);

            theme->base_background         = base;
            theme->base_border             = mantle;
            theme->title_bar_background    = surface1;
            theme->title_bar_border        = surface2;
            theme->tab_background          = surface2;
            theme->tab_border              = surface2;
            theme->inactive_tab_background = surface1;
            theme->inactive_tab_border     = surface1;
            theme->button_background       = surface0;
            theme->button_border           = surface1;
            theme->secondary_button_background = surface1;
            theme->secondary_button_border     = surface1;

            theme->outline   = surface2;
            theme->on_curve  = green;
            theme->off_curve = red;
        }
    }

    state->running = true;

    state->theme_index = 3;
    state->theme = global_themes[state->theme_index];
    state->target_theme = global_themes[state->theme_index];
    state->font_size = 11.0f;

    state->frames_to_render = 4;

    state->ttf_arena = arena_create();
    state->ttf_font = &ttf_font_nil;

    gfx_create(str8_literal("MSDF-gen"), 1280, 720);
    render_init();
    render_create();
    font_cache_create();
    gfx_set_update_function(update);
    msdf_cache_create(32, gfx_send_wakeup_event);

    // NOTE(simon): Load config
    // TODO(simon): Replace this with a proper structured text format
    {
        Arena_Temporary scratch = arena_get_scratch(0, 0);
        Str8 config = { 0 };
        os_file_read(scratch.arena, str8_literal("msdf.config"), &config);
        Str8List lines = str8_split_by_codepoints(scratch.arena, config, str8_literal("\n"));
        for (Str8Node *line = lines.first; line; line = line->next) {
            U64 colon_index = str8_first_index_of(line->string, ':');
            Str8 property   = str8_prefix(line->string, colon_index);
            Str8 value      = str8_skip(line->string, colon_index + 1);
            while (value.size && *value.data == ' ') {
                value = str8_skip(value, 1);
            }

            if (str8_equal(property, str8_literal("codepoint"))) {
                U64Decode decode = u64_from_str8(value);
                state->selected_codepoint = (U32) decode.value;
                if (decode.size == 0) {
                    os_console_print(str8_format(scratch.arena, "Could not parse codepoint '%.*s'\n", str8_expand(value)));
                }
            } else {
                os_console_print(str8_format(scratch.arena, "Unknown property '%.*s'\n", str8_expand(property)));
            }
        }
        arena_end_temporary(scratch);
    }

    state->ui = ui_create();
    state->panel_root = panel_create(state);
    state->panel_root->percentage_of_parent = 1.0f;
    state->panel_root->split_axis = Axis2_X;
    {
        Panel *left = panel_create(state);
        push_command(Command_OpenTab, .panel = handle_from_panel(left), .tab_specification = str8_literal("Theme"));
        push_command(Command_OpenTab, .panel = handle_from_panel(left), .tab_specification = str8_literal("GlyphList"));

        Panel *right = panel_create(state);
        push_command(Command_OpenTab, .panel = handle_from_panel(right), .tab_specification = str8_literal("GlyphView"));

        left->percentage_of_parent = 0.65f;
        right->percentage_of_parent = 0.35f;
        panel_insert(state->panel_root, 0, left);
        panel_insert(state->panel_root, left, right);

        state->active_panel = handle_from_panel(left);
    }

    if (arguments.first->next) {
        push_command(Command_LoadFont, .path = arguments.first->next->string);
    }

    while (state->running) {
        update();
        msdf_cache_update();
    }

    return 0;
}
