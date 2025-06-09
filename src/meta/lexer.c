internal V2U64 location_from_source_pointer(Str8 source, U8 *location) {
    V2U64 result = { 0 };

    if (source.data <= location && location < source.data + source.size) {
        result = v2u64(1, 1);

        U8 *ptr = source.data;
        U8 *opl = source.data + source.size;
        while (ptr < location) {
            if (*ptr == '\n') {
                ++ptr;
                result.x = 1;
                ++result.y;
            } else if (*ptr == '\r') {
                ++ptr;
                if (ptr < opl && *ptr == '\n') {
                    ++ptr;
                }
                result.x = 1;
                ++result.y;
            } else if (*ptr == '\t') {
                ++ptr;
                result.x += 4;
            } else {
                StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                ptr += decode.size;
                ++result.x;
            }
        }
    }

    return result;
}

internal V2U64 location_from_source_token(Str8 source, Token token) {
    V2U64 result = location_from_source_pointer(source, token.string.data);
    return result;
}

internal TokenArray tokens_from_string(Arena *arena, Str8 name, Str8 source) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);
    TokenList token_list = { 0 };

    B32 start_of_line    = true;
    B32 in_pre_processor = false;
    for (U8 *ptr = source.data, *opl = source.data + source.size; ptr < opl;) {
        // NOTE(simon): Skip whitespace and identify pre-processor statements.
        while (ptr < opl) {
            if (*ptr == ' ' || *ptr == '\t') {
                ++ptr;
            } else if (*ptr == '\n') {
                start_of_line = true;
                in_pre_processor = false;
                ++ptr;
            } else if (*ptr == '\r') {
                start_of_line = true;
                in_pre_processor = false;
                ++ptr;

                if (ptr < opl && *ptr == '\n') {
                    ++ptr;
                }
            } else if (*ptr == '\\') {
                if (!in_pre_processor) {
                    V2U64 location = location_from_source_pointer(source, ptr);
                    log_warning_format("%.*s:%lu:%lu: Use of '\\' outside of pre-processor directives, this was probably not intended.\n", str8_expand(name), location.y, location.x);
                }

                ++ptr;
                if (ptr < opl && *ptr == '\n') {
                    ++ptr;
                } else if (ptr < opl && *ptr == '\r') {
                    ++ptr;
                    if (ptr < opl && *ptr == '\n') {
                        ++ptr;
                    }
                } else {
                    V2U64 location = location_from_source_pointer(source, ptr);
                    log_error_format("%.*s:%lu:%lu: Use of '\\' other than as a line continuation.\n", str8_expand(name), location.y, location.x);
                }
            } else if (start_of_line && *ptr == '#') {
                start_of_line = false;
                in_pre_processor = true;
                ++ptr;
            } else {
                break;
            }
        }

        start_of_line = false;

        // NOTE(simon): Done if no more characters.
        if (ptr >= opl) {
            break;
        }

        Token token = { 0 };
        if (in_pre_processor) {
            token.flags |= Token_Flag_PreProcessor;
        }

        U8 *start = ptr;
        switch (*ptr) {
            // NOTE(simon): Single-character symbols.
            case '(': case ')': case ',': case ':':
            case ';': case '?': case '[': case ']':
            case '{': case '}': case '~': {
                token.kind = Token_Symbol;
                ++ptr;
            } break;

            // NOTE(simon): Optional equals.
            case '!': case '%': case '*': case '=':
            case '^': {
                token.kind = Token_Symbol;

                if (ptr + 2 <= opl && ptr[1] == '=') {
                    ptr += 2;
                } else {
                    ++ptr;
                }
            } break;

            // NOTE(simon): Optional duplicate or equals.
            case '&': case '+': case '<': case '>':
            case '|': {
                token.kind = Token_Symbol;

                if (ptr + 2 <= opl && (ptr[1] == ptr[0] || ptr[1] == '=')) {
                    ptr += 2;
                } else {
                    ++ptr;
                }
            } break;

            // NOTE(simon): Other simple symbols.
            case '#': {
                token.kind = Token_Symbol;

                if (ptr + 2 <= opl && ptr[1] == '#') {
                    ptr += 2;
                } else {
                    ++ptr;
                }
            } break;
            case '-': {
                token.kind = Token_Symbol;

                if (ptr + 2 < opl && (ptr[1] == '-' || ptr[1] == '=' || ptr[1] == '>')) {
                    ptr += 2;
                } else {
                    ++ptr;
                }
            } break;

            // NOTE(simon): Division and comments.
            case '/': {
                ++ptr;

                if (ptr < opl && *ptr == '/') {
                    token.kind = Token_Comment;
                    ++ptr;

                    while (ptr < opl) {
                        if (*ptr == '\n' || *ptr == '\r') {
                            break;
                        }

                        StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                        ptr += decode.size;
                    }
                } else if (ptr < opl && *ptr == '*') {
                    token.kind = Token_Comment;
                    ++ptr;

                    B32 closed = false;

                    while (ptr < opl) {
                        if (ptr + 2 <= opl && ptr[0] == '*' && ptr[1] == '/') {
                            ptr += 2;
                            closed = true;
                            break;
                        } else if (ptr + 2 <= opl && ptr[0] == '/' && ptr[1] == '*') {
                            V2U64 location = location_from_source_pointer(source, ptr);
                            log_error_format("%.*s:%lu:%lu: Nested block comments are not allowed.\n", str8_expand(name), location.y, location.x);
                        } else if (*ptr == '\n') {
                            ++ptr;
                        } else if (*ptr == '\r') {
                            ++ptr;
                            if (ptr < opl && *ptr == '\n') {
                                ++ptr;
                            }
                        } else {
                            StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                            ptr += decode.size;
                        }
                    }

                    if (!closed) {
                        V2U64 location = location_from_source_pointer(source, start);
                        log_error_format("%.*s:%lu:%lu: Unclosed block comment.\n", str8_expand(name), location.y, location.x);
                    }
                } else if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;

            // NOTE(simon): Character and string literals.
            case '"': case '\'': {
                U8 opening = *ptr;
                ++ptr;

                if (opening == '"') {
                    token.kind = Token_StringLiteral;
                } else {
                    token.kind = Token_CharacterLiteral;
                }

                B32 closed = false;
                while (ptr < opl) {
                    if (*ptr == '\n' || *ptr == '\r') {
                        break;
                    } else if (*ptr == opening) {
                        ++ptr;
                        closed = true;
                        break;
                    }

                    StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                    ptr += decode.size;
                    if (decode.codepoint == '\\') {
                        StringDecode second_decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                        ptr += second_decode.size;
                    }
                }

                if (!closed) {
                    V2U64 location = location_from_source_pointer(source, start);
                    if (opening == '"') {
                        log_error_format("%.*s:%lu:%lu: Unclosed string literal.\n", str8_expand(name), location.y, location.x);
                    } else {
                        log_error_format("%.*s:%lu:%lu: Unclosed character literal.\n", str8_expand(name), location.y, location.x);
                    }
                }
            } break;

            // NOTE(simon): Numbers, identifiers, and unknowns.
            default: {
                if ((*ptr == '0' <= *ptr && *ptr <= '9') || *ptr == '.') {
                    B32 is_zero = *ptr == '0';
                    B32 is_dot  = *ptr == '.';
                    ++ptr;

                    if (is_zero && ptr < opl && (*ptr == 'b' || *ptr == 'B')) {
                        // NOTE(simon): Binary literals.
                        token.kind = Token_IntegerLiteral;
                        ++ptr;

                        while (ptr < opl && '0' <= *ptr && *ptr <= '1') {
                            ++ptr;
                        }
                    } else if (is_zero && ptr < opl && (*ptr == 'x' || *ptr == 'X')) {
                        // NOTE(simon): Hexadecimal literals.
                        token.kind = Token_IntegerLiteral;
                        ++ptr;

                        while (ptr < opl && (('0' <= *ptr && *ptr <= '9') || ('a' <= *ptr && *ptr <= 'f') || ('A' <= *ptr && *ptr <= 'F'))) {
                            ++ptr;
                        }
                    } else {
                        // NOTE(simon): Integers or floats starting with '.'.
                        if (is_dot) {
                            token.kind = Token_FloatLiteral;
                        } else {
                            token.kind = Token_IntegerLiteral;
                        }
                        while (ptr < opl && '0' <= *ptr && *ptr <= '9') {
                            ++ptr;
                        }
                    }

                    // NOTE(simon): Floats.
                    if (!is_dot && ptr < opl && *ptr == '.') {
                        token.kind = Token_FloatLiteral;
                        ++ptr;

                        while (ptr < opl && '0' <= *ptr && *ptr <= '9') {
                            ++ptr;
                        }
                    }

                    // NOTE(simon): Exponents.
                    if (!(is_dot && ptr - start == 1) && ptr < opl && *ptr == 'e') {
                        token.kind = Token_FloatLiteral;
                        ++ptr;

                        if (ptr < opl && (*ptr == '+' || *ptr == '-')) {
                            ++ptr;
                        }

                        while (ptr < opl && '0' <= *ptr && *ptr <= '9') {
                            ++ptr;
                        }
                    }

                    // NOTE(simon): It was the '.' operator.
                    if (is_dot && ptr - start == 1) {
                        token.kind = Token_Symbol;
                    }
                } else if (('a' <= *ptr && *ptr <= 'z') || ('A' <= *ptr && *ptr <= 'Z') || *ptr == '_') {
                    ++ptr;
                    token.kind = Token_Identifier;
                    while (('0' <= *ptr && *ptr <= '9') || ('a' <= *ptr && *ptr <= 'z') || ('A' <= *ptr && *ptr <= 'Z') || *ptr == '_') {
                        ++ptr;
                    }
                } else {
                    token.kind = Token_Unknown;
                    StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                    ptr += decode.size;
                }
            } break;
        }

        token.string = str8_range(start, ptr);

        TokenChunk *chunk = token_list.last_chunk;
        if (!chunk || chunk->count == array_count(chunk->tokens)) {
            chunk = arena_push_struct_no_zero(scratch.arena, TokenChunk);
            chunk->next = 0;
            chunk->previous = 0;
            chunk->count = 0;

            dll_push_back(token_list.first_chunk, token_list.last_chunk, chunk);
            ++token_list.chunk_count;
        }

        chunk->tokens[chunk->count] = token;
        ++chunk->count;
        ++token_list.total_count;
    }

    // NOTE(simon): Flatten out the token array.
    TokenArray tokens = { 0 };
    tokens.tokens = arena_push_array_no_zero(arena, Token, token_list.total_count);

    for (TokenChunk *chunk = token_list.first_chunk; chunk; chunk = chunk->next) {
        memory_copy(&tokens.tokens[tokens.count], chunk->tokens, chunk->count * sizeof(*chunk->tokens));
        tokens.count += chunk->count;
    }

    arena_end_temporary(scratch);
    return tokens;
}
