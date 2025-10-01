// TODO(simon): Introduce a freelist for FontCache_Region, that way they can be
// reused instead of overallocating them.
// TODO(simon): Cleanup

global FontCache_State global_font_cache_state;
global FontCache_Font  global_font_cache_null_font;

internal FontCache_Atlas *font_cache_atlas_create(Arena *arena, V2U32 size) {
    FontCache_Atlas *atlas = arena_push_struct(arena, FontCache_Atlas);

    V2U32 ceiled_size = v2u32(
            u32_ceil_to_power_of_2(size.width),
            u32_ceil_to_power_of_2(size.height)
    );
    atlas->texture   = render_texture_create(ceiled_size, Render_TextureFormat_R8, 0);
    atlas->root      = arena_push_struct(arena, FontCache_Region);
    atlas->root_size = ceiled_size;

    return atlas;
}

internal R2U32 font_cache_atlas_allocate(Arena *arena, FontCache_Atlas *atlas, V2U32 minimum_size) {
    V2U32 position = v2u32(0, 0);
    V2U32 size     = v2u32(0, 0);

    FontCache_Region *selected_region = 0;
    V2U32 region_size = atlas->root_size;
    if (minimum_size.width && minimum_size.height) {
        for (FontCache_Region *region = atlas->root, *next = 0; region; region = next, next = 0) {
            if (region->occupied) {
                break;
            }

            // NOTE(simon): Find smallest child that has enough space.
            V2U32 next_size = v2u32(region_size.width / 2, region_size.height / 2);
            FontCache_Region *best_child = 0;
            if (next_size.width >= minimum_size.width && next_size.height >= minimum_size.height) {
                U32 best_corner = 4;
                for (U32 corner = 0; corner < 4; ++corner) {
                    if (!region->children[corner]) {
                        region->children[corner]                     = arena_push_struct(arena, FontCache_Region);
                        region->children[corner]->parent             = region;
                        region->children[corner]->max_availible_size = next_size;
                    }

                    V2U32 new_corner_size = region->children[corner]->max_availible_size;
                    if (new_corner_size.x >= minimum_size.x && new_corner_size.y >= minimum_size.y) {
                        // NOTE(simon): Compare against the best match if we have any.
                        if (best_corner != 4) {
                            V2U32 best_corner_size = region->children[corner]->max_availible_size;
                            if (new_corner_size.x < best_corner_size.x && new_corner_size.y < best_corner_size.y) {
                                best_corner = corner;
                            }
                        } else {
                            best_corner = corner;
                        }
                    }
                }

                if (best_corner != 4) {
                    best_child = region->children[best_corner];
                    position.x += (best_corner & 0x01 ? next_size.x : 0);
                    position.y += (best_corner & 0x02 ? next_size.y : 0);
                }
            }

            // NOTE(simon): Use this node if we cannot find a child node and we
            // fit. Otherwise, investigate the best child node.
            B32 can_be_used = (region->occupied_children == 0);
            if (can_be_used && !best_child) {
                size = region_size;
                selected_region = region;
            } else {
                next = best_child;
                region_size = next_size;
            }
        }
    }

    if (selected_region) {
        selected_region->occupied           = true;
        selected_region->max_availible_size = v2u32(0, 0);

        // NOTE(simon): Update all parents.
        for (FontCache_Region *parent = selected_region->parent; parent; parent = parent->parent) {
            ++parent->occupied_children;
            parent->max_availible_size.x = u32_max(
                u32_max(parent->children[0]->max_availible_size.x, parent->children[1]->max_availible_size.x),
                u32_max(parent->children[2]->max_availible_size.x, parent->children[3]->max_availible_size.x)
            );
            parent->max_availible_size.y = u32_max(
                u32_max(parent->children[0]->max_availible_size.y, parent->children[1]->max_availible_size.y),
                u32_max(parent->children[2]->max_availible_size.y, parent->children[3]->max_availible_size.y)
            );
        }
    }

    R2U32 result = r2u32_from_position_size(position, size);
    return result;
}

internal Void font_cache_atlas_free(FontCache_Atlas *atlas, R2U32 rectangle) {
    // NOTE(simon): Find region corresponding to rectangle.
    FontCache_Region *selected_region = 0;
    V2U32 position    = v2u32(0, 0);
    V2U32 region_size = atlas->root_size;

    if (region_size.width && region_size.height) {
        for (FontCache_Region *region = atlas->root, *next = 0; region; region = next, next = 0) {
            V2U32 next_size = v2u32(region_size.width / 2, region_size.height / 2);

            FontCache_Region *best_child = 0;
            for (U32 corner = 0; corner < 4; ++corner) {
                V2U32 child_position = v2u32(
                    position.x + (corner & 0x01 ? next_size.x : 0),
                    position.y + (corner & 0x02 ? next_size.y : 0)
                );
                R2U32 child_rectangle = r2u32_from_position_size(child_position, next_size);

                // NOTE(simon): Only one of the children can contain the rectangle
                // as the children don't overlap.
                if (r2u32_contains_r2u32(child_rectangle, rectangle)) {
                    position = child_position;
                    best_child = region->children[corner];
                    break;
                }
            }

            // NOTE(simon): Either recurse into the best child or this is the node
            // to free.
            if (best_child) {
                next = best_child;
                region_size = next_size;
            } else {
                selected_region = region;
            }
        }
    }

    if (selected_region && selected_region->occupied) {
        selected_region->occupied           = false;
        selected_region->max_availible_size = region_size;

        // NOTE(simon): Update all parents.
        for (FontCache_Region *parent = selected_region->parent; parent; parent = parent->parent) {
            --parent->occupied_children;
            parent->max_availible_size.x = u32_max(
                u32_max(parent->children[0]->max_availible_size.x, parent->children[1]->max_availible_size.x),
                u32_max(parent->children[2]->max_availible_size.x, parent->children[3]->max_availible_size.x)
            );
            parent->max_availible_size.y = u32_max(
                u32_max(parent->children[0]->max_availible_size.y, parent->children[1]->max_availible_size.y),
                u32_max(parent->children[2]->max_availible_size.y, parent->children[3]->max_availible_size.y)
            );
        }
    }
}



internal FontCache_Font *font_cache_font_from_path(Str8 path) {
    prof_function_begin();
    FontCache_State *state = &global_font_cache_state;

    FontCache_Font *result = 0;

    // NOTE(simon): Lookup the font from the path.
    U64 hash = str8_hash(path);
    FontCache_FontList *fonts = &state->font_table[hash % state->font_table_size];
    for (FontCache_Font *font = fonts->first; font; font = font->hash_next) {
        if (font->hash == hash) {
            result = font;
            break;
        }
    }

    // NOTE(simon): Load the font if it doesn't exist yet.
    if (!result) {
        result = arena_push_struct(state->arena, FontCache_Font);

        result->hash = hash;
        Str8 data = { 0 };
        os_file_read(state->arena, path, &data);
        result->font = raster_load(state->arena, data);

        Font_Metrics metrics = raster_get_font_metrics(result->font);

        result->ascent       = (F32) metrics.ascent;
        result->descent      = (F32) metrics.descent;
        result->units_per_em = (F32) metrics.units_per_em;

        dll_insert_next_previous_zero(fonts->first, fonts->last, fonts->last, result, hash_next, hash_previous, 0);
    }

    prof_function_end();
    return result;
}

internal FontCache_Font *font_cache_font_from_static_data(Str8 *data) {
    prof_function_begin();
    FontCache_State *state = &global_font_cache_state;

    FontCache_Font *result = 0;

    // NOTE(simon): Lookup the font from the path.
    U64 hash = u64_hash(integer_from_pointer(data));
    FontCache_FontList *fonts = &state->font_table[hash % state->font_table_size];
    for (FontCache_Font *font = fonts->first; font; font = font->hash_next) {
        if (font->hash == hash) {
            result = font;
            break;
        }
    }

    // NOTE(simon): Load the font if it doesn't exist yet.
    if (!result) {
        result = arena_push_struct(state->arena, FontCache_Font);

        result->hash = hash;
        result->font = raster_load(state->arena, *data);

        Font_Metrics metrics = raster_get_font_metrics(result->font);

        result->ascent       = (F32) metrics.ascent;
        result->descent      = (F32) metrics.descent;
        result->units_per_em = (F32) metrics.units_per_em;

        dll_insert_next_previous_zero(fonts->first, fonts->last, fonts->last, result, hash_next, hash_previous, 0);
    }

    prof_function_end();
    return result;
}



internal FontCache_Text font_cache_text_prefix(FontCache_Text text, U64 size) {
    prof_function_begin();

    FontCache_Text result = { 0 };
    result.ascent      = text.ascent;
    result.descent     = text.descent;
    result.letters     = text.letters;
    result.size.height = text.size.height;

    U64 total_decode_size = 0;
    while (result.letter_count < text.letter_count && total_decode_size < size) {
        total_decode_size += result.letters[result.letter_count].decode_size;
        result.size.width += result.letters[result.letter_count].advance;
        ++result.letter_count;
    }

    prof_function_end();
    return result;
}



// NOTE(simon): `size` is in pixels per em
internal FontCache_Text font_cache_text(Arena *arena, FontCache_Font *font, Str8 text, U32 size) {
    prof_function_begin();
    FontCache_State *state = &global_font_cache_state;

    FontCache_Text result = { 0 };

    result.letters = arena_push_array(arena, FontCache_Letter, text.size);

    result.ascent  = f32_ceil(font->ascent  * (F32) size / font->units_per_em);
    result.descent = f32_ceil(font->descent * (F32) size / font->units_per_em);
    result.size.height = f32_ceil(result.ascent - result.descent);

    U64 font_hash = hash_combine(font->hash, u64_hash(size));
    U8 *ptr = text.data;
    U8 *opl = text.data + text.size;
    while (ptr < opl) {
        StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
        ptr += decode.size;

        FontCache_Glyph *glyph = 0;
        // NOTE(simon): Lookup the glyph from the font and codepoint.
        U64 hash = hash_combine(font_hash, u64_hash(decode.codepoint));
        FontCache_GlyphList *glyphs = &state->glyph_table[hash & (state->glyph_table_size - 1)];
        for (FontCache_Glyph *candidate_glyph = glyphs->first; candidate_glyph; candidate_glyph = candidate_glyph->hash_next) {
            if (candidate_glyph->font == font && candidate_glyph->codepoint == decode.codepoint && candidate_glyph->point_size == size) {
                glyph = candidate_glyph;
                break;
            }
        }

        // NOTE(simon): Generate the glyph if it doesn't exist yet.
        if (!glyph) {
            Arena_Temporary scratch = arena_get_scratch(&state->arena, 1);
            MSDF_RasterResult raster_result = raster_generate(scratch.arena, font->font, decode.codepoint, size);

            // NOTE(simon): Select glyph atlas.
            FontCache_Atlas *selected_atlas = 0;
            for (FontCache_Atlas *atlas = state->first_atlas; atlas; atlas = atlas->next ) {
                V2U32 max_availible_size = atlas->root->max_availible_size;
                if (max_availible_size.x >= raster_result.size.x && max_availible_size.y >= raster_result.size.y) {
                    selected_atlas = atlas;
                    break;
                }
            }

            // NOTE(simon): Allocate a new atlas if we couldn't find one with enough space.
            if (!selected_atlas) {
                V2U32 default_size = v2u32(1024, 1024);
                selected_atlas = font_cache_atlas_create(state->arena, default_size);
                dll_push_back(state->first_atlas, state->last_atlas, selected_atlas);
            }

            // NOTE(simon): Create glyph.
            V2U32 atlas_size = render_size_from_texture(selected_atlas->texture);
            glyph = arena_push_struct(state->arena, FontCache_Glyph);
            glyph->font       = font;
            glyph->codepoint  = decode.codepoint;
            glyph->point_size = size;
            glyph->region     = font_cache_atlas_allocate(state->arena, selected_atlas, raster_result.size);
            glyph->source = r2f32(
                (F32) glyph->region.min.x,
                (F32) glyph->region.min.y,
                (F32) (glyph->region.min.x + raster_result.size.x),
                (F32) (glyph->region.min.y + raster_result.size.y)
            );
            glyph->texture       = selected_atlas->texture;
            glyph->offset        = v2f32(raster_result.left_side_bearing, (F32) raster_result.min.y);
            glyph->size          = v2f32((F32) raster_result.size.width, (F32) raster_result.size.height);
            glyph->advance_width = raster_result.advance_width;

            // NOTE(simon): Insert into atlas.
            render_texture_update(glyph->texture, glyph->region.min, raster_result.size, raster_result.data);

            dll_insert_next_previous_zero(glyphs->first, glyphs->last, glyphs->last, glyph, hash_next, hash_previous, 0);
            arena_end_temporary(scratch);
        }

        FontCache_Letter *letter = &result.letters[result.letter_count++];
        letter->texture     = glyph->texture;
        letter->offset      = glyph->offset;
        letter->size        = glyph->size;
        letter->source      = glyph->source;
        letter->advance     = glyph->advance_width;
        letter->decode_size = decode.size;

        result.size.width += glyph->advance_width;
    }

    arena_pop_amount(arena, (text.size - result.letter_count) * sizeof(FontCache_Letter));

    prof_function_end();
    return result;
}



// NOTE(simon): Measuring
internal V2F32 font_cache_size_from_font_text_size(FontCache_Font *font, Str8 text, U32 size) {
    prof_function_begin();
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    FontCache_Text text_run = font_cache_text(scratch.arena, font, text, size);
    V2F32 result = text_run.size;
    arena_end_temporary(scratch);
    prof_function_end();
    return result;
}

internal U64 font_cache_offset_from_text_position(FontCache_Text text, F32 position) {
    prof_function_begin();

    F32 best_pixel_diff  = f32_infinity();
    U64 best_byte_offset = 0;
    F32 pixel_offset     = 0.0f;
    U64 byte_offset      = 0;
    for (U64 i = 0; i <= text.letter_count; ++i) {
        F32 pixel_diff = f32_abs(position - pixel_offset);
        if (pixel_diff <= best_pixel_diff) {
            best_pixel_diff  = pixel_diff;
            best_byte_offset = byte_offset;
        }

        if (i < text.letter_count) {
            pixel_offset += text.letters[i].advance;
            byte_offset  += text.letters[i].decode_size;
        }
    }

    prof_function_end();
    return best_byte_offset;
}

internal U64 font_cache_offset_from_font_text_size_position(FontCache_Font *font, Str8 text, U32 size, F32 position) {
    prof_function_begin();
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    FontCache_Text text_run = font_cache_text(scratch.arena, font, text, size);
    U64 result = font_cache_offset_from_text_position(text_run, position);
    arena_end_temporary(scratch);
    prof_function_end();
    return result;
}



internal Void font_cache_create(Void) {
    FontCache_State *result = &global_font_cache_state;

    Arena *arena = arena_create();
    result->arena = arena;
    result->font_table_size = 32;
    result->font_table      = arena_push_array(result->arena, FontCache_FontList, result->font_table_size);
    result->glyph_table_size = 4096;
    result->glyph_table      = arena_push_array(result->arena, FontCache_GlyphList, result->glyph_table_size);
}
