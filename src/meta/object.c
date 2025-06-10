internal Object_Section *object_add_section(Arena *arena, Object *object, Str8 name) {
    Object_Section *section = arena_push_struct(arena, Object_Section);
    section->name = str8_copy(arena, name);

    dll_push_back(object->first_section, object->last_section, section);
    ++object->section_count;

    return section;
}

internal Object_Symbol *object_add_symbol(Arena *arena, Object *object, Str8 name) {
    Object_Symbol *symbol = arena_push_struct(arena, Object_Symbol);
    symbol->name = str8_copy(arena, name);

    dll_push_back(object->first_symbol, object->last_symbol, symbol);
    ++object->symbol_count;

    return symbol;
}
