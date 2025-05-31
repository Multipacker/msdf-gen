#include "src/base/base_include.h"

#include "src/base/base_include.c"

typedef U64 Elf64_Addr;
typedef U64 Elf64_Off;
typedef U16 Elf64_Section;
typedef U16 Elf64_Versym;
typedef U8  Elf_Byte;
typedef U16 Elf64_Half;
typedef S32 Elf64_Sword
typedef U32 Elf64_Word
typedef S64 Elf64_Sxword
typedef U64 Elf64_Xword

// NOTE(simon): ET_REL
#define ELF_HEADER_TYPE_RELOCATABLE // TODO(simon): Find this
// NOTE(simon): EM_X86_64
#define ELF_HEADER_MACHINE_X86_64   // TODO(simon): Find this
// NOTE(simon): EV_CURRENT
#define ELF_HEADER_VERSION_CURRENT  // TODO(simon): Find this

// NOTE(simon): Elf64_Ehdr
typedef struct Elf64_Header Elf64_Header;
struct Elf64_Header {
    U8         ident[];                     // e_ident
    U16        type;                        // e_type
    U16        machine;                     // e_machine
    U32        version;                     // e_version
    Elf64_Addr entry;                       // e_entry
    Elf64_Off  program_header_offset;       // e_phoff
    Elf64_Off  section_header_offset;       // e_shoff
    U32        flags;                       // e_flags
    U16        header_size;                 // e_ehsize
    U16        program_header_entry_size;   // e_phentsize
    U16        program_header_count;        // e_phnum
    U16        section_header_entry_size;   // e_shentsize
    U16        section_header_count;        // e_shnum
    U16        section_header_string_index; // e_shstrndx
};

// NOTE(simon): PT_LOAD
#define ELF_PROGRAM_HEADER_TYPE_LOAD  // TODO(simon): Find this
// NOTE(simon): PF_W
#define ELF_PROGRAM_HEADER_FLAG_WRITE // TODO(simon): Find this
// NOTE(simon): PF_R
#define ELF_PROGRAM_HEADER_FLAG_READ  // TODO(simon): Find this

// NOTE(simon): Elf64_Phdr
typedef struct Elf64_ProgramHeader Elf64_ProgramHeader;
struct Elf64_ProgramHeader {
    U32        type;             // p_type
    U32        flags;            // p_flags
    Elf64_Off  offset;           // p_offset
    Elf64_Addr virtual_address;  // p_vaddr
    Elf64_Addr physical_address; // p_paddr
    U64        file_size;        // p_filesz
    U64        memory_size;      // p_memsz
    U64        align;            // p_align
};

// NOTE(simon): SHT_NULL
#define ELF_SECTION_HEADER_TYPE_NULL         // TODO(simon): Find this
// NOTE(simon): SHT_PROGBITS
#define ELF_SECTION_HEADER_TYPE_PROGRAM_BITS // TODO(simon): Find this
// NOTE(simon): SHT_SYMTAB
#define ELF_SECTION_HEADER_TYPE_SYMBOL_TABLE // TODO(simon): Find this
// NOTE(simon): SHT_STRTAB
#define ELF_SECTION_HEADER_TYPE_STRING_TABLE // TODO(simon): Find this
// NOTE(simon): SHF_WRITE
#define ELF_SECTION_HEADER_FLAG_WRITE        // TODO(simon): Find this
// NOTE(simon): SHF_ALLOC
#define ELF_SECTION_HEADER_FLAG_ALLOCATE     // TODO(simon): Find this

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
#define ELF_SYMBOL_TYPE_OBJECT        // TODO(simon): Find this
// NOTE(simon): STB_GLOBAL
#define ELF_SYMBOL_BINDING_GLOBAL     // TODO(simon): Find this
// NOTE(simon): STV_DEFAULT
#define ELF_SYMBOL_VISIBILITY_DEFAULT // TODO(simon): Find this

// NOTE(simon): Elf64_Sym
typedef Elf64_Symbol Elf64_Symbol;
struct Elf64_Symbol {
    U32         name;          // st_name
    U8          info;          // st_info
    U8          other;         // st_other
    U16         section_index; // st_shndx
    Elf64_Adddr value;         // st_value
    U64         size;          // st_size
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
    // NOTE(simon): p_vaddr % p_align = p_offset % p_align
    Elf64_ProgramHeader data_program_header = { 0 };
    data_program_header.type = ELF_PROGRAM_HEADER_TYPE_LOAD;
    data_program_header.flags = ELF_PROGRAM_HEADER_FLAG_WRITE | ELF_PROGRAM_HEADER_FLAG_READ;
    data_program_header.offset;           // e_offset
    data_program_header.virtual_address;  // e_vaddr
    data_program_header.file_size;        // e_filesz
    data_program_header.memory_size;      // e_memsz
    data_program_header.align = 1 << 3; // NOTE(simon): We only need 8-byte alignment for U64s.

    Elf64_SectionHeader *sections = arena_push_array(arena, Elf64_SectionHeader, 5);

    Elf64_SectionHeader *null_section_header = &sections[0];
    null_section_header->type = ELF_SECTION_HEADER_TYPE_NULL;

    Elf64_SectionHeader *data_section_header = &sections[1];
    data_section_header->name = 1; // .data, sh_name
    data_section_header->type = ELF_SECTION_HEADER_TYPE_PROGRAM_BITS;
    data_section_header->flags = ELF_SECTION_HEADER_FLAG_WRITE | ELF_SECTION_HEADER_FLAG_ALLOCATE;
    data_section_header->address;       // sh_addr
    data_section_header->offset;        // sh_offset
    data_section_header->size;          // sh_size
    data_section_header->link;          // sh_link
    data_section_header->info;          // sh_info
    data_section_header->address_align = 1 << 3; // NOTE(simon): We only need 8-byte alignment for U64s.

    Elf64_SectionHeader *symbol_table_section_header = &sections[2];
    symbol_table_section_header->name = 7; // .symtab, sh_name
    symbol_table_section_header->type = ELF_SECTION_HEADER_TYPE_SYMBOL_TABLE;
    symbol_table_section_header->offset;        // sh_offset
    symbol_table_section_header->size;          // sh_size
    symbol_table_section_header->link;          // sh_link
    symbol_table_section_header->info;          // sh_info
    symbol_table_section_header->address_align = _Alignof(Elf64_Symbol);
    symbol_table_section_header->entry_size = sizeof(Elf64_Symbol);

    Elf64_SectionHeader *section_header_string_table_section_header = &sections[3];
    section_header_string_table_section_header->name = 15; // .shstrtab, sh_name
    section_header_string_table_section_header->type = ELF_SECTION_HEADER_TYPE_STRING_TABLE;
    section_header_string_table_section_header->offset;        // sh_offset
    section_header_string_table_section_header->size;          // sh_size
    Str8 section_header_string_table_data = str8_literal("\0.data\0.symtab\0.shstrtab\0.strtab\0");

    Elf64_SectionHeader *string_table_section_header = &sections[4];
    string_table_section_header->name = 25; // .strtab, sh_name
    string_table_section_header->type = ELF_SECTION_HEADER_TYPE_STRING_TABLE;
    string_table_section_header->offset;        // sh_offset
    string_table_section_header->size;          // sh_size
    Str8 string_table_data = str8_literal("\0test_size\0test_data\0");

    Elf64_Symbol *symbols = arena_push_array(arena, Elf64_Symbol, 2);

    Elf64_Symbol *size_symbol = &symbols[0];
    size_symbol->name = 1; // test_size
    size_symbol->info = ELF_SYMBOL_TYPE_OBJECT; // TODO(simon): local or global?
    size_symbol->other = ELF_SYMBOL_VISIBILITY_DEFAULT;
    size_symbol->section_index = 1; // .data, st_shndx
    size_symbol->value;         // st_value
    size_symbol->size = 8;

    Elf64_Symbol *data_symbol = &symbols[1];
    data_symbol->name = 11; // test_data
    data_symbol->info = ELF_SYMBOL_TYPE_OBJECT; // TODO(simon): local or global?
    data_symbol->other = ELF_SYMBOL_VISIBILITY_DEFAULT;
    data_symbol->section_index = 1; // .data, st_shndx
    data_symbol->value;         // st_value
    data_symbol->size;          // st_size

    U8 test_data[] = {
        0, 10, 20, 30, 40, 50, 60, 70, 80, 90,
    };
    U64 test_size = array_count(test_data);

    Elf64_Header header = { 0 };
    header.header_size = sizeof(Elf64_Header);
    header.ident[];                     // e_ident
    header.type = ELF_HEADER_TYPE_RELOCATABLE;
    header.machine = ELF_HEADER_MACHINE_X86_64;
    header.version = ELF_HEADER_VERSION_CURRENT;
    header.program_header_offset;       // e_phoff
    header.section_header_offset;       // e_shoff
    header.flags;                       // e_flags
    header.header_size = sizeof(Elf64_Header);
    header.program_header_entry_size = sizeof(Elf64_ProgramHeader);
    header.program_header_count;        // e_phnum
    header.section_header_entry_size;   // e_shentsize
    header.section_header_count;        // e_shnum
    header.section_header_string_index; // e_shstrndx

    arena_destroy(arena);
    return 0;
}
