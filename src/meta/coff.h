#ifndef COFF_H
#define COFF_H

typedef enum {
    Coff_HeaderMachine_Unknown = 0x00,
} Coff_HeaderMachine;

typedef struct Coff_Header Coff_Header;
struct Coff_Header {
    U16 machine;              // Machine
    U16 section_count;        // NumberOfSections
    U32 time_date_stamp;      // TimeDateStamp
    U32 symbol_table_offset;  // PointerToSymbolTable
    U32 symbol_count;         // NumberOfSymbols
    U16 optional_header_size; // SizeOfOptionalHeader
    U16 characteristics;      // Characteristics
};

typedef enum {
    // IMAGE_SCN_CNT_INITIALIZED_DATA
    Coff_SectionFlag_InitializedData = 1 << 6,
    // IMAGE_SCN_CNT_READ
    Coff_SectionFlag_Read            = 1 << 30,
    // IMAGE_SCN_CNT_WRITE
    Coff_SectionFlag_Write           = 1 << 31,
} Coff_SectionFlags;


typedef struct Coff_Section Coff_Section;
struct Coff_Section {
    U8 name[8];             // Name
    U32 virtual_size;       // VirtualSize
    U32 virtual_address;    // VirtualAddress
    U32 raw_data_size;      // SizeOfRawData
    U32 raw_data_offset;    // PointerToRawData
    U32 relocation_offset;  // PointerToRelocations
    U32 line_number_offset; // PointerToLinenumbers
    U16 relocation_count;   // NumberOfRelocations
    U16 line_number_count;  // NumberOfLinenumbers
    U32 characteristics;    // Characteristics
};

typedef packed_struct({
    union {
        U8 name[8];           // ShortName
        struct {
            U32 reserved;
            U32 name_offset;  // Offset
        };
    };                        // Name
    U32 value;                // Value
    S16 section_number;       // SectionNumber
    U16 type;                 // Type
    U8 storage_class;         // StorageClass
    U8 auxilary_symbol_count; // NumberOfAuxSymbols
}) Coff_Symbol;

typedef enum {
    // IMAGE_SYM_TYPE_NULL
    Coff_SymbolType_Null = 0,
} Coff_SymbolType;

typedef enum {
    // IMAGE_SYM_CLASS_EXTERNAL
    Coff_SymbolStorageClass_External = 2,
    // IMAGE_SYM_CLASS_STATIC
    Coff_SymbolStorageClass_Static   = 3,
} Coff_SymbolStorageClass;

internal Str8List coff_binary_from_object(Arena *arena, Object object);

#endif // COFF_H
