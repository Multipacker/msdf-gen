#ifndef OBJECT_H
#define OBJECT_H

typedef struct Object_Symbol Object_Symbol;
struct Object_Symbol {
    Object_Symbol *next;
    Object_Symbol *previous;

    Str8 name;
    Str8 section_name;
    Str8 data;
    U32  align;
};

typedef enum {
    Object_SectionFlag_Write = 1 << 0,
} Object_SectionFlags;

typedef struct Object_Section Object_Section;
struct Object_Section {
    Object_Section *next;
    Object_Section *previous;

    Str8                name;
    Object_SectionFlags flags;
};


typedef struct Object Object;
struct Object {
    Object_Section *first_section;
    Object_Section *last_section;
    U64      section_count;

    Object_Symbol *first_symbol;
    Object_Symbol *last_symbol;
    U64     symbol_count;
};

internal Object_Section *object_add_section(Arena *arena, Object *object, Str8 name);
internal Object_Symbol  *object_add_symbol(Arena *arena, Object *object, Str8 name);

#if OS_LINUX
# define platform_binary_from_object(arena, object) elf_binary_from_object(arena, object)
#elif OS_WINDOWS
# define platform_binary_from_object(arena, object) coff_binary_from_object(arena, object)
#else
# error Object file generation is not implemented for you platform
#endif

#endif // OBJECT_H
