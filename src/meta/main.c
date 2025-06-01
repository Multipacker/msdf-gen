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

internal S32 os_run(Str8List arguments) {
    /*
     * NOTE(simon):
     * Symbols need:
     * * name
     * * section name
     * * data
     * * data alignment
     * * type (function, object)
     * * visibility?
     */

    Arena *arena = arena_create();

    // NOTE(simon): Test data
    U8 test_data[] = {
        0, 10, 20, 30, 40, 50, 60, 70, 80, 90,
    };
    U64 test_size = array_count(test_data);



    // NOTE(simon): Symbols
    U64 symbol_count = 3;
    Elf64_Symbol *symbols = arena_push_array(arena, Elf64_Symbol, symbol_count);
    Str8 symbol_names = str8_literal("\0test_size\0test_data\0");

    Elf64_Symbol *undefined_symbol = &symbols[0];

    Elf64_Symbol *size_symbol = &symbols[1];
    size_symbol->name          = 1; // test_size
    size_symbol->info          = ELF_SYMBOL_INFO_FROM_BINDING_TYPE(ELF_SYMBOL_BINDING_GLOBAL, ELF_SYMBOL_TYPE_OBJECT);
    size_symbol->other         = ELF_SYMBOL_VISIBILITY_DEFAULT;
    size_symbol->section_index = 1; // .data
    size_symbol->value         = 0; // Start of .data
    size_symbol->size          = sizeof(test_size);

    Elf64_Symbol *data_symbol = &symbols[2];
    data_symbol->name          = 11; // test_data
    data_symbol->info          = ELF_SYMBOL_INFO_FROM_BINDING_TYPE(ELF_SYMBOL_BINDING_GLOBAL, ELF_SYMBOL_TYPE_OBJECT);
    data_symbol->other         = ELF_SYMBOL_VISIBILITY_DEFAULT;
    data_symbol->section_index = 1; // .data
    data_symbol->value         = sizeof(test_size); // After test_data in .data
    data_symbol->size          = sizeof(test_data);



    // NOTE(simon): Sections
    U64 section_count = 5;
    Elf64_SectionHeader *sections = arena_push_array(arena, Elf64_SectionHeader, section_count);
    Str8 section_names = str8_literal("\0.data\0.symtab\0.shstrtab\0.strtab\0");

    Elf64_SectionHeader *null_section_header = &sections[0];
    null_section_header->type = ELF_SECTION_HEADER_TYPE_NULL;

    Elf64_SectionHeader *data_section_header = &sections[1];
    data_section_header->name          = 1; // .data
    data_section_header->type          = ELF_SECTION_HEADER_TYPE_PROGRAM_BITS;
    data_section_header->flags         = ELF_SECTION_HEADER_FLAG_WRITE | ELF_SECTION_HEADER_FLAG_ALLOCATE;
    data_section_header->offset        = sizeof(Elf64_Header) + section_count * sizeof(Elf64_SectionHeader) + symbol_count * sizeof(Elf64_Symbol) + section_names.size + symbol_names.size + 2;
    data_section_header->size          = sizeof(test_size) + sizeof(test_data);
    data_section_header->address_align = u64_max(8, 1);

    Elf64_SectionHeader *symbol_table_section_header = &sections[2];
    symbol_table_section_header->name          = 7; // .symtab
    symbol_table_section_header->type          = ELF_SECTION_HEADER_TYPE_SYMBOL_TABLE;
    symbol_table_section_header->offset        = sizeof(Elf64_Header) + section_count * sizeof(Elf64_SectionHeader);
    symbol_table_section_header->size          = symbol_count * sizeof(Elf64_Symbol);
    symbol_table_section_header->link          = 4; // .strtab
    symbol_table_section_header->info          = 1; // Last local symbol index + 1, sh_info
    symbol_table_section_header->address_align = _Alignof(Elf64_Symbol);
    symbol_table_section_header->entry_size    = sizeof(Elf64_Symbol);

    Elf64_SectionHeader *section_header_string_table_section_header = &sections[3];
    section_header_string_table_section_header->name          = 15; // .shstrtab
    section_header_string_table_section_header->type          = ELF_SECTION_HEADER_TYPE_STRING_TABLE;
    section_header_string_table_section_header->offset        = sizeof(Elf64_Header) + section_count * sizeof(Elf64_SectionHeader) + symbol_count * sizeof(Elf64_Symbol);
    section_header_string_table_section_header->size          = section_names.size;
    section_header_string_table_section_header->address_align = 1;

    Elf64_SectionHeader *string_table_section_header = &sections[4];
    string_table_section_header->name          = 25; // .strtab
    string_table_section_header->type          = ELF_SECTION_HEADER_TYPE_STRING_TABLE;
    string_table_section_header->offset        = sizeof(Elf64_Header) + section_count * sizeof(Elf64_SectionHeader) + symbol_count * sizeof(Elf64_Symbol) + section_names.size;
    string_table_section_header->size          = symbol_names.size;
    string_table_section_header->address_align = 1;



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
    header.section_header_count        = (U16) section_count;
    header.section_header_string_index = 3; // .shstrtab, e_shstrndx

    Str8List output = { 0 };
    str8_list_push(arena, &output, str8((U8 *) &header, sizeof(header)));
    str8_list_push(arena, &output, str8((U8 *) sections, section_count * sizeof(Elf64_SectionHeader)));
    str8_list_push(arena, &output, str8((U8 *) symbols, symbol_count * sizeof(Elf64_Symbol)));
    str8_list_push(arena, &output, section_names);
    str8_list_push(arena, &output, symbol_names);
    U8 padding[2] = {};
    str8_list_push(arena, &output, str8(padding, sizeof(padding)));
    str8_list_push(arena, &output, str8((U8 *) &test_size, sizeof(test_size)));
    str8_list_push(arena, &output, str8(test_data, sizeof(test_data)));

    os_file_write(str8_literal("test_elf.o"), output);

    arena_destroy(arena);
    return 0;
}
