#ifndef VIEWS_H
#define VIEWS_H

#define TABS \
    X(Null,        "",                  view_null)       \
    X(GlyphList,   "Glyph list",        view_glyph_list) \
    X(GlyphView,   "Glyph view",        view_glyph)      \
    X(RenderStats, "Render statistics", view_stats)      \
    X(Theme,       "Theme",             view_theme)      \
    X(Test,        "Test",              view_test)

#define X(name, ...) Tab_##name,
typedef enum {
    TABS
    Tab_COUNT,
} TabKind;
#undef X

#define X(name, display_name, build) PANEL_BUILD_FUNCTION(build);
TABS
#undef X

#define X(name, display_name, build) { str8_literal_compile(#name), str8_literal_compile(display_name), build, },
global TabSpecification tab_specifications[] = {
    TABS
};
#undef X

TabKind tab_kind_from_string(Str8 string);
TabSpecification *tab_specification_from_string(Str8 string);

#endif // VIEWS_H
