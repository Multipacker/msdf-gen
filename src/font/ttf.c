internal F32 ttf_f2dot14_to_f32(TTF_F2Dot14 x) {
    F32 result = (F32) (x & 0x7FFF) / 16384.0f;

    if (x & 0x8000) {
        result -= 2.0f;
    }

    return result;
}

internal B32 ttf_parse_font_tables(Str8 data, TTF_Font *font) {
    B32 success = true;

    U32 table_count = 0;
    if (data.size >= sizeof(TTF_OffsetSubtable)) {
        TTF_OffsetSubtable *offset_subtable = (TTF_OffsetSubtable *) &data.data[0];

        U32 scaler_type = u32_big_to_local_endian(offset_subtable->scaler_type);
        if (scaler_type == TTF_SCALER_TYPE_TRUE || scaler_type == TTF_SCALER_TYPE_1) {
            table_count = u16_big_to_local_endian(offset_subtable->num_tables);
        } else {
            error_emit(str8_literal("ERROR(font/ttf): Unknown scaler type."));
            success = false;
        }
    } else {
        error_emit(str8_literal("ERROR(font/ttf): Not enough data for offset subtable."));
        success = false;
    }

    TTF_TableDirectoryEntry *table_directory_entries = 0;
    if (success) {
        Str8 table_directory_data = str8_skip(data, sizeof(TTF_OffsetSubtable));
        if (table_directory_data.size >= table_count * sizeof(TTF_TableDirectoryEntry)) {
            table_directory_entries = (TTF_TableDirectoryEntry *) table_directory_data.data;
        } else {
            error_emit(str8_literal("ERROR(font/ttf): Not enough data for offset subtable."));
            success = false;
        }
    }

    if (success) {
        for (U32 i = 0; i < table_count && success; ++i) {
            TTF_TableDirectoryEntry *entry = &table_directory_entries[i];
            U32 tag       = u32_big_to_local_endian(entry->tag);
            U32 check_sum = u32_big_to_local_endian(entry->check_sum);
            U32 offset    = u32_big_to_local_endian(entry->offset);
            U32 length    = u32_big_to_local_endian(entry->length);

            if (offset + length <= data.size) {
                Str8 table_data = str8_substring(data, offset, length);

                // TODO: Verify the check sum.

                for (U32 i = 0; i < TTF_Table_COUNT && success; ++i) {
                    if (tag == ttf_table_tags[i]) {
                        if (!font->tables[i].data) {
                            font->tables[i] = table_data;
                        } else {
                            error_emit(str8_literal("ERROR(font/ttf): Duplicated entry in the table directory."));
                            success = false;
                        }
                    }
                }
            } else {
                error_emit(str8_literal("ERROR(font/ttf): Table is referencing data outside of the file."));
                success = false;
            }
        }
    }

    if (success) {
        for (U32 i = 0; i < TTF_Table_MaxRequired && success; ++i) {
            if (!font->tables[i].data) {
                error_emit(str8_literal("ERROR(font/ttf): Not all required tables are present."));
                success = false;
            }
        }
    }

    return success;
}

internal B32 ttf_parse_head_table(TTF_Font *font) {
    B32 success = true;

    if (font->tables[TTF_Table_Head].size >= sizeof(TTF_HeadTable)) {
        TTF_HeadTable *head = (TTF_HeadTable *) font->tables[TTF_Table_Head].data;

        TTF_Fixed version             = u32_big_to_local_endian(head->version);
        U32       magic_number        = u32_big_to_local_endian(head->magic_number);
        U16       flags               = u16_big_to_local_endian(head->flags);
        U16       units_per_em        = u16_big_to_local_endian(head->units_per_em);
        TTF_FWord x_min               = u16_big_to_local_endian(head->x_min);
        TTF_FWord y_min               = u16_big_to_local_endian(head->y_min);
        TTF_FWord x_max               = u16_big_to_local_endian(head->x_max);
        TTF_FWord y_max               = u16_big_to_local_endian(head->y_max);
        U16       lowest_rec_ppem     = u16_big_to_local_endian(head->lowest_rec_ppem);
        S16       font_direction_hint = s16_big_to_local_endian(head->font_direction_hint);
        S16       index_to_loc_format = s16_big_to_local_endian(head->index_to_loc_format);
        S16       glyph_data_format   = s16_big_to_local_endian(head->glyph_data_format);

        if (success && version != TTF_MAKE_VERSION(1, 0)) {
            error_emit(str8_literal("ERROR(font/ttf): Unsupported version of head table."));
            success = false;
        }

        // TODO: Validate check_sum_adjustment

        if (success && magic_number != TTF_MAGIC_NUMBER) {
            error_emit(str8_literal("ERROR(font/ttf): Wrong magic number."));
            success = false;
        }

        if (success && (flags & 0x0020) != 0) {
            error_emit(str8_literal("ERROR(font/ttf): Flags required to be unset are set."));
            success = false;
        }

        if (success && !(64 <= units_per_em && units_per_em <= 16384)) {
            error_emit(str8_literal("ERROR(font/ttf): Invalid number of units per em."));
            success = false;
        }

        if (success && !(-2 <= font_direction_hint && font_direction_hint <= 2)) {
            error_emit(str8_literal("ERROR(font/ttf): Invalid font direction hint."));
            success = false;
        }

        if (success && !(index_to_loc_format == 0 || index_to_loc_format == 1)) {
            error_emit(str8_literal("ERROR(font/ttf): Unknown index to location format."));
            success = false;
        }

        if (success && glyph_data_format != 0) {
            error_emit(str8_literal("ERROR(font/ttf): Unknown glyph data format."));
            success = false;
        }

        font->is_long_loca_format = (index_to_loc_format == 1);
        font->funits_per_em       = units_per_em;
        font->lowest_rec_ppem     = lowest_rec_ppem;
    } else {
        error_emit(str8_literal("ERROR(font/ttf): Not enough data in head table."));
        success = false;
    }

    return success;
}

internal B32 ttf_parse_maxp_table(Arena *arena, TTF_Font *font) {
    B32 success = true;

    if (font->tables[TTF_Table_Maxp].size >= sizeof(TTF_MaxpTable)) {
        TTF_MaxpTable *maxp = (TTF_MaxpTable *) font->tables[TTF_Table_Maxp].data;

        TTF_Fixed version                = u32_big_to_local_endian(maxp->version);
        U16       num_glyphs             = u16_big_to_local_endian(maxp->num_glyphs);
        U16       max_zones              = u16_big_to_local_endian(maxp->max_zones);
        U16       max_component_depth    = u16_big_to_local_endian(maxp->max_component_depth);
        U16       max_contours           = u16_big_to_local_endian(maxp->max_contours);
        U16       max_component_contours = u16_big_to_local_endian(maxp->max_component_contours);
        U16       max_points             = u16_big_to_local_endian(maxp->max_points);
        U16       max_component_points   = u16_big_to_local_endian(maxp->max_component_points);

        if (success && version != TTF_MAKE_VERSION(1, 0)) {
            error_emit(str8_literal("ERROR(font/ttf): Unsupported version of maxp table."));
            success = false;
        }

        if (success && !(1 <= max_zones && max_zones <= 2)) {
            error_emit(str8_literal("ERROR(font/ttf): Max zones must between 1 and 2 inclusive."));
            success = false;
        }

        if (success && max_component_depth > 16) {
            error_emit(str8_literal("ERROR(font/ttf): Max component depth is outside of the legal range."));
            success = false;
        }

        font->glyph_count = num_glyphs;
        font->contour_capacity = u16_max(max_contours, max_component_contours);
        font->point_capacity   = u16_max(max_points,   max_component_points);
    } else {
        error_emit(str8_literal("ERROR(font/ttf): Not enough data in maxp table."));
        success = false;
    }

    return success;
}

internal B32 ttf_parse_metric_data(TTF_Font *font, Font *result_font) {
    B32 success = true;

    TTF_HheaTable *hhea = (TTF_HheaTable *) font->tables[TTF_Table_Hhea].data;
    if (font->tables[TTF_Table_Hhea].size >= sizeof(TTF_HheaTable)) {
        TTF_Fixed version             = u32_big_to_local_endian(hhea->version);
        S16       caret_slope_rise    = s16_big_to_local_endian(hhea->caret_slope_rise);
        S16       caret_slope_run     = s16_big_to_local_endian(hhea->caret_slope_run);
        S16       metric_data_format  = s16_big_to_local_endian(hhea->metric_data_format);
        U16       advance_width_count = u16_big_to_local_endian(hhea->num_of_long_hor_metrics);

        if (success && version != TTF_MAKE_VERSION(1, 0)) {
            error_emit(str8_literal("ERROR(font/ttf): Unsupported version of hhea table."));
            success = false;
        }

        if (success && caret_slope_rise == 0 && caret_slope_run == 0) {
            error_emit(str8_literal("ERROR(font/ttf): Both the caret slopes rise and run are 0."));
            success = false;
        }

        if (success && !(hhea->reserved0 == 0 && hhea->reserved1 == 0 && hhea->reserved2 == 0 && hhea->reserved3 == 0)) {
            error_emit(str8_literal("ERROR(font/ttf): Reserved fields in hhea are not set to 0."));
            success = false;
        }

        if (success && metric_data_format != 0) {
            error_emit(str8_literal("ERROR(font/ttf): Unknown metric data format."));
            success = false;
        }

        if (success && advance_width_count == 0) {
            error_emit(str8_literal("ERROR(font/ttf): There must be at least one long-form entry in the hmtx table."));
            success = false;
        }
    } else {
        error_emit(str8_literal("ERROR(font/ttf): Not enough data in hhea table."));
        success = false;
    }

    if (success) {
        U32 advance_width_count     = u16_big_to_local_endian(hhea->num_of_long_hor_metrics);
        U32 left_side_bearing_count = font->glyph_count - advance_width_count;

        Str8 hmtx_data = font->tables[TTF_Table_Hmtx];
        if (hmtx_data.size >= advance_width_count * sizeof(TTF_HmtxMetrics) + left_side_bearing_count * sizeof(TTF_FWord)) {
            TTF_HmtxMetrics *metrics            = (TTF_HmtxMetrics *) hmtx_data.data;
            TTF_FWord       *left_side_bearings = (TTF_FWord *) &hmtx_data.data[advance_width_count * sizeof(TTF_HmtxMetrics)];

            TTF_UFWord last_advance_width = 0;
            for (U32 i = 0; i < advance_width_count; ++i) {
                last_advance_width = u16_big_to_local_endian(metrics[i].advance_width);
                if (font->ttf_to_internal_glyph_indicies[i]) {
                    U32 internal_index = font->ttf_to_internal_glyph_indicies[i] - 1;
                    result_font->glyphs[internal_index].advance_width     = u16_big_to_local_endian(metrics[i].advance_width);
                    result_font->glyphs[internal_index].left_side_bearing = s16_big_to_local_endian(metrics[i].left_side_bearing);
                }
            }

            for (U32 i = 0; i < left_side_bearing_count; ++i) {
                if (font->ttf_to_internal_glyph_indicies[i]) {
                    U32 internal_index = font->ttf_to_internal_glyph_indicies[advance_width_count + i] - 1;
                    result_font->glyphs[internal_index].advance_width     = last_advance_width;
                    result_font->glyphs[internal_index].left_side_bearing = s16_big_to_local_endian(left_side_bearings[i]);
                }
            }
        } else {
            error_emit(str8_literal("ERROR(font/ttf): Not enough data in hmtx table."));
            success = false;
        }
    }

    return success;
}

internal U32 ttf_rank_subtable(TTF_CmapSubtable *subtable) {
    U16 platform_id          = u16_big_to_local_endian(subtable->platform_id);
    U16 platform_specifig_id = u16_big_to_local_endian(subtable->platform_specific_id);

    U32 rank = 0;

    switch (platform_id) {
        case TTF_CMAP_PLATFORM_UNICODE: {
            switch (platform_specifig_id) {
                case TTF_CMAP_UNICODE_1_0:                 rank = 3; break;
                case TTF_CMAP_UNICODE_1_1:                 rank = 4; break;
                case TTF_CMAP_UNICODE_DEPRECATED:          rank = 0; break;
                case TTF_CMAP_UNICODE_2_0_BMP:             rank = 2; break;
                case TTF_CMAP_UNICODE_2_0_NON_BMP:         rank = 6; break;
                case TTF_CMAP_UNICODE_VARIATION_SEQUENCES: rank = 0; break;
                case TTF_CMAP_UNICODE_LAST_RESORT:         rank = 0; break;
                default:                                   rank = 0; break;
            }
        } break;
        case TTF_CMAP_PLATFORM_WINDOWS: {
            switch (platform_specifig_id) {
                case TTF_CMAP_WINDOWS_SYMBOL:      rank = 0; break;
                case TTF_CMAP_WINDOWS_UNICODE_BMP: rank = 1; break;
                case TTF_CMAP_WINDOWS_SHIFT_JIS:   rank = 0; break;
                case TTF_CMAP_WINDOWS_PRC:         rank = 0; break;
                case TTF_CMAP_WINDOWS_BIG_FIVE:    rank = 0; break;
                case TTF_CMAP_WINDOWS_JOHAB:       rank = 0; break;
                case TTF_CMAP_WINDOWS_UNICODE_4:   rank = 5; break;
                default:                           rank = 0; break;
            }
        } break;
        default: {
            rank = 0;
        } break;
    }

    return rank;
}

internal Void ttf_add_codepoint_to_ttf_glyph_mapping(TTF_Font *font, U32 codepoint, U16 glyph_index) {
    if (glyph_index) {
        if (!font->ttf_to_internal_glyph_indicies[glyph_index]) {
            font->ttf_to_internal_glyph_indicies[glyph_index] = font->internal_glyph_count + 1;
            font->internal_to_ttf_glyph_indicies[font->internal_glyph_count] = glyph_index;
            ++font->internal_glyph_count;
        }

        font->codepoints[font->codepoint_count]     = codepoint;
        font->glyph_indicies[font->codepoint_count] = font->ttf_to_internal_glyph_indicies[glyph_index] - 1;
        ++font->codepoint_count;
    }
}

// TODO: Ensure that no codepoint is mapped to glyph index 0xFFFF.
// TODO: Unicode varition sequence subtables.
// TODO: Allow for loading noncontiguous sets of characters.
internal B32 ttf_get_character_map(Arena *arena, TTF_Font *font, FontDescription *font_description) {
    Str8 cmap_data = font->tables[TTF_Table_Cmap];

    B32 success = true;

    TTF_CmapTable    *cmap           = 0;
    U32               subtable_count = 0;
    TTF_CmapSubtable *subtables      = 0;

    if (cmap_data.size >= sizeof(TTF_CmapTable)) {
        cmap = (TTF_CmapTable *) cmap_data.data;
        U16 version    = u16_big_to_local_endian(cmap->version);
        subtable_count = u16_big_to_local_endian(cmap->number_subtables);

        if (success && version != 0) {
            error_emit(str8_literal("ERROR(font/ttf): Unsupported version of cmap table."));
            success = false;
        }
    } else {
        error_emit(str8_literal("ERROR(font/ttf): Not enough data for cmap table."));
        success = false;
    }

    if (success) {
        Str8 subtable_data = str8_skip(cmap_data, sizeof(TTF_CmapTable));
        if (subtable_data.size >= subtable_count * sizeof(TTF_CmapSubtable)) {
            subtables = (TTF_CmapSubtable *) subtable_data.data;
        } else {
            error_emit(str8_literal("ERROR(font/ttf): Not enough data for cmap subtables."));
            success = false;
        }
    }

    TTF_CmapSubtable *subtable = 0;
    if (success) {
        U32 rank = 0;
        for (U32 i = 0; i < subtable_count; ++i) {
            TTF_CmapSubtable *possible_subtable = &subtables[i];
            U32 new_rank = ttf_rank_subtable(possible_subtable);
            if (new_rank > rank) {
                subtable = possible_subtable;
                rank     = new_rank;
            }
        }

        if (!subtable) {
            error_emit(str8_literal("ERROR(font/ttf): Could not find a suitable cmap subtable."));
            success = false;
        }
    }

    if (success && u32_big_to_local_endian(subtable->offset) + sizeof(U16) > cmap_data.size) {
        error_emit(str8_literal("ERROR(font/ttf): Subtable is outside of cmap table."));
        success = false;
    }

    if (success) {
        font->ttf_to_internal_glyph_indicies = arena_push_array(arena, U16, font->glyph_count); // NOTE: The stored indicies are 1-based.
        font->internal_to_ttf_glyph_indicies = arena_push_array(arena, U16, font->glyph_count);

        // Internal glyph 0 always maps to ttf glyph 0 and MUST exist.
        font->internal_glyph_count = 1;
        font->ttf_to_internal_glyph_indicies[0] = 1;

        Str8 subtable_data = str8_skip(cmap_data, u32_big_to_local_endian(subtable->offset));
        U16  format        = u16_big_to_local_endian(*(U16 *) subtable_data.data);

        switch (format) {
            case 0: {
                // TODO: Should we allow subtables that are larger here? Technically we can load them, but they might be wrong.
                if (subtable_data.size == sizeof(TTF_CmapFormat0)) {
                    TTF_CmapFormat0 *format = (TTF_CmapFormat0 *) subtable_data.data;
                    font->codepoints        = arena_push_array(arena, U32, array_count(format->glyph_index_array));
                    font->glyph_indicies    = arena_push_array(arena, U32, array_count(format->glyph_index_array));
                    font->codepoint_count   = 0;

                    for (U32 code = 0; code < array_count(format->glyph_index_array); ++code) {
                        if (font_description->codepoint_first <= code && code <= font_description->codepoint_last) {
                            ttf_add_codepoint_to_ttf_glyph_mapping(font, code, format->glyph_index_array[code]);
                        }
                    }
                } else {
                    error_emit(str8_literal("ERROR(font/ttf): Not enough data for cmap format 0."));
                    success = false;
                }
            } break;
            case 2: {
                error_emit(str8_literal("ERROR(font/ttf): Cmap format 2 is not supported."));
                success = false;
            } break;
            case 4: {
                if (subtable_data.size >= sizeof(TTF_CmapFormat4)) {
                    TTF_CmapFormat4 *format = (TTF_CmapFormat4 *) subtable_data.data;

                    U32 length        = u16_big_to_local_endian(format->length);
                    U32 segment_count = u16_big_to_local_endian(format->seg_count_x2) / 2;

                    if (length <= subtable_data.size && length >= sizeof(TTF_CmapFormat4) + (4 * segment_count + 1) * sizeof(U16)) {
                        U32 offset = sizeof(TTF_CmapFormat4);
                        U16 *end_code = (U16 *) (subtable_data.data + offset);
                        offset += segment_count * sizeof(U16);
                        // Reserved field
                        offset += sizeof(U16);
                        U16 *start_code = (U16 *) (subtable_data.data + offset);
                        offset += segment_count * sizeof(U16);
                        U16 *id_delta = (U16 *) (subtable_data.data + offset);
                        offset += segment_count * sizeof(U16);
                        U16 *id_range_offset = (U16 *) (subtable_data.data + offset);
                        offset += segment_count * sizeof(U16);

                        U32 glyph_index_array_count = (subtable_data.size - (sizeof(TTF_CmapFormat4) + (4 * segment_count + 1) * sizeof(U16))) / sizeof(U16);

                        // TODO: Maybe warn if the last segment doesn't map 0xFFFF, but still load the font.
                        // NOTE: Even though it is required by the spec that
                        // the last segment map 0xFFFF, we don't depend on it
                        // and work regardless. Therefor, we won't report an
                        // error in this case.

                        // Calculate the maxiumum number of code points and allocate space for all of them.
                        U32 code_count = 0;
                        for (U32 i = 0; i < segment_count; ++i) {
                            code_count += u16_big_to_local_endian(end_code[i]) - u16_big_to_local_endian(start_code[i]) + 1;
                        }

                        font->codepoints      = arena_push_array(arena, U32, code_count);
                        font->glyph_indicies  = arena_push_array(arena, U32, code_count);
                        font->codepoint_count = 0;

                        for (U32 i = 0; i < segment_count; ++i) {
                            U16 start  = u16_big_to_local_endian(start_code[i]);
                            U16 end    = u16_big_to_local_endian(end_code[i]);
                            U16 offset = u16_big_to_local_endian(id_range_offset[i]) / 2;
                            U16 delta  = u16_big_to_local_endian(id_delta[i]);

                            // NOTE: This is fine to read without a byteswap.
                            if (id_range_offset[i] == 0) {
                                for (U32 code = start; code <= end; ++code) {
                                    if (font_description->codepoint_first <= code && code <= font_description->codepoint_last) {
                                        U16 glyph_index = (delta + code) % 65536;
                                        ttf_add_codepoint_to_ttf_glyph_mapping(font, code, glyph_index);
                                    }
                                }
                            } else {
                                U32 lowest_glyph_index_index  = i + offset + (start - start);
                                U32 highest_glyph_index_index = i + offset + (end   - start);

                                if (segment_count <= lowest_glyph_index_index && highest_glyph_index_index <= segment_count + glyph_index_array_count) {
                                    for (U32 code = start; code <= end; ++code) {
                                        if (font_description->codepoint_first <= code && code <= font_description->codepoint_last) {
                                            U16 raw_glyph_index = u16_big_to_local_endian(id_range_offset[i + offset + (code - start)]);
                                            U16 glyph_index = (raw_glyph_index + delta) % 65536;

                                            if (raw_glyph_index) {
                                                ttf_add_codepoint_to_ttf_glyph_mapping(font, code, glyph_index);
                                            }
                                        }
                                    }
                                } else {
                                    error_emit(str8_literal("ERROR(font/ttf): Format 4 cmap access data outside of its subtable."));
                                    success = false;
                                }
                            }
                        }
                    } else {
                        error_emit(str8_literal("ERROR(font/ttf): Not enough data for cmap format 4."));
                        success = false;
                    }
                } else {
                    error_emit(str8_literal("ERROR(font/ttf): Not enough data for cmap format 4."));
                    success = false;
                }
            } break;
            case 6: {
                if (subtable_data.size >= sizeof(TTF_CmapFormat6)) {
                    TTF_CmapFormat6 *format = (TTF_CmapFormat6 *) subtable_data.data;

                    U32 first_code  = u16_big_to_local_endian(format->first_code);
                    U32 entry_count = u16_big_to_local_endian(format->entry_count);

                    if (subtable_data.size >= sizeof(TTF_CmapFormat6) + entry_count * sizeof(U16)) {
                        font->codepoints      = arena_push_array(arena, U32, entry_count);
                        font->glyph_indicies  = arena_push_array(arena, U32, entry_count);
                        font->codepoint_count = 0;

                        U16 *glyph_index_array = (U16 *) &subtable_data.data[sizeof(*format)];

                        for (U32 i = 0; i < entry_count; ++i) {
                            if (font_description->codepoint_first <= first_code + i && first_code + i <= font_description->codepoint_last) {
                                U32 glyph_index = u16_big_to_local_endian(glyph_index_array[i]);
                                ttf_add_codepoint_to_ttf_glyph_mapping(font, first_code + i, glyph_index);
                            }
                        }
                    } else {
                        error_emit(str8_literal("ERROR(font/ttf): Not enough data for cmap format 6."));
                        success = false;
                    }
                } else {
                    error_emit(str8_literal("ERROR(font/ttf): Not enough data for cmap format 6."));
                    success = false;
                }
            } break;
            case 8: {
                error_emit(str8_literal("ERROR(font/ttf): Cmap format 8 is not supported."));
                success = false;
            } break;
            case 10: {
                error_emit(str8_literal("ERROR(font/ttf): Cmap format 10 is not supported."));
                success = false;
            } break;
            case 12: {
                if (subtable_data.size >= sizeof(TTF_CmapFormat12)) {
                    TTF_CmapFormat12 *format = (TTF_CmapFormat12 *) subtable_data.data;

                    U32 length      = u32_big_to_local_endian(format->length);
                    U32 group_count = u32_big_to_local_endian(format->n_groups);

                    if (length <= subtable_data.size) {
                        // Calculate the maxiumum number of code points and allocate space for all of them.
                        U32 code_count = 0;
                        TTF_CmapFormat12Group *groups = (TTF_CmapFormat12Group *) &subtable_data.data[sizeof(*format)];
                        for (U32 i = 0; i < group_count; ++i) {
                            code_count += u32_big_to_local_endian(groups[i].end_char_code) - u32_big_to_local_endian(groups[i].start_char_code) + 1;
                        }
                        font->codepoints      = arena_push_array(arena, U32, code_count);
                        font->glyph_indicies  = arena_push_array(arena, U32, code_count);
                        font->codepoint_count = 0;

                        for (U32 i = 0; i < group_count; ++i) {
                            U32 first_codepoint   = u32_big_to_local_endian(groups[i].start_char_code);
                            U32 last_codepoint    = u32_big_to_local_endian(groups[i].end_char_code);
                            U32 first_glyph_index = u32_big_to_local_endian(groups[i].start_glyph_code);

                            for (U32 code = first_codepoint; code <= last_codepoint; ++code) {
                                if (font_description->codepoint_first <= code && code <= font_description->codepoint_last) {
                                    U32 glyph_index = first_glyph_index + (code - first_codepoint);
                                    ttf_add_codepoint_to_ttf_glyph_mapping(font, code, glyph_index);
                                }
                            }
                        }
                    } else {
                        error_emit(str8_literal("ERROR(font/ttf): Not enough data for cmap format 12."));
                        success = false;
                    }
                } else {
                    error_emit(str8_literal("ERROR(font/ttf): Not enough data for cmap format 12."));
                    success = false;
                }
            } break;
            case 13: {
                error_emit(str8_literal("ERROR(font/ttf): Cmap format 13 is not supported."));
                success = false;
            } break;
            case 14: {
                error_emit(str8_literal("ERROR(font/ttf): Cmap format 14 is not supported."));
                success = false;
            } break;
            default: {
                error_emit(str8_literal("ERROR(font/ttf): Unknown cmap format."));
                success = false;
            } break;
        }
    }

    return success;
}

internal B32 ttf_get_glyph_outlines(TTF_Font *font, U32 glyph_index, U32 contour_capacity, U32 point_capacity, TTF_Glyph *result_glyph) {
    B32 success = true;

    Str8 glyph_data = font->raw_glyph_data[glyph_index];
    U32  read_index = 0;

    TTF_GlyphHeader *header = 0;
    S16 contour_count       = 0;
    if (success && glyph_data.size >= read_index + sizeof(TTF_GlyphHeader)) {
        header = (TTF_GlyphHeader *) &glyph_data.data[read_index];
        read_index += sizeof(TTF_GlyphHeader);

        result_glyph->x_min = s16_big_to_local_endian(header->x_min);
        result_glyph->y_min = s16_big_to_local_endian(header->y_min);
        result_glyph->x_max = s16_big_to_local_endian(header->x_max);
        result_glyph->y_max = s16_big_to_local_endian(header->y_max);

        contour_count = s16_big_to_local_endian(header->number_of_contours);
    } else {
        error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph outlines."));
        success = false;
    }

    if (success && contour_count > 0) {
        if ((U32) contour_count > contour_capacity) {
            error_emit(str8_literal("ERROR(font/ttf): Glyph contains too many contours."));
            success = false;
        }

        if (success && glyph_data.size >= read_index + contour_count * sizeof(U16)) {
            result_glyph->contour_count = contour_count;

            for (S32 i = 0; i < contour_count; ++i) {
                result_glyph->contour_end_points[i] = u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]);
                read_index += sizeof(U16);
            }

            result_glyph->point_count = result_glyph->contour_end_points[result_glyph->contour_count - 1] + 1;
        } else {
            error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph contours."));
            success = false;
        }

        if (result_glyph->point_count > point_capacity) {
            error_emit(str8_literal("ERROR(font/ttf): Glyph contains too many points."));
            success = false;
        }

        // NOTE: Skip over instructions.
        if (success && glyph_data.size >= read_index + sizeof(U16)) {
            U16 instruction_length = u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]);
            read_index += sizeof(U16);

            if (glyph_data.size >= read_index + instruction_length * sizeof(U8)) {
                read_index += instruction_length * sizeof(U8);
            } else {
                error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph instructions."));
                success = false;
            }
        } else {
            error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph instructions."));
            success = false;
        }

        // NOTE: Any valid ttf-file will have at least one x-coordinate after
        // the tags, guaranteeing that we have at least two bytes.
        for (U32 point_index = 0; success && point_index < result_glyph->point_count;) {
            if (glyph_data.size >= read_index + 2 * sizeof(U8)) {
                U8 flag = glyph_data.data[read_index++];
                U32 repeat_count = 1;

                if (flag & TTF_SIMPLE_GLYPH_FLAGS_REPEAT) {
                    repeat_count += glyph_data.data[read_index++];
                }

                if (result_glyph->point_count >= point_index + repeat_count) {
                    for (U32 i = 0; i < repeat_count; ++i) {
                        result_glyph->flags[point_index++] = flag;
                    }
                } else {
                    error_emit(str8_literal("ERROR(font/ttf): Too many glyph point flags."));
                    success = false;
                }
            } else {
                error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph point flags."));
                success = false;
            }
        }

        if (success) {
            TTF_FWord previous = 0;
            for (U32 point_index = 0; success && point_index < result_glyph->point_count; ++point_index) {
                U8 flag = result_glyph->flags[point_index];
                if (flag & TTF_SIMPLE_GLYPH_FLAGS_SHORT_X) {
                    if (glyph_data.size >= read_index + sizeof(U8)) {
                        if (flag & TTF_SIMPLE_GLYPH_FLAGS_SAME_OR_POSITIVE_X) {
                            previous += glyph_data.data[read_index++];
                        } else {
                            previous -= glyph_data.data[read_index++];
                        }
                    } else {
                        error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph point x-coordinates."));
                        success = false;
                    }
                } else {
                    if (flag & TTF_SIMPLE_GLYPH_FLAGS_SAME_OR_POSITIVE_X) {
                        // Nothing to do.
                    } else {
                        if (glyph_data.size >= read_index + sizeof(TTF_FWord)) {
                            previous += s16_big_to_local_endian(*(TTF_FWord *) &glyph_data.data[read_index]);
                            read_index += sizeof(TTF_FWord);
                        } else {
                            error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph point x-coordinates."));
                            success = false;
                        }
                    }
                }
                result_glyph->x_coordinates[point_index] = previous;
            }
        }

        if (success) {
            TTF_FWord previous = 0;
            for (U32 point_index = 0; success && point_index < result_glyph->point_count; ++point_index) {
                U8 flag = result_glyph->flags[point_index];
                if (flag & TTF_SIMPLE_GLYPH_FLAGS_SHORT_Y) {
                    if (glyph_data.size >= read_index + sizeof(U8)) {
                        if (flag & TTF_SIMPLE_GLYPH_FLAGS_SAME_OR_POSITIVE_Y) {
                            previous += glyph_data.data[read_index++];
                        } else {
                            previous -= glyph_data.data[read_index++];
                        }
                    } else {
                        error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph point y-coordinates."));
                        success = false;
                    }
                } else {
                    if (flag & TTF_SIMPLE_GLYPH_FLAGS_SAME_OR_POSITIVE_Y) {
                        // Nothing to do.
                    } else {
                        if (glyph_data.size >= read_index + sizeof(TTF_FWord)) {
                            previous += s16_big_to_local_endian(*(TTF_FWord *) &glyph_data.data[read_index]);
                            read_index += sizeof(TTF_FWord);
                        } else {
                            error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph point y-coordinates."));
                            success = false;
                        }
                    }
                }
                result_glyph->y_coordinates[point_index] = previous;
            }
        }
    } else if (success && contour_count < 0) {
        B32 has_more_components = true;
        while (success && has_more_components) {
            U16 flags = 0;

            TTF_Glyph component_glyph = { 0 };
            component_glyph.contour_end_points = &result_glyph->contour_end_points[result_glyph->contour_count];
            component_glyph.flags              = &result_glyph->flags[result_glyph->point_count];
            component_glyph.x_coordinates      = &result_glyph->x_coordinates[result_glyph->point_count];
            component_glyph.y_coordinates      = &result_glyph->y_coordinates[result_glyph->point_count];
            F32 a = 1.0, b = 0.0;
            F32 c = 0.0, d = 1.0;
            F32 e = 0.0, f = 0.0;
            F32 m = 1.0, n = 1.0;
            U16 compound_point_index  = 0;
            U16 component_point_index = 0;

            if (success && glyph_data.size >= read_index + 2 * sizeof(U16)) {
                flags = u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]);
                read_index += sizeof(U16);

                U16 component_glyph_index = u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]);
                read_index += sizeof(U16);

                success = ttf_get_glyph_outlines(font, component_glyph_index, contour_count - result_glyph->contour_count, point_capacity - result_glyph->point_count, &component_glyph);
            } else {
                error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph component flags."));
                success = false;
            }

            if (success) {
                if (flags & TTF_COMPOUND_GLYPH_FLAGS_ARG_1_AND_2_ARE_WORDS && glyph_data.size < read_index + 2 * sizeof(U16)) {
                    error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph component arguments."));
                    success = false;
                } else if (glyph_data.size < read_index + 2 * sizeof(U8)) {
                    error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph component arguments."));
                    success = false;
                }
            }

            if (success) {
                if (flags & TTF_COMPOUND_GLYPH_FLAGS_ARGS_ARE_XY_VALUES) {
                    if (flags & TTF_COMPOUND_GLYPH_FLAGS_ARG_1_AND_2_ARE_WORDS) {
                        e = s16_big_to_local_endian(*(TTF_FWord *) &glyph_data.data[read_index]);
                        read_index += sizeof(S16);
                        f = s16_big_to_local_endian(*(TTF_FWord *) &glyph_data.data[read_index]);
                        read_index += sizeof(S16);
                    } else {
                        e = *(S8 *) &glyph_data.data[read_index++];
                        f = *(S8 *) &glyph_data.data[read_index++];
                    }
                } else {
                    if (flags & TTF_COMPOUND_GLYPH_FLAGS_ARG_1_AND_2_ARE_WORDS) {
                        compound_point_index = u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]);
                        read_index += sizeof(U16);
                        component_point_index = u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]);
                        read_index += sizeof(U16);
                    } else {
                        compound_point_index  = *(U8 *) &glyph_data.data[read_index++];
                        component_point_index = *(U8 *) &glyph_data.data[read_index++];
                    }
                }
            }

            // TODO: Verify that only one of the following flags is set.
            if (success) {
                if (flags & TTF_COMPOUND_GLYPH_FLAGS_WE_HAVE_A_SCALE) {
                    if (glyph_data.size >= read_index + sizeof(TTF_F2Dot14)) {
                        a = d = ttf_f2dot14_to_f32(u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]));
                        read_index += sizeof(TTF_F2Dot14);
                    } else {
                        error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph component transform."));
                        success = false;
                    }
                } else if (flags & TTF_COMPOUND_GLYPH_FLAGS_WE_HAVE_AN_X_AND_Y_SCALE) {
                    if (glyph_data.size >= read_index + 2 * sizeof(TTF_F2Dot14)) {
                        a = ttf_f2dot14_to_f32(u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]));
                        read_index += sizeof(TTF_F2Dot14);
                        d = ttf_f2dot14_to_f32(u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]));
                        read_index += sizeof(TTF_F2Dot14);
                    } else {
                        error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph component transform."));
                        success = false;
                    }
                } else if (flags & TTF_COMPOUND_GLYPH_FLAGS_WE_HAVE_A_TWO_BY_TWO) {
                    if (glyph_data.size >= read_index + 4 * sizeof(TTF_F2Dot14)) {
                        a = ttf_f2dot14_to_f32(u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]));
                        read_index += sizeof(TTF_F2Dot14);
                        b = ttf_f2dot14_to_f32(u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]));
                        read_index += sizeof(TTF_F2Dot14);
                        c = ttf_f2dot14_to_f32(u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]));
                        read_index += sizeof(TTF_F2Dot14);
                        d = ttf_f2dot14_to_f32(u16_big_to_local_endian(*(U16 *) &glyph_data.data[read_index]));
                        read_index += sizeof(TTF_F2Dot14);
                    } else {
                        error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyph component transform."));
                        success = false;
                    }
                }
            }

            if (success && !(flags & TTF_COMPOUND_GLYPH_FLAGS_ARGS_ARE_XY_VALUES) && !(compound_point_index < result_glyph->point_count && component_point_index < component_glyph.point_count)) {
                error_emit(str8_literal("ERROR(font/ttf): Phantom points are not supported, glyph component alignemnt indicies are out of range.."));
                success = false;
            }

            if (success) {
                if (flags & TTF_COMPOUND_GLYPH_FLAGS_SCALED_COMPONENT_OFFSET) {
                    m = f32_sqrt(a * a + c * c);
                    n = f32_sqrt(b * b + d * d);
                }

                for (U32 i = 0; i < component_glyph.point_count; ++i) {
                    TTF_FWord x = component_glyph.x_coordinates[i];
                    TTF_FWord y = component_glyph.y_coordinates[i];
                    component_glyph.x_coordinates[i] = a * x + c * y + m * e;
                    component_glyph.y_coordinates[i] = b * x + d * y + n * f;
                }

                if (!(flags & TTF_COMPOUND_GLYPH_FLAGS_ARGS_ARE_XY_VALUES)) {
                    U32 offset_x = result_glyph->x_coordinates[compound_point_index] - component_glyph.x_coordinates[component_point_index];
                    U32 offset_y = result_glyph->y_coordinates[compound_point_index] - component_glyph.y_coordinates[component_point_index];
                    for (U32 i = 0; i < component_glyph.point_count; ++i) {
                        component_glyph.x_coordinates[i] += offset_x;
                        component_glyph.y_coordinates[i] += offset_y;
                    }
                }
            }

            if (success) {
                for (U32 i = 0; i < component_glyph.contour_count; ++i) {
                    component_glyph.contour_end_points[i] += result_glyph->point_count;
                }

                result_glyph->contour_count += component_glyph.contour_count;
                result_glyph->point_count   += component_glyph.point_count;
            }

            if (success) {
                has_more_components = flags & TTF_COMPOUND_GLYPH_FLAGS_MORE_COMPONENTS;
            }
        }
    }

    return success;
}

internal MSDF_Glyph ttf_expand_contours_to_msdf(Arena *arena, TTF_Font *font, U32 glyph_index) {
    MSDF_Glyph result = { 0 };
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    TTF_Glyph glyph = { 0 };
    glyph.contour_end_points = arena_push_array(scratch.arena, U16,       font->contour_capacity);
    glyph.flags              = arena_push_array(scratch.arena, U8,        font->point_capacity);
    glyph.x_coordinates      = arena_push_array(scratch.arena, TTF_FWord, font->point_capacity);
    glyph.y_coordinates      = arena_push_array(scratch.arena, TTF_FWord, font->point_capacity);

    ttf_get_glyph_outlines(font, glyph_index, font->contour_capacity, font->point_capacity, &glyph);

    result.x_min = glyph.x_min;
    result.y_min = glyph.y_min;
    result.x_max = glyph.x_max;
    result.y_max = glyph.y_max;

    for (U32 contour_index = 0, point_index = 0; contour_index < glyph.contour_count; ++contour_index) {
        MSDF_Contour *contour = arena_push_struct_zero(arena, MSDF_Contour);

        U32 prev_index    = glyph.contour_end_points[contour_index] - 1;
        U32 current_index = glyph.contour_end_points[contour_index];

        TTF_FWord prev_x           = glyph.x_coordinates[prev_index];
        TTF_FWord prev_y           = glyph.y_coordinates[prev_index];
        B32       prev_on_curve    = (glyph.flags[prev_index] & TTF_SIMPLE_GLYPH_FLAGS_ON_CURVE);
        TTF_FWord current_x        = glyph.x_coordinates[current_index];
        TTF_FWord current_y        = glyph.y_coordinates[current_index];
        U8        current_on_curve = (glyph.flags[current_index] & TTF_SIMPLE_GLYPH_FLAGS_ON_CURVE);

        for (; point_index <= glyph.contour_end_points[contour_index]; ++point_index) {
            TTF_FWord next_x        = glyph.x_coordinates[point_index];
            TTF_FWord next_y        = glyph.y_coordinates[point_index];
            B32       next_on_curve = (glyph.flags[point_index] & TTF_SIMPLE_GLYPH_FLAGS_ON_CURVE);

            if (current_on_curve) {
                if (next_on_curve) {
                    MSDF_Segment *line = arena_push_struct_zero(arena, MSDF_Segment);
                    line->kind  = MSDF_SEGMENT_LINE;
                    line->p0    = v2f32(current_x, current_y);
                    line->p1    = v2f32(next_x, next_y);
                    line->flags = 0;
                    dll_push_back(contour->first_segment, contour->last_segment, line);
                }
            } else {
                MSDF_Segment *bezier = arena_push_struct_zero(arena, MSDF_Segment);
                bezier->kind  = MSDF_SEGMENT_QUADRATIC_BEZIER;
                bezier->p0    = (prev_on_curve ? v2f32(prev_x, prev_y) : v2f32(((F32) prev_x + (F32) current_x) * 0.5f, ((F32) prev_y + (F32) current_y) * 0.5f));
                bezier->p1    = v2f32(current_x, current_y);
                bezier->p2    = (next_on_curve ? v2f32(next_x, next_y) : v2f32(((F32) next_x + (F32) current_x) * 0.5f, ((F32) next_y + (F32) current_y) * 0.5f));
                bezier->flags = 0;
                dll_push_back(contour->first_segment, contour->last_segment, bezier);

                if (points_are_collinear(bezier->p0, bezier->p1, bezier->p2)) {
                    bezier->kind = MSDF_SEGMENT_LINE;
                    bezier->p1   = bezier->p2;
                }
            }

            prev_x           = current_x;
            prev_y           = current_y;
            prev_on_curve    = current_on_curve;
            current_x        = next_x;
            current_y        = next_y;
            current_on_curve = next_on_curve;
        }

        dll_push_back(result.first_contour, result.last_contour, contour);
    }

    arena_end_temporary(scratch);
    return result;
}

internal B32 ttf_parse_loca_table(Arena *arena, TTF_Font *ttf_font) {
    B32 success = true;

    ttf_font->raw_glyph_data = arena_push_array_zero(arena, Str8, ttf_font->glyph_count);

    Str8 loca_data = ttf_font->tables[TTF_Table_Loca];
    Str8 glyf_data = ttf_font->tables[TTF_Table_Glyf];
    if (ttf_font->is_long_loca_format) {
        if (loca_data.size >= (ttf_font->glyph_count + 1) * sizeof(U32)) {
            U32 *offsets = (U32 *) loca_data.data;

            for (U32 i = 0; i < ttf_font->glyph_count; ++i) {
                U32 start = u32_big_to_local_endian(offsets[i + 0]);
                U32 end   = u32_big_to_local_endian(offsets[i + 1]);

                if (start <= end && end <= glyf_data.size) {
                    ttf_font->raw_glyph_data[i] = str8_substring(glyf_data, start, end - start);
                } else {
                    error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyf table."));
                    success = false;
                }
            }
        } else {
            error_emit(str8_literal("ERROR(font/ttf): Not enough data for long loca table."));
            success = false;
        }

    } else {
        if (loca_data.size >= (ttf_font->glyph_count + 1) * sizeof(U16)) {
            U16 *offsets = (U16 *) loca_data.data;

            for (U32 i = 0; i < ttf_font->glyph_count; ++i) {
                U32 start = 2 * (U32) u16_big_to_local_endian(offsets[i + 0]);
                U32 end   = 2 * (U32) u16_big_to_local_endian(offsets[i + 1]);

                if (start <= end && end <= glyf_data.size) {
                    ttf_font->raw_glyph_data[i] = str8_substring(glyf_data, start, end - start);
                } else {
                    error_emit(str8_literal("ERROR(font/ttf): Not enough data for glyf table."));
                    success = false;
                }
            }
        } else {
            error_emit(str8_literal("ERROR(font/ttf): Not enough data for long loca table."));
            success = false;
        }
    }

    return success;
}

internal B32 ttf_load(Arena *arena, Str8 font_path, TTF_Font *ttf_font) {
    B32 success = true;

    Str8 font_data = { 0 };

    if (success) {
        success = os_file_read(arena, font_path, &font_data);
    }

    if (success) {
        success = ttf_parse_font_tables(font_data, ttf_font);
    }

    if (success) {
        success = ttf_parse_head_table(ttf_font);
    }

    if (success) {
        success = ttf_parse_maxp_table(arena, ttf_font);
    }

    if (success) {
        success = ttf_parse_loca_table(arena, ttf_font);
    }

    return success;
}
