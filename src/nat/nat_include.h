#ifndef NAT_INCLUDE_H
#define NAT_INCLUDE_H

/*
 * Features:
 * * Single line comments with //
 * * Nested multiline comments with / * and * / (spaces added between
 *   characters becuase C doesn't support them)
 * * Multiline literals between ''' """ and ```
 * * Single line literals between ' " and `
 * * Grouping with () [] {} <>
 */

typedef enum {
    Nat_Token_Identifier,
    Nat_Token_Number,
    Nat_Token_Literal,
    Nat_Token_Punctuator,
    Nat_Token_Whitespace,
    Nat_Token_Newline,
    Nat_Token_Comment,
    Nat_Token_Unknown,
} Nat_TokenKind;

typedef enum {
    Nat_TokenFlag_UnclosedComment    = 1 << 0,
    Nat_TokenFlag_UnclosedLiteral    = 1 << 1,
    Nat_TokenFlag_MultilineLiteral   = 1 << 2,
    Nat_TokenFlag_SingleQuoteLiteral = 1 << 3,
    Nat_TokenFlag_DoubleQuoteLiteral = 1 << 4,
    Nat_TokenFlag_TickLiteral        = 1 << 5,
} Nat_TokenFlag;

typedef struct Nat_Token Nat_Token;
struct Nat_Token {
    Nat_TokenKind kind;
    Nat_TokenFlag flags;
    Str8 source;
};

typedef struct Nat_TokenChunk Nat_TokenChunk;
struct Nat_TokenChunk {
    Nat_TokenChunk *next;
    U64             count;
    Nat_Token       tokens[1000];
};

typedef struct Nat_TokenList Nat_TokenList;
struct Nat_TokenList {
    Nat_TokenChunk *first;
    Nat_TokenChunk *last;
    U64             total_token_count;
};

typedef struct Nat_TokenArray Nat_TokenArray;
struct Nat_TokenArray {
    Nat_Token *tokens;
    U64        count;
};

typedef struct Nat_Location Nat_Location;
struct Nat_Location {
    U32 line;
    U32 column;
};

typedef struct Nat_Error Nat_Error;
struct Nat_Error {
    Nat_Error   *next;
    Nat_Location location;
    Str8         message;
};

typedef struct Nat_ErrorList Nat_ErrorList;
struct Nat_ErrorList {
    Nat_Error *first;
    Nat_Error *last;
    U64        total_error_count;
};

typedef struct Nat_LexerResult Nat_LexerResult;
struct Nat_LexerResult {
    Nat_TokenArray tokens;
    Nat_ErrorList  errors;
};

internal Nat_TokenArray  nat_token_array_from_list(Arena *arena, Nat_TokenList list);
internal Nat_Location    nat_location_from_token(Str8 source, Nat_Token token);
internal Nat_LexerResult nat_tokens_from_string(Arena *arena, Str8 source);

#endif // NAT_INCLUDE_H
