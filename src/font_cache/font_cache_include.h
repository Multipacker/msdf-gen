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
    Render_Texture texture;

    FontCache_Region *root;
    V2U32             root_size;
};

// NOTE(simon): Atlas manipulation
internal FontCache_Atlas *font_cache_atlas_create(Arena *arena, Render_Context *render, V2U32 size);
internal R2U32            font_cache_atlas_allocate(Arena *arena, FontCache_Atlas *atlas, V2U32 minimum_size);
internal Void             font_cache_atlas_free(FontCache_Atlas *atlas, R2U32 size);

#endif // FONT_CACHE_INCLUDE_H
