// TODO(simon): Introduce a freelist for FontCache_Region, that way they can be
// reused instead of overallocating them.

internal FontCache_Atlas *font_cache_atlas_create(Arena *arena, Render_Context *render, V2U32 size) {
    FontCache_Atlas *atlas = arena_push_struct_zero(arena, FontCache_Atlas);

    V2U32 ceiled_size = v2u32(
            u32_ceil_to_power_of_2(size.width),
            u32_ceil_to_power_of_2(size.height)
    );
    atlas->texture   = render_texture_create(render, ceiled_size, Render_TextureFormat_R8, 0);
    atlas->root      = arena_push_struct_zero(arena, FontCache_Region);
    atlas->root_size = ceiled_size;

    return atlas;
}

internal R2U32 font_cache_atlas_allocate(Arena *arena, FontCache_Atlas *atlas, V2U32 minimum_size) {
    V2U32 position = v2u32(0, 0);
    V2U32 size     = v2u32(0, 0);

    FontCache_Region *selected_region = 0;
    V2U32 region_size = atlas->root_size;
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
                    region->children[corner]                     = arena_push_struct_zero(arena, FontCache_Region);
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
