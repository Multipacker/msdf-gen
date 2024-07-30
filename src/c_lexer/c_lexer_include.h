#ifndef C_LEXER_INCLUDE_H
#define C_LEXER_INCLUDE_H

typedef enum {
    CProc_Token_HeaderName,
    CProc_Token_Identifier,
    CProc_Token_Number,
    CProc_Token_CharacterConstant,
    CProc_Token_StringLiteral,
    CProc_Token_Punctuator,
    CProc_Token_Whitespace,
    CProc_Token_Newline,
    CProc_Token_Comment,
    CProc_Token_Unknown,
} CProc_Token_Kind;

typedef struct CProc_Token CProc_Token;
struct CProc_Token {
    CProc_Token_Kind kind;
    Str8 source;
};

typedef struct CProc_TokenArray CProc_TokenArray;
struct CProc_TokenArray {
    CProc_Token *tokens;
    U64      count;
};

typedef struct CProc_Error CProc_Error;
struct CProc_Error {
    CProc_Error *next;
    Str8         message;
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

internal CProc_LexerResult cproc_tokens_from_string(Arena *arena, Str8 source);

#endif // C_LEXER_INCLUDE_H
