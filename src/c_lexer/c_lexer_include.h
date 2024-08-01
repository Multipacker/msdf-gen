#ifndef C_LEXER_INCLUDE_H
#define C_LEXER_INCLUDE_H

typedef enum {
    CProc_Token_HeaderName        = 1 << 0,
    CProc_Token_Identifier        = 1 << 1,
    CProc_Token_Number            = 1 << 2,
    CProc_Token_CharacterConstant = 1 << 3,
    CProc_Token_StringLiteral     = 1 << 4,
    CProc_Token_Punctuator        = 1 << 5,
    CProc_Token_Whitespace        = 1 << 6,
    CProc_Token_Newline           = 1 << 7,
    CProc_Token_Comment           = 1 << 8,
    CProc_Token_Unknown           = 1 << 9,

    CProc_Token_BrokenDelimiter   = 1 << 10,
} CProc_Token_Kind;

typedef struct CProc_Token CProc_Token;
struct CProc_Token {
    CProc_Token_Kind kind;
    Str8 source;
};

typedef struct CProc_TokenArray CProc_TokenArray;
struct CProc_TokenArray {
    CProc_Token *tokens;
    U64          count;
};

typedef struct CProc_Location CProc_Location;
struct CProc_Location {
    U32 line;
    U32 column;
};

typedef struct CProc_Error CProc_Error;
struct CProc_Error {
    CProc_Error   *next;
    CProc_Location location;
    Str8           message;
};

typedef struct CProc_ErrorList CProc_ErrorList;
struct CProc_ErrorList {
    CProc_Error *first;
    CProc_Error *last;
    U64          count;
};

typedef struct CProc_LexerResult CProc_LexerResult;
struct CProc_LexerResult {
    CProc_TokenArray tokens;
    CProc_ErrorList  errors;
};

internal Str8              cproc_string_from_token(Arena *arena, CProc_Token token);
internal CProc_Location    cproc_location_from_token(Str8 source, CProc_Token token);
internal CProc_LexerResult cproc_tokens_from_string(Arena *arena, Str8 source);

#endif // C_LEXER_INCLUDE_H
