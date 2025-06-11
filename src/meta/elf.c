internal Str8List elf_binary_from_object(Arena *arena, Object object) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    enum {
        Section_Null,
        Section_SectionStringTable,
        Section_SymbolTable,
        Section_SymbolStringTable,
        Section_COUNT,
    };

    // NOTE(simon): Flatten sections and symbols.
    U32 symbol_count = 1 + (U32) object.symbol_count;
    Object_Symbol *symbols = arena_push_array(scratch.arena, Object_Symbol, symbol_count);
    for (Object_Symbol *symbol = object.first_symbol, *ptr = &symbols[1]; symbol; symbol = symbol->next, ++ptr) {
        *ptr = *symbol;
    }
    U32 section_count = Section_COUNT + (U32) object.section_count;
    Object_Section *sections = arena_push_array(scratch.arena, Object_Section, section_count);
    for (Object_Section *section = object.first_section, *ptr = &sections[4]; section; section = section->next, ++ptr) {
        *ptr = *section;
    }

    // NOTE(simon): Set names for extra sections.
    sections[Section_SectionStringTable].name  = str8_literal(".shstrtab");
    sections[Section_SymbolTable].name         = str8_literal(".symtab");
    sections[Section_SymbolStringTable].name   = str8_literal(".strtab");

    // NOTE(simon): Layout symbol data.
    Str8List *section_data       = arena_push_array(scratch.arena, Str8List, section_count);
    U32 *section_alignments      = arena_push_array(scratch.arena, U32, section_count);
    U16 *symbol_section_indicies = arena_push_array(scratch.arena, U16, symbol_count);
    U64 *symbol_data_offsets     = arena_push_array(scratch.arena, U64, symbol_count);
    for (U64 symbol_index = 0; symbol_index < symbol_count; ++symbol_index) {
        Object_Symbol *symbol = &symbols[symbol_index];

        // NOTE(simon): Find section.
        U64 section_index = 0;
        for (; section_index < section_count; ++section_index) {
            if (str8_equal(sections[section_index].name, symbol->section_name)) {
                break;
            }
        }

        if (section_index < section_count) {
            symbol_section_indicies[symbol_index] = (U16) section_index;
            section_alignments[section_index] = u32_max(section_alignments[section_index], symbol->align);

            // NOTE(simon): Generate padding.
            U64 alignment = section_data[section_index].total_size & (symbol->align - 1);
            U64 padding_size = symbol->align - alignment;
            if (alignment && padding_size) {
                U8 *padding_bytes = arena_push_array(arena, U8, padding_size);
                str8_list_push(scratch.arena, &section_data[section_index], str8(padding_bytes, padding_size));
            }

            symbol_data_offsets[symbol_index] = section_data[section_index].total_size;
            str8_list_push(scratch.arena, &section_data[section_index], symbol->data);
        } else if (symbol->name.size != 0) {
            log_error_format("Symbol '%.*s' refers to nonexistent section '%.*s'.\n", str8_expand(symbol->name), str8_expand(symbol->section_name));
        }
    }

    // NOTE(simon): Build symbol names.
    U32 *symbol_name_offsets = arena_push_array_no_zero(scratch.arena, U32, symbol_count);
    U64 symbol_string_table_size = 0;
    for (U64 i = 0; i < symbol_count; ++i) {
        symbol_name_offsets[i] = (U32) symbol_string_table_size;
        symbol_string_table_size += symbols[i].name.size + 1;
    }
    U8 *symbol_string_table = arena_push_array_no_zero(arena, U8, symbol_string_table_size);
    for (U64 i = 0; i < symbol_count; ++i) {
        memory_copy(&symbol_string_table[symbol_name_offsets[i]], symbols[i].name.data, symbols[i].name.size);
        symbol_string_table[symbol_name_offsets[i] + symbols[i].name.size] = 0;
    }

    // NOTE(simon): Build section names.
    U32 *section_name_offsets = arena_push_array_no_zero(scratch.arena, U32, section_count);
    U64 section_string_table_size = 0;
    for (U64 i = 0; i < section_count; ++i) {
        section_name_offsets[i] = (U32) section_string_table_size;
        section_string_table_size += sections[i].name.size + 1;
    }
    U8 *section_string_table = arena_push_array_no_zero(arena, U8, section_string_table_size);
    for (U64 i = 0; i < section_count; ++i) {
        memory_copy(&section_string_table[section_name_offsets[i]], sections[i].name.data, sections[i].name.size);
        section_string_table[section_name_offsets[i] + sections[i].name.size] = 0;
    }

    // NOTE(simon): Build symbols.
    Elf64_Symbol *elf_symbols = arena_push_array(arena, Elf64_Symbol, symbol_count);
    for (U64 i = 1; i < symbol_count; ++i) {
        Object_Symbol *symbol     = &symbols[i];
        Elf64_Symbol  *elf_symbol = &elf_symbols[i];

        elf_symbol->name          = symbol_name_offsets[i];
        elf_symbol->info          = ELF_SYMBOL_INFO_FROM_BINDING_TYPE(Elf_SymbolBinding_Global, Elf_SymbolType_Object);
        elf_symbol->other         = Elf_SymbolVisibility_Default;
        elf_symbol->section_index = symbol_section_indicies[i];
        elf_symbol->value         = symbol_data_offsets[i];
        elf_symbol->size          = symbol->data.size;
    }

    // NOTE(simon): Attach data to extra sections.
    str8_list_push(scratch.arena, &section_data[Section_SectionStringTable], str8(section_string_table, section_string_table_size));
    str8_list_push(scratch.arena, &section_data[Section_SymbolTable],        str8((U8 *) elf_symbols, symbol_count * sizeof(Elf64_Symbol)));
    section_alignments[Section_SymbolTable] = _Alignof(Elf64_Symbol);
    str8_list_push(scratch.arena, &section_data[Section_SymbolStringTable],  str8(symbol_string_table, symbol_string_table_size));

    // NOTE(simon): Build sections.
    Elf64_SectionHeader *elf_sections = arena_push_array(arena, Elf64_SectionHeader, section_count);
    U64 section_data_offset = sizeof(Elf64_Header) + section_count * sizeof(Elf64_SectionHeader);
    for (U64 i = 1; i < section_count; ++i) {
        Object_Section      *section     = &sections[i];
        Elf64_SectionHeader *elf_section = &elf_sections[i];

        elf_section->name          = section_name_offsets[i];
        elf_section->type          = Elf_SectionHeaderType_ProgramBits;
        elf_section->flags         = Elf_SectionHeaderFlag_Allocate;
        elf_section->offset        = section_data_offset;
        elf_section->size          = section_data[i].total_size;
        elf_section->address_align = section_alignments[i];

        if (section->flags & Object_SectionFlag_Write) {
            elf_section->flags |= Elf_SectionHeaderFlag_Write;
        }

        section_data_offset += section_data[i].total_size;
    }

    // NOTE(simon): Set data for extra sections.
    elf_sections[Section_SectionStringTable].type = Elf_SectionHeaderType_StringTable;
    elf_sections[Section_SymbolTable].type        = Elf_SectionHeaderType_SymbolTable;
    elf_sections[Section_SymbolTable].entry_size  = sizeof(Elf64_Symbol);
    elf_sections[Section_SymbolTable].link        = Section_SymbolStringTable;
    elf_sections[Section_SymbolTable].info        = 1; // NOTE(simon): Last local symbol index + 1.
    elf_sections[Section_SymbolStringTable].type  = Elf_SectionHeaderType_StringTable;

    // NOTE(simon): Build header.
    Elf64_Header *header = arena_push_struct(arena, Elf64_Header);
    header->identification[Elf_HeaderIdentification_Magic0]     = 0x7F;
    header->identification[Elf_HeaderIdentification_Magic1]     = 'E';
    header->identification[Elf_HeaderIdentification_Magic2]     = 'L';
    header->identification[Elf_HeaderIdentification_Magic3]     = 'F';
    header->identification[Elf_HeaderIdentification_Class]      = Elf_HeaderClass_64;
    header->identification[Elf_HeaderIdentification_Data]       = Elf_HeaderData_2Lsb;
    header->identification[Elf_HeaderIdentification_Version]    = Elf_HeaderVersion_Current;
    header->identification[Elf_HeaderIdentification_OsAbi]      = Elf_OsAbi_None;
    header->identification[Elf_HeaderIdentification_AbiVersion] = 0;
    header->type                        = Elf_HeaderType_Relocatable;
    header->machine                     = Elf_HeaderMachine_X86_64;
    header->version                     = Elf_HeaderVersion_Current;
    header->section_header_offset       = sizeof(Elf64_Header);
    header->header_size                 = sizeof(Elf64_Header);
    header->section_header_entry_size   = sizeof(Elf64_SectionHeader);
    header->section_header_count        = (U16) section_count;
    header->section_header_string_index = Section_SectionStringTable;

    // NOTE(simon): Build output.
    Str8List output = { 0 };
    str8_list_push(arena, &output, str8((U8 *) header, sizeof(*header)));
    str8_list_push(arena, &output, str8((U8 *) elf_sections, section_count * sizeof(Elf64_SectionHeader)));
    for (U64 i = 0; i < section_count; ++i) {
        for (Str8Node *node = section_data[i].first; node; node = node->next) {
            str8_list_push(arena, &output, node->string);
        }
    }

    arena_end_temporary(scratch);
    return output;
}
