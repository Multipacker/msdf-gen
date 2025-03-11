#include <fontconfig/fontconfig.h>

global Gfx_LinuxState global_gfx_linux_state;

internal Void gfx_linux_init(Void) {
    Gfx_LinuxState *state = &global_gfx_linux_state;
    state->arena = arena_create_reserve(kilobytes(512));

    FcInit();

    // NOTE(simon): Acquire system root.
    FcConfig *config = FcConfigReference(0);
    Str8 sys_root = str8_cstr((CStr) FcConfigGetSysRoot(config));
    FcConfigDestroy(config);

    for (Gfx_Font font = 0; font < Gfx_Font_COUNT; ++font) {
        Arena_Temporary scratch = arena_get_scratch(0, 0);
        // TODO(simon): Properties that could be interesting to look at:
        // * FC_CHARSET
        // * FC_FONTFORMAT
        // * FC_FONT_WRAPPER

        // NOTE(simon): Fill pattern based on the kind of font.
        FcPattern *pattern = FcPatternCreate();
        switch (font) {
            case Gfx_Font_Default: {
            } break;
            case Gfx_Font_Proportional: {
                FcPatternAddInteger(pattern, FC_SPACING, FC_PROPORTIONAL);
            } break;
            case Gfx_Font_Monospace: {
                FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *) "monospace");
                FcPatternAddInteger(pattern, FC_SPACING, FC_MONO);
            } break;
            case Gfx_Font_SansSerif: {
                FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *) "sans serif");
            } break;
            case Gfx_Font_Serif: {
                FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *) "serif");
            } break;
            case Gfx_Font_COUNT: {
            } break;
        }

        // NOTE(simon): Perform user configured substitutions and apply fontconfigs
        // defaults.
        // TODO(simon): FcMatchPattern or FcMatchFont, which one do we use?
        FcConfigSubstitute(0, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);

        FcResult match_result = 0;
        FcPattern *selected_font = FcFontMatch(0, pattern, &match_result);
        if (match_result == FcResultMatch) {
            FcChar8 *sys_relative_path = 0;
            FcResult get_result = FcPatternGetString(selected_font, FC_FILE, 0, &sys_relative_path);
            if (get_result == FcResultMatch) {
                Str8List path_parts = { 0 };
                str8_list_push(scratch.arena, &path_parts, sys_root);
                str8_list_push(scratch.arena, &path_parts, str8_literal("/"));
                str8_list_push(scratch.arena, &path_parts, str8_cstr((CStr) sys_relative_path));
                state->font_paths[font] = str8_join(state->arena, &path_parts);
            }
            FcPatternDestroy(selected_font);
        }

        FcPatternDestroy(pattern);
        arena_end_temporary(scratch);
    }
}

// NOTE(simon): System fonts
internal Str8 gfx_font_path(Arena *arena, Gfx_Font font) {
    Gfx_LinuxState *state = &global_gfx_linux_state;
    Str8 result = str8_copy(arena, state->font_paths[font]);
    return result;
}
