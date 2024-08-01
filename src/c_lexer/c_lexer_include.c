#define CPROC_UNIVERSAL_CHARACTER_RANGES \
    X(0x00A8,  0x00A8)  \
    X(0x00AA,  0x00AA)  \
    X(0x00AD,  0x00AD)  \
    X(0x00AF,  0x00AF)  \
    X(0x00B2,  0x00B5)  \
    X(0x00B7,  0x00BA)  \
    X(0x00BC,  0x00BE)  \
    X(0x00C0,  0x00D6)  \
    X(0x00D8,  0x00F6)  \
    X(0x00F8,  0x00FF)  \
    X(0x0100,  0x167F)  \
    X(0x1681,  0x180D)  \
    X(0x180F,  0x1FFF)  \
    X(0x200B,  0x200D)  \
    X(0x202A,  0x202E)  \
    X(0x203F,  0x2040)  \
    X(0x2054,  0x2054)  \
    X(0x2060,  0x206F)  \
    X(0x2070,  0x218F)  \
    X(0x2460,  0x24FF)  \
    X(0x2776,  0x2793)  \
    X(0x2C00,  0x2DFF)  \
    X(0x2E80,  0x2FFF)  \
    X(0x3004,  0x3007)  \
    X(0x3021,  0x302F)  \
    X(0x3031,  0x303F)  \
    X(0x3040,  0xD7FF)  \
    X(0xF900,  0xFD3D)  \
    X(0xFD40,  0xFDCF)  \
    X(0xFDF0,  0xFE44)  \
    X(0xFE47,  0xFFFD)  \
    X(0x10000, 0x1FFFD) \
    X(0x20000, 0x2FFFD) \
    X(0x30000, 0x3FFFD) \
    X(0x40000, 0x4FFFD) \
    X(0x50000, 0x5FFFD) \
    X(0x60000, 0x6FFFD) \
    X(0x70000, 0x7FFFD) \
    X(0x80000, 0x8FFFD) \
    X(0x90000, 0x9FFFD) \
    X(0xA0000, 0xAFFFD) \
    X(0xB0000, 0xBFFFD) \
    X(0xC0000, 0xCFFFD) \
    X(0xD0000, 0xDFFFD) \
    X(0xE0000, 0xEFFFD)

typedef struct CProc_Stream CProc_Stream;
struct CProc_Stream {
    U8 *start;
    U8 *end;
    U8 *cursor;

    U64 index;
    B32 is_end;

    U8 *source_start;
    U8 *source_cursor;
    U8 *source_end;
};

typedef struct CProc_BufferedStream CProc_BufferedStream;
struct CProc_BufferedStream {
    U8 *start;
    U8 *cursor;
    U8 *end;
    U8 *mark;

    U8  buffer[4];
    U8 *index[5];
    B32 is_end[5];

    CProc_Stream source;
};

internal U8 *cproc_stream_index(CProc_Stream *stream) {
    U64 index = stream->index + (stream->cursor - stream->start);
    U8 *result = &stream->source_start[index];
    return result;
}

internal Void cproc_stream_refill(CProc_Stream *stream) {
    local U8 zeroes[256] = { 0 };
    local U8 one_backslash[] = { '\\', };

    typedef enum {
        State_Start,
        State_SeenBackslash,
        State_SeenCarridgeReturn,
        State_Done,
    } State;

    // NOTE(simon): This is one of the pre-conditions for the function, but it
    // is easy to guard against.
    if (stream->cursor != stream->end) {
        return;
    }

    stream->index += stream->end - stream->start;

    State state = State_Start;
    while (stream->source_cursor < stream->source_end && state != State_Done) {
        if (state == State_Start) {
            if (*stream->source_cursor == '\\') {
                ++stream->source_cursor;
                state = State_SeenBackslash;
            } else {
                // NOTE(simon): Find next '\'.
                U8 *next_backslash = stream->source_cursor;
                while (next_backslash < stream->source_end && *next_backslash != '\\') {
                    ++next_backslash;
                }

                // NOTE(simon): Return up until '\' or the rest of the buffer
                // if we don't have one.
                stream->start         = stream->source_cursor;
                stream->cursor        = stream->source_cursor;
                stream->end           = next_backslash;
                stream->source_cursor = next_backslash;
                state = State_Done;
            }
        } else if (state == State_SeenBackslash) {
            if (*stream->source_cursor == '\n') {
                // NOTE(simon): Escape of '\n'.
                ++stream->source_cursor;
                state = State_Start;
                stream->index += 2;
            } else if (*stream->source_cursor == '\r') {
                // NOTE(simon): Escape, might include more characters.
                ++stream->source_cursor;
                state = State_SeenCarridgeReturn;
            } else {
                // NOTE(simon): Not a line escape, send '\'.
                stream->start  = one_backslash;
                stream->cursor = one_backslash;
                stream->end    = one_backslash + array_count(one_backslash);
                state = State_Done;
            }
        } else if (state == State_SeenCarridgeReturn) {
            if (*stream->source_cursor == '\n') {
                // NOTE(simon): Escape of '\r\n'.
                ++stream->source_cursor;
                state = State_Start;
                stream->index += 3;
            } else {
                // NOTE(simon): Escape of '\r'.
                state = State_Start;
                stream->index += 2;
            }
        }
    }

    if (state == State_Start || state == State_SeenCarridgeReturn) {
        // NOTE(simon): Reached the end of the stream, potentially having read
        // a '\r' escape.
        stream->start  = zeroes;
        stream->cursor = zeroes;
        stream->end    = zeroes + array_count(zeroes);
        stream->is_end = true;
    } else if (state == State_SeenBackslash) {
        // NOTE(simon): Not a line escape, send '\'.
        stream->start  = one_backslash;
        stream->cursor = one_backslash;
        stream->end    = one_backslash + array_count(one_backslash);
    }
}

internal U8 *cproc_buffered_stream_index(CProc_BufferedStream *stream) {
    U8 *result = 0;

    if (stream->start == stream->buffer) {
        // NOTE(simon): Discontinues stream, there could be any amount of line
        // escapes between bytes.
        result = stream->index[stream->cursor - stream->start];
    } else {
        // NOTE(simon): Continues stream, all bytes are right after one
        // another.
        result = stream->index[array_count(stream->buffer)] + (stream->cursor - stream->start);
    }

    return result;
}

internal B32 cproc_buffered_stream_is_end(CProc_BufferedStream *stream) {
    B32 result = false;

    if (stream->start == stream->buffer) {
        // NOTE(simon): Discontinues stream, lookup per character.
        result = stream->is_end[stream->cursor - stream->start];
    } else {
        // NOTE(simon): Continues stream, all bytes are right after one
        // another. Lookup per continues stream.
        result = stream->is_end[array_count(stream->buffer)];
    }

    return result;
}

// NOTE(simon): Always allows you to read 4 characters after the call.
// Pre-condition:  start <= cursor <= end
// Post-condition: start <= cursor && cursor + 4 <= end
internal Void cproc_buffered_stream_refill(CProc_BufferedStream *stream) {
    if (stream->cursor >= stream->mark) {
        U64 source_read = stream->source.cursor - stream->source.start;
        U64 source_left = stream->source.end    - stream->source.cursor;
        U64 buffer_size = array_count(stream->buffer);
        U64 bytes_left  = stream->end - stream->cursor;

        if (source_read >= bytes_left && source_left >= buffer_size) {
            // NOTE(simon): Switch to new buffer
            stream->start    = stream->source.start + source_read - bytes_left;
            stream->cursor   = stream->source.start + source_read - bytes_left;
            stream->end      = stream->source.end;
            stream->mark     = stream->source.end - buffer_size;
            stream->index[buffer_size]  = cproc_stream_index(&stream->source) - bytes_left;
            stream->is_end[buffer_size] = stream->source.is_end;
            stream->source.cursor = stream->source.end;
        } else {
            // NOTE(simon): Switch to temporary transition buffer.
            for (U64 i = 0; i < bytes_left; ++i) {
                stream->buffer[i] = *stream->cursor;
                stream->index[i]  = cproc_buffered_stream_index(stream);
                stream->is_end[i] = cproc_buffered_stream_is_end(stream);
                ++stream->cursor;
            }

            stream->start  = stream->buffer;
            stream->cursor = stream->buffer;
            stream->end    = stream->buffer + buffer_size;
            stream->mark   = stream->end - buffer_size;

            U64 new_cursor_index = bytes_left;
            while (new_cursor_index < buffer_size) {
                if (stream->source.cursor == stream->source.end) {
                    cproc_stream_refill(&stream->source);
                }

                U64 bytes_availible = stream->source.end - stream->source.cursor;
                U64 bytes_needed = buffer_size - new_cursor_index;
                U64 bytes_to_read = u64_min(bytes_availible, bytes_needed);
                for (U64 i = 0; i < bytes_to_read; ++i) {
                    stream->buffer[new_cursor_index] = *stream->source.cursor;
                    stream->index[new_cursor_index]  = cproc_stream_index(&stream->source);
                    stream->is_end[new_cursor_index] = stream->source.is_end;
                    ++stream->source.cursor;
                    ++new_cursor_index;
                }
                stream->index[new_cursor_index] = cproc_stream_index(&stream->source);
            }
        }
    }
}

// NOTE(simon): Assumes that the input begin with either '\u' or \U'.
internal U32 cproc_read_universal_character_name(CProc_BufferedStream *stream) {
    U32 result = { 0 };

    U32 expected_character_count = 2 + (stream->cursor[1] == 'u' ? 4 : 8);
    stream->cursor += 2;

    U32 digits_read = 0;
    for (; digits_read < expected_character_count; ++digits_read) {
        cproc_buffered_stream_refill(stream);

        U8 character = *stream->cursor;
        U32 digit = 0;

        if ('0' <= character && character <= '9') {
            digit = character - '0';
        } else if ('a' <= character && character <= 'f') {
            digit = 10 + character - 'a';
        } else if ('A' <= character && character <= 'F') {
            digit = 10 + character - 'A';
        } else {
            break;
        }

        result = result * 16 + digit;
        ++stream->cursor;
    }

    // TODO(simon): Accept any attempt at a universal character name, but error if it is outside of the ranges.
#define X(low, high) (low <= result && result <= high) ||
    // NOTE(simon): Are we a valid universal character name?
    if (digits_read != expected_character_count || !(CPROC_UNIVERSAL_CHARACTER_RANGES 0)) {
    }
#undef X

    return result;
}

// NOTE(simon): Assumes that the input starts with a '\'.
internal U32 cproc_read_escape_sequence(CProc_BufferedStream *stream) {
    U32 result = { 0 };

    // NOTE(simon): Consume '\'
    ++stream->cursor;

    cproc_buffered_stream_refill(stream);
    U8 character1 = stream->cursor[1];
    switch (character1) {
        // NOTE(simon): Simple escape sequences.
        case '\'': case '"': case '?': case '\\': {
            stream->cursor += 2;
            result = character1;
        } break;
        case 'a': {
            stream->cursor += 2;
            result = '\a';
        } break;
        case 'b': {
            stream->cursor += 2;
            result = '\b';
        } break;
        case 'f': {
            stream->cursor += 2;
            result = '\f';
        } break;
        case 'n': {
            stream->cursor += 2;
            result = '\n';
        } break;
        case 'r': {
            stream->cursor += 2;
            result = '\r';
        } break;
        case 't': {
            stream->cursor += 2;
            result = '\t';
        } break;
        case 'v': {
            stream->cursor += 2;
            result = '\v';
        } break;
        // NOTE(simon): Universal character names.
        case 'u': case 'U': {
            result = cproc_read_universal_character_name(stream);
        } break;
        // NOTE(simon): Hexadecimal escape sequence.
        case 'x': {
            stream->cursor += 2;

            U64 character_count = 0;

            for (;;) {
                cproc_buffered_stream_refill(stream);

                U8 character = *stream->cursor;
                U32 digit = 0;

                if ('0' <= character && character <= '9') {
                    digit = character - '0';
                } else if ('a' <= character && character <= 'f') {
                    digit = 10 + character - 'a';
                } else if ('A' <= character && character <= 'F') {
                    digit = 10 + character - 'A';
                } else {
                    break;
                }

                result = result * 16 + digit;
                ++stream->cursor;
            }

            if (character_count == 0) {
                // TODO(simon): Error if less than one digit is specified.
            }
        } break;
        // NOTE(simon): Octal escape sequence.
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': {
            ++stream->cursor;

            for (;;) {
                U8 character = *stream->cursor;
                U32 digit = 0;

                if ('0' <= character && character <= '7') {
                    digit = character - '0';
                } else {
                    break;
                }

                result = result * 16 + digit;
                ++stream->cursor;
            }
        } break;
        default: {
            // TODO(simon): Error as it is not a valid escape sequence.
        } break;
    }

    return result;
}

internal B32 cproc_token_is_identifier(CProc_Token token, Str8 identifier) {
    B32 result = false;

    if (token.kind == CProc_Token_Identifier) {
        Arena_Temporary scratch = arena_get_scratch(0, 0);

        Str8 token_identifer = cproc_string_from_token(scratch.arena, token);
        if (token_identifer.size == identifier.size) {
            result = memory_equal(token_identifer.data, identifier.data, identifier.size);
        }

        arena_end_temporary(scratch);
    }

    return result;
}

internal Str8 cproc_string_from_token(Arena *arena, CProc_Token token) {
    // NOTE(simon): The maximum number of generated bytes cannot exceed the
    // size of the source string.
    U8 *start  = arena_push_array(arena, U8, token.source.size);
    U8 *end    = start + token.source.size;
    U8 *cursor = start;

    CProc_BufferedStream stream = { 0 };
    stream.source.source_start  = token.source.data;
    stream.source.source_cursor = token.source.data;
    stream.source.source_end    = token.source.data + token.source.size;


    while (!cproc_buffered_stream_is_end(&stream)) {
        cproc_buffered_stream_refill(&stream);

        U32 character = 0;
        if (stream.cursor[0] == '\\') {
            character = cproc_read_escape_sequence(&stream);
        } else {
            StringDecode decode = string_decode_utf8(stream.cursor, stream.end - stream.cursor);
            character = decode.codepoint;
            stream.cursor += decode.size;
        }

        U64 size = string_encode_utf8(cursor, character);
        cursor += size;
    }

    arena_pop_amount(arena, end - cursor);

    Str8 result = str8_range(start, cursor);

    // NOTE(simon): Skip delimiters.
    if (token.kind & CProc_Token_CharacterConstant || token.kind & CProc_Token_StringLiteral || token.kind & CProc_Token_HeaderName) {
        result = str8_skip(result, 1);
        if (!(token.kind & CProc_Token_BrokenDelimiter)) {
            result = str8_chop(result, 1);
        }
    }

    return result;
}

internal CProc_Location cproc_location_from_token(Str8 source, CProc_Token token) {
    CProc_Location result = { 0 };
    result.line     = 1;
    result.column   = 1;

    U8 *start      = source.data;
    U8 *end        = source.data + source.size;
    U8 *line_start = start;

    // NOTE(simon): Find the line.
    for (U8 *cursor = start; cursor < end && cursor < token.source.data;) {
        if (*cursor == '\n') {
            ++result.line;
            ++cursor;
            line_start = cursor;
        } else if (cursor + 1 < end && cursor[0] == '\r' && cursor[1] == '\n') {
            ++result.line;
            cursor += 2;
            line_start = cursor;
        } else {
            ++cursor;
        }
    }

    // NOTE(simon): Find the column.
    for (U8 *cursor = line_start; cursor < end && cursor < token.source.data;) {
        StringDecode decode = string_decode_utf8(cursor, end - cursor);
        ++result.column;
        cursor += decode.size;
    }

    return result;
}

internal CProc_LexerResult cproc_tokens_from_string(Arena *arena, Str8 source) {
    typedef enum {
        State_Normal,
        State_MaybeDirective,
        State_DirectiveStart,
        State_Include,
    } State;
    typedef struct CProc_TokenChunk CProc_TokenChunk;
    struct CProc_TokenChunk {
        CProc_TokenChunk *next;
        CProc_TokenChunk *previous;

        U64     count;
        CProc_Token tokens[1000];
    };
    typedef struct CProc_TokenList CProc_TokenList;
    struct CProc_TokenList {
        CProc_TokenChunk *first;
        CProc_TokenChunk *last;
        U64 token_count;
    };

    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    CProc_BufferedStream stream = { 0 };
    stream.source.source_start  = source.data;
    stream.source.source_cursor = source.data;
    stream.source.source_end    = source.data + source.size;

    State state = State_Normal;
    CProc_TokenList list = { 0 };
    CProc_ErrorList errors = { 0 };

    while (!cproc_buffered_stream_is_end(&stream)) {
        CProc_Token token = { 0 };

        cproc_buffered_stream_refill(&stream);

        token.source.data = cproc_buffered_stream_index(&stream);

        U8 character0 = stream.cursor[0];
        U8 character1 = stream.cursor[1];
        U8 character2 = stream.cursor[2];
        U8 character3 = stream.cursor[3];
        U32 two   = character0 <<  8 | character1;
        U32 three = character0 << 16 | character1 <<  8 | character2;
        U32 four  = character0 << 24 | character1 << 16 | character2 << 8 | character3;

#define CASE2(a, b)       case a << 8 | b
#define CASE3(a, b, c)    case a << 16 | b << 8 | c
        switch (character0) {
            // NOTE(simon): pp-number.
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
                goto cproc_number;
            } break;
            case '_':
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': 
            case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':           case 'v': case 'w': case 'x': case 'y': case 'z': 
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K':           case 'M': 
            case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':           case 'V': case 'W': case 'X': case 'Y': case 'Z':  {
                goto identifier;
            } break;
            // NOTE(simon): Might be starting a character literal or an identifier.
            case 'L': case 'u': case 'U': {
                if (character1 == '\'') {
                    stream.cursor += 2;
                    goto character;
                } else if (character1 == '"') {
                    stream.cursor += 2;
                    goto string;
                } else if (character1 == '8' && character2 == '"') {
                    stream.cursor += 3;
                    goto string;
                } else {
                    goto identifier;
                }
            } break;
            // NOTE(simon): Single character punctuators.
            case '[': case ']': case '(': case ')': case '{': case '}': case '~': case '?': case ';': case ',': {
                token.kind = CProc_Token_Punctuator;
                ++stream.cursor;
            } break;
            // NOTE(simon): Potentially double character punctuators or comments.
            case '-': case '+': case '&': case '|': case '^': case '!': case '*': case '#': case '=': case '/': case ':': {
                switch (two) {
                    CASE2('-', '-'): CASE2('-', '>'): CASE2('-', '='):
                    CASE2('+', '+'): CASE2('+', '='):
                    CASE2('&', '&'): CASE2('&', '='):
                    CASE2('|', '|'): CASE2('|', '='):
                    CASE2('^', '='):
                    CASE2('!', '='):
                    CASE2('*', '='):
                    CASE2('#', '#'):
                    CASE2('=', '='):
                    CASE2('/', '='):
                    CASE2(':', '>'): {
                        token.kind = CProc_Token_Punctuator;
                        stream.cursor += 2;
                    } break;
                    // NOTE(simon): Single line comment. Consume up to but not
                    // including the next new line character.
                    CASE2('/', '/'): {
                        token.kind = CProc_Token_Comment;
                        stream.cursor += 2;

                        while (!cproc_buffered_stream_is_end(&stream) && !(*stream.cursor == '\n' || *stream.cursor == '\r')) {
                            ++stream.cursor;
                            cproc_buffered_stream_refill(&stream);
                        }
                    } break;
                    // NOTE(simon): Multiline comment. Consume up to and
                    // including the next '*/'.
                    CASE2('/', '*'): {
                        token.kind = CProc_Token_Comment;
                        stream.cursor += 2;

                        while (!cproc_buffered_stream_is_end(&stream) && !(stream.cursor[0] == '*' && stream.cursor[1] == '/')) {
                            ++stream.cursor;
                            cproc_buffered_stream_refill(&stream);
                        }

                        // NOTE(simon): Check for unclosed comment.
                        if (stream.cursor[0] == '*' && stream.cursor[1] == '/') {
                            stream.cursor += 2;
                        } else {
                            stream.cursor = stream.end;

                            CProc_Error *error = arena_push_struct_zero(arena, CProc_Error);
                            error->message = str8_literal("Unclosed multiline comment.");
                            error->location = cproc_location_from_token(source, token);
                            sll_queue_push(errors.first, errors.last, error);
                        }
                    } break;
                    default: {
                        token.kind = CProc_Token_Punctuator;
                        ++stream.cursor;
                    } break;
                }
            } break;
            // NOTE(simon): Whitespace.
            case ' ': case '\t': case '\v': case '\f': {
                token.kind = CProc_Token_Whitespace;
                while (*stream.cursor == ' ' || *stream.cursor == '\t' || *stream.cursor == '\v' || *stream.cursor == '\f') {
                    ++stream.cursor;
                    cproc_buffered_stream_refill(&stream);
                }
            } break;
            case '\n': {
                token.kind = CProc_Token_Newline;
                ++stream.cursor;
            } break;
            case '\r': {
                if (character1 == '\n') {
                    token.kind = CProc_Token_Newline;
                    stream.cursor += 2;
                } else {
                    token.kind = CProc_Token_Newline;
                    ++stream.cursor;
                }
            } break;
            // NOTE(simon): '.', pp-number or '...'.
            case '.': {
                if (three == ('.' << 16 | '.' << 8 | '.')) {
                    token.kind = CProc_Token_Punctuator;
                    stream.cursor += 3;
                } else if ('0' <= character1 && character1 <= '9') {
                    goto cproc_number;
                } else {
                    token.kind = CProc_Token_Punctuator;
                    ++stream.cursor;
                }
            } break;
            // NOTE(simon): Potentially header name or tripple character punctuator.
            case '<': {
                if (state == State_Include) {
                    token.kind = CProc_Token_HeaderName;
                    while (!cproc_buffered_stream_is_end(&stream) && !(*stream.cursor == '\n' || *stream.cursor == '\r' || *stream.cursor == '>')) {
                        StringDecode decode = string_decode_utf8(stream.cursor, stream.end - stream.cursor);
                        stream.cursor += decode.size;
                        cproc_buffered_stream_refill(&stream);
                    }

                    if (*stream.cursor == '>') {
                        ++stream.cursor;
                    } else {
                        CProc_Error *error = arena_push_struct_zero(arena, CProc_Error);
                        error->message = str8_literal("Unclosed header-name.");
                        error->location = cproc_location_from_token(source, token);
                        sll_queue_push(errors.first, errors.last, error);
                        token.kind |= CProc_Token_BrokenDelimiter;
                    }
                } else if (three == ('<' << 16 | '<' << 8 | '=')) {
                    token.kind = CProc_Token_Punctuator;
                    stream.cursor += 3;
                } else if (two == ('<' << 8 | '<') || two == ('<' << 8 | '=') || two == ('<' << 8 | ':') || two == ('<' << 8 | '%')) {
                    token.kind = CProc_Token_Punctuator;
                    stream.cursor += 2;
                } else {
                    token.kind = CProc_Token_Punctuator;
                    ++stream.cursor;
                }
            } break;
            // NOTE(simon): Potentially triple character punctuator.
            case '>': {
                if (three == ('>' << 16 | '>' << 8 | '=')) {
                    token.kind = CProc_Token_Punctuator;
                    stream.cursor += 3;
                } else if (two == ('>' << 8 | '>') || two == ('>' << 8 | '=')) {
                    token.kind = CProc_Token_Punctuator;
                    stream.cursor += 2;
                } else {
                    token.kind = CProc_Token_Punctuator;
                    ++stream.cursor;
                }
            } break;
            // NOTE(simon): Potentially quadruple character punctuators.
            case '%': {
                if (four == ('%' << 24 | ':' << 16 | '%' << 8 | ':')) {
                    token.kind = CProc_Token_Punctuator;
                    stream.cursor += 4;
                } else {
                    switch (two) {
                        CASE2('%', '='): CASE2('%', '>'): CASE2('%', ':'): {
                            token.kind = CProc_Token_Punctuator;
                            stream.cursor += 2;
                        } break;
                        default: {
                            token.kind = CProc_Token_Punctuator;
                            ++stream.cursor;
                        } break;
                    }
                }
            } break;
            case '\'': {
                ++stream.cursor;
                goto character;
            } break;
            // NOTE(simon): Header names or strings.
            case '"': {
                if (state == State_Include) {
                    token.kind = CProc_Token_HeaderName;
                    ++stream.cursor;
                    cproc_buffered_stream_refill(&stream);
                    while (!cproc_buffered_stream_is_end(&stream) && !(*stream.cursor == '\n' || *stream.cursor == '\r' || *stream.cursor == '"')) {
                        StringDecode decode = string_decode_utf8(stream.cursor, stream.end - stream.cursor);
                        stream.cursor += decode.size;
                        cproc_buffered_stream_refill(&stream);
                    }

                    if (*stream.cursor == '"') {
                        ++stream.cursor;
                    } else {
                        CProc_Error *error = arena_push_struct_zero(arena, CProc_Error);
                        error->message = str8_literal("Unclosed header-name.");
                        error->location = cproc_location_from_token(source, token);
                        sll_queue_push(errors.first, errors.last, error);
                        token.kind |= CProc_Token_BrokenDelimiter;
                    }
                } else {
                    ++stream.cursor;
                    goto string;
                }
            } break;
            // NOTE(simon): Universal character name starting an identifier or unknown.
            case '\\': {
                if (character0 == '\\' && (character1 == 'u' || character1 == 'U')) {
                    goto identifier;
                } else {
                    token.kind = CProc_Token_Unknown;
                    ++stream.cursor;
                }
            } break;
            default: {
                // NOTE(simon): If it is inside the universal character name
                // ranges, then it is an identifier, otherwise it is unknown.
                StringDecode decode = string_decode_utf8(stream.cursor, stream.end - stream.cursor);
#define X(low, high) (low <= decode.codepoint && decode.codepoint <= high) ||
                if (CPROC_UNIVERSAL_CHARACTER_RANGES 0) {
                    goto identifier;
                } else {
                    token.kind = CProc_Token_Unknown;
                    ++stream.cursor;
                    CProc_Error *error = arena_push_struct_zero(arena, CProc_Error);
                    error->message = str8_literal("Unknown character.");
                    error->location = cproc_location_from_token(source, token);
                }
#undef X
            } break;
        }
#undef CASE3
#undef CASE2

        while (0) {
            identifier: {
                token.kind = CProc_Token_Identifier;
                for (;;) {
                    cproc_buffered_stream_refill(&stream);
                    character0 = stream.cursor[0];
                    character1 = stream.cursor[1];
                    B32 is_digit       = '0' <= character0 && character0 <= '9';
                    B32 is_lower_alpha = 'a' <= character0 && character0 <= 'z';
                    B32 is_upper_alpha = 'A' <= character0 && character0 <= 'Z';

                    if (character0 == '_' || is_digit || is_lower_alpha || is_upper_alpha) {
                        ++stream.cursor;
                    } else if (character0 == '\\' && (character1 == 'u' || character1 == 'U')) {
                        U32 character = cproc_read_universal_character_name(&stream);
                    } else {
                        StringDecode decode = string_decode_utf8(stream.cursor, stream.end - stream.cursor);
#define X(low, high) (low <= decode.codepoint && decode.codepoint <= high) ||
                        if (CPROC_UNIVERSAL_CHARACTER_RANGES 0) {
                            stream.cursor += decode.size;
                        } else {
                            break;
                        }
#undef X
                    }
                }
            } break;
            cproc_number: {
                token.kind = CProc_Token_Number;
                for (;;) {
                    cproc_buffered_stream_refill(&stream);
                    character0 = stream.cursor[0];
                    character1 = stream.cursor[1];
                    B32 is_digit       = '0' <= character0 && character0 <= '9';
                    B32 is_lower_alpha = 'a' <= character0 && character0 <= 'z';
                    B32 is_upper_alpha = 'A' <= character0 && character0 <= 'Z';
                    B32 is_exponent    = character0 == 'e' || character0 == 'E';
                    B32 is_power       = character0 == 'p' || character0 == 'P';
                    B32 has_sign       = character1 == '+' || character1 == '-';

                    if (character0 == '.' || is_digit || character0 == '_' || is_lower_alpha || is_upper_alpha) {
                        ++stream.cursor;
                    } else if ((is_exponent || is_power) && has_sign) {
                        stream.cursor += 2;
                    } else if (character0 == '\\' && (character1 == 'u' || character1 == 'U')) {
                        U32 character = cproc_read_universal_character_name(&stream);
                    } else {
                        StringDecode decode = string_decode_utf8(stream.cursor, stream.end - stream.cursor);

#define X(low, high) (low <= decode.codepoint && decode.codepoint <= high) ||
                        if (CPROC_UNIVERSAL_CHARACTER_RANGES 0) {
                            stream.cursor += decode.size;
                        } else {
                            break;
                        }
#undef X
                    }
                }
            } break;
            character: {
                token.kind = CProc_Token_CharacterConstant;

                U64 character_count = 0;

                while (!cproc_buffered_stream_is_end(&stream) && *stream.cursor != '\'') {
                    if (stream.cursor[0] == '\n' || stream.cursor[0] == '\r') {
                        // NOTE(simon): Not allowed, error is reported as
                        // unclosed character-constant.
                        break;
                    } else if (stream.cursor[0] == '\\') {
                        // NOTE(simon): Escape sequences.
                        U32 character = cproc_read_escape_sequence(&stream);
                        ++character_count;
                    } else {
                        // NOTE(simon): Anything in the source character set, which is all of UTF-8.
                        StringDecode decode = string_decode_utf8(stream.cursor, stream.end - stream.cursor);
                        stream.cursor += decode.size;
                        ++character_count;
                    }
                    cproc_buffered_stream_refill(&stream);
                }

                if (character_count != 1) {
                    CProc_Error *error = arena_push_struct_zero(arena, CProc_Error);
                    error->message = str8_literal("Character constants must have exactly one character.");
                    error->location = cproc_location_from_token(source, token);
                    sll_queue_push(errors.first, errors.last, error);
                }

                if (*stream.cursor == '\'') {
                    ++stream.cursor;
                } else {
                    CProc_Error *error = arena_push_struct_zero(arena, CProc_Error);
                    error->message = str8_literal("Unclosed character-constant.");
                    error->location = cproc_location_from_token(source, token);
                    sll_queue_push(errors.first, errors.last, error);
                    token.kind |= CProc_Token_BrokenDelimiter;
                }
            } break;
            string: {
                token.kind = CProc_Token_StringLiteral;
                while (!cproc_buffered_stream_is_end(&stream) && *stream.cursor != '"') {
                    if (stream.cursor[0] == '\n' || stream.cursor[0] == '\r') {
                        // NOTE(simon): Not allowed, error is reported as
                        // unclosed string-literal.
                        break;
                    } else if (stream.cursor[0] == '\\') {
                        // NOTE(simon): Escape sequences.
                        U32 character = cproc_read_escape_sequence(&stream);
                    } else {
                        // NOTE(simon): Anything in the source character set, which is all of UTF-8.
                        StringDecode decode = string_decode_utf8(stream.cursor, stream.end - stream.cursor);
                        stream.cursor += decode.size;
                    }
                    cproc_buffered_stream_refill(&stream);
                }

                if (*stream.cursor == '"') {
                    ++stream.cursor;
                } else {
                    CProc_Error *error = arena_push_struct_zero(arena, CProc_Error);
                    error->message = str8_literal("Unclosed string-literal.");
                    error->location = cproc_location_from_token(source, token);
                    sll_queue_push(errors.first, errors.last, error);
                    token.kind |= CProc_Token_BrokenDelimiter;
                }
            } break;
        }

        token.source = str8_range(token.source.data, cproc_buffered_stream_index(&stream));

        // NOTE(simon): Update the state machine for if we are in an include or not.
        if (token.kind == CProc_Token_Newline) {
            state = State_MaybeDirective;
        } else {
            if (state == State_MaybeDirective && (token.kind == CProc_Token_Whitespace || token.kind == CProc_Token_Comment)) {
                state = State_MaybeDirective;
            } else if (state == State_MaybeDirective && token.source.size == 1 && token.source.data[0] == '#') {
                state = State_DirectiveStart;
            } else if (state == State_DirectiveStart && (token.kind == CProc_Token_Whitespace || token.kind == CProc_Token_Comment)) {
                state = State_DirectiveStart;
            } else if (state == State_DirectiveStart && cproc_token_is_identifier(token, str8_literal("include"))) {
                state = State_Include;
            } else if (state == State_Include) {
                state = State_Include;
            } else {
                state = State_Normal;
            }
        }

        // NOTE(simon): Add to list.
        CProc_TokenChunk *chunk = list.last;
        if (!chunk || chunk->count >= array_count(chunk->tokens)) {
            chunk = arena_push_struct_zero(scratch.arena, CProc_TokenChunk);
            dll_push_back(list.first, list.last, chunk);
        }

        chunk->tokens[chunk->count] = token;
        ++chunk->count;
        ++list.token_count;
    }

    // NOTE(simon): Compact into an array.
    CProc_TokenArray tokens = { 0 };
    tokens.tokens = arena_push_array(arena, CProc_Token, list.token_count);
    for (CProc_TokenChunk *chunk = list.first; chunk; chunk = chunk->next) {
        memory_copy(&tokens.tokens[tokens.count], chunk->tokens, chunk->count * sizeof(*chunk->tokens));
        tokens.count += chunk->count;
    }

    arena_end_temporary(scratch);

    CProc_LexerResult result = { 0 };
    result.tokens = tokens;
    result.errors = errors;

    return result;
}
