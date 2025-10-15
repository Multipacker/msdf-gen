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

typedef struct U64Decode U64Decode;
struct U64Decode {
    U64 value;
    U64 size;
};

typedef struct FuzzyMatch FuzzyMatch;
struct FuzzyMatch {
    FuzzyMatch *next;
    FuzzyMatch *previous;
    U64 min;
    U64 max;
};

typedef struct FuzzyMatchList FuzzyMatchList;
struct FuzzyMatchList {
    FuzzyMatch *first;
    FuzzyMatch *last;
    U64 needle_parts;
    U64 count;
    U64 total_length;
};

internal Str8 str8(U8 *data, U64 size);
internal Str8 str8_range(U8 *start, U8 *opl);
internal Str8 str8_copy(Arena *arena, Str8 string);
internal Str8 str8_cstr(CStr data);
internal Str8 str8_copy_cstr(Arena *arena, U8 *data);

internal Str16 str16(U16 *data, U64 size);
internal Str16 str16_cstr16(CStr16 data);

#define str8_literal(literal) ((Str8) { .data = (U8 *) (literal), .size = sizeof(literal) - 1, })
#define str8_literal_compile(literal) { .data = (U8 *) (literal), .size = sizeof(literal) - 1, }
#define str8_expand(string) (int) (string).size, (char *) (string).data

internal Str8 str8_prefix(Str8 string, U64 size);
internal Str8 str8_postfix(Str8 string, U64 size);
internal Str8 str8_skip(Str8 string, U64 size);
internal Str8 str8_chop(Str8 string, U64 size);
internal Str8 str8_substring(Str8 string, U64 start, U64 size);

internal Str8 str8_skip_last_slash(Str8 string);
internal Str8 str8_chop_last_slash(Str8 string);

internal B32 str8_equal(Str8 a, Str8 b);

// NOTE(simon): String lists.
internal Void str8_list_append(Arena *arena, Str8List *list, Str8List others);
internal Void str8_list_push_explicit(Str8List *list, Str8 string, Str8Node *node);
internal Void str8_list_push(Arena *arena, Str8List *list, Str8 string);
internal Void str8_list_push_format(Arena *arena, Str8List *list, CStr format, ...);
internal Void str8_list_push_format_list(Arena *arena, Str8List *list, CStr format, va_list arguments);
internal Str8 str8_join(Arena *arena, Str8List list);

// NOTE(simon): Formatting.
internal Str8 str8_concatenate(Arena *arena, Str8 a, Str8 b);
internal Str8 str8_format(Arena *arena, CStr format, ...);
internal Str8 str8_format_list(Arena *arena, CStr format, va_list arguments);

internal Str8List str8_split_by_codepoints(Arena *arena, Str8 string, Str8 codepoints);

// NOTE(simon): Encoding and decoding.
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

internal U64 str8_next_codepoint_offset(Str8 string, U64 start_offset, Side side);

// NOTE(simon): Basic parsing routines.
internal U64Decode u64_from_str8(Str8 string);

// NOTE(simon): String transformations.
internal Str8 str8_lowercase_ascii(Arena *arena, Str8 string);
internal Str8 str8_uppercase_ascii(Arena *arena, Str8 string);

// NOTE(simon): Searching
internal U64 str8_first_index_of(Str8 string, U32 codepoint);
internal U64 str8_last_index_of(Str8 string, U32 codepoint);
internal U64 str8_find(U64 offset, Str8 needle, Str8 haystack);
internal S64 str8_compare_ascii(Str8 a, Str8 b);
internal FuzzyMatchList str8_fuzzy_match(Arena *arena, Str8 needle, Str8 haystack);
internal FuzzyMatchList fuzzy_match_list_copy(Arena *arena, FuzzyMatchList fuzzy_matches);

#endif // STRING_H
