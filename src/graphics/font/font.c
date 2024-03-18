internal AtlasAllocation atlas_allocate(Arena *arena, AtlasAllocator *allocator, U32 min_width, U32 min_height) {
    AtlasAllocation result = { 0 };

    if (min_width != 0 && min_height != 0) {
        if (allocator->width == 0 || allocator->height == 0) {
            allocator->width  = u32_ceil_to_power_of_2(min_width);
            allocator->height = u32_ceil_to_power_of_2(min_height);
        }

        AtlasRow *selected_row = 0;

        for (AtlasRow *row = allocator->rows_first; row; row = row->next) {
            // TODO: First fit or best fit?
            if (min_width <= row->width_left && min_height <= row->height) {
                selected_row = row;
                break;
            }
        }

        if (!selected_row) {
            U32 allocated_height = u32_ceil_to_power_of_2(min_height);
            while (min_width > allocator->width - allocator->x || allocated_height > allocator->height - allocator->y) {
                if (allocator->width < allocator->height) {
                    allocator->x      = allocator->width;
                    allocator->y      = 0;
                    allocator->width *= 2;
                } else {
                    allocator->x       = 0;
                    allocator->y       = allocator->height;
                    allocator->height *= 2;
                }

                if (allocator->rows_first) {
                    allocator->rows_last->next = allocator->free_list;
                    allocator->free_list = allocator->rows_first;
                    allocator->rows_first = 0;
                    allocator->rows_last = 0;
                }
            }

            if (allocator->free_list) {
                selected_row = allocator->free_list;
                sll_stack_pop(allocator->free_list);
            } else {
                selected_row = arena_push_struct(arena, AtlasRow);
            }

            sll_queue_push(allocator->rows_first, allocator->rows_last, selected_row);
            selected_row->x          = allocator->x;
            selected_row->y          = allocator->y;
            selected_row->width_left = allocator->width - allocator->x;
            selected_row->height     = allocated_height;

            allocator->y += selected_row->height;
        }

        result.x      = selected_row->x;
        result.y      = selected_row->y;
        result.width  = min_width;
        result.height = selected_row->height;

        selected_row->x          += min_width;
        selected_row->width_left -= min_width;
    }

    return result;
}

// Shar's algorithm adapted from https://probablydance.com/2023/04/27/beautiful-branchless-binary-search/#more-11376
internal U32 font_codepoint_to_glyph_index(Font *font, U32 codepoint) {
    if (font->codepoint_count == 0) {
        return 0;
    }

    U32 index = 0;
    U32 step = u32_floor_to_power_of_2(font->codepoint_count);

    if (step != font->codepoint_count && font->codepoints[index + step] < codepoint) {
        if (font->codepoint_count == step + 1) {
            return 0;
        }
        step = u32_floor_to_power_of_2(font->codepoint_count - step - 1);
        index = font->codepoint_count - step;
    }

    for (step /= 2; step != 0; step /= 2) {
        if (font->codepoints[index + step] < codepoint) {
            index += step;
        }
    }

    index += (font->codepoints[index] < codepoint);

    if (font->codepoints[index] == codepoint) {
        return font->glyph_indicies[index];
    } else {
        return 0;
    }
}
