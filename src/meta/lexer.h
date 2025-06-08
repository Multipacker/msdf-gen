#ifndef LEXER_H
#define LEXER_H

typedef enum {
    Token_EndOfFile,
    Token_StringLiteral,
    Token_CharacterLiteral,
    Token_IntegerLiteral,
    Token_FloatLiteral,
    Token_Comment,
    Token_Identifier,
    Token_Symbol,
    Token_Unknown,
} TokenKind;

typedef enum {
    Token_Flag_PreProcessor = 1 << 0,
} TokenFlags;

typedef struct Token Token;
struct Token {
    TokenKind  kind;
    Str8       string;
    TokenFlags flags;
    U64        start_line;
    U64        start_column;
    U64        end_line;
    U64        end_column;
};

typedef struct TokenChunk TokenChunk;
struct TokenChunk {
    TokenChunk *next;
    TokenChunk *previous;
    U64 count;
    Token tokens[1000];
};

typedef struct TokenList TokenList;
struct TokenList {
    TokenChunk *first_chunk;
    TokenChunk *last_chunk;
    U64 total_count;
    U64 chunk_count;
};

typedef struct TokenArray TokenArray;
struct TokenArray {
    Token *tokens;
    U64    count;
};

internal TokenArray tokens_from_string(Arena *arena, Str8 name, Str8 source);

#endif // LEXER_H
