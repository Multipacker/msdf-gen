#ifndef STRING_H
#define STRING_H

typedef struct {
    U8 *data;
    U64 size;
} Str8;

typedef struct {
    U16 *data;
    U64 size;
} Str16;

typedef struct {
    U32 *data;
    U64 size;
} Str32;

typedef struct Str8Node Str8Node;
struct Str8Node {
    Str8Node *next;
    Str8Node *previous;
    Str8 string;
};

typedef struct {
    Str8Node *first;
    Str8Node *last;
    U64 node_count;
    U64 total_size;
} Str8List;

typedef struct {
    U32 codepoint;
    U64 size;
} StringDecode;

internal Str8 str8(U8 *data, U64 size);
internal Str8 str8_range(U8 *start, U8 *opl);
internal Str8 str8_copy(Arena *arena, Str8 string);
internal Str8 str8_cstr(CStr data);
internal Str8 str8_copy_cstr(Arena *arena, U8 *data);

#define str8_literal(literal) ((Str8) { .data = (U8 *) (literal), .size = sizeof(literal) - 1})
#define str8_expand(string) (int) (string).size, (char *) (string).data

internal Str8 str8_prefix(Str8 string, U64 size);
internal Str8 str8_postfix(Str8 string, U64 size);
internal Str8 str8_skip(Str8 string, U64 size);
internal Str8 str8_chop(Str8 string, U64 size);
internal Str8 str8_substring(Str8 string, U64 start, U64 size);

internal B32 str8_equal(Str8 a, Str8 b);
internal B32 str8_first_index_of(Str8 string, U32 codepoint, U64 *result_index);
internal B32 str8_last_index_of(Str8 string, U32 codepoint, U64 *result_index);

internal Void str8_list_append(Arena *arena, Str8List *list, Str8List others);
internal Void str8_list_push_explicit(Str8List *list, Str8 string, Str8Node *node);
internal Void str8_list_push(Arena *arena, Str8List *list, Str8 string);
internal Str8 str8_join(Arena *arena, Str8List *list);

internal Str8 str8_format(Arena *arena, CStr format, ...);
internal Str8 str8_format_list(Arena *arena, CStr format, va_list arguments);

internal Str8List str8_split_by_codepoints(Arena *arena, Str8 string, Str8 codepoints);

internal StringDecode string_decode_utf8(U8 *string, U64 size);
internal U64          string_encode_utf8(U8 *destination, U32 codepoint);
internal StringDecode string_decode_utf16(U16 *string, U64 size);
internal U64          string_encode_utf16(U16 *destination, U32 codepoint);

internal Str32 str32_from_str8(Arena *arena, Str8  string);
internal Str8  str8_from_str32(Arena *arena, Str32 string);
internal Str16 str16_from_str8(Arena *arena, Str8  string);
internal Str8  str8_from_str16(Arena *arena, Str16 string);

internal CStr   cstr_from_str8(Arena *arena, Str8 string);
internal CStr16 cstr16_from_str8(Arena *arena, Str8 string);

#endif // STRING_H
