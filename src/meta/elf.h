#ifndef ELF_H
#define ELF_H

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

typedef enum {
    // NOTE(simon): ELFCLASS64
    Elf_HeaderClass_64 = 2,
} Elf_HeaderClass;

typedef enum {
    // NOTE(simon): ELFDATA2LSB
    Elf_HeaderData_2Lsb = 1,
} Elf_HeaderData;

typedef enum {
    // NOTE(simon): EFLOSABI_NONE
    Elf_OsAbi_None = 0,
    // NOTE(simon): EFLOSABI_GNU / ELFOSABI_LINUX
    Elf_OsAbi_Linux = 3,
} Elf_OsAbi;

typedef enum {
    // NOTE(simon): ET_REL
    Elf_HeaderType_Relocatable = 1,
} Elf_HeaderType;

typedef enum {
    // NOTE(simon): EM_X86_64
    Elf_HeaderMachine_X86_64 = 62,
} Elf_HeaderMachine;

typedef enum {
    // NOTE(simon): EV_CURRENT
    Elf_HeaderVersion_Current = 1,
} Elf_HeaderVersion;

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

typedef enum {
    // NOTE(simon): SHT_NULL
    Elf_SectionHeaderType_Null        = 0,
    // NOTE(simon): SHT_PROGBITS
    Elf_SectionHeaderType_ProgramBits = 1,
    // NOTE(simon): SHT_SYMTAB
    Elf_SectionHeaderType_SymbolTable = 2,
    // NOTE(simon): SHT_STRTAB
    Elf_SectionHeaderType_StringTable = 3,
} Elf_SectionHeaderType;

typedef enum {
    // NOTE(simon): SHF_WRITE
    Elf_SectionHeaderFlag_Write    = 1 << 0,
    // NOTE(simon): SHF_ALLOC
    Elf_SectionHeaderFlag_Allocate = 1 << 1,
} Elf_SectionHeaderFlags;

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

typedef enum {
    // NOTE(simon): STT_OBJECT
    Elf_SymbolType_Object = 1,
} Elf_SymbolType;

typedef enum {
    // NOTE(simon): STB_LOCAL
    Elf_SymbolBinding_Local  = 0,
    // NOTE(simon): STB_GLOBAL
    Elf_SymbolBinding_Global = 1,
} Elf_SymbolBinding;

typedef enum {
    // NOTE(simon): STV_DEFAULT
    Elf_SymbolVisibility_Default = 0,
} Elf_SymbolVisibility;

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

internal Str8List elf_binary_from_object(Arena *arena, Object object);

#endif // ELF_H
