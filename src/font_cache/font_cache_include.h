#ifndef FONT_CACHE_INCLUDE_H
#define FONT_CACHE_INCLUDE_H

typedef struct FontCache_Region FontCache_Region;
struct FontCache_Region {
    FontCache_Region *parent;
    FontCache_Region *children[4];
    V2U32             max_availible_size;
    B32               occupied;
    U32               occupied_children;
};

typedef struct FontCache_Atlas FontCache_Atlas;
struct FontCache_Atlas {
    FontCache_Atlas *next;
    FontCache_Atlas *previous;

    Render_Texture texture;

    FontCache_Region *root;
    V2U32             root_size;
};

typedef struct FontCache_Font FontCache_Font;
struct FontCache_Font {
    FontCache_Font *hash_next;
    FontCache_Font *hash_previous;

    U64          hash;
    Font_Raster *font;
    F32          ascent;
    F32          descent;
    F32          units_per_em;
};

typedef struct FontCache_FontList FontCache_FontList;
struct FontCache_FontList {
    FontCache_Font *first;
    FontCache_Font *last;
};

typedef struct FontCache_Glyph FontCache_Glyph;
struct FontCache_Glyph {
    FontCache_Glyph *hash_next;
    FontCache_Glyph *hash_previous;

    // NOTE(simon): Lookup information
    U64 hash;
    FontCache_Font *font;
    U32 codepoint;
    U32 point_size;

    R2U32 region;
    R2F32 source;
    Render_Texture texture;

    V2F32 offset;
    V2F32 size;
    F32   advance_width;
};

typedef struct FontCache_GlyphList FontCache_GlyphList;
struct FontCache_GlyphList {
    FontCache_Glyph *first;
    FontCache_Glyph *last;
};

typedef struct FontCache_State FontCache_State;
struct FontCache_State {
    Arena *arena;

    FontCache_Atlas *first_atlas;
    FontCache_Atlas *last_atlas;

    FontCache_FontList *font_table;
    U32                 font_table_size;

    FontCache_GlyphList *glyph_table;
    U32                  glyph_table_size;
};

typedef struct FontCache_Letter FontCache_Letter;
struct FontCache_Letter {
    Render_Texture texture;
    V2F32          offset;
    V2F32          size;
    R2F32          source;
    F32            advance;
    U64            decode_size;
};

typedef struct FontCache_Text FontCache_Text;
struct FontCache_Text {
    FontCache_Letter *letters;
    U64               letter_count;

    V2F32 size;
    F32   ascent;
    F32   descent;
};

// NOTE(simon): Atlas manipulation
internal FontCache_Atlas *font_cache_atlas_create(Arena *arena, V2U32 size);
internal R2U32            font_cache_atlas_allocate(Arena *arena, FontCache_Atlas *atlas, V2U32 minimum_size);
internal Void             font_cache_atlas_free(FontCache_Atlas *atlas, R2U32 size);

// NOTE(simon): Cache lookups
internal FontCache_Font  *font_cache_font_from_path(Str8 path);

// NOTE(simon): Substring on layed out text.
internal FontCache_Text font_cache_text_prefix(FontCache_Text text, U64 size);

// NOTE(simon): Layout
internal FontCache_Text font_cache_text(Arena *arena, FontCache_Font *font, Str8 text, U32 size);

// NOTE(simon): Measuring
internal V2F32 font_cache_size_from_font_text_size(FontCache_Font *font, Str8 text, U32 size);
internal U64   font_cache_offset_from_text_position(FontCache_Text text, F32 position);
internal U64   font_cache_offset_from_font_text_size_position(FontCache_Font *font, Str8 text, U32 size, F32 position);

internal Void font_cache_create(Void);

#endif // FONT_CACHE_INCLUDE_H
