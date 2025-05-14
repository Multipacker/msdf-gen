internal B32 points_are_collinear(V2F32 a, V2F32 b, V2F32 c) {
    F32 epsilon = 0.0001f;
    V2F32 ba = v2f32_subtract(a, b);
    V2F32 bc = v2f32_subtract(c, b);
    B32 are_collinear = f32_abs(v2f32_cross(ba, bc)) < epsilon;
    return are_collinear;
}

internal F32 ttf_f2dot14_to_f32(TTF_F2Dot14 x) {
    F32 result = (F32) (x & 0x7FFF) / 16384.0f;
    if (x & 0x8000) {
        result -= 2.0f;
    }
    return result;
}



// NOTE(simon): Helpers for parsing.
typedef struct TTF_Parser TTF_Parser;
struct TTF_Parser {
    U8 *data;
    U64 size;
    B32 out_of_data;
};

internal F32 ttf_read_f2dot14_as_f32(TTF_Parser *data) {
    F32 result = 0.0f;
    if (sizeof(TTF_F2Dot14) <= data->size) {
        TTF_F2Dot14 fixed_point = (TTF_F2Dot14) (data->data[0] << 8 | data->data[1] << 0);
        result = ttf_f2dot14_to_f32(fixed_point);
        data->data += sizeof(TTF_F2Dot14);
        data->size -= sizeof(TTF_F2Dot14);
    } else {
        data->out_of_data = true;
    }
    return result;
}

internal U8 ttf_read_u8(TTF_Parser *data) {
    U8 result = 0;
    if (sizeof(U8) <= data->size) {
        result = data->data[0];
        data->data += sizeof(U8);
        data->size -= sizeof(U8);
    } else {
        data->out_of_data = true;
    }
    return result;
}

internal S8 ttf_read_s8(TTF_Parser *data) {
    S8 result = 0;
    if (sizeof(S8) <= data->size) {
        result = (S8) data->data[0];
        data->data += sizeof(S8);
        data->size -= sizeof(S8);
    } else {
        data->out_of_data = true;
    }
    return result;
}

internal U16 ttf_read_u16(TTF_Parser *data) {
    U16 result = 0;
    if (sizeof(U16) <= data->size) {
        result = (U16) (data->data[0] << 8 | data->data[1] << 0);
        data->data += sizeof(U16);
        data->size -= sizeof(U16);
    } else {
        data->out_of_data = true;
    }
    return result;
}

internal S16 ttf_read_s16(TTF_Parser *data) {
    S16 result = 0;
    if (sizeof(S16) <= data->size) {
        result = (S16) (data->data[0] << 8 | data->data[1] << 0);
        data->data += sizeof(S16);
        data->size -= sizeof(S16);
    } else {
        data->out_of_data = true;
    }
    return result;
}

internal U32 ttf_read_u32(TTF_Parser *data) {
    U32 result = 0;
    if (sizeof(U32) <= data->size) {
        result = (U32) (data->data[0] << 24 | data->data[1] << 16 | data->data[2] << 8 | data->data[3] << 0);
        data->data += sizeof(U32);
        data->size -= sizeof(U32);
    } else {
        data->out_of_data = true;
    }
    return result;
}

internal U64 ttf_read_u64(TTF_Parser *data) {
    U64 result = 0;
    if (sizeof(U64) <= data->size) {
        result |= (U64) data->data[0] << 56;
        result |= (U64) data->data[1] << 48;
        result |= (U64) data->data[2] << 40;
        result |= (U64) data->data[3] << 32;
        result |= (U64) data->data[4] << 24;
        result |= (U64) data->data[5] << 16;
        result |= (U64) data->data[6] <<  8;
        result |= (U64) data->data[7] <<  0;
        data->data += sizeof(U64);
        data->size -= sizeof(U64);
    } else {
        data->out_of_data = true;
    }
    return result;
}

internal Str8 ttf_read_bytes(TTF_Parser *data, U64 size) {
    Str8 result = { 0 };
    if (size <= data->size) {
        result = str8(data->data, size);
        data->data += size;
        data->size -= size;
    } else {
        data->out_of_data = true;
    }
    return result;
}

internal TTF_Parser ttf_sub_parser(TTF_Parser *data, U64 size) {
    TTF_Parser result = { 0 };
    if (size <= data->size) {
        result.data = data->data;
        result.size = size;
        data->data += size;
        data->size -= size;
    } else {
        data->out_of_data = true;
    }
    return result;
}



internal B32 ttf_parse_font_tables(Arena *arena, Str8 data, TTF_Font *font) {
    TTF_Parser parser = { 0 };
    parser.data = data.data;
    parser.size = data.size;

    U32 scaler_type    = ttf_read_u32(&parser);
    U16 num_tables     = ttf_read_u16(&parser);
    U16 search_range   = ttf_read_u16(&parser);
    U16 entry_selector = ttf_read_u16(&parser);
    U16 range_shift    = ttf_read_u16(&parser);

    if (parser.out_of_data) {
        log_error(str8_literal("Not enough data for offset subtable.\n"));
        num_tables = 0;
    }

    if (!(scaler_type == TTF_SCALER_TYPE_TRUE || scaler_type == TTF_SCALER_TYPE_1)) {
        log_error(str8_literal("Unknown scaler type.\n"));
        num_tables = 0;
    }

    // NOTE(simon): Parse table directory entries.
    for (U32 i = 0; i < num_tables; ++i) {
        U32 tag       = ttf_read_u32(&parser);
        U32 check_sum = ttf_read_u32(&parser);
        U32 offset    = ttf_read_u32(&parser);
        U32 length    = ttf_read_u32(&parser);

        if (parser.out_of_data) {
            log_error(str8_literal("Not enough data for offset subtable.\n"));
            break;
        }

        Str8 table_data = str8_substring(data, offset, length);

        if (table_data.size != length) {
            log_error(str8_literal("Table is referencing data outside of the file.\n"));
            continue;
        }

        // TODO(simon): Verify the check sum.

        for (U32 j = 0; j < TTF_Table_COUNT; ++j) {
            if (tag == ttf_table_tags[j]) {
                if (!font->tables[j].data) {
                    font->tables[j] = table_data;
                } else {
                    log_error(str8_literal("Duplicated entry in the table directory.\n"));
                }
                break;
            }
        }
    }

    // NOTE(simon): Verify that we have all required tables.
    B32 all_present = true;
    for (U32 i = 0; i < TTF_Table_MaxRequired; ++i) {
        all_present &= font->tables[i].data != 0;
    }

    if (!all_present) {
        log_error(str8_literal("Not all required tables are present.\n"));
    }

    return all_present;
}

internal TTF_HmtxMetrics ttf_get_metrics(TTF_Font *font, U32 glyph_index) {
    TTF_HmtxMetrics result = { 0 };

    if (glyph_index < font->glyph_count) {
        result = font->metrics[glyph_index];
    }

    return result;
}

internal Void ttf_codepoint_range_list_push(Arena *arena, TTF_CodepointRangeList *list, TTF_CodepointRange range) {
    if (range.size) {
        TTF_CodepointRangeNode *node = arena_push_struct_no_zero(arena, TTF_CodepointRangeNode);
        node->range = range;
        dll_push_back(list->first, list->last, node);
        ++list->range_count;
    }
}

internal U32 ttf_glyph_index_from_font_codepoint(TTF_Font *font, U32 codepoint) {
    TTF_CodepointMap map = font->codepoint_map;

    S64 low = 0;
    S64 high = (S64) map.range_count - 1;
    while (low < high) {
        S64 middle = (low + high) / 2;
        TTF_CodepointRange range = map.ranges[middle];

        if (codepoint < range.first_codepoint) {
            high = middle - 1;
        } else if (range.first_codepoint + range.size - 1 < codepoint) {
            low = middle + 1;
        } else {
            low = high = middle;
        }
    }

    TTF_CodepointRange range = { 0 };
    if (low == high) {
        range = map.ranges[low];
    }

    U32 glyph_index = 0;
    if (range.first_codepoint <= codepoint && codepoint < range.first_codepoint + range.size) {
        glyph_index = range.first_glyph_index + (codepoint - range.first_codepoint);
    }

    return glyph_index;
}

// TODO: Ensure that no codepoint is mapped to glyph index 0xFFFF.
// TODO: Unicode varition sequence subtables.
internal TTF_CodepointMap ttf_get_codepoint_map(Arena *arena, TTF_Font *font) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);
    Str8 cmap_data = font->tables[TTF_Table_Cmap];

    TTF_Parser cmap_parser = { 0 };
    cmap_parser.data = cmap_data.data;
    cmap_parser.size = cmap_data.size;

    U16 version        = ttf_read_u16(&cmap_parser);
    U16 subtable_count = ttf_read_u16(&cmap_parser);

    if (cmap_parser.out_of_data) {
        log_error(str8_literal("Not enough data for cmap table.\n"));
    }

    if (version != 0) {
        log_error(str8_literal("Unsupported version of cmap table.\n"));
    }

    // NOTE(simon): Pick the best subtable.
    U32 subtable_offset = 0;
    U32 rank = 0;
    for (U32 i = 0; i < subtable_count; ++i) {
        U16 platform_id          = ttf_read_u16(&cmap_parser);
        U16 platform_specifig_id = ttf_read_u16(&cmap_parser);
        U32 offset               = ttf_read_u32(&cmap_parser);

        if (cmap_parser.out_of_data) {
            log_error(str8_literal("Not enough data for cmap subtables.\n"));
            subtable_count = (U16) i;
            break;
        }

        U32 new_rank = 0;
        if (platform_id < TTF_CmapPlatform_COUNT && platform_specifig_id < TTF_CMAP_MAX_PLATFORM_SPECIFIC_ID) {
            new_rank = ttf_cmap_subtable_ids_to_rank[platform_id][platform_specifig_id];
        }

        if (new_rank > rank) {
            subtable_offset = offset;
            rank            = new_rank;
        }
    }

    // NOTE(simon): Acquire subtable data.
    TTF_Parser parser = { 0 };
    if (subtable_offset) {
        Str8 subtable_data = str8_skip(cmap_data, subtable_offset);
        parser.data = subtable_data.data;
        parser.size = subtable_data.size;
    } else {
        log_error(str8_literal("Could not find a suitable cmap subtable.\n"));
    }

    U32 format = ttf_read_u16(&parser);
    if (parser.out_of_data) {
        log_error(str8_literal("Not enough data for cmap subtable.\n"));
        format = U32_MAX;
    }

    // NOTE(simon): Collect subtable mappings into our own unified set of mapping.
    TTF_CodepointRangeList ranges = { 0 };
    switch (format) {
        case 0: {
            // TODO(simon): Should we use the length field for the parser?
            U16 length   = ttf_read_u16(&parser);
            U16 language = ttf_read_u16(&parser);

            // NOTE(simon): Build contiguous ranges.
            TTF_CodepointRange range = { 0 };
            for (U32 i = 0; i < 256; ++i) {
                U32 glyph_index = ttf_read_u8(&parser);

                if (range.size && range.first_glyph_index + range.size == glyph_index) {
                    // NOTE(simon): Extend the range.
                    ++range.size;
                } else {
                    ttf_codepoint_range_list_push(scratch.arena, &ranges, range);

                    // NOTE(simon): Start a new range.
                    range.first_codepoint = i;
                    range.first_glyph_index = glyph_index;
                    range.size = 1;
                }
            }

            // NOTE(simon): Push the last range which would not be handled by the above.
            ttf_codepoint_range_list_push(scratch.arena, &ranges, range);

            if (parser.out_of_data) {
                log_error(str8_literal("Not enough data for cmap format 0.\n"));
            }
        } break;
        case 2: {
            log_error(str8_literal("Cmap format 2 is not supported.\n"));
        } break;
        case 4: {
            U16 length         = ttf_read_u16(&parser);
            U16 language       = ttf_read_u16(&parser);
            U16 seg_count_x2   = ttf_read_u16(&parser);
            U16 search_range   = ttf_read_u16(&parser);
            U16 entry_selector = ttf_read_u16(&parser);
            U16 range_shift    = ttf_read_u16(&parser);

            U16 segment_count = seg_count_x2 / 2;

            TTF_Parser end_codes        = ttf_sub_parser(&parser, segment_count * sizeof(U16));
            U16 reserved_pad            = ttf_read_u16(&parser);
            TTF_Parser start_codes      = ttf_sub_parser(&parser, segment_count * sizeof(U16));
            TTF_Parser id_deltas        = ttf_sub_parser(&parser, segment_count * sizeof(U16));
            TTF_Parser id_range_offsets = ttf_sub_parser(&parser, segment_count * sizeof(U16));
            Str8 glyph_index_array      = ttf_read_bytes(&parser, parser.size);

            if (parser.out_of_data) {
                // NOTE(simon): It is more difficult to recover from running
                // out of data for this cmap. Don't do anything.
                log_error(str8_literal("Not enough data for cmap format 4.\n"));
                segment_count = 0;
            }

            for (U32 segment_index = 0; segment_index < segment_count; ++segment_index) {
                // NOTE(simon): The extra amount we need to move to get out of
                // the id_range_offset table and into the glyph_index_array.
                S32 extra_id_range_offset = (S32) id_range_offsets.size;

                U16 end_code        = ttf_read_u16(&end_codes);
                U16 start_code      = ttf_read_u16(&start_codes);
                U16 id_delta        = ttf_read_u16(&id_deltas);
                U16 id_range_offset = ttf_read_u16(&id_range_offsets);

                if (id_range_offset) {
                    for (U32 code = start_code; code <= end_code; ++code) {
                        S32 index = id_range_offset + (S32) ((code - start_code) * sizeof(U16)) - extra_id_range_offset;

                        U32 glyph_index = 0;
                        if (0 <= index && index + (S32) sizeof(U16) <= (S32) glyph_index_array.size) {
                            glyph_index = (U32) (glyph_index_array.data[index + 0] << 8 | glyph_index_array.data[index + 1] << 0);
                        } else {
                            log_error(str8_literal("Indexing out of bounds for cmap format 4.\n"));
                        }

                        // NOTE(simon): Only map codepoints that don't map to
                        // the missing glyph.
                        if (glyph_index) {
                            TTF_CodepointRange range = { 0 };
                            range.first_codepoint    = code;
                            range.first_glyph_index  = (glyph_index + id_delta) % 65536;
                            range.size               = 1;

                            ttf_codepoint_range_list_push(scratch.arena, &ranges, range);
                        }
                    }
                } else {
                    // TODO(simon): Add these as ranges directly.
                    for (U32 code = start_code; code <= end_code; ++code) {
                        TTF_CodepointRange range = { 0 };
                        range.first_codepoint    = code;
                        range.first_glyph_index  = (code + id_delta) % 65536;
                        range.size               = 1;

                        ttf_codepoint_range_list_push(scratch.arena, &ranges, range);
                    }
                }
            }
        } break;
        case 6: {
            // TODO(simon): Should we use the length field for the parser?
            U16 length      = ttf_read_u16(&parser);
            U16 language    = ttf_read_u16(&parser);
            U16 first_code  = ttf_read_u16(&parser);
            U16 entry_count = ttf_read_u16(&parser);

            // NOTE(simon): Build contiguous ranges.
            TTF_CodepointRange range = { 0 };
            for (U32 codepoint_offset = 0; codepoint_offset < entry_count; ++codepoint_offset) {
                U32 glyph_index = ttf_read_u16(&parser);

                if (range.size && range.first_glyph_index + range.size == glyph_index) {
                    // NOTE(simon): Extend the range.
                    ++range.size;
                } else {
                    ttf_codepoint_range_list_push(scratch.arena, &ranges, range);

                    // NOTE(simon): Start a new range.
                    range.first_codepoint = first_code + codepoint_offset;
                    range.first_glyph_index = glyph_index;
                    range.size = 1;
                }
            }

            // NOTE(simon): Push the last range which would not be handled by the above.
            ttf_codepoint_range_list_push(scratch.arena, &ranges, range);

            if (parser.out_of_data) {
                log_error(str8_literal("Not enough data for cmap format 6.\n"));
            }
        } break;
        case 8: {
            log_error(str8_literal("Cmap format 8 is not supported.\n"));
        } break;
        case 10: {
            log_error(str8_literal("Cmap format 10 is not supported.\n"));
        } break;
        case 12: {
            // TODO(simon): Should we use the length field for the parser?
            U16 reserved    = ttf_read_u16(&parser);
            U32 length      = ttf_read_u32(&parser);
            U32 language    = ttf_read_u32(&parser);
            U32 group_count = ttf_read_u32(&parser);

            // NOTE(simon): Convert group to ranges.
            for (U32 i = 0; i < group_count; ++i) {
                U32 start_char_code  = ttf_read_u32(&parser);
                U32 end_char_code    = ttf_read_u32(&parser);
                U32 start_glyph_code = ttf_read_u32(&parser);

                TTF_CodepointRange range = { 0 };
                range.first_codepoint   = start_char_code;
                range.first_glyph_index = start_glyph_code;
                range.size              = end_char_code - start_char_code + 1;
                ttf_codepoint_range_list_push(scratch.arena, &ranges, range);
            }

            if (parser.out_of_data) {
                log_error(str8_literal("Not enough data for cmap format 12.\n"));
            }
        } break;
        case 13: {
            log_error(str8_literal("Cmap format 13 is not supported.\n"));
        } break;
        case 14: {
            log_error(str8_literal("Cmap format 14 is not supported.\n"));
        } break;
        // TODO(simon): Switch U32_MAX and similar to be macros to allow use in switch cases.
        case 0xFFFFFFFF: {
            // NOTE(simon): Invalid cmap or we couldn't find a suitable one.
        } break;
        default: {
            log_error(str8_literal("Unknown cmap format.\n"));
        } break;
    }

    // NOTE(simon): Filter out mappings to the missing glyph.
    for (TTF_CodepointRangeNode *node = ranges.first, *next = 0; node; node = next) {
        next = node->next;

        // NOTE(simon): Mappings to the missing glyph can only be at the start
        // of a range.
        if (node->range.first_glyph_index == 0) {
            ++node->range.first_codepoint;
            ++node->range.first_glyph_index;
            --node->range.size;
        }

        // NOTE(simon): Remove empty ranges.
        if (node->range.size == 0) {
            dll_remove(ranges.first, ranges.last, node);
            --ranges.range_count;
        }
    }

    // TODO(simon): Sort ranges.

    // TODO(simon): Merge ranges

    // NOTE(simon): Copy to output.
    TTF_CodepointMap codepoint_map = { 0 };
    codepoint_map.ranges = arena_push_array(arena, TTF_CodepointRange, ranges.range_count);
    for (TTF_CodepointRangeNode *node = ranges.first; node; node = node->next, ++codepoint_map.range_count) {
        codepoint_map.ranges[codepoint_map.range_count] = node->range;
        codepoint_map.codepoint_count += node->range.size;
    }

    arena_end_temporary(scratch);
    return codepoint_map;
}

internal TTF_Glyph ttf_get_glyph_outlines(Arena *arena, TTF_Font *font, U32 glyph_index) {
    TTF_Glyph result = { 0 };

    TTF_Parser parser = { 0 };
    if (glyph_index < font->glyph_count) {
        Str8 raw_data = font->raw_glyph_data[glyph_index];
        parser.data = raw_data.data;
        parser.size = raw_data.size;
    }

    S16 contour_count = ttf_read_s16(&parser);
    // TODO(simon): Calculate these rather than reading this redundant data.
    result.min.x = ttf_read_s16(&parser);
    result.min.y = ttf_read_s16(&parser);
    result.max.x = ttf_read_s16(&parser);
    result.max.y = ttf_read_s16(&parser);

    if (parser.out_of_data) {
        log_error(str8_literal("Not enough data for glyph header.\n"));
    }

    if (contour_count > 0) {
        result.contour_count = contour_count;
        result.contour_end_points = arena_push_array_no_zero(arena, U16, (U64) result.contour_count);

        for (S32 contour_index = 0; contour_index < contour_count; ++contour_index) {
            result.contour_end_points[contour_index] = ttf_read_u16(&parser);
        }

        if (parser.out_of_data) {
            log_error(str8_literal("Not enough data for glyph contours.\n"));
            result.contour_count = 0;
        } else {
            result.point_count = result.contour_end_points[result.contour_count - 1] + 1;
        }

        result.point_flags       = arena_push_array_no_zero(arena, U8,    (U64) result.point_count);
        result.point_coordinates = arena_push_array_no_zero(arena, V2F32, (U64) result.point_count);

        // NOTE: Read instructions.
        U16  instruction_length = ttf_read_u16(&parser);
        Str8 instructions       = ttf_read_bytes(&parser, instruction_length);

        if (parser.out_of_data) {
            log_error(str8_literal("Not enough data for glyph instructions.\n"));
        }

        // NOTE(simon): Unpack flags.
        for (S32 point_index = 0; point_index < result.point_count;) {
            U8 flag = ttf_read_u8(&parser);

            S32 repeat_count = 1;
            if (flag & TTF_SimpleGlyphFlags_Repeat) {
                repeat_count += ttf_read_u8(&parser);
            }

            // NOTE(simon): Insert flags.
            if (point_index + repeat_count <= result.point_count) {
                for (S32 i = 0; i < repeat_count; ++i) {
                    result.point_flags[point_index++] = flag;
                }
            } else {
                log_error(str8_literal("Too many glyph point flags.\n"));
                break;
            }
        }

        if (parser.out_of_data) {
            log_error(str8_literal("Not enough data for glyph point flags.\n"));
            result.point_count = 0;
        }

        // NOTE(simon): Unpack X-coordinates.
        F32 previous_x = 0.0f;
        for (S32 point_index = 0; point_index < result.point_count; ++point_index) {
            U8 flag = result.point_flags[point_index];
            if (flag & TTF_SimpleGlyphFlags_ShortX) {
                U8 delta = ttf_read_u8(&parser);
                previous_x += (flag & TTF_SimpleGlyphFlags_SameOrPositiveX ? delta : -delta);
            } else if (!(flag & TTF_SimpleGlyphFlags_SameOrPositiveX)) {
                previous_x += ttf_read_s16(&parser);
            }
            result.point_coordinates[point_index].x = previous_x;
        }

        // NOTE(simon): Unpack Y-coordinates.
        F32 previous_y = 0.0f;
        for (S32 point_index = 0; point_index < result.point_count; ++point_index) {
            U8 flag = result.point_flags[point_index];
            if (flag & TTF_SimpleGlyphFlags_ShortY) {
                U8 delta = ttf_read_u8(&parser);
                previous_y += (flag & TTF_SimpleGlyphFlags_SameOrPositiveY ? delta : -delta);
            } else if (!(flag & TTF_SimpleGlyphFlags_SameOrPositiveY)) {
                previous_y += ttf_read_s16(&parser);
            }
            result.point_coordinates[point_index].y = previous_y;
        }

        if (parser.out_of_data) {
            log_error(str8_literal("Not enough data for glyph point coordinates.\n"));
            result.point_count = 0;
        }
    } else if (contour_count < 0) {
        Arena_Temporary scratch = arena_get_scratch(&arena, 1);

        typedef struct Component Component;
        struct Component {
            Component *next;
            Component *previous;
            TTF_Glyph  glyph;
            M3F32      transform;
            V2S32      alignment;
        };

        Component *first_component = 0;
        Component *last_component  = 0;
        S32 component_count        = 0;

        for (B32 has_more_components = true; has_more_components;) {
            Component *component = arena_push_struct(scratch.arena, Component);
            dll_push_back(first_component, last_component, component);
            ++component_count;

            component->transform = m3f32_identity();
            component->alignment = v2s32(-1, -1);

            U16 flags = ttf_read_u16(&parser);
            U16 component_glyph_index = ttf_read_u16(&parser);
            component->glyph = ttf_get_glyph_outlines(arena, font, component_glyph_index);

            if (parser.out_of_data) {
                log_error(str8_literal("Not enough data for glyph component flags.\n"));
            }

            B32 args_are_words = (flags & TTF_CompoundGlyphFlag_Arg1And2AreWords) != 0;

            if (flags & TTF_CompoundGlyphFlag_ArgsAreXYValues) {
                // NOTE(simon): Read offset.
                if (args_are_words) {
                    component->transform.m[0][2] = ttf_read_s16(&parser);
                    component->transform.m[1][2] = ttf_read_s16(&parser);
                } else {
                    component->transform.m[0][2] = ttf_read_s8(&parser);
                    component->transform.m[1][2] = ttf_read_s8(&parser);
                }
            } else {
                // NOTE(simon): Read alignment points.
                if (args_are_words) {
                    component->alignment.x = ttf_read_u16(&parser);
                    component->alignment.y = ttf_read_u16(&parser);
                } else {
                    component->alignment.x = ttf_read_u8(&parser);
                    component->alignment.y = ttf_read_u8(&parser);
                }
            }

            if (parser.out_of_data) {
                log_error(str8_literal("Not enough data for glyph component arguments.\n"));
            }

            B32 has_two_by_two    = (flags & TTF_CompoundGlyphFlag_WeHaveATwoByTwo) != 0;
            B32 has_x_and_y_scale = (flags & TTF_CompoundGlyphFlag_WeHaveAnXAndYScale) != 0;
            B32 has_scale         = (flags & TTF_CompoundGlyphFlag_WeHaveAScale) != 0;

            // NOTE(simon): Is more than one set?
            if (has_two_by_two + has_x_and_y_scale + has_scale > 1) {
                log_error(str8_literal("More than one component transformation is specified.\n"));
            }

            // NOTE(simon): Read component transform.
            if (has_two_by_two) {
                component->transform.m[0][0] = ttf_read_f2dot14_as_f32(&parser);
                component->transform.m[1][0] = ttf_read_f2dot14_as_f32(&parser);
                component->transform.m[0][1] = ttf_read_f2dot14_as_f32(&parser);
                component->transform.m[1][1] = ttf_read_f2dot14_as_f32(&parser);
            } else if (has_x_and_y_scale) {
                component->transform.m[0][0] = ttf_read_f2dot14_as_f32(&parser);
                component->transform.m[1][1] = ttf_read_f2dot14_as_f32(&parser);
            } else if (has_scale) {
                F32 scale = ttf_read_f2dot14_as_f32(&parser);
                component->transform.m[0][0] = scale;
                component->transform.m[1][1] = scale;
            }

            if (parser.out_of_data && (has_two_by_two || has_x_and_y_scale || has_scale)) {
                log_error(str8_literal("Not enough data for glyph component transform.\n"));
            }

            // NOTE(simon): Rescale offset.
            if (flags & TTF_CompoundGlyphFlag_ScaledComponentOffset) {
                F32 x_scale = v2f32_length(v2f32(component->transform.m[0][0], component->transform.m[0][1]));
                F32 y_scale = v2f32_length(v2f32(component->transform.m[1][0], component->transform.m[1][1]));
                component->transform.m[0][2] *= x_scale;
                component->transform.m[1][2] *= y_scale;
            }

            has_more_components = flags & TTF_CompoundGlyphFlag_MoreComponents;
        }

        // NOTE(simon): Flatten components into an array.
        result.components = arena_push_array(arena, TTF_Glyph, (U64) component_count);
        result.transforms = arena_push_array(arena, M3F32,     (U64) component_count);
        result.alignment  = arena_push_array(arena, V2S32,     (U64) component_count);
        for (Component *component = first_component; component; component = component->next) {
            result.components[result.component_count] = component->glyph;
            result.transforms[result.component_count] = component->transform;
            result.alignment[result.component_count]  = component->alignment;
            ++result.component_count;
        }

        arena_end_temporary(scratch);
    }

    return result;
}

internal TTF_Glyph ttf_flatten_glyph(Arena *arena, TTF_Glyph glyph) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    // NOTE(simon): Flatten all components and count contours and points.
    TTF_Glyph *components = arena_push_array(scratch.arena, TTF_Glyph, (U64) glyph.component_count);
    S32 contour_count = glyph.contour_count;
    S32 point_count   = glyph.point_count;
    for (S32 component_index = 0; component_index < glyph.component_count; ++component_index) {
        components[component_index] = ttf_flatten_glyph(scratch.arena, glyph.components[component_index]);
        contour_count += components[component_index].contour_count;
        point_count   += components[component_index].point_count;
    }

    TTF_Glyph result = { 0 };
    result.min                = glyph.min;
    result.max                = glyph.max;
    result.contour_end_points = arena_push_array(arena, U16,   (U64) contour_count);
    result.point_flags        = arena_push_array(arena, U8,    (U64) point_count);
    result.point_coordinates  = arena_push_array(arena, V2F32, (U64) point_count);

    // NOTE(simon): Copy over data from this glyph.
    memory_copy(result.contour_end_points, glyph.contour_end_points, (U64) glyph.contour_count * sizeof(*result.contour_end_points));
    memory_copy(result.point_flags,        glyph.point_flags,        (U64) glyph.point_count   * sizeof(*result.point_flags));
    memory_copy(result.point_coordinates,  glyph.point_coordinates,  (U64) glyph.point_count   * sizeof(*result.point_coordinates));
    result.contour_count += glyph.contour_count;
    result.point_count   += glyph.point_count;

    // NOTE(simon): Copy over data from components.
    for (S32 component_index = 0; component_index < glyph.component_count; ++component_index) {
        TTF_Glyph component = components[component_index];

        // NOTE(simon): Transform the component.
        for (S32 point_index = 0; point_index < component.point_count; ++point_index) {
            component.point_coordinates[point_index] = m3f32_multiply_v2f32(glyph.transforms[component_index], component.point_coordinates[point_index]);
        }

        // NOTE(simon): Align the component.
        if (glyph.alignment[component_index].x != -1 && glyph.alignment[component_index].y != -1) {
            S32 compound_point_index  = glyph.alignment[component_index].x;
            S32 component_point_index = glyph.alignment[component_index].y;

            if (compound_point_index < result.point_count && component_point_index < component.point_count) {
                V2F32 offset = v2f32_subtract(result.point_coordinates[compound_point_index], component.point_coordinates[component_point_index]);
                for (S32 point_index = 0; point_index < component.point_count; ++point_index) {
                    result.point_coordinates[point_index] = v2f32_add(result.point_coordinates[point_index], offset);
                }
            } else {
                log_error(str8_literal("Phantom points are not supported, glyph component alignemnt indicies are out of range.\n"));
            }
        }

        // NOTE(simon): Renumber contour end points.
        for (S32 contour_index = 0; contour_index < component.contour_count; ++contour_index) {
            component.contour_end_points[contour_index] += (U16) result.point_count;
        }

        // NOTE(simon): Copy over data to flattened glyph.
        memory_copy(result.contour_end_points + result.contour_count, component.contour_end_points, (U64) component.contour_count * sizeof(*result.contour_end_points));
        memory_copy(result.point_flags        + result.point_count,   component.point_flags,        (U64) component.point_count   * sizeof(*result.point_flags));
        memory_copy(result.point_coordinates  + result.point_count,   component.point_coordinates,  (U64) component.point_count   * sizeof(*result.point_coordinates));
        result.contour_count += component.contour_count;
        result.point_count   += component.point_count;
    }

    arena_end_temporary(scratch);

    return result;
}

internal MSDF_Glyph ttf_expand_contours_to_msdf(Arena *arena, TTF_Font *font, U32 glyph_index) {
    prof_function_begin();
    MSDF_Glyph result = { 0 };
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    TTF_Glyph raw_glyph = ttf_get_glyph_outlines(scratch.arena, font, glyph_index);
    TTF_Glyph glyph     = ttf_flatten_glyph(scratch.arena, raw_glyph);

    result.min = glyph.min;
    result.max = glyph.max;

    for (S32 contour_index = 0, point_index = 0; contour_index < glyph.contour_count; ++contour_index) {
        // FIXME(simon): Apparently there can be empty contours now? We need a
        // more robust parser in general, this is only a temporary solution to
        // a terrible problem.
        if ((glyph.contour_end_points[contour_index] - point_index) == 0) {
            point_index = glyph.contour_end_points[contour_index] + 1;
            continue;
        }

        MSDF_Contour *contour = arena_push_struct(arena, MSDF_Contour);

        U32 prev_index    = glyph.contour_end_points[contour_index] - 1;
        U32 current_index = glyph.contour_end_points[contour_index];

        V2F32 prev             = glyph.point_coordinates[prev_index];
        B32   prev_on_curve    = (glyph.point_flags[prev_index] & TTF_SimpleGlyphFlags_OnCurve);
        V2F32 current          = glyph.point_coordinates[current_index];
        B32   current_on_curve = (glyph.point_flags[current_index] & TTF_SimpleGlyphFlags_OnCurve);

        for (; point_index <= glyph.contour_end_points[contour_index]; ++point_index) {
            V2F32 next          = glyph.point_coordinates[point_index];
            B32   next_on_curve = (glyph.point_flags[point_index] & TTF_SimpleGlyphFlags_OnCurve);

            if (current_on_curve) {
                if (next_on_curve) {
                    MSDF_Segment *line = arena_push_struct(arena, MSDF_Segment);
                    line->kind  = MSDF_Segment_Line;
                    line->p0    = current;
                    line->p1    = next;
                    line->flags = 0;
                    dll_push_back(contour->first_segment, contour->last_segment, line);
                }
            } else {
                MSDF_Segment *bezier = arena_push_struct(arena, MSDF_Segment);
                bezier->kind  = MSDF_Segment_QuadraticBezier;
                bezier->p0    = (prev_on_curve ? prev : v2f32_scale(v2f32_add(prev, current), 0.5f));
                bezier->p1    = current;
                bezier->p2    = (next_on_curve ? next : v2f32_scale(v2f32_add(next, current), 0.5f));
                bezier->flags = 0;
                dll_push_back(contour->first_segment, contour->last_segment, bezier);

                if (points_are_collinear(bezier->p0, bezier->p1, bezier->p2)) {
                    bezier->kind = MSDF_Segment_Line;
                    bezier->p1   = bezier->p2;
                }
            }

            prev             = current;
            prev_on_curve    = current_on_curve;
            current          = next;
            current_on_curve = next_on_curve;
        }

        dll_push_back(result.first_contour, result.last_contour, contour);
    }

    arena_end_temporary(scratch);
    prof_function_end();
    return result;
}

internal TTF_Font *ttf_load(Arena *arena, Str8 font_path) {
    TTF_Font *font = arena_push_struct(arena, TTF_Font);
    Str8 font_data = { 0 };

    B32 good = true;

    if (good) {
        good = os_file_read(arena, font_path, &font_data);
        if (!good) {
            log_error(str8_literal("Could not read file.\n"));
        }
    }

    if (good) {
        good = ttf_parse_font_tables(arena, font_data, font);
    }

    S32 loca_format = S32_MAX;

    // NOTE(simon): Parse head table
    if (good) {
        Str8 head_data = font->tables[TTF_Table_Head];
        TTF_Parser parser = { 0 };
        parser.data = head_data.data;
        parser.size = head_data.size;

        TTF_Fixed        version              = ttf_read_u32(&parser);
        TTF_Fixed        font_revision        = ttf_read_u32(&parser);
        TTF_Fixed        check_sum_adjustment = ttf_read_u32(&parser);
        U32              magic_number         = ttf_read_u32(&parser);
        U16              flags                = ttf_read_u16(&parser);
        U16              units_per_em         = ttf_read_u16(&parser);
        TTF_LongDateTime created              = ttf_read_u64(&parser);
        TTF_LongDateTime modified             = ttf_read_u64(&parser);
        TTF_FWord        x_min                = ttf_read_s16(&parser);
        TTF_FWord        y_min                = ttf_read_s16(&parser);
        TTF_FWord        x_max                = ttf_read_s16(&parser);
        TTF_FWord        y_max                = ttf_read_s16(&parser);
        U16              mac_style            = ttf_read_u16(&parser);
        U16              lowest_rec_ppem      = ttf_read_u16(&parser);
        S16              font_direction_hint  = ttf_read_s16(&parser);
        S16              index_to_loc_format  = ttf_read_s16(&parser);
        S16              glyph_data_format    = ttf_read_s16(&parser);

        if (parser.out_of_data) {
            log_error(str8_literal("Not enough data in head table.\n"));
            good = false;
        }

        if (version != TTF_MAKE_VERSION(1, 0)) {
            log_error(str8_literal("Unsupported version of head table.\n"));
            good = false;
        }

        // TODO: Validate check_sum_adjustment

        if (magic_number != TTF_MAGIC_NUMBER) {
            log_error(str8_literal("Wrong magic number.\n"));
        }

        if ((flags & 0x0020) != 0) {
            log_error(str8_literal("Flags required to be unset are set.\n"));
        }

        if (!(64 <= units_per_em && units_per_em <= 16384)) {
            log_error(str8_literal("Invalid number of units per em.\n"));
        }

        if (!(-2 <= font_direction_hint && font_direction_hint <= 2)) {
            log_error(str8_literal("Invalid font direction hint.\n"));
        }

        if (glyph_data_format != 0) {
            log_error(str8_literal("Unknown glyph data format.\n"));
            good = false;
        }

        if (good) {
            font->funits_per_em   = units_per_em;
            font->lowest_rec_ppem = lowest_rec_ppem;
            loca_format           = index_to_loc_format;
        }
    }

    // NOTE(simon): Parse maxp table.
    if (good) {
        Str8 maxp_data = font->tables[TTF_Table_Maxp];
        TTF_Parser parser = { 0 };
        parser.data = maxp_data.data;
        parser.size = maxp_data.size;

        TTF_Fixed version            = ttf_read_u32(&parser);
        U16 num_glyphs               = ttf_read_u16(&parser);
        U16 max_points               = ttf_read_u16(&parser);
        U16 max_contours             = ttf_read_u16(&parser);
        U16 max_component_points     = ttf_read_u16(&parser);
        U16 max_component_contours   = ttf_read_u16(&parser);
        U16 max_zones                = ttf_read_u16(&parser);
        U16 max_twilight_points      = ttf_read_u16(&parser);
        U16 max_storage              = ttf_read_u16(&parser);
        U16 max_function_defs        = ttf_read_u16(&parser);
        U16 max_instruction_defs     = ttf_read_u16(&parser);
        U16 max_stack_elements       = ttf_read_u16(&parser);
        U16 max_size_of_instructions = ttf_read_u16(&parser);
        U16 max_component_elements   = ttf_read_u16(&parser);
        U16 max_component_depth      = ttf_read_u16(&parser);

        if (parser.out_of_data) {
            log_error(str8_literal("Not enough data in maxp table.\n"));
            good = false;
        }

        if (version != TTF_MAKE_VERSION(1, 0)) {
            log_error(str8_literal("Unsupported version of maxp table.\n"));
            good = false;
        }

        if (!(1 <= max_zones && max_zones <= 2)) {
            log_error(str8_literal("Max zones must between 1 and 2 inclusive.\n"));
        }

        if (max_component_depth > 16) {
            log_error(str8_literal("Max component depth is outside of the legal range.\n"));
        }

        if (good) {
            font->glyph_count = num_glyphs;
        }
    }

    // NOTE(simon): Get glyph data ranges.
    if (good) {
        font->raw_glyph_data = arena_push_array(arena, Str8, font->glyph_count);

        Str8 loca_data = font->tables[TTF_Table_Loca];
        Str8 glyf_data = font->tables[TTF_Table_Glyf];

        // NOTE(simon): Extract glyph locations.
        TTF_Parser parser = { 0 };
        parser.data = loca_data.data;
        parser.size = loca_data.size;
        if (loca_format == 0) {
            U32 start = 2 * ttf_read_u16(&parser);
            for (S64 i = 0; i < font->glyph_count; ++i) {
                U32 end = 2 * ttf_read_u16(&parser);
                if (parser.out_of_data) {
                    log_error(str8_literal("Not enough data for short loca table.\n"));
                    break;
                }

                Str8 data = str8_substring(glyf_data, start, end - start);

                if (start > end) {
                    log_error(str8_literal("Invalid short loca range (start must be less than end).\n"));
                    data.size = 0;
                }

                if (end > glyf_data.size) {
                    log_error(str8_literal("Not enough data for glyf table.\n"));
                    data.size = 0;
                }

                font->raw_glyph_data[i] = data;
                start = end;
            }
        } else if (loca_format == 1) {
            U32 start = ttf_read_u32(&parser);
            for (S64 i = 0; i < font->glyph_count; ++i) {
                U32 end = ttf_read_u32(&parser);
                if (parser.out_of_data) {
                    log_error(str8_literal("Not enough data for long loca table.\n"));
                    break;
                }

                Str8 data = str8_substring(glyf_data, start, end - start);

                if (start > end) {
                    log_error(str8_literal("Invalid long loca range (start must be less than end).\n"));
                    data.size = 0;
                }

                if (end > glyf_data.size) {
                    log_error(str8_literal("Not enough data for glyf table.\n"));
                    data.size = 0;
                }

                font->raw_glyph_data[i] = data;
                start = end;
            }
        } else if (loca_format == S32_MAX) {
            log_error(str8_literal("Not enough data in head table to read loca format.\n"));
        } else {
            log_error(str8_literal("Unknown index to location format.\n"));
        }
    }

    U32 advance_width_count = 0;

    // NOTE(simon): Parse metrics.
    if (good) {
        Str8 hhea_data = font->tables[TTF_Table_Hhea];
        TTF_Parser hhea_parser = { 0 };
        hhea_parser.data = hhea_data.data;
        hhea_parser.size = hhea_data.size;

        TTF_Fixed  version                 = ttf_read_u32(&hhea_parser);
        TTF_FWord  ascent                  = ttf_read_s16(&hhea_parser);
        TTF_FWord  descent                 = ttf_read_s16(&hhea_parser);
        TTF_FWord  line_gap                = ttf_read_s16(&hhea_parser);
        TTF_UFWord advance_width_max       = ttf_read_u16(&hhea_parser);
        TTF_FWord  min_left_side_bearing   = ttf_read_s16(&hhea_parser);
        TTF_FWord  min_right_side_bearing  = ttf_read_s16(&hhea_parser);
        TTF_FWord  x_max_extent            = ttf_read_s16(&hhea_parser);
        S16        caret_slope_rise        = ttf_read_s16(&hhea_parser);
        S16        caret_slope_run         = ttf_read_s16(&hhea_parser);
        TTF_FWord  caret_offset            = ttf_read_s16(&hhea_parser);
        S16        reserved0               = ttf_read_s16(&hhea_parser);
        S16        reserved1               = ttf_read_s16(&hhea_parser);
        S16        reserved2               = ttf_read_s16(&hhea_parser);
        S16        reserved3               = ttf_read_s16(&hhea_parser);
        S16        metric_data_format      = ttf_read_s16(&hhea_parser);
        U16        num_of_long_hor_metrics = ttf_read_u16(&hhea_parser);

        if (hhea_parser.out_of_data) {
            log_error(str8_literal("Not enough data in hhea table.\n"));
            good = false;
        }

        if (version != TTF_MAKE_VERSION(1, 0)) {
            log_error(str8_literal("Unsupported version of hhea table.\n"));
            good = false;
        }

        if (caret_slope_rise == 0 && caret_slope_run == 0) {
            log_error(str8_literal("Both the caret slopes rise and run are 0.\n"));
        }

        if (metric_data_format != 0) {
            log_error(str8_literal("Unknown metric data format.\n"));
            good = false;
        }

        if (advance_width_max == 0) {
            log_error(str8_literal("There must be at least one long-form entry in the hmtx table.\n"));
            good = false;
        }

        if (good) {
            advance_width_count = advance_width_max;
        }
    }

    // NOTE(simon): Parse horizontal metrics.
    if (good) {
        Str8 hmtx_data = font->tables[TTF_Table_Hmtx];
        TTF_Parser hmtx_parser = { 0 };
        hmtx_parser.data = hmtx_data.data;
        hmtx_parser.size = hmtx_data.size;

        font->metrics = arena_push_array(arena, TTF_HmtxMetrics, font->glyph_count);

        // NOTE(simon): Read both advance width and left side bearing.
        for (U32 glyph_index = 0; glyph_index < advance_width_count; ++glyph_index) {
            font->metrics[glyph_index].advance_width     = ttf_read_u16(&hmtx_parser);
            font->metrics[glyph_index].left_side_bearing = ttf_read_s16(&hmtx_parser);
        }

        // NOTE(simon): Only read left side bearing
        for (U32 glyph_index = advance_width_count; glyph_index < font->glyph_count; ++glyph_index) {
            font->metrics[glyph_index].advance_width     = font->metrics[advance_width_count - 1].advance_width;
            font->metrics[glyph_index].left_side_bearing = ttf_read_s16(&hmtx_parser);
        }

        if (hmtx_parser.out_of_data) {
            log_error(str8_literal("Not enough data in hmtx table.\n"));
        }
    }

    if (good) {
        font->codepoint_map = ttf_get_codepoint_map(arena, font);
    }

    return font;
}
