internal TokenArray tokens_from_string(Arena *arena, Str8 name, Str8 source) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);
    TokenList token_list = { 0 };

    U8 *ptr = source.data;
    U8 *opl = source.data + source.size;
    U64 line   = 1;
    U64 column = 1;
    B32 in_pre_processor = false;
    B32 start_of_line    = true;
    while (ptr < opl) {
        // NOTE(simon): Skip whitespace and identify pre-processor statements.
        while (ptr < opl) {
            if (*ptr == ' ') {
                ++column;
                ++ptr;
            } else if (*ptr == '\t') {
                // NOTE(simon): Tabs are 4 columns for now.
                column += 4;
                ++ptr;
            } else if (*ptr == '\n') {
                start_of_line = true;
                in_pre_processor = false;
                ++line;
                column = 1;
                ++ptr;
            } else if (*ptr == '\r') {
                start_of_line = true;
                in_pre_processor = false;
                ++line;
                column = 1;
                ++ptr;

                if (ptr < opl && *ptr == '\n') {
                    ++ptr;
                }
            } else if (*ptr == '\\') {
                if (!in_pre_processor) {
                    log_warning_format("%.*s:%lu:%lu: Use of '\\' outside of pre-processor directives, this was probably not intended.\n", str8_expand(name), line, column);
                }

                column = 1;
                ++ptr;
                if (ptr < opl && *ptr == '\n') {
                    ++line;
                    column = 1;
                    ++ptr;
                } else if (ptr < opl && *ptr == '\r') {
                    ++line;
                    column = 1;
                    ++ptr;
                    if (ptr < opl && *ptr == '\n') {
                        ++ptr;
                    }
                } else {
                    log_error_format("%.*s:%lu:%lu: Use of '\\' other than as a line continuation.\n", str8_expand(name), line, column);
                }
            } else if (start_of_line && *ptr == '#') {
                in_pre_processor = true;
                ++column;
                ++ptr;
            } else {
                break;
            }
        }

        start_of_line = false;

        // NOTE(simon): Break if no more tokens.
        if (ptr >= opl) {
            break;
        }

        // NOTE(simon): Build token meta data.
        Token token = { 0 };
        if (in_pre_processor) {
            token.flags |= Token_Flag_PreProcessor;
        }
        token.start_line   = line;
        token.start_column = column;

        U8 *start = ptr;
        switch (*ptr) {
            case '!': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '"': {
                token.kind = Token_StringLiteral;
                ++column;
                ++ptr;
                B32 closed = false;
                while (ptr < opl) {
                    StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                    ptr += decode.size;
                    if (decode.codepoint == '"') {
                        ++column;
                        closed = true;
                        break;
                    } else if (decode.codepoint == '\\') {
                        StringDecode second_decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                        ptr += second_decode.size;
                        if (second_decode.codepoint == '\t') {
                            // NOTE(simon): Tabs are 4 columns for now.
                            column += 4;
                        } else {
                            ++column;
                        }
                    } else if (decode.codepoint == '\t') {
                        // NOTE(simon): Tabs are 4 columns for now.
                        column += 4;
                    } else {
                        ++column;
                    }
                }

                if (!closed) {
                    log_error_format("%.*s:%lu:%lu: Unclosed string literal.\n", str8_expand(name), token.start_line, token.start_column);
                }
            } break;
            case '#': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '#') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '%': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '&': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '&') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '\'': {
                token.kind = Token_CharacterLiteral;
                ++column;
                ++ptr;
                B32 closed = false;
                while (ptr < opl) {
                    StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                    ptr += decode.size;
                    if (decode.codepoint == '\'') {
                        ++column;
                        closed = true;
                        break;
                    } else if (decode.codepoint == '\\') {
                        StringDecode second_decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                        ptr += second_decode.size;
                        if (second_decode.codepoint == '\t') {
                            // NOTE(simon): Tabs are 4 columns for now.
                            column += 4;
                        } else {
                            ++column;
                        }
                    } else if (decode.codepoint == '\t') {
                        // NOTE(simon): Tabs are 4 columns for now.
                        column += 4;
                    } else {
                        ++column;
                    }
                }

                if (!closed) {
                    log_error_format("%.*s:%lu:%lu: Unclosed character literal.\n", str8_expand(name), token.start_line, token.start_column);
                }
            } break;
            case '(': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            case ')': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            case '*': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '+': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '+') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case ',': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            case '-': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '-') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '/': {
                ++column;
                ++ptr;
                if (ptr < opl && *ptr == '/') {
                    token.kind = Token_Comment;
                    ++column;
                    ++ptr;

                    while (ptr < opl) {
                        if (*ptr == '\n' || *ptr == '\r') {
                            break;
                        }

                        StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                        ptr += decode.size;

                        if (decode.codepoint == '\t') {
                            // NOTE(simon): Tabs are 4 columns for now.
                            column += 4;
                        } else {
                            ++column;
                        }
                    }
                } else if (ptr < opl && *ptr == '*') {
                    token.kind = Token_Comment;
                    ++column;
                    ++ptr;

                    B32 closed = false;

                    while (ptr < opl) {
                        if (ptr + 2 <= opl && ptr[0] == '*' && ptr[1] == '/') {
                            column += 2;
                            ptr += 2;
                            closed = true;
                            break;
                        } else if (ptr + 2 <= opl && ptr[0] == '/' && ptr[1] == '*') {
                            log_error_format("%.*s:%lu:%lu: '/*' within block comment.\n", str8_expand(name), line, column);
                        }

                        StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                        ptr += decode.size;

                        if (decode.codepoint == '\t') {
                            // NOTE(simon): Tabs are 4 columns for now.
                            column += 4;
                        } else if (decode.codepoint == '\n') {
                            start_of_line = true;
                            in_pre_processor = false;
                            ++line;
                            column = 1;
                        } else if (*ptr == '\r') {
                            start_of_line = true;
                            in_pre_processor = false;
                            ++line;
                            column = 1;

                            if (ptr < opl && *ptr == '\n') {
                                ++ptr;
                            }
                        } else {
                            ++column;
                        }
                    }

                    if (!closed) {
                        log_error_format("%.*s:%lu:%lu: Unclosed block comment.\n", str8_expand(name), token.start_line, token.start_column);
                    }
                } else if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case ':': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            case ';': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            case '<': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '<') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '=': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '>': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '>') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '?': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            case '[': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            case ']': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            case '^': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '{': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            case '|': {
                ++column;
                ++ptr;

                if (ptr < opl && *ptr == '=') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else if (ptr < opl && *ptr == '|') {
                    token.kind = Token_Symbol;
                    ++column;
                    ++ptr;
                } else {
                    token.kind = Token_Symbol;
                }
            } break;
            case '}': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            case '~': {
                token.kind = Token_Symbol;
                ++column;
                ++ptr;
            } break;
            default: {
                if ((*ptr == '0' <= *ptr && *ptr <= '9') || *ptr == '.') {
                    B32 is_zero = *ptr == '0';
                    B32 is_dot  = *ptr == '.';
                    ++column;
                    ++ptr;

                    if (is_zero && ptr < opl && (*ptr == 'b' || *ptr == 'B')) {
                        // NOTE(simon): Binary literals.
                        token.kind = Token_IntegerLiteral;
                        ++column;
                        ++ptr;

                        while (ptr < opl && '0' <= *ptr && *ptr <= '1') {
                            ++column;
                            ++ptr;
                        }
                    } else if (is_zero && ptr < opl && (*ptr == 'x' || *ptr == 'X')) {
                        // NOTE(simon): Hexadecimal literals.
                        token.kind = Token_IntegerLiteral;
                        ++column;
                        ++ptr;

                        while (ptr < opl && (('0' <= *ptr && *ptr <= '9') || ('a' <= *ptr && *ptr <= 'f') || ('A' <= *ptr && *ptr <= 'F'))) {
                            ++column;
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
                            ++column;
                            ++ptr;
                        }
                    }

                    // NOTE(simon): Floats.
                    if (!is_dot && ptr < opl && *ptr == '.') {
                        token.kind == Token_FloatLiteral;
                        ++column;
                        ++ptr;

                        while (ptr < opl && '0' <= *ptr && *ptr <= '9') {
                            ++column;
                            ++ptr;
                        }
                    }

                    // NOTE(simon): Exponents.
                    if (!(is_dot && ptr - start == 1) && ptr < opl && *ptr == 'e') {
                        token.kind == Token_FloatLiteral;
                        ++column;
                        ++ptr;

                        if (ptr < opl && (*ptr == '+' || *ptr == '-')) {
                            ++column;
                            ++ptr;
                        }

                        while (ptr < opl && '0' <= *ptr && *ptr <= '9') {
                            ++column;
                            ++ptr;
                        }
                    }

                    // NOTE(simon): It was the '.' operator.
                    if (is_dot && ptr - start == 1) {
                        token.kind == Token_Symbol;
                    }
                } else if (('a' <= *ptr && *ptr <= 'z') || ('A' <= *ptr && *ptr <= 'Z') || *ptr == '_') {
                    ++column;
                    ++ptr;
                    token.kind = Token_Identifier;
                    while (('0' <= *ptr && *ptr <= '9') || ('a' <= *ptr && *ptr <= 'z') || ('A' <= *ptr && *ptr <= 'Z') || *ptr == '_') {
                        ++column;
                        ++ptr;
                    }
                } else {
                    token.kind = Token_Unknown;
                    StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                    ptr += decode.size;
                    ++column;
                }
            } break;
        }

        token.string     = str8_range(start, ptr);
        token.end_line   = line;
        token.end_column = column;

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
