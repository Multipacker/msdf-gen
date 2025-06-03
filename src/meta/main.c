#include "src/base/base_include.h"

#include "src/base/base_include.c"

typedef U64 Elf64_Addr;
typedef U64 Elf64_Off;
typedef U16 Elf64_Section;
typedef U16 Elf64_Versym;
typedef U8  Elf_Byte;
typedef U16 Elf64_Half;
typedef S32 Elf64_Sword;
typedef U32 Elf64_Word;
typedef S64 Elf64_Sxword;
typedef U64 Elf64_Xword;

typedef enum {
    Elf_HeaderIdentification_Magic0,
    Elf_HeaderIdentification_Magic1,
    Elf_HeaderIdentification_Magic2,
    Elf_HeaderIdentification_Magic3,
    Elf_HeaderIdentification_Class,
    Elf_HeaderIdentification_Data,
    Elf_HeaderIdentification_Version,
    Elf_HeaderIdentification_OsAbi,
    Elf_HeaderIdentification_AbiVersion,
    Elf_HeaderIdentification_COUNT = 16,
} Elf_HeaderIdentification;

// NOTE(simon): ELFCLASS64
#define ELF_HEADER_CLASS_64  2
// NOTE(simon): ELFDATA2LSB
#define ELF_HEADER_DATA_2LSB 1
// NOTE(simon): EFLOSABI_NONE
#define ELF_OS_ABI_NONE      0
// NOTE(simon): EFLOSABI_GNU / ELFOSABI_LINUX
#define ELF_OS_ABI_LINUX     3

// NOTE(simon): ET_REL
#define ELF_HEADER_TYPE_RELOCATABLE 1
// NOTE(simon): EM_X86_64
#define ELF_HEADER_MACHINE_X86_64   62
// NOTE(simon): EV_CURRENT
#define ELF_HEADER_VERSION_CURRENT  1

// NOTE(simon): Elf64_Ehdr
typedef struct Elf64_Header Elf64_Header;
struct Elf64_Header {
    U8         identification[Elf_HeaderIdentification_COUNT]; // e_ident
    U16        type;                                           // e_type
    U16        machine;                                        // e_machine
    U32        version;                                        // e_version
    Elf64_Addr entry;                                          // e_entry
    Elf64_Off  program_header_offset;                          // e_phoff
    Elf64_Off  section_header_offset;                          // e_shoff
    U32        flags;                                          // e_flags
    U16        header_size;                                    // e_ehsize
    U16        program_header_entry_size;                      // e_phentsize
    U16        program_header_count;                           // e_phnum
    U16        section_header_entry_size;                      // e_shentsize
    U16        section_header_count;                           // e_shnum
    U16        section_header_string_index;                    // e_shstrndx
};

// NOTE(simon): SHT_NULL
#define ELF_SECTION_HEADER_TYPE_NULL         0
// NOTE(simon): SHT_PROGBITS
#define ELF_SECTION_HEADER_TYPE_PROGRAM_BITS 1
// NOTE(simon): SHT_SYMTAB
#define ELF_SECTION_HEADER_TYPE_SYMBOL_TABLE 2
// NOTE(simon): SHT_STRTAB
#define ELF_SECTION_HEADER_TYPE_STRING_TABLE 3
// NOTE(simon): SHF_WRITE
#define ELF_SECTION_HEADER_FLAG_WRITE        0x01
// NOTE(simon): SHF_ALLOC
#define ELF_SECTION_HEADER_FLAG_ALLOCATE     0x02

// NOTE(simon): Elf64_Shdr
typedef struct Elf64_SectionHeader Elf64_SectionHeader;
struct Elf64_SectionHeader {
    U32        name;          // sh_name
    U32        type;          // sh_type
    U64        flags;         // sh_flags
    Elf64_Addr address;       // sh_addr
    Elf64_Off  offset;        // sh_offset
    U64        size;          // sh_size
    U32        link;          // sh_link
    U32        info;          // sh_info
    U64        address_align; // sh_addralign
    U64        entry_size;    // sh_entsize
};

// NOTE(simon): STT_OBJECT
#define ELF_SYMBOL_TYPE_OBJECT        1
// NOTE(simon): STB_LOCAL
#define ELF_SYMBOL_BINDING_LOCAL      0
// NOTE(simon): STB_GLOBAL
#define ELF_SYMBOL_BINDING_GLOBAL     1
// NOTE(simon): STV_DEFAULT
#define ELF_SYMBOL_VISIBILITY_DEFAULT 0

#define ELF_SYMBOL_INFO_FROM_BINDING_TYPE(binding, type) (((binding) & 0x0F) << 4 | ((type) & 0x0F) << 0)

// NOTE(simon): Elf64_Sym
typedef struct Elf64_Symbol Elf64_Symbol;
struct Elf64_Symbol {
    U32        name;          // st_name
    U8         info;          // st_info
    U8         other;         // st_other
    U16        section_index; // st_shndx
    Elf64_Addr value;         // st_value
    U64        size;          // st_size
};



typedef struct Symbol Symbol;
struct Symbol {
    Symbol *next;
    Symbol *previous;

    // NOTE(simon): Specified data.
    Str8 name;
    Str8 section_name;
    Str8 data;
    U64  align;

    // NOTE(simon): Computed data.
    U32 name_index;
    U64 offset;
};

typedef struct Section Section;
struct Section {
    Section *next;
    Section *previous;

    // NOTE(simon): Specified data.
    Str8     name;
    U32      type;
    U64      flags;
    U64      address;
    Str8List data;
    // TODO(simon): Maybe it would be better to have this be a pointer to the
    // section. Specifically for the symbol table, what if we have multiple
    // string tables? Is that even possible? If so, which one do we use? Do
    // they have different names.
    Str8     link_name;
    U32      info;
    U64      address_align;
    U64      entry_size;

    // NOTE(simon): Computed data.
    U32 name_index;
    U64 offset;
};

typedef struct Builder Builder;
struct Builder {
    Symbol *first_symbol;
    Symbol *last_symbol;
    U64 symbol_count;

    Section *first_section;
    Section *last_section;
    U64 section_count;
};

internal Symbol *create_symbol(Arena *arena, Builder *builder, Str8 name) {
    Symbol *symbol = arena_push_struct(arena, Symbol);
    symbol->name = name;

    dll_push_back(builder->first_symbol, builder->last_symbol, symbol);
    ++builder->symbol_count;

    return symbol;
}

internal Section *create_section(Arena *arena, Builder *builder, Str8 name) {
    Section *section = arena_push_struct(arena, Section);
    section->name = name;

    dll_push_back(builder->first_section, builder->last_section, section);
    ++builder->section_count;

    return section;
}

internal U64 section_index_from_name(Builder *builder, Str8 name) {
    U64 result = 0;

    U64 index = 1;
    for (Section *section = builder->first_section; section; section = section->next, ++index) {
        if (str8_equal(section->name, name)) {
            result = index;
            break;
        }
    }

    return result;
}

internal S32 os_run(Str8List arguments) {
    Arena *arena = arena_create();

    // NOTE(simon): Test data
    U8 test_data[] = {
        0, 10, 20, 30, 40, 50, 60, 70, 80, 90,
    };
    U64 test_size = array_count(test_data);



    Builder builder = { 0 };

    // NOTE(simon): Symbols
    Symbol *size_symbol = create_symbol(arena, &builder, str8_literal("test_size"));
    size_symbol->section_name = str8_literal(".data");
    size_symbol->data         = str8((U8 *) &test_size, sizeof(test_size));
    size_symbol->align        = 8;

    Symbol *data_symbol = create_symbol(arena, &builder, str8_literal("test_data"));
    data_symbol->section_name = str8_literal(".data");
    data_symbol->data         = str8(test_data, sizeof(test_data));
    data_symbol->align        = 1;



    // NOTE(simon): Sections
    Section *data_section = create_section(arena, &builder, str8_literal(".data"));
    data_section->type  = ELF_SECTION_HEADER_TYPE_PROGRAM_BITS;
    data_section->flags = ELF_SECTION_HEADER_FLAG_WRITE | ELF_SECTION_HEADER_FLAG_ALLOCATE;






    // NOTE(simon): Layout symbols.
    for (Section *section = builder.first_section; section; section = section->next) {
        U64 offset = 0;
        for (Symbol *symbol = builder.first_symbol; symbol; symbol = symbol->next) {
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
    Section *string_table_section = create_section(arena, &builder, str8_literal(".strtab"));
    string_table_section->type          = ELF_SECTION_HEADER_TYPE_STRING_TABLE;
    string_table_section->address_align = 1;

    U32 symbol_name_buffer_size = 1;
    for (Symbol *symbol = builder.first_symbol; symbol; symbol = symbol->next) {
        symbol->name_index = symbol_name_buffer_size;
        symbol_name_buffer_size += symbol->name.size + 1;
    }
    U8 *symbol_name_buffer = arena_push_array(arena, U8, symbol_name_buffer_size);
    for (Symbol *symbol = builder.first_symbol; symbol; symbol = symbol->next) {
        memory_copy(&symbol_name_buffer[symbol->name_index], symbol->name.data, symbol->name.size);
    }
    str8_list_push(arena, &string_table_section->data, str8(symbol_name_buffer, symbol_name_buffer_size));



    // NOTE(simon): Create symbols.
    U64 elf_symbol_count = 1 + builder.symbol_count;
    Elf64_Symbol *elf_symbols = arena_push_array(arena, Elf64_Symbol, elf_symbol_count);
    U64 elf_symbol_index = 1;
    for (Symbol *symbol = builder.first_symbol; symbol; symbol = symbol->next) {
        Elf64_Symbol *elf_symbol = &elf_symbols[elf_symbol_index];
        elf_symbol->name          = symbol->name_index;
        elf_symbol->info          = ELF_SYMBOL_INFO_FROM_BINDING_TYPE(ELF_SYMBOL_BINDING_GLOBAL, ELF_SYMBOL_TYPE_OBJECT);
        elf_symbol->other         = ELF_SYMBOL_VISIBILITY_DEFAULT;
        elf_symbol->section_index = (U16) section_index_from_name(&builder, symbol->section_name);;
        elf_symbol->value         = symbol->offset;
        elf_symbol->size          = symbol->data.size;

        ++elf_symbol_index;
    }

    Section *symbol_table_section = create_section(arena, &builder, str8_literal(".symtab"));
    symbol_table_section->type          = ELF_SECTION_HEADER_TYPE_SYMBOL_TABLE;
    symbol_table_section->link_name     = str8_literal(".strtab");
    symbol_table_section->info          = 1; // Last local symbol index + 1
    symbol_table_section->address_align = _Alignof(Elf64_Symbol);
    symbol_table_section->entry_size    = sizeof(Elf64_Symbol);
    str8_list_push(arena, &symbol_table_section->data, str8((U8 *) elf_symbols, elf_symbol_count * sizeof(Elf64_Symbol)));




    // NOTE(simon): Compute section string table.
    Section *section_header_string_table_section = create_section(arena, &builder, str8_literal(".shstrtab"));
    section_header_string_table_section->type          = ELF_SECTION_HEADER_TYPE_STRING_TABLE;
    section_header_string_table_section->address_align = 1;

    U32 section_name_buffer_size = 1;
    for (Section *section = builder.first_section; section; section = section->next) {
        section->name_index = section_name_buffer_size;
        section_name_buffer_size += section->name.size + 1;
    }
    U8 *section_name_buffer = arena_push_array(arena, U8, section_name_buffer_size);
    for (Section *section = builder.first_section; section; section = section->next) {
        memory_copy(&section_name_buffer[section->name_index], section->name.data, section->name.size);
    }
    str8_list_push(arena, &section_header_string_table_section->data, str8(section_name_buffer, section_name_buffer_size));



    // NOTE(simon): Layout section content.
    {
        U64 total_offset = sizeof(Elf64_Header) + (1 + builder.section_count) * sizeof(Elf64_SectionHeader);
        for (Section *section = builder.first_section; section; section = section->next) {
            section->offset = total_offset;
            total_offset += section->data.total_size;
        }
    }



    // NOTE(simon): Create sections.
    U64 elf_section_count = 1 + builder.section_count;
    Elf64_SectionHeader *elf_sections = arena_push_array(arena, Elf64_SectionHeader, elf_section_count);
    U64 elf_section_index = 1;
    for (Section *section = builder.first_section; section; section = section->next) {
        Elf64_SectionHeader *elf_section = &elf_sections[elf_section_index];
        elf_section->name          = section->name_index;
        elf_section->type          = section->type;
        elf_section->flags         = section->flags;
        elf_section->address       = section->address;
        elf_section->offset        = section->offset;
        elf_section->size          = section->data.total_size;
        elf_section->link          = (U32) section_index_from_name(&builder, section->link_name);
        elf_section->info          = section->info;
        elf_section->address_align = section->address_align;
        elf_section->entry_size    = section->entry_size;

        ++elf_section_index;
    }



    // NOTE(simon): Header
    Elf64_Header header = { 0 };
    header.identification[Elf_HeaderIdentification_Magic0]     = 0x7F;
    header.identification[Elf_HeaderIdentification_Magic1]     = 'E';
    header.identification[Elf_HeaderIdentification_Magic2]     = 'L';
    header.identification[Elf_HeaderIdentification_Magic3]     = 'F';
    header.identification[Elf_HeaderIdentification_Class]      = ELF_HEADER_CLASS_64;
    header.identification[Elf_HeaderIdentification_Data]       = ELF_HEADER_DATA_2LSB;
    header.identification[Elf_HeaderIdentification_Version]    = ELF_HEADER_VERSION_CURRENT;
    header.identification[Elf_HeaderIdentification_OsAbi]      = ELF_OS_ABI_NONE;
    header.identification[Elf_HeaderIdentification_AbiVersion] = 0;
    header.type                        = ELF_HEADER_TYPE_RELOCATABLE;
    header.machine                     = ELF_HEADER_MACHINE_X86_64;
    header.version                     = ELF_HEADER_VERSION_CURRENT;
    header.section_header_offset       = sizeof(Elf64_Header);
    header.header_size                 = sizeof(Elf64_Header);
    header.section_header_entry_size   = sizeof(Elf64_SectionHeader);
    header.section_header_count        = (U16) elf_section_count;
    header.section_header_string_index = (U16) section_index_from_name(&builder, str8_literal(".shstrtab"));



    // NOTE(simon): Output
    Str8List output = { 0 };
    str8_list_push(arena, &output, str8((U8 *) &header, sizeof(header)));
    str8_list_push(arena, &output, str8((U8 *) elf_sections, elf_section_count * sizeof(Elf64_SectionHeader)));
    for (Section *section = builder.first_section; section; section = section->next) {
        for (Str8Node *data = section->data.first; data; data = data->next) {
            str8_list_push(arena, &output, data->string);
        }
    }

    os_file_write(str8_literal("test_elf.o"), output);

    arena_destroy(arena);
    return 0;
}
