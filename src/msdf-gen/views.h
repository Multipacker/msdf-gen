#ifndef VIEWS_H
#define VIEWS_H

PANEL_BUILD_FUNCTION(view_glyph_list);
PANEL_BUILD_FUNCTION(view_glyph);
PANEL_BUILD_FUNCTION(view_stats);
PANEL_BUILD_FUNCTION(view_theme);

#define TABS \
    X(Null,        "",                  0)               \
    X(GlyphList,   "Glyph list",        view_glyph_list) \
    X(GlyphView,   "Glyph view",        view_glyph)      \
    X(RenderStats, "Render statistics", view_stats)      \
    X(Theme,       "Theme",             view_theme)

#define X(name, ...) Tab_##name,
typedef enum {
    TABS
    Tab_COUNT,
} TabKind;
#undef X

#define X(name, display_name, build) { str8_literal_compile(#name), str8_literal_compile(display_name), build, },
global TabSpecification tab_specifications[] = {
    TABS
};
#undef X

TabKind tab_kind_from_string(Str8 string);
TabSpecification *tab_specification_from_string(Str8 string);

#endif // VIEWS_H
