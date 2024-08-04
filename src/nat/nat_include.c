internal Nat_TokenArray nat_token_array_from_list(Arena *arena, Nat_TokenList list) {
    Nat_TokenArray result = { 0 };

    result.tokens = arena_push_array(arena, Nat_Token, list.total_token_count);
    for (Nat_TokenChunk *chunk = list.first; chunk; chunk = chunk->next) {
        memory_copy(
            &result.tokens[result.count],
            chunk->tokens,
            chunk->count * sizeof(*chunk->tokens)
        );
        result.count += chunk->count;
    }

    return result;
}

internal Nat_Location nat_location_from_token(Str8 source, Nat_Token token) {
    Nat_Location result = { 0 };
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

internal Nat_LexerResult nat_tokens_from_string(Arena *arena, Str8 source) {
    Nat_LexerResult result = { 0 };

    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    U8 *start = source.data;
    U8 *end = source.data + source.size;
    U8 *cursor = start;
    Nat_TokenList token_list = { 0 };
    while (cursor < end) {
        Nat_Token token = { 0 };
        token.source.data = cursor;

        if (
            ('0' <= cursor[0] && cursor[0] <= '9') ||
            (cursor + 1 < end && cursor[0] == '-' && '0' <= cursor[1] && cursor[1] <= '9') ||
            (cursor + 1 < end && cursor[0] == '.' && '0' <= cursor[1] && cursor[1] <= '9')
        ) {
            ++cursor;

            token.kind = Nat_Token_Number;
            while (
                cursor < end && (
                    *cursor == '_' ||
                    *cursor == '.' ||
                    ('0' <= *cursor && *cursor <= '9') ||
                    ('a' <= *cursor && *cursor <= 'z') ||
                    ('A' <= *cursor && *cursor <= 'Z')
                )
            ) {
                ++cursor;
            }
        } else if (('a' <= cursor[0] && cursor[0] <= 'z') || ('A' <= cursor[0] && cursor[0] <= 'Z')) {
            token.kind = Nat_Token_Identifier;
            while (
                cursor < end && (
                    *cursor == '_' ||
                    ('0' <= *cursor && *cursor <= '9') ||
                    ('a' <= *cursor && *cursor <= 'z') ||
                    ('A' <= *cursor && *cursor <= 'Z')
                )
            ) {
                ++cursor;
            }
        } else if (
            cursor + 2 < end && (
                (cursor[0] == '\'' && cursor[1] == '\'' && cursor[2] == '\'') ||
                (cursor[0] == '"'  && cursor[1] == '"'  && cursor[2] == '"')  ||
                (cursor[0] == '`'  && cursor[1] == '`'  && cursor[2] == '`')
            )
        ) {
            U8 delimiter = cursor[0];
            cursor += 3;

            token.kind = Nat_Token_Literal;
            token.flags |= Nat_TokenFlag_MultilineLiteral;
            token.flags |=
                delimiter == '\'' ? Nat_TokenFlag_SingleQuoteLiteral :
                delimiter == '"' ? Nat_TokenFlag_DoubleQuoteLiteral :
                Nat_TokenFlag_TickLiteral;

            while (
                cursor + 2 < end &&
                !(cursor[0] == delimiter && cursor[1] == delimiter && cursor[2] == delimiter)
            ) {
                ++cursor;
            }

            if (
                cursor + 2 < end &&
                !(cursor[0] == delimiter && cursor[1] == delimiter && cursor[2] == delimiter)
            ) {
                cursor += 3;
            } else {
                cursor = end;
                token.flags |= Nat_TokenFlag_UnclosedLiteral;
            }
        } else if (cursor[0] == '\'' || cursor[0] == '"' || cursor[0] == '`') {
            U8 delimiter = cursor[0];
            ++cursor;

            token.kind = Nat_Token_Literal;
            token.flags |=
                delimiter == '\'' ? Nat_TokenFlag_SingleQuoteLiteral :
                delimiter == '"'  ? Nat_TokenFlag_DoubleQuoteLiteral :
                Nat_TokenFlag_TickLiteral;

            while (cursor < end && *cursor != delimiter) {
                if (*cursor == '\n' || *cursor == '\r') {
                    token.flags |= Nat_TokenFlag_UnclosedLiteral;
                    break;
                }

                ++cursor;
            }

            if (cursor < end && *cursor == delimiter) {
                ++cursor;
            } else {
                token.flags |= Nat_TokenFlag_UnclosedLiteral;
            }
        } else if (cursor[0] == '\n') {
            token.kind = Nat_Token_Newline;
            ++cursor;
        } else if (cursor + 1 < end && cursor[0] == '\r' && cursor[1] == '\n') {
            token.kind = Nat_Token_Newline;
            cursor += 2;
        } else if (cursor[0] == ' ' || cursor[0] == '\t') {
            token.kind = Nat_Token_Whitespace;
            while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
                ++cursor;
            }
        } else if (
            cursor[0] == '[' || cursor[0] == ']' ||
            cursor[0] == '(' || cursor[0] == ')' ||
            cursor[0] == '{' || cursor[0] == '}' ||
            cursor[0] == '<' || cursor[0] == '>' ||
            cursor[0] == ',' || cursor[0] == ':'
        ) {
            token.kind = Nat_Token_Punctuator;
            ++cursor;
        } else if (cursor + 1 < end && cursor[0] == '/' && cursor[1] == '/') {
            token.kind = Nat_Token_Comment;
            while (cursor < end && *cursor != '\n' && *cursor != '\r') {
                ++cursor;
            }
        } else if (cursor + 1 < end && cursor[0] == '/' && cursor[1] == '*') {
            token.kind = Nat_Token_Comment;
            cursor += 2;
            U64 opened = 1;
            while (cursor + 1 < end && opened) {
                if (cursor[0] == '/' && cursor[1] == '*') {
                    ++opened;
                    cursor += 2;
                } else if (cursor[0] == '*' && cursor[1] == '/') {
                    --opened;
                    cursor += 2;
                } else {
                    ++cursor;
                }
            }

            if (opened) {
                cursor = end;
                token.flags |= Nat_TokenFlag_UnclosedComment;
            }
        } else {
            token.kind = Nat_Token_Unknown;
            StringDecode decode = string_decode_utf8(cursor, end - cursor);
            cursor += decode.size;
        }

        token.source = str8_range(token.source.data, cursor);

        // NOTE(simon): Generate errors.
        {
            Nat_Error *error = 0;
            if (token.flags & Nat_TokenFlag_UnclosedComment) {
                error = arena_push_struct_zero(arena, Nat_Error);
                error->message = str8_literal("Unclosed comment.");
            } else if (token.flags & Nat_TokenFlag_UnclosedLiteral) {
                error = arena_push_struct_zero(arena, Nat_Error);
                error->message = str8_literal("Unclosed literal.");
            } else if (token.kind & Nat_Token_Unknown) {
                error = arena_push_struct_zero(arena, Nat_Error);
                error->message = str8_literal("Unknown character.");
            }

            if (error) {
                error->location = nat_location_from_token(source, token);
                sll_queue_push(result.errors.first, result.errors.last, error);
                ++result.errors.total_error_count;
            }
        }

        Nat_TokenChunk *chunk = token_list.last;
        if (!chunk || chunk->count >= array_count(chunk->tokens)) {
            chunk = arena_push_struct_zero(scratch.arena, Nat_TokenChunk);
            sll_queue_push(token_list.first, token_list.last, chunk);
        }

        chunk->tokens[chunk->count] = token;
        ++chunk->count;
        ++token_list.total_token_count;
    }

    result.tokens = nat_token_array_from_list(arena, token_list);

    arena_end_temporary(scratch);

    return result;
}
