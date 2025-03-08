global MSDFCache_State global_msdf_cache_state;

internal Void msdf_cache_generate_glyphs_thread_entry(Void *data) {
    MSDFCache_State *state = &global_msdf_cache_state;

    for (;;) {
        MSDFCache_Request request = { 0 };
        os_mutex_scope(state->request_mutex) {
            if (state->request_write - state->request_read == 0) {
                os_condition_variable_wait(state->request_condition_variable, state->request_mutex, U64_MAX);
            }
            request = state->request_buffer[state->request_read & (state->request_size - 1)];
            ++state->request_read;
        }

        MSDFCache_Result result = { 0 };
        result.arena  = arena_create();
        result.raster = msdf_generate_from_glyph_index(result.arena, request.font, request.glyph_index, state->glyph_size);

        os_mutex_scope(state->result_mutex) {
            if (state->result_write - state->result_read == state->result_size) {
                os_condition_variable_wait(state->result_condition_variable, state->result_mutex, U64_MAX);
            }

            state->result_buffer[state->result_write & (state->result_size - 1)] = result;
            ++state->result_write;
            if (state->wakeup) {
                state->wakeup();
            }
        }
    }
}

internal Void msdf_cache_create(U32 glyph_size, VoidFunction *wakeup) {
    MSDFCache_State *state = &global_msdf_cache_state;

    state->arena       = arena_create();
    state->glyph_arena = arena_create();
    state->glyph_size  = glyph_size;
    state->wakeup      = wakeup;

    state->request_size   = 128;
    state->request_buffer = arena_push_array(state->arena, MSDFCache_Request, state->request_size);
    state->request_mutex  = os_mutex_create();
    state->request_condition_variable = os_condition_variable_create();

    state->result_size   = 128;
    state->result_buffer = arena_push_array(state->arena, MSDFCache_Result, state->result_size);
    state->result_mutex  = os_mutex_create();
    state->result_condition_variable = os_condition_variable_create();

    OS_Thread thread = os_thread_start(msdf_cache_generate_glyphs_thread_entry, 0);
    os_thread_set_name(thread, str8_literal("MSDF generator"));
    os_thread_detach(thread);
}

internal MSDFCache_Glyph *msdf_cache_get_glyph(TTF_Font *font, U32 codepoint) {
    MSDFCache_State *state = &global_msdf_cache_state;

    Arena_Temporary scratch = arena_get_scratch(0, 0);

    U32 glyph_index = ttf_glyph_index_from_font_codepoint(font, codepoint);
    U64 hash = u64_hash(glyph_index);
    MSDFCache_GlyphList *glyphs = &state->glyph_lists[hash % array_count(state->glyph_lists)];

    MSDFCache_Glyph *result = &global_glyph_null;

    for (MSDFCache_Glyph *glyph = glyphs->first; glyph; glyph = glyph->next) {
        if (glyph->glyph_index == glyph_index) {
            result = glyph;
            break;
        }
    }

    // NOTE(simon): Generate the glyph if it doesn't exist yet.
    if (result == &global_glyph_null) {
        os_mutex_scope(state->request_mutex) {
            // TODO(simon): Try stealing old slots for new entries. This would
            // result in some memory churn, but old and possibly irrelevant
            // (for now) glyphs would not get generated.
            if (state->request_write - state->request_read < state->request_size) {
                result = arena_push_struct_zero(state->glyph_arena, MSDFCache_Glyph);
                result->glyph_index = glyph_index;
                dll_push_back(glyphs->first, glyphs->last, result);

                MSDFCache_Request request = { 0 };
                request.font = font;
                request.glyph_index = glyph_index;

                state->request_buffer[state->request_write & (state->request_size - 1)] = request;
                ++state->request_write;
                os_condition_variable_signal(state->request_condition_variable);
            }
        }
    }

    // NOTE(simon): No glyph while we are loading.
    if (!result->loaded) {
        result = &global_glyph_null;
    }

    arena_end_temporary(scratch);
    return result;
}

internal Void msdf_cache_update(Void) {
    prof_function_begin();

    MSDFCache_State *state = &global_msdf_cache_state;

    os_mutex_scope(state->result_mutex) {
        while (state->result_write - state->result_read) {
            MSDFCache_Result work = state->result_buffer[state->result_read & (state->result_size - 1)];
            ++state->result_read;

            MSDFCache_Glyph *result = &global_glyph_null;

            U64 hash = u64_hash(work.raster.glyph_index);
            MSDFCache_GlyphList *glyphs = &state->glyph_lists[hash % array_count(state->glyph_lists)];

            for (MSDFCache_Glyph *glyph = glyphs->first; glyph; glyph = glyph->next) {
                if (glyph->glyph_index == work.raster.glyph_index) {
                    result = glyph;
                    break;
                }
            }

            if (result && !result->loaded) {
                // NOTE(simon): Select glyph atlas.
                MSDFCache_Atlas *selected_atlas = 0;
                for (MSDFCache_Atlas *atlas = state->first_atlas; atlas && !selected_atlas; atlas = atlas->next) {
                    for (U64 i = 0; i < array_count(atlas->occupancy); ++i) {
                        if (~atlas->occupancy[i] != 0) {
                            selected_atlas = atlas;
                            break;
                        }
                    }
                }

                // NOTE(simon): Allocate a new atlas if we couldn't find one with space in it.
                if (!selected_atlas) {
                    V2U32 size = v2u32(state->glyph_size * ATLAS_GLYPHS_PER_SIDE, state->glyph_size * ATLAS_GLYPHS_PER_SIDE);
                    selected_atlas = arena_push_struct_zero(state->glyph_arena, MSDFCache_Atlas);
                    selected_atlas->texture = render_texture_create(size, Render_TextureFormat_RGBA8, 0);
                    dll_push_back(state->first_atlas, state->last_atlas, selected_atlas);
                }

                // NOTE(simon): Allocate atlas region.
                U32 glyph_location = 0;
                while (~selected_atlas->occupancy[glyph_location / 64] == 0) {
                    glyph_location += 64;
                }
                while ((selected_atlas->occupancy[glyph_location / 64] & (U64) ((U64) 1 << glyph_location % 64)) != 0) {
                    ++glyph_location;
                }
                selected_atlas->occupancy[glyph_location / 64] |= ((U64) 1 << glyph_location % 64);

                V2U32 atlas_position = v2u32(
                    state->glyph_size * (glyph_location % ATLAS_GLYPHS_PER_SIDE),
                    state->glyph_size * (glyph_location / ATLAS_GLYPHS_PER_SIDE)
                );

                render_texture_update(
                    selected_atlas->texture,
                    atlas_position,
                    v2u32(state->glyph_size, state->glyph_size),
                    work.raster.data
                );

                // This adjustment increases the size of glyphs to acount for the
                // source needing to include a 1/2 texel border for rendering. This
                // makes sure that the glyphs have the same visual size.
                F32 scale = ((F32) state->glyph_size - 1.0f) / ((F32) state->glyph_size - 2.0f) - 1.0f;
                V2F32 adjustment = v2f32_scale(v2f32_subtract(work.raster.max, work.raster.min), 0.5f * scale);

                result->advance_pt = work.raster.advance_width;
                result->rectangle_pt = r2f32(
                    work.raster.min.x - adjustment.x,
                    work.raster.min.y - adjustment.y,
                    work.raster.max.x + adjustment.x,
                    work.raster.max.y + adjustment.y
                );

                result->uv = r2f32(
                    (F32) atlas_position.x + 0.5f,
                    (F32) atlas_position.y + 0.5f,
                    (F32) atlas_position.x + (F32) state->glyph_size - 0.5f,
                    (F32) atlas_position.y + (F32) state->glyph_size - 0.5f
                );
                result->texture = selected_atlas->texture;
                for (MSDF_LogEntry *src_entry = work.raster.log.first; src_entry; src_entry = src_entry->next) {
                    MSDF_LogEntry *entry = arena_push_struct_zero(state->glyph_arena, MSDF_LogEntry);
                    entry->description = str8_copy(state->arena, src_entry->description);
                    for (MSDF_LogGroup *src_group = src_entry->first_group; src_group; src_group = src_group->next) {
                        MSDF_LogGroup *group = arena_push_struct_zero(state->glyph_arena, MSDF_LogGroup);
                        for (MSDF_LogGeometry *src_geometry = src_group->first_geometry; src_geometry; src_geometry = src_geometry->next) {
                            MSDF_LogGeometry *geometry = arena_push_struct_zero(state->glyph_arena, MSDF_LogGeometry);
                            memory_copy(geometry, src_geometry, sizeof(*geometry));
                            dll_push_back(group->first_geometry, group->last_geometry, geometry);
                        }
                        dll_push_back(entry->first_group, entry->last_group, group);
                        ++entry->group_count;
                    }
                    dll_push_back(result->log.first, result->log.last, entry);
                    ++result->log.count;
                }
                result->loaded = true;
            }

            arena_destroy(work.arena);
        }
        os_condition_variable_signal(state->result_condition_variable);
    }

    prof_function_end();
}

// NOTE(simon): Ideally we would not need this and the cache would be LRU like
// so that it replaces old entries instead.
internal Void msdf_cache_clear(Void) {
    MSDFCache_State *state = &global_msdf_cache_state;

    os_mutex_scope(state->request_mutex) {
        state->request_write = 0;
        state->request_read  = 0;
    }

    // TODO(simon): Wait for current request to finish.

    os_mutex_scope(state->result_mutex) {
        state->result_write = 0;
        state->result_read  = 0;
    }

    for (MSDFCache_Atlas *atlas = state->first_atlas; atlas; atlas = atlas->next) {
        render_texture_destroy(atlas->texture);
    }

    state->first_atlas = 0;
    state->last_atlas  = 0;
    memory_zero(state->glyph_lists, sizeof(state->glyph_lists));

    arena_reset(state->glyph_arena);
}
