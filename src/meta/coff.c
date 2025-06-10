internal Str8List coff_binary_from_object(Arena *arena, Object object) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    // NOTE(simon): Flatten sections and symbols.
    Object_Symbol *symbols = arena_push_array_no_zero(scratch.arena, Object_Symbol, object.symbol_count);
    for (Object_Symbol *symbol = object.first_symbol, *ptr = symbols; symbol; symbol = symbol->next, ++ptr) {
        *ptr = *symbol;
    }
    Object_Section *sections = arena_push_array_no_zero(scratch.arena, Object_Section, object.section_count);
    for (Object_Section *section = object.first_section, *ptr = sections; section; section = section->next, ++ptr) {
        *ptr = *section;
    }

    // NOTE(simon): Layout symbol data.
    Str8List *section_data       = arena_push_array(scratch.arena, Str8List, object.section_count);
    U32 *section_alignments      = arena_push_array(scratch.arena, U32, object.section_count);
    S16 *symbol_section_indicies = arena_push_array(scratch.arena, S16, object.symbol_count);
    U32 *symbol_data_offsets     = arena_push_array(scratch.arena, U32, object.symbol_count);
    for (U64 symbol_index = 0; symbol_index < object.symbol_count; ++symbol_index) {
        Object_Symbol *symbol = &symbols[symbol_index];

        // NOTE(simon): Find section.
        U64 section_index = 0;
        for (; section_index < object.section_count; ++section_index) {
            if (str8_equal(sections[section_index].name, symbol->section_name)) {
                break;
            }
        }

        if (section_index < object.section_count) {
            symbol_section_indicies[symbol_index] = 1 + (S16) section_index; // NOTE(simon): 1 based indexing.
            section_alignments[section_index] = u32_max(section_alignments[section_index], symbol->align);

            // NOTE(simon): Generate padding.
            U64 alignment = section_data[section_index].total_size & (symbol->align - 1);
            U64 padding_size = symbol->align - alignment;
            if (alignment && padding_size) {
                U8 *padding_bytes = arena_push_array(arena, U8, padding_size);
                str8_list_push(scratch.arena, &section_data[section_index], str8(padding_bytes, padding_size));
            }

            symbol_data_offsets[symbol_index] = (U32) section_data[section_index].total_size;
            str8_list_push(scratch.arena, &section_data[section_index], symbol->data);
        } else {
            log_error_format("Symbol '%.*s' refers to nonexistent section '%.*s'.\n", str8_expand(symbol->name), str8_expand(symbol->section_name));
        }
    }

    // NOTE(simon): Build symbol names.
    U32 *symbol_name_offsets = arena_push_array_no_zero(scratch.arena, U32, object.symbol_count);
    U32 string_table_size = sizeof(U32); // NOTE(simon): Size of the string table header.
    for (U64 i = 0; i < object.symbol_count; ++i) {
        symbol_name_offsets[i] = string_table_size;
        string_table_size += symbols[i].name.size + 1; // NOTE(simon): Include the null-terminator.
    }
    U8 *string_table = arena_push_array_no_zero(arena, U8, string_table_size);
    memory_copy(string_table, &string_table_size, sizeof(string_table_size));
    for (U64 i = 0; i < object.symbol_count; ++i) {
        memory_copy(&string_table[symbol_name_offsets[i]], symbols[i].name.data, symbols[i].name.size);
        string_table[symbol_name_offsets[i] + symbols[i].name.size] = 0;
    }

    // NOTE(simon): Build symbols.
    Coff_Symbol *coff_symbols = arena_push_array(arena, Coff_Symbol, object.symbol_count);
    for (U64 i = 0; i < object.symbol_count; ++i) {
        Object_Symbol *symbol      = &symbols[i];
        Coff_Symbol   *coff_symbol = &coff_symbols[i];

        coff_symbol->name_offset           = symbol_name_offsets[i];
        coff_symbol->value                 = symbol_data_offsets[i];
        coff_symbol->section_number        = symbol_section_indicies[i];
        coff_symbol->type                  = Coff_SymbolType_Null;
        coff_symbol->storage_class         = Coff_SymbolStorageClass_External;
        coff_symbol->auxilary_symbol_count = 0;
    }

    // NOTE(simon): Build sections.
    Coff_Section *coff_sections = arena_push_array(arena, Coff_Section, object.section_count);
    U32 section_data_offset = (U32) (sizeof(Coff_Header) + object.section_count * sizeof(Coff_Section) + object.symbol_count * sizeof(Coff_Symbol) + string_table_size);
    for (U64 i = 0; i < object.section_count; ++i) {
        Object_Section *section      = &sections[i];
        Coff_Section   *coff_section = &coff_sections[i];

        if (section->name.size <= sizeof(coff_section->name)) {
            memory_copy(coff_section->name, section->name.data, section->name.size);
        } else {
            log_error_format("Section name '%.*s' is too long.\n", str8_expand(section->name));
        }
        if (section->name.size < sizeof(coff_section->name)) {
            coff_section->name[section->name.size] = 0;
        }

        U32 alignment = 0;
        if (section_alignments[i]) {
            alignment = 1 + u32_count_trailing_zeros(section_alignments[i]);
        }

        coff_section->raw_data_size   = (U32) section_data[i].total_size;
        coff_section->raw_data_offset = section_data_offset;
        coff_section->characteristics = Coff_SectionFlag_InitializedData | (alignment << 20) | Coff_SectionFlag_Read;
        if (section->flags & Object_SectionFlag_Write) {
            coff_section->characteristics |= Coff_SectionFlag_Write;
        }

        section_data_offset += section_data[i].total_size;
    }

    // NOTE(simon): Build header.
    Coff_Header *header = arena_push_struct(arena, Coff_Header);
    header->machine             = Coff_HeaderMachine_X64;
    header->section_count       = (U16) object.section_count;
    header->time_date_stamp     = 0; // TODO(simon): Maybe set an actual timestamp?
    header->symbol_table_offset = (U32) (sizeof(Coff_Header) + object.section_count * sizeof(Coff_Section));
    header->symbol_count        = (U32) object.symbol_count;
    header->characteristics     = 0; // TODO(simon): We might care about IMAGE_FILE_DEBUG_STRIPPED (0x0800)

    // NOTE(simon): Build output.
    Str8List output = { 0 };
    str8_list_push(arena, &output, str8((U8 *) header, sizeof(*header)));
    str8_list_push(arena, &output, str8((U8 *) coff_sections, object.section_count * sizeof(Coff_Section)));
    str8_list_push(arena, &output, str8((U8 *) coff_symbols,  object.symbol_count * sizeof(Coff_Symbol)));
    str8_list_push(arena, &output, str8(string_table, string_table_size));
    for (U64 i = 0; i < object.section_count; ++i) {
        for (Str8Node *node = section_data[i].first; node; node = node->next) {
            str8_list_push(arena, &output, node->string);
        }
    }

    arena_end_temporary(scratch);
    return output;
}
