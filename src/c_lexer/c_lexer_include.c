#define PP_UNIVERSAL_CHARACTER_RANGES \
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

typedef enum {
    PPToken_HeaderName,
    PPToken_Identifier,
    PPToken_Number,
    PPToken_CharacterConstant,
    PPToken_StringLiteral,
    PPToken_Punctuator,
    PPToken_Whitespace,
    PPToken_Newline,
    PPToken_Comment,
    PPToken_Unknown,
    PPToken_EndOfFile,
} PPToken_Kind;

typedef struct PPToken PPToken;
struct PPToken {
    PPToken_Kind kind;
    Str8 raw;
};

typedef struct PP_Character PP_Character;
struct PP_Character {
    U32 codepoint;
    U32 size;
};

typedef struct PP_State PP_State;
struct PP_State {
    Str8Node *segment;
    U8 *cursor;

    Str8List segments;
};

typedef struct PP_Stream PP_Stream;
struct PP_Stream {
    U8 *start;
    U8 *end;
    U8 *cursor;

    U64 index;
    B32 is_end;

    U8 *source_start;
    U8 *source_cursor;
    U8 *source_end;
};

internal U64 pp_stream_index(PP_Stream *stream) {
    U64 result  = stream->index + (stream->cursor - stream->start);
    return result;
}

internal Void pp_stream_refill(PP_Stream *stream) {
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

typedef struct PP_BufferedStream PP_BufferedStream;
struct PP_BufferedStream {
    U8 *start;
    U8 *cursor;
    U8 *end;
    U8 *mark;

    U8  buffer[5];
    U64 index[5];

    PP_Stream source;
};

internal U64 pp_buffered_stream_index(PP_BufferedStream *stream) {
    U64 result = 0;

    if (stream->start == stream->buffer) {
        // NOTE(simon): Discontinues stream, there could be any amount of line
        // escapes between bytes.
        result = stream->index[stream->cursor - stream->start];
    } else {
        // NOTE(simon): Continues stream, all bytes are right after one
        // another.
        result = stream->index[0] + (stream->cursor - stream->start);
    }

    return result;
}

// NOTE(simon): Always allows you to read 4 characters after the call.
// Pre-condition:  start <= cursor <= end
// Post-condition: start <= cursor && cursor + 4 <= end
internal Void pp_buffered_stream_refill(PP_BufferedStream *stream) {
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
            stream->index[0] = pp_stream_index(&stream->source) - bytes_left;
            stream->source.cursor = stream->source.end;
        } else {
            // NOTE(simon): Switch to temporary transition buffer.
            for (U64 i = 0; i < bytes_left; ++i) {
                stream->buffer[i] = *stream->cursor;
                stream->index[i]  = pp_buffered_stream_index(stream);
                ++stream->cursor;
            }

            stream->start  = stream->buffer;
            stream->cursor = stream->buffer;
            stream->end    = stream->buffer + buffer_size;
            stream->mark   = stream->end - buffer_size;

            U64 new_cursor_index = bytes_left;
            while (new_cursor_index < buffer_size) {
                if (stream->source.cursor == stream->source.end) {
                    pp_stream_refill(&stream->source);
                }

                U64 bytes_availible = stream->source.end - stream->source.cursor;
                U64 bytes_needed = buffer_size - new_cursor_index;
                U64 bytes_to_read = u64_min(bytes_availible, bytes_needed);
                for (U64 i = 0; i < bytes_to_read; ++i) {
                    stream->buffer[new_cursor_index] = *stream->source.cursor;
                    stream->index[new_cursor_index]  = pp_stream_index(&stream->source);
                    ++stream->source.cursor;
                    ++new_cursor_index;
                }
            }
        }
    }
}

internal B32 pp_has_at_least(PP_State *state, U64 amount) {
    // NOTE(simon): Decrement one as we need to count the character the cursor
    // is on.
    --amount;

    Str8Node *segment = state->segment;
    U8 *cursor = state->cursor;
    while (segment && cursor + amount >= segment->string.data + segment->string.size) {
        amount -= segment->string.size - (cursor - segment->string.data);
        segment = segment->next;
        cursor = segment ? segment->string.data : 0;
    }

    B32 result = segment != 0;

    return result;
}

internal U8 pp_peek(PP_State *state, U64 amount) {
    Str8Node *segment = state->segment;
    U8 *cursor = state->cursor;
    while (segment && cursor + amount >= segment->string.data + segment->string.size) {
        amount -= segment->string.size - (cursor - segment->string.data);
        segment = segment->next;
        cursor = segment ? segment->string.data : 0;
    }

    U8 result = 0;
    if (segment) {
        result = cursor[amount];
    }

    return result;
}

internal Void pp_eat(PP_State *state, U64 amount) {
    Str8Node *segment = state->segment;
    U8 *cursor = state->cursor;
    while (segment && cursor + amount >= segment->string.data + segment->string.size) {
        amount -= segment->string.size - (cursor - segment->string.data);
        segment = segment->next;
        cursor = segment ? segment->string.data : 0;
    }

    if (segment) {
        state->cursor = cursor + amount;
    }
    state->segment = segment;
}

// NOTE(simon): Assumes that the input begin with either '\u' or \U'.
internal PP_Character pp_read_universal_character_name(PP_State *state) {
    PP_Character result = { 0 };

    U32 expected_character_count = 2 + (pp_peek(state, 1) == 'u' ? 4 : 8);
    pp_eat(state, 2);

    U32 digits_read = 0;
    for (; digits_read < expected_character_count; ++digits_read) {
        U8 character = pp_peek(state, 0);
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

        result.codepoint = result.codepoint * 16 + digit;
        pp_eat(state, 1);
    }

    // TODO(simon): Accept any attempt at a universal character name, but error if it is outside of the ranges.
#define X(low, high) (low <= result.codepoint && result.codepoint <= high) ||
    // NOTE(simon): Are we a valid universal character name?
    if (digits_read != expected_character_count || !(PP_UNIVERSAL_CHARACTER_RANGES 0)) {
    }
#undef X

    return result;
}

// NOTE(simon): Assumes that the input starts with a '\'.
internal PP_Character pp_read_escape_sequence(PP_State *state) {
    PP_Character result = { 0 };

    // NOTE(simon): Consume '\'
    pp_eat(state, 1);

    U8 character1 = pp_peek(state, 0);
    switch (character1) {
        // NOTE(simon): Simple escape sequences.
        case '\'': case '"': case '?': case '\\': {
            pp_eat(state, 1);
            result.codepoint = character1;
        } break;
        case 'a': {
            pp_eat(state, 1);
            result.codepoint = '\a';
        } break;
        case 'b': {
            pp_eat(state, 1);
            result.codepoint = '\b';
        } break;
        case 'f': {
            pp_eat(state, 1);
            result.codepoint = '\f';
        } break;
        case 'n': {
            pp_eat(state, 1);
            result.codepoint = '\n';
        } break;
        case 'r': {
            pp_eat(state, 1);
            result.codepoint = '\r';
        } break;
        case 't': {
            pp_eat(state, 1);
            result.codepoint = '\t';
        } break;
        case 'v': {
            pp_eat(state, 1);
            result.codepoint = '\v';
        } break;
        // NOTE(simon): Universal character names.
        case 'u': case 'U': {
            result = pp_read_universal_character_name(state);
        } break;
        // NOTE(simon): Hexadecimal escape sequence.
        case 'x': {
            pp_eat(state, 1);

            for (; pp_has_at_least(state, 1); pp_eat(state, 1)) {
                U8 character = pp_peek(state, 1);
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

                result.codepoint = result.codepoint * 16 + digit;
            }

            // TODO(simon): Error if less than one digit is specified.
        } break;
        // NOTE(simon): Octal escape sequence.
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': {
            for (; pp_has_at_least(state, 1); pp_eat(state, 1)) {
                U8 character = pp_peek(state, 0);
                U32 digit = 0;

                if ('0' <= character && character <= '7') {
                    digit = character - '0';
                } else {
                    break;
                }

                result.codepoint = result.codepoint * 16 + digit;
            }
        } break;
        default: {
            // TODO(simon): Error as it is not a valid escape sequence.
        } break;
    }

    return result;
}

internal Str8List pp_segments_from_string(Arena *arena, Str8 string) {
    Str8List segments = { 0 };

    U8 *segment_start = string.data;
    U8 *segment_end   = segment_start;
    U8 *end = string.data + string.size;

    while (segment_end + 2 < end) {
        if (segment_end[0] == '\\' && segment_end[1] == '\n') {
            if (segment_start < segment_end) {
                str8_list_push(arena, &segments, str8_range(segment_start, segment_end));
            }
            segment_start = segment_end + 2;
            segment_end   = segment_start;
        } else if (segment_end[0] == '\\' && segment_end[1] == '\r' && segment_end[2] == '\n') {
            if (segment_start < segment_end) {
                str8_list_push(arena, &segments, str8_range(segment_start, segment_end));
            }
            segment_start = segment_end + 3;
            segment_end   = segment_start;
        } else {
            ++segment_end;
        }
    }

    // NOTE(simon): We migth still have a line escape left.
    if (segment_end + 1 < end && segment_end[0] == '\\' && segment_end[1] == '\n') {
        if (segment_start < segment_end) {
            str8_list_push(arena, &segments, str8_range(segment_start, segment_end));
        }
        segment_start = segment_end + 2;
        segment_end   = segment_start;
    }

    // NOTE(simon): There might be a segment that hasn't been outputed.
    if (segment_end < end) {
        str8_list_push(arena, &segments, str8_range(segment_start, end));
    }

    return segments;
}

internal PPToken pp_next_token(PP_State *state, B32 read_header_names) {
    PPToken token = { 0 };
    token.raw.data = state->cursor;

    if (!pp_has_at_least(state, 1)) {
        token.kind = PPToken_EndOfFile;
    } else {
        U8 character0 = pp_peek(state, 0);
        U8 character1 = pp_peek(state, 1);
        U8 character2 = pp_peek(state, 2);
        U8 character3 = pp_peek(state, 3);
        U32 two   = *state->cursor <<  8 | character1;
        U32 three = *state->cursor << 16 | character1 <<  8 | character2;
        U32 four  = *state->cursor << 24 | character1 << 16 | character2 << 8 | character3;

#define CASE2(a, b)       case a << 8 | b
#define CASE3(a, b, c)    case a << 16 | b << 8 | c
        switch (*state->cursor) {
            // NOTE(simon): pp-number.
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
                goto pp_number;
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
                    pp_eat(state, 2);
                    goto character;
                } else if (character1 == '"') {
                    pp_eat(state, 2);
                    goto string;
                } else if (character1 == '8' && character2 == '"') {
                    pp_eat(state, 3);
                    goto string;
                } else {
                    goto identifier;
                }
            } break;
            // NOTE(simon): Single character punctuators.
            case '[': case ']': case '(': case ')': case '{': case '}': case '~': case '?': case ';': case ',': {
                token.kind = PPToken_Punctuator;
                pp_eat(state, 1);
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
                        token.kind = PPToken_Punctuator;
                        pp_eat(state, 2);
                    } break;
                    // NOTE(simon): Single line comment. Consume up to but not
                    // including the next new line character.
                    CASE2('/', '/'): {
                        token.kind = PPToken_Comment;
                        while (pp_has_at_least(state, 1) && pp_peek(state, 0) != '\n' && pp_peek(state, 0) != '\r') {
                            pp_eat(state, 1);
                        }
                    } break;
                    // NOTE(simon): Multiline comment. Consume up to and
                    // including the next '*/'.
                    CASE2('/', '*'): {
                        token.kind = PPToken_Comment;
                        state->cursor += 2;

                        while (pp_has_at_least(state, 2) && !(pp_peek(state, 0) == '*' && pp_peek(state, 1) == '/')) {
                            pp_eat(state, 1);
                        }

                        // NOTE(simon): Check for unclosed comment.
                        if (pp_has_at_least(state, 2) && pp_peek(state, 0) == '*' && pp_peek(state, 1) == '/') {
                            pp_eat(state, 2);
                        } else {
                            // TODO:(simon): Unclosed comment, report error.
                        }
                    } break;
                    default: {
                        token.kind = PPToken_Punctuator;
                        pp_eat(state, 1);
                    } break;
                }
            } break;
            // NOTE(simon): Whitespace.
            // TODO(simon): Is it correct to interpret '\v' and '\f' as
            // whitespace instead of newlines?
            case ' ': case '\t': case '\v': case '\f': {
                token.kind = PPToken_Whitespace;
                while (pp_has_at_least(state, 0) && (pp_peek(state, 0) == ' ' || pp_peek(state, 0) == '\t' || pp_peek(state, 0) == '\v' || pp_peek(state, 0) == '\f')) {
                    pp_eat(state, 1);
                }
            } break;
            case '\n': {
                token.kind = PPToken_Newline;
                pp_eat(state, 1);
            } break;
            case '\r': {
                if (character1 == '\n') {
                    token.kind = PPToken_Newline;
                    pp_eat(state, 2);
                } else {
                    token.kind = PPToken_Unknown;
                    pp_eat(state, 1);
                }
            } break;
            // NOTE(simon): '.', pp-number or '...'.
            case '.': {
                if (three == ('.' << 16 | '.' << 8 | '.')) {
                    token.kind = PPToken_Punctuator;
                    pp_eat(state, 3);
                } else if ('0' <= character1 && character1 <= '9') {
                    goto pp_number;
                } else {
                    token.kind = PPToken_Punctuator;
                    pp_eat(state, 1);
                }
            } break;
            // NOTE(simon): Potentially header name or tripple character punctuator.
            case '<': {
                if (read_header_names) {
                    token.kind = PPToken_HeaderName;
                    pp_eat(state, 1);
                    while (pp_has_at_least(state, 1) && !(pp_peek(state, 0) == '\n' || pp_peek(state, 0) == '\r' || pp_peek(state, 0) == '>')) {
                        U8 buffer[4] = { pp_peek(state, 0), pp_peek(state, 1), pp_peek(state, 2), pp_peek(state, 3), };
                        StringDecode decode = string_decode_utf8(buffer, array_count(buffer));
                        pp_eat(state, decode.size);
                    }

                    if (pp_has_at_least(state, 1) && pp_peek(state, 0) == '>') {
                        pp_eat(state, 1);
                    } else {
                        // TODO(simon): Error on unclosed header name.
                    }
                } else if (three == ('<' << 16 | '<' << 8 | '=')) {
                    token.kind = PPToken_Punctuator;
                    pp_eat(state, 3);
                } else if (two == ('<' << 8 | '<') || two == ('<' << 8 | '=') || two == ('<' << 8 | ':') || two == ('<' << 8 | '%')) {
                    token.kind = PPToken_Punctuator;
                    pp_eat(state, 2);
                } else {
                    token.kind = PPToken_Punctuator;
                    pp_eat(state, 1);
                }
            } break;
            // NOTE(simon): Potentially triple character punctuator.
            case '>': {
                if (three == ('>' << 16 | '>' << 8 | '=')) {
                    token.kind = PPToken_Punctuator;
                    pp_eat(state, 3);
                } else if (two == ('>' << 8 | '>') || two == ('>' << 8 | '=')) {
                    token.kind = PPToken_Punctuator;
                    pp_eat(state, 2);
                } else {
                    token.kind = PPToken_Punctuator;
                    pp_eat(state, 1);
                }
            } break;
            // NOTE(simon): Potentially quadruple character punctuators.
            case '%': {
                if (four == ('%' << 24 | ':' << 16 | '%' << 8 | ':')) {
                    token.kind = PPToken_Punctuator;
                    pp_eat(state, 4);
                } else {
                    switch (two) {
                        CASE2('%', '='): CASE2('%', '>'): CASE2('%', ':'): {
                            token.kind = PPToken_Punctuator;
                            pp_eat(state, 2);
                        } break;
                        default: {
                            token.kind = PPToken_Punctuator;
                            pp_eat(state, 1);
                        } break;
                    }
                }
            } break;
            case '\'': {
                pp_eat(state, 1);
                goto character;
            } break;
            // NOTE(simon): Header names or strings.
            case '"': {
                if (read_header_names) {
                    token.kind = PPToken_HeaderName;
                    pp_eat(state, 1);
                    while (pp_has_at_least(state, 1) && !(pp_peek(state, 0) == '\n' || pp_peek(state, 0) == '\r' || pp_peek(state, 0) == '"')) {
                        U8 buffer[4] = { pp_peek(state, 0), pp_peek(state, 1), pp_peek(state, 2), pp_peek(state, 3), };
                        StringDecode decode = string_decode_utf8(buffer, array_count(buffer));
                        pp_eat(state, decode.size);
                    }

                    if (pp_has_at_least(state, 1) && pp_peek(state, 0) == '"') {
                        pp_eat(state, 1);
                    } else {
                        // TODO(simon): Error on unclosed header name.
                    }
                } else {
                    pp_eat(state, 1);
                    goto string;
                }
            } break;
            // NOTE(simon): Universal character name starting an identifier or unknown.
            case '\\': {
                if (character0 == '\\' && (character1 == 'u' || character1 == 'U')) {
                    goto identifier;
                } else {
                    token.kind = PPToken_Unknown;
                    pp_eat(state, 1);
                }
            } break;
            default: {
                // NOTE(simon): If it is inside the universal character name
                // ranges, then it is an identifier, otherwise it is unknown.
                U8 buffer[4] = { pp_peek(state, 0), pp_peek(state, 1), pp_peek(state, 2), pp_peek(state, 3), };
                StringDecode decode = string_decode_utf8(buffer, array_count(buffer));
#define X(low, high) (low <= decode.codepoint && decode.codepoint <= high) ||
                if (PP_UNIVERSAL_CHARACTER_RANGES 0) {
                    goto identifier;
                } else {
                    token.kind = PPToken_Unknown;
                    pp_eat(state, 1);
                }
#undef X
            } break;
        }
#undef CASE3
#undef CASE2

        while (0) {
            identifier: {
                token.kind = PPToken_Identifier;
                while (pp_has_at_least(state, 1)) {
                    character0 = pp_peek(state, 0);
                    character1 = pp_peek(state, 1);
                    B32 is_digit       = '0' <= character0 && character0 <= '9';
                    B32 is_lower_alpha = 'a' <= character0 && character0 <= 'z';
                    B32 is_upper_alpha = 'A' <= character0 && character0 <= 'Z';
                    if (character0 == '_' || is_digit || is_lower_alpha || is_upper_alpha) {
                        pp_eat(state, 1);
                    } else if (character0 == '\\' && (character1 == 'u' || character1 == 'U')) {
                        PP_Character character = pp_read_universal_character_name(state);
                    } else {
                        U8 buffer[4] = { pp_peek(state, 0), pp_peek(state, 1), pp_peek(state, 2), pp_peek(state, 3), };
                        StringDecode decode = string_decode_utf8(buffer, array_count(buffer));
#define X(low, high) (low <= decode.codepoint && decode.codepoint <= high) ||
                        if (PP_UNIVERSAL_CHARACTER_RANGES 0) {
                            pp_eat(state, decode.size);
                        } else {
                            break;
                        }
#undef X
                    }
                }
            } break;
            pp_number: {
                token.kind = PPToken_Number;
                while (pp_has_at_least(state, 1)) {
                    character0 = pp_peek(state, 0);
                    character1 = pp_peek(state, 1);

                    B32 is_digit       = '0' <= character0 && character0 <= '9';
                    B32 is_lower_alpha = 'a' <= character0 && character0 <= 'z';
                    B32 is_upper_alpha = 'A' <= character0 && character0 <= 'Z';
                    B32 is_exponent    = character0 == 'e' || character0 == 'E';
                    B32 is_power       = character0 == 'p' || character0 == 'P';
                    B32 has_sign       = character1 == '+' || character1 == '-';

                    if (character0 == '.' || is_digit || character0 == '_' || is_lower_alpha || is_upper_alpha) {
                        pp_eat(state, 1);
                    } else if ((is_exponent || is_power) && has_sign) {
                        pp_eat(state, 2);
                    } else if (character0 == '\\' && (character1 == 'u' || character1 == 'U')) {
                        PP_Character character = pp_read_universal_character_name(state);
                    } else {
                        U8 buffer[4] = { pp_peek(state, 0), pp_peek(state, 1), pp_peek(state, 2), pp_peek(state, 3), };
                        StringDecode decode = string_decode_utf8(buffer, array_count(buffer));

#define X(low, high) (low <= decode.codepoint && decode.codepoint <= high) ||
                        if (PP_UNIVERSAL_CHARACTER_RANGES 0) {
                            pp_eat(state, decode.size);
                        } else {
                            break;
                        }
#undef X
                    }
                }
            } break;
            character: {
                token.kind = PPToken_CharacterConstant;

                while (pp_has_at_least(state, 1) && pp_peek(state, 0) != '\'') {
                    switch (*state->cursor) {
                        // NOTE(simon): Not allowed.
                        case '\n': case '\r': {
                            pp_eat(state, 1);
                            // TODO(simon): Error
                        } break;
                        // NOTE(simon): Escape sequences.
                        case '\\': {
                            PP_Character character = pp_read_escape_sequence(state);
                        } break;
                        // NOTE(simon): Anything in the source character set, which is all of UTF-8.
                        default: {
                            U8 buffer[4] = { pp_peek(state, 0), pp_peek(state, 1), pp_peek(state, 2), pp_peek(state, 3), };
                            StringDecode decode = string_decode_utf8(buffer, array_count(buffer));
                            pp_eat(state, decode.size);
                        } break;
                    }
                }

                // TODO(simon): Error if no characters are read.

                if (pp_has_at_least(state, 1) && pp_peek(state, 0) == '\'') {
                    pp_eat(state, 1);
                } else {
                    // TODO(simon): Error on unclosed character constant.
                }
            } break;
            string: {
                token.kind = PPToken_StringLiteral;
                while (pp_has_at_least(state, 1) && pp_peek(state, 0) != '"') {
                    switch (*state->cursor) {
                        // NOTE(simon): Not allowed.
                        case '\n': case '\r': {
                            pp_eat(state, 1);
                            // TODO(simon): Error
                        } break;
                        // NOTE(simon): Escape sequences.
                        case '\\': {
                            PP_Character character = pp_read_escape_sequence(state);
                        } break;
                        // NOTE(simon): Anything in the source character set, which is all of UTF-8.
                        default: {
                            U8 buffer[4] = { pp_peek(state, 0), pp_peek(state, 1), pp_peek(state, 2), pp_peek(state, 3), };
                            StringDecode decode = string_decode_utf8(buffer, array_count(buffer));
                            pp_eat(state, decode.size);
                        } break;
                    }
                }

                if (pp_has_at_least(state, 1) && pp_peek(state, 0) == '"') {
                    pp_eat(state, 1);
                } else {
                    // TODO(simon): Error on unclosed string.
                }
            } break;
        }
    }

    token.raw = str8_range(token.raw.data, state->cursor);
    return token;
}

internal Void c_lexer_test(Void) {
    Arena *arena = arena_create();

    Str8 source = { 0 };
    if (os_file_read(arena, str8_literal("test"), &source)) {
        PP_State state = { 0 };
        state.segments = pp_segments_from_string(arena, source);
        state.segment = state.segments.first;
        state.cursor = state.segment->string.data;

        Str8List output_list = { 0 };

        for (;;) {
            PPToken token = pp_next_token(&state, true);
            CStr token_kind = 0;
            switch (token.kind) {
                case PPToken_HeaderName:        token_kind = "HeaderName";        break;
                case PPToken_Identifier:        token_kind = "Identifier";        break;
                case PPToken_Number:            token_kind = "Number";            break;
                case PPToken_CharacterConstant: token_kind = "CharacterConstant"; break;
                case PPToken_StringLiteral:     token_kind = "StringLiteral";     break;
                case PPToken_Punctuator:        token_kind = "Punctuator";        break;
                case PPToken_Whitespace:        token_kind = "Whitespace";        break;
                case PPToken_Newline:           token_kind = "Newline";           break;
                case PPToken_Comment:           token_kind = "Comment";           break;
                case PPToken_Unknown:           token_kind = "Unknown";           break;
                case PPToken_EndOfFile:         token_kind = "EndOfFile";         break;
            }
            str8_list_push(arena, &output_list, str8_format(arena, "%.*s: %s\n", str8_expand(token.raw), token_kind));
            if (token.kind == PPToken_EndOfFile) {
                break;
            }
        }

        Str8 output = str8_join(arena, &output_list);
        os_console_print(output);
    }

    arena_destroy(arena);
}
