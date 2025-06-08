internal Elf_Symbol *elf_create_symbol(Arena *arena, Elf_Object *object, Str8 name) {
    Elf_Symbol *symbol = arena_push_struct(arena, Elf_Symbol);
    symbol->name = name;

    dll_push_back(object->first_symbol, object->last_symbol, symbol);
    ++object->symbol_count;

    return symbol;
}

internal Elf_Section *elf_create_section(Arena *arena, Elf_Object *object, Str8 name, Elf_SectionHeaderType type) {
    Elf_Section *section = arena_push_struct(arena, Elf_Section);
    section->name = name;
    section->type = type;

    dll_push_back(object->first_section, object->last_section, section);
    ++object->section_count;

    return section;
}

internal U64 elf_section_index_from_name(Elf_Object *object, Str8 name) {
    U64 result = 0;

    U64 index = 1;
    for (Elf_Section *section = object->first_section; section; section = section->next, ++index) {
        if (str8_equal(section->name, name)) {
            result = index;
            break;
        }
    }

    return result;
}

internal Str8List elf_generate(Arena *arena, Elf_Object *object) {
    // NOTE(simon): Layout symbols.
    for (Elf_Section *section = object->first_section; section; section = section->next) {
        U64 offset = 0;
        for (Elf_Symbol *symbol = object->first_symbol; symbol; symbol = symbol->next) {
            if (str8_equal(section->name, symbol->section_name)) {
                U64 aligned_offset = u64_round_up_to_power_of_2(offset, symbol->align);

                // NOTE(simon): Padding for alignment.
                if (offset != aligned_offset) {
                    U8 *padding = arena_push_array(arena, U8, aligned_offset - offset);
                    str8_list_push(arena, &section->data, str8(padding, aligned_offset - offset));
                }

                symbol->offset = aligned_offset;

                str8_list_push(arena, &section->data, symbol->data);
                section->address_align = u64_max(section->address_align, symbol->align);

                offset = aligned_offset + symbol->data.size;
            }
        }
    }

    // NOTE(simon): Create symbol string table.
    Elf_Section *string_table_section = elf_create_section(arena, object, str8_literal(".strtab"), Elf_SectionHeaderType_StringTable);
    string_table_section->address_align = 1;

    U32 symbol_name_buffer_size = 1;
    for (Elf_Symbol *symbol = object->first_symbol; symbol; symbol = symbol->next) {
        symbol->name_index = symbol_name_buffer_size;
        symbol_name_buffer_size += symbol->name.size + 1;
    }
    U8 *symbol_name_buffer = arena_push_array(arena, U8, symbol_name_buffer_size);
    for (Elf_Symbol *symbol = object->first_symbol; symbol; symbol = symbol->next) {
        memory_copy(&symbol_name_buffer[symbol->name_index], symbol->name.data, symbol->name.size);
    }
    str8_list_push(arena, &string_table_section->data, str8(symbol_name_buffer, symbol_name_buffer_size));

    // NOTE(simon): Create symbols.
    U64 elf_symbol_count = 1 + object->symbol_count;
    Elf64_Symbol *elf_symbols = arena_push_array(arena, Elf64_Symbol, elf_symbol_count);
    U64 elf_symbol_index = 1;
    for (Elf_Symbol *symbol = object->first_symbol; symbol; symbol = symbol->next) {
        Elf64_Symbol *elf_symbol = &elf_symbols[elf_symbol_index];
        elf_symbol->name          = symbol->name_index;
        elf_symbol->info          = ELF_SYMBOL_INFO_FROM_BINDING_TYPE(Elf_SymbolBinding_Global, Elf_SymbolType_Object);
        elf_symbol->other         = Elf_SymbolVisibility_Default;
        elf_symbol->section_index = (U16) elf_section_index_from_name(object, symbol->section_name);;
        elf_symbol->value         = symbol->offset;
        elf_symbol->size          = symbol->data.size;

        ++elf_symbol_index;
    }

    Elf_Section *symbol_table_section = elf_create_section(arena, object, str8_literal(".symtab"), Elf_SectionHeaderType_SymbolTable);
    symbol_table_section->link_name     = str8_literal(".strtab");
    symbol_table_section->info          = 1; // Last local symbol index + 1
    symbol_table_section->address_align = _Alignof(Elf64_Symbol);
    symbol_table_section->entry_size    = sizeof(Elf64_Symbol);
    str8_list_push(arena, &symbol_table_section->data, str8((U8 *) elf_symbols, elf_symbol_count * sizeof(Elf64_Symbol)));

    // NOTE(simon): Compute section string table.
    Elf_Section *section_header_string_table_section = elf_create_section(arena, object, str8_literal(".shstrtab"), Elf_SectionHeaderType_StringTable);
    section_header_string_table_section->address_align = 1;

    U32 section_name_buffer_size = 1;
    for (Elf_Section *section = object->first_section; section; section = section->next) {
        section->name_index = section_name_buffer_size;
        section_name_buffer_size += section->name.size + 1;
    }
    U8 *section_name_buffer = arena_push_array(arena, U8, section_name_buffer_size);
    for (Elf_Section *section = object->first_section; section; section = section->next) {
        memory_copy(&section_name_buffer[section->name_index], section->name.data, section->name.size);
    }
    str8_list_push(arena, &section_header_string_table_section->data, str8(section_name_buffer, section_name_buffer_size));

    // NOTE(simon): Layout section content.
    {
        U64 total_offset = sizeof(Elf64_Header) + (1 + object->section_count) * sizeof(Elf64_SectionHeader);
        for (Elf_Section *section = object->first_section; section; section = section->next) {
            section->offset = total_offset;
            total_offset += section->data.total_size;
        }
    }

    // NOTE(simon): Create sections.
    U64 elf_section_count = 1 + object->section_count;
    Elf64_SectionHeader *elf_sections = arena_push_array(arena, Elf64_SectionHeader, elf_section_count);
    U64 elf_section_index = 1;
    for (Elf_Section *section = object->first_section; section; section = section->next) {
        Elf64_SectionHeader *elf_section = &elf_sections[elf_section_index];
        elf_section->name          = section->name_index;
        elf_section->type          = section->type;
        elf_section->flags         = section->flags;
        elf_section->address       = section->address;
        elf_section->offset        = section->offset;
        elf_section->size          = section->data.total_size;
        elf_section->link          = (U32) elf_section_index_from_name(object, section->link_name);
        elf_section->info          = section->info;
        elf_section->address_align = section->address_align;
        elf_section->entry_size    = section->entry_size;

        ++elf_section_index;
    }

    // NOTE(simon): Header
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
    header->section_header_count        = (U16) elf_section_count;
    header->section_header_string_index = (U16) elf_section_index_from_name(object, str8_literal(".shstrtab"));

    // NOTE(simon): Output
    Str8List output = { 0 };
    str8_list_push(arena, &output, str8((U8 *) header, sizeof(*header)));
    str8_list_push(arena, &output, str8((U8 *) elf_sections, elf_section_count * sizeof(Elf64_SectionHeader)));
    for (Elf_Section *section = object->first_section; section; section = section->next) {
        for (Str8Node *data = section->data.first; data; data = data->next) {
            str8_list_push(arena, &output, data->string);
        }
    }

    return output;
}
