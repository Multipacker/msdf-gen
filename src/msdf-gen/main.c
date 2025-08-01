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
 * * Combine preview and glyph view into one inspector
 */

/*
 * FIXME:
 * * Tabbars are not clipped to the panels section of the screen, so they can
 *   overlap other panels.
 */

#include "core.h"

#include "core.c"
#include "views.c"

internal Str8 trim_whitespace(Str8 string) {
    U8 *ptr = string.data;
    U8 *opl = string.data + string.size;

    while (ptr < opl && (ptr[0] == ' ' || ptr[0] == '\t')) {
        ++ptr;
    }

    while (ptr < opl && (opl[-1] == ' ' || opl[-1] == '\t')) {
        --opl;
    }

    Str8 result = str8_range(ptr, opl);
    return result;
}

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
            theme->name = str8_literal("OpenColor Dark");

            V4F32 gray0 = color_from_srgba_u32(0xf8f9faff);
            V4F32 gray1 = color_from_srgba_u32(0xf1f3f5ff);
            V4F32 gray2 = color_from_srgba_u32(0xe9ecefff);
            V4F32 gray3 = color_from_srgba_u32(0xdee2e6ff);
            V4F32 gray4 = color_from_srgba_u32(0xced4daff);
            V4F32 gray5 = color_from_srgba_u32(0xadb5bdff);
            V4F32 gray6 = color_from_srgba_u32(0x868e96ff);
            V4F32 gray7 = color_from_srgba_u32(0x495057ff);
            V4F32 gray8 = color_from_srgba_u32(0x343a40ff);
            V4F32 gray9 = color_from_srgba_u32(0x212529ff);

            V4F32 red6 = color_from_srgba_u32(0xfa5252ff);

            V4F32 green6 = color_from_srgba_u32(0x40c057ff);

            V4F32 orange4 = color_from_srgba_u32(0xffa94dff);

            theme->text  = gray0;
            theme->weak_text = gray5;
            theme->hover = gray4;
            theme->cursor = gray0;
            theme->selection = gray1;
            theme->selection.a = 0.3f;
            theme->focus = orange4;

            theme->disabled_overlay         = gray6;
            theme->disabled_overlay.a       = 0.5f;
            theme->drop_site_overlay        = gray4;
            theme->drop_site_overlay.a      = 0.5f;
            theme->inactive_panel_overlay   = v4f32(0.0f, 0.0f, 0.0f, 0.8f);
            theme->inactive_panel_overlay.a = 0.5f;
            theme->drop_shadow              = v4f32(0.0f, 0.0f, 0.0f, 0.8f);

            theme->base_background         = gray9;
            theme->base_border             = gray8;
            theme->title_bar_background    = gray8;
            theme->title_bar_border        = gray7;
            theme->tab_background          = gray6;
            theme->tab_border              = gray5;
            theme->inactive_tab_background = gray7;
            theme->inactive_tab_border     = gray6;
            theme->button_background       = gray8;
            theme->button_border           = gray7;
            theme->secondary_button_background = gray7;
            theme->secondary_button_border     = gray6;

            theme->outline   = gray6;
            theme->on_curve  = green6;
            theme->off_curve = red6;
        }

        {
            Theme *theme = &global_themes[1];
            theme->name = str8_literal("OpenColor Light");

            V4F32 gray0 = color_from_srgba_u32(0xf8f9faff);
            V4F32 gray1 = color_from_srgba_u32(0xf1f3f5ff);
            V4F32 gray2 = color_from_srgba_u32(0xe9ecefff);
            V4F32 gray3 = color_from_srgba_u32(0xdee2e6ff);
            V4F32 gray4 = color_from_srgba_u32(0xced4daff);
            V4F32 gray5 = color_from_srgba_u32(0xadb5bdff);
            V4F32 gray6 = color_from_srgba_u32(0x868e96ff);
            V4F32 gray7 = color_from_srgba_u32(0x495057ff);
            V4F32 gray8 = color_from_srgba_u32(0x343a40ff);
            V4F32 gray9 = color_from_srgba_u32(0x212529ff);

            V4F32 red6 = color_from_srgba_u32(0xfa5252ff);

            V4F32 green6 = color_from_srgba_u32(0x40c057ff);

            V4F32 orange6 = color_from_srgba_u32(0xfd7e14ff);

            theme->text  = gray9;
            theme->weak_text = gray7;
            theme->hover = gray6;
            theme->cursor = gray9;
            theme->selection = gray6;
            theme->selection.a = 0.3f;
            theme->focus = orange6;

            theme->disabled_overlay         = gray6;
            theme->disabled_overlay.a       = 0.5f;
            theme->drop_site_overlay        = gray4;
            theme->drop_site_overlay.a      = 0.5f;
            theme->inactive_panel_overlay   = gray6;
            theme->inactive_panel_overlay.a = 0.25f;
            theme->drop_shadow              = v4f32(0.0f, 0.0f, 0.0f, 0.8f);

            theme->base_background         = gray0;
            theme->base_border             = gray1;
            theme->title_bar_background    = gray1;
            theme->title_bar_border        = gray2;
            theme->tab_background          = gray2;
            theme->tab_border              = gray3;
            theme->inactive_tab_background = gray3;
            theme->inactive_tab_border     = gray4;
            theme->button_background       = gray2;
            theme->button_border           = gray3;
            theme->secondary_button_background = gray3;
            theme->secondary_button_border     = gray4;

            theme->outline   = gray6;
            theme->on_curve  = green6;
            theme->off_curve = red6;
        }

        {
            Theme *theme = &global_themes[2];
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

        {
            Theme *theme = &global_themes[3];
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
            Theme *theme = &global_themes[4];
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
            Theme *theme = &global_themes[5];
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
    }

    state->running = true;

    state->theme_index = 0;
    state->theme = global_themes[state->theme_index];
    state->target_theme = global_themes[state->theme_index];
    state->font_size = 11.0f;

    state->frames_to_render = 4;

    state->ttf_arena = arena_create();
    state->ttf_font = &ttf_font_nil;

    gfx_init();
    render_init();

    state->window = gfx_window_create(str8_literal("MSDF-gen"), 1280, 720);
    state->render = render_create(state->window);

    font_cache_create();
    gfx_set_update_function(update);
    msdf_cache_create(32, gfx_send_wakeup_event);

    // NOTE(simon): Load config
    {
        Arena_Temporary scratch = arena_get_scratch(0, 0);

        Str8 config = { 0 };

        Str8 current_directory = os_current_directory(scratch.arena);
        Str8 file_path = str8_format(scratch.arena, "%.*s/msdf.config", str8_expand(current_directory));
        os_file_read(scratch.arena, file_path, &config);

        for (U64 start_of_line = 0; start_of_line < config.size;) {
            U64 end_of_line = str8_first_index_of(str8_skip(config, start_of_line), '\n');
            Str8 line = str8_substring(config, start_of_line, end_of_line - start_of_line);

            U64 colon_index = str8_first_index_of(line, ':');

            Str8 key   = trim_whitespace(str8_prefix(line, colon_index));
            Str8 value = trim_whitespace(str8_skip(line, colon_index + 1));

            if (str8_equal(key, str8_literal("codepoint"))) {
                U64Decode decode = u64_from_str8(value);
                state->codepoint = (U32) decode.value;
            } else {
                gfx_message(
                    true,
                    str8_literal("Could not parse configuration file"),
                    str8_format(scratch.arena, "Unknown config key '%.*s'.", str8_expand(key))
                );
            }

            start_of_line = end_of_line + 1;
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

        Panel *right_top = panel_create(state);
        push_command(Command_OpenTab, .panel = handle_from_panel(right_top), .tab_specification = str8_literal("GlyphView"));

        Panel *right_bottom = panel_create(state);
        push_command(Command_OpenTab, .panel = handle_from_panel(right_bottom), .tab_specification = str8_literal("GlyphDebug"));

        Panel *right = panel_create(state);
        right->split_axis = Axis2_Y;
        right_top->percentage_of_parent = 0.7f;
        right_bottom->percentage_of_parent = 0.3f;
        panel_insert(right, 0, right_top);
        panel_insert(right, right_top, right_bottom);

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
    }

    return 0;
}
