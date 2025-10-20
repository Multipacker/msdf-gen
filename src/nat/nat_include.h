#ifndef NAT_INCLUDE_H
#define NAT_INCLUDE_H

typedef enum {
    Nat_TokenFlag_Unknown      = 1 << 0,
    Nat_TokenFlag_Newline      = 1 << 1,
    Nat_TokenFlag_Whitespace   = 1 << 2,
    Nat_TokenFlag_Comment      = 1 << 3,
    Nat_TokenFlag_Punctuation  = 1 << 4,
    Nat_TokenFlag_Identifier   = 1 << 5,
    Nat_TokenFlag_String       = 1 << 6,
    Nat_TokenFlag_Number       = 1 << 7,
    Nat_TokenFlag_SingleQuoted = 1 << 8,
    Nat_TokenFlag_DoubleQuoted = 1 << 9,
    Nat_TokenFlag_Ticked       = 1 << 10,
    Nat_TokenFlag_Multiline    = 1 << 11,
    Nat_TokenFlag_Unclosed     = 1 << 12,

    Nat_TokenFlag_Label = Nat_TokenFlag_Identifier | Nat_TokenFlag_String | Nat_TokenFlag_Number | Nat_TokenFlag_Punctuation,
} Nat_TokenFlags;

typedef struct Nat_Token Nat_Token;
struct Nat_Token {
    Nat_TokenFlags flags;
    Str8           raw;
};

typedef struct Nat_TokenChunk Nat_TokenChunk;
struct Nat_TokenChunk {
    Nat_TokenChunk *next;
    Nat_TokenChunk *previous;
    Nat_Token       tokens[1000];
    U64             count;
};

typedef struct Nat_TokenList Nat_TokenList;
struct Nat_TokenList {
    Nat_TokenChunk *first_chunk;
    Nat_TokenChunk *last_chunk;
    U64             chunk_count;
    U64             token_count;
};

typedef struct Nat_TokenArray Nat_TokenArray;
struct Nat_TokenArray {
    Nat_Token *tokens;
    U64        count;
};

typedef enum {
    // NOTE(simon): Separators.
    Nat_NodeFlag_IsBeforeComma      = 1 << 0,
    Nat_NodeFlag_IsAfterComma       = 1 << 1,
    Nat_NodeFlag_IsBeforeSemicolon  = 1 << 2,
    Nat_NodeFlag_IsAfterSemicolon   = 1 << 3,

    // NOTE(simon): Delimiters.
    Nat_NodeFlag_HasParenLeft       = 1 << 4,
    Nat_NodeFlag_HasParenRight      = 1 << 5,
    Nat_NodeFlag_HasBraceLeft       = 1 << 6,
    Nat_NodeFlag_HasBraceRight      = 1 << 7,
    Nat_NodeFlag_HasBracketLeft     = 1 << 8,
    Nat_NodeFlag_HasBracketRight    = 1 << 9,

    // NOTE(simon): String delimiters.
    Nat_NodeFlag_StringSingleQuoted = 1 << 10,
    Nat_NodeFlag_StringDoubleQuoted = 1 << 11,
    Nat_NodeFlag_StringTicked       = 1 << 12,
    Nat_NodeFlag_StringMultiline    = 1 << 13,

    // NOTE(simon): Label kinds.
    Nat_NodeFlag_Punctuation        = 1 << 14,
    Nat_NodeFlag_Identifier         = 1 << 15,
    Nat_NodeFlag_String             = 1 << 16,
    Nat_NodeFlag_Number             = 1 << 17,
} Nat_NodeFlags;

typedef struct Nat_Node Nat_Node;
struct Nat_Node {
    Nat_Node *next;
    Nat_Node *previous;
    Nat_Node *first;
    Nat_Node *last;
    Nat_Node *parent;

    Nat_NodeFlags flags;
    Str8 string;
    Str8 raw;
};

global Nat_Node nat_nil_node = {
    .next     = &nat_nil_node,
    .previous = &nat_nil_node,
    .first    = &nat_nil_node,
    .last     = &nat_nil_node,
    .parent   = &nat_nil_node,
};

internal Nat_TokenArray nat_token_array_from_list(Arena *arena, Nat_TokenList list);
internal Void nat_test(Void);

#endif // NAT_INCLUDE_H
