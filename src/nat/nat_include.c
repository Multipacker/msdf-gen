internal B32 nat_node_is_nil(Nat_Node *node) {
    B32 result = !node || node == &nat_nil_node;
    return result;
}

internal Void nat_node_push_child(Nat_Node *parent, Nat_Node *child) {
    dll_insert_next_previous_zero(parent->first, parent->last, parent->last, child, next, previous, &nat_nil_node);
    child->parent = parent;
}

internal Nat_TokenArray nat_token_array_from_list(Arena *arena, Nat_TokenList list) {
    Nat_TokenArray array = { 0 };
    array.tokens = arena_push_array_no_zero(arena, Nat_Token, list.token_count);

    for (Nat_TokenChunk *chunk = list.first_chunk; chunk; chunk = chunk->next) {
        memory_copy(&array.tokens[array.count], chunk->tokens, chunk->count * sizeof(Nat_Token));
        array.count += chunk->count;
    }

    return array;
}

internal Nat_TokenArray nat_token_array_from_string(Arena *arena, Str8 source) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    U8 *ptr = source.data;
    U8 *opl = source.data + source.size;
    Nat_TokenList token_list = { 0 };

    while (ptr < opl) {
        Nat_Token token = { 0 };

        // NOTE(simon): Newlines
        if (token.flags == 0 && *ptr == '\n') {
            token.flags |= Nat_TokenFlag_Newline;
            token.raw = str8(ptr, 1);
            ++ptr;
        }

        // NOTE(simon): Whitespace
        if (token.flags == 0 && (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\v')) {
            token.flags |= Nat_TokenFlag_Whitespace;
            token.raw.data = ptr;
            do {
                ++ptr;
            } while (ptr < opl && (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\v'));
            token.raw.size = (U64) (ptr - token.raw.data);
        }

        // NOTE(simon): Single-line comments
        if (token.flags == 0 && ptr + 2 <= opl && ptr[0] == '/' && ptr[1] == '/') {
            token.flags |= Nat_TokenFlag_Comment;
            token.raw.data = ptr + 2;
            ptr += 2;
            while (ptr < opl && *ptr != '\n') {
                ++ptr;
            }
            token.raw.size = (U64) (ptr - token.raw.data);
        }

        // NOTE(simon): Multi-line comments
        if (token.flags == 0 && ptr + 2 <= opl && ptr[0] == '/' && ptr[1] == '*') {
            token.flags |= Nat_TokenFlag_Comment | Nat_TokenFlag_Multiline;
            token.raw.data = ptr + 2;
            ptr += 2;
            U64 opened = 1;
            while (ptr < opl && opened > 0) {
                if (ptr + 2 <= opl && ptr[0] == '/' && ptr[1] == '*') {
                    ptr += 2;
                    ++opened;
                } else if (ptr + 2 <= opl && ptr[0] == '*' && ptr[1] == '/') {
                    ptr += 2;
                    --opened;
                } else {
                    ++ptr;
                }
            }
            if (opened > 0) {
                token.flags |= Nat_TokenFlag_Unclosed;
                token.raw.size = (U64) (ptr - token.raw.data);
            } else {
                token.raw.size = (U64) (ptr - token.raw.data - 2);
            }
        }

        // NOTE(simon): Numbers
        if (
            token.flags == 0 && (
                (ptr < opl && '0' <= ptr[0] && ptr[0] <= '9') ||
                (ptr + 2 <= opl && ptr[0] == '.' && '0' <= ptr[1] && ptr[1] <= '9') ||
                (ptr + 2 <= opl && ptr[0] == '-' && '0' <= ptr[1] && ptr[1] <= '9')
            )
        ) {
            token.flags |= Nat_TokenFlag_Number;
            token.raw.data = ptr;
            while (
                ptr < opl && (
                    ('a' <= *ptr && *ptr <= 'z') ||
                    ('A' <= *ptr && *ptr <= 'Z') ||
                    ('0' <= *ptr && *ptr <= '9') ||
                    *ptr == '.' ||
                    *ptr == '_'
                )
            ) {
                ++ptr;
            }
            token.raw.size = (U64) (ptr - token.raw.data);
        }

        // NOTE(simon): Identifiers
        if (
            token.flags == 0 && ptr < opl && (
                ('a' <= *ptr && *ptr <= 'z') ||
                ('A' <= *ptr && *ptr <= 'Z')
            )
        ) {
            token.flags |= Nat_TokenFlag_Identifier;
            token.raw.data = ptr;
            while (
                ptr < opl && (
                    ('a' <= *ptr && *ptr <= 'z') ||
                    ('A' <= *ptr && *ptr <= 'Z') ||
                    ('0' <= *ptr && *ptr <= '9') ||
                    *ptr == '_'
                )
            ) {
                ++ptr;
            }
            token.raw.size = (U64) (ptr - token.raw.data);
        }

        // NOTE(simon): Multi-line strings
        if (
            token.flags == 0 && ptr + 3 <= opl && (
                (ptr[0] == '\'' && ptr[1] == '\'' && ptr[2] == '\'') ||
                (ptr[0] == '"'  && ptr[1] == '"'  && ptr[2] == '"')  ||
                (ptr[0] == '`'  && ptr[1] == '`'  && ptr[2] == '`')
            )
        ) {
            U8 delimiter = *ptr;
            token.flags |= Nat_TokenFlag_String | Nat_TokenFlag_Multiline;
            token.flags |= (*ptr == '\'') * Nat_TokenFlag_SingleQuoted;
            token.flags |= (*ptr == '"')  * Nat_TokenFlag_DoubleQuoted;
            token.flags |= (*ptr == '`')  * Nat_TokenFlag_Ticked;
            token.raw.data = ptr + 3;
            ptr += 3;
            while (ptr <= opl) {
                if (ptr == opl) {
                    token.flags |= Nat_TokenFlag_Unclosed;
                    token.raw.size = (U64) (ptr - token.raw.data);
                    break;
                } else if (ptr + 3 <= opl && ptr[0] == delimiter && ptr[1] == delimiter && ptr[2] == delimiter) {
                    token.raw.size = (U64) (ptr - token.raw.data);
                    ptr += 3;
                    break;
                } else {
                    ++ptr;
                }
            }
        }

        // NOTE(simon): Single-line strings
        if (token.flags == 0 && (*ptr == '\'' || *ptr == '"' || *ptr == '`')) {
            U8 delimiter = *ptr;
            token.flags |= Nat_TokenFlag_String;
            token.flags |= (*ptr == '\'') * Nat_TokenFlag_SingleQuoted;
            token.flags |= (*ptr == '"')  * Nat_TokenFlag_DoubleQuoted;
            token.flags |= (*ptr == '`')  * Nat_TokenFlag_Ticked;
            token.raw.data = ptr + 1;
            ++ptr;
            B32 escaped = false;
            while (ptr <= opl) {
                if (ptr == opl || *ptr == '\n') {
                    token.flags |= Nat_TokenFlag_Unclosed;
                    token.raw.size = (U64) (ptr - token.raw.data);
                    break;
                } else if (escaped) {
                    escaped = false;
                    ++ptr;
                } else if (*ptr == delimiter) {
                    token.raw.size = (U64) (ptr - token.raw.data);
                    ptr += 1;
                    break;
                } else if (*ptr == '\\') {
                    escaped = true;
                    ++ptr;
                } else {
                    ++ptr;
                }
            }
        }

        // NOTE(simon): Punctuation
        if (
            token.flags == 0 && (
                *ptr == '!' || *ptr == '#'  || *ptr == '$' || *ptr == '%' ||
                *ptr == '&' || *ptr == '('  || *ptr == ')' || *ptr == '*' ||
                *ptr == '+' || *ptr == ','  || *ptr == '-' || *ptr == '.' ||
                *ptr == '/' || *ptr == ':'  || *ptr == ';' || *ptr == '<' ||
                *ptr == '=' || *ptr == '>'  || *ptr == '?' || *ptr == '@' ||
                *ptr == '[' || *ptr == '\\' || *ptr == ']' || *ptr == '^' ||
                *ptr == '{' || *ptr == '|'  || *ptr == '}' || *ptr == '~'
            )
        ) {
            token.flags |= Nat_TokenFlag_Punctuation;
            token.raw = str8(ptr, 1);
            ++ptr;
        }

        // NOTE(simon): Errors.
        if (token.flags == 0) {
            token.flags |= Nat_TokenFlag_Unknown;
            ++ptr;
        }

        if (token.flags != 0) {
            Nat_TokenChunk *chunk = token_list.last_chunk;
            if (!chunk || chunk->count == array_count(chunk->tokens)) {
                chunk = arena_push_struct_no_zero(scratch.arena, Nat_TokenChunk);
                chunk->count = 0;

                dll_push_back(token_list.first_chunk, token_list.last_chunk, chunk);
                ++token_list.chunk_count;
            }

            chunk->tokens[chunk->count] = token;
            ++chunk->count;
            ++token_list.token_count;
        }
    }

    Nat_TokenArray token_array = nat_token_array_from_list(arena, token_list);

    arena_end_temporary(scratch);
    return token_array;
}

internal Nat_Node *nat_create_node(Arena *arena, Str8 string, Str8 raw, Nat_NodeFlags flags) {
    Nat_Node *node = arena_push_struct(arena, Nat_Node);
    node->next     = &nat_nil_node;
    node->previous = &nat_nil_node;
    node->first    = &nat_nil_node;
    node->last     = &nat_nil_node;
    node->parent   = &nat_nil_node;
    node->flags    = flags;
    node->string   = string;
    node->raw      = raw;
    return node;
}

internal Nat_Node *nat_parse_from_tokens(Arena *arena, Str8 filename, Str8 source, Nat_TokenArray tokens) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    Nat_Node *root = nat_create_node(arena, filename, source, 0);

    typedef enum {
        WorkKind_Main,
        WorkKind_MainImplicit,
        WorkKind_OptionalChildren,
        WorkKind_ChildrenScan,
    } WorkKind;

    typedef struct Work Work;
    struct Work {
        Work     *next;
        WorkKind  kind;
        Nat_Node *parent;
        Nat_NodeFlags accumulated_flags;
    };
    Work start_work     = { 0, WorkKind_Main, root, };
    Work *work_top      = &start_work;
    Work *work_freelist = 0;

#define push_work(work_kind, parent_node)                  \
    do {                                                   \
        Work *work = work_freelist;                        \
        if (work) {                                        \
            sll_stack_pop(work_freelist);                  \
        } else {                                           \
            work = arena_push_struct(scratch.arena, Work); \
        }                                                  \
        memory_zero_struct(work);                          \
        work->kind = work_kind;                            \
        work->parent = parent_node;                        \
        sll_stack_push(work_top, work);                    \
    } while (0);
#define pop_work()                           \
    do {                                     \
        Work *work = work_top;               \
        sll_stack_pop(work_top);             \
        sll_stack_push(work_freelist, work); \
    } while (0);
    Nat_Token *tokens_first = tokens.tokens;
    Nat_Token *tokens_opl = tokens.tokens + tokens.count;
    Nat_Token *token = tokens_first;
    while (token < tokens_opl) {
        // NOTE(simon): whitespace, comments -> consume
        if (token->flags & (Nat_TokenFlag_Newline | Nat_TokenFlag_Whitespace | Nat_TokenFlag_Comment)) {
            ++token;
            goto end;
        }

        // NOTE(simon): [optional children] : following label -> top work has children.
        if (work_top->kind == WorkKind_OptionalChildren && (token->flags & Nat_TokenFlag_Punctuation) && str8_equal(token->raw, str8_literal(":"))) {
            Nat_Node *parent = work_top->parent;
            pop_work();
            push_work(WorkKind_ChildrenScan, parent);
            ++token;
            goto end;
        }

        // NOTE(simon): [optional children] anything but : -> pop
        if (work_top->kind == WorkKind_OptionalChildren) {
            pop_work();
            goto end;
        }

        // NOTE(simon): [main] , -> consume, mark
        if (
            work_top->kind == WorkKind_Main && (token->flags & Nat_TokenFlag_Punctuation) && (
                str8_equal(token->raw, str8_literal(",")) ||
                str8_equal(token->raw, str8_literal(";"))
            )
        ) {
            Nat_Node *parent = work_top->parent;
            if (!nat_node_is_nil(parent->last)) {
                parent->last->flags         |= str8_equal(token->raw, str8_literal(",")) ? Nat_NodeFlag_IsBeforeComma     : 0;
                parent->last->flags         |= str8_equal(token->raw, str8_literal(";")) ? Nat_NodeFlag_IsBeforeSemicolon : 0;
                work_top->accumulated_flags |= str8_equal(token->raw, str8_literal(",")) ? Nat_NodeFlag_IsAfterComma      : 0;
                work_top->accumulated_flags |= str8_equal(token->raw, str8_literal(";")) ? Nat_NodeFlag_IsAfterSemicolon  : 0;
            }
            ++token;
            goto end;
        }

        // NOTE(simon): [main] (, {, [ -> consume, push new main
        if (
            work_top->kind == WorkKind_Main && (token->flags & Nat_TokenFlag_Punctuation) && (
                str8_equal(token->raw, str8_literal("(")) ||
                str8_equal(token->raw, str8_literal("{")) ||
                str8_equal(token->raw, str8_literal("["))
            )
        ) {
            Nat_Node *parent = work_top->parent;
            Nat_NodeFlags flags = work_top->accumulated_flags;
            flags |= str8_equal(token->raw, str8_literal("(")) ? Nat_NodeFlag_HasParenLeft   : 0;
            flags |= str8_equal(token->raw, str8_literal("{")) ? Nat_NodeFlag_HasBraceLeft   : 0;
            flags |= str8_equal(token->raw, str8_literal("[")) ? Nat_NodeFlag_HasBracketLeft : 0;
            Nat_Node *node = nat_create_node(arena, str8_literal(""), str8_literal(""), flags);
            work_top->accumulated_flags = 0;
            nat_node_push_child(parent, node);

            push_work(WorkKind_Main, node);
            ++token;
            goto end;
        }

        // NOTE(simon): [children scan] (, {, [ -> explicitly delimited children.
        if (
            work_top->kind == WorkKind_ChildrenScan && (token->flags & Nat_TokenFlag_Punctuation) && (
                str8_equal(token->raw, str8_literal("(")) ||
                str8_equal(token->raw, str8_literal("{")) ||
                str8_equal(token->raw, str8_literal("["))
            )
        ) {
            Nat_Node *parent = work_top->parent;
            parent->flags |= str8_equal(token->raw, str8_literal("(")) ? Nat_NodeFlag_HasParenLeft   : 0;
            parent->flags |= str8_equal(token->raw, str8_literal("{")) ? Nat_NodeFlag_HasBraceLeft   : 0;
            parent->flags |= str8_equal(token->raw, str8_literal("[")) ? Nat_NodeFlag_HasBracketLeft : 0;
            pop_work()
            push_work(WorkKind_Main, parent);
            ++token;
            goto end;
        }

        // NOTE(simon): [children scan] anything else -> assume implicit chilren
        if (work_top->kind == WorkKind_ChildrenScan) {
            Nat_Node *parent = work_top->parent;
            pop_work();
            push_work(WorkKind_MainImplicit, parent);
            goto end;
        }

        // NOTE(simon): [main implicit] ,, ;, ), }, ] -> pop
        if (
            work_top->kind == WorkKind_MainImplicit && (token->flags & Nat_TokenFlag_Punctuation) && (
                str8_equal(token->raw, str8_literal(",")) ||
                str8_equal(token->raw, str8_literal(";")) ||
                str8_equal(token->raw, str8_literal(")")) ||
                str8_equal(token->raw, str8_literal("}")) ||
                str8_equal(token->raw, str8_literal("]"))
            )
        ) {
            pop_work();
            goto end;
        }

        // NOTE(simon): [main] ), }, ] -> consume, pop new main
        if (
            work_top->kind == WorkKind_Main && (token->flags & Nat_TokenFlag_Punctuation) && (
                str8_equal(token->raw, str8_literal(")")) ||
                str8_equal(token->raw, str8_literal("}")) ||
                str8_equal(token->raw, str8_literal("]"))
            )
        ) {
            Nat_Node *parent = work_top->parent;
            parent->flags |= str8_equal(token->raw, str8_literal(")")) ? Nat_NodeFlag_HasParenRight   : 0;
            parent->flags |= str8_equal(token->raw, str8_literal("}")) ? Nat_NodeFlag_HasBraceRight   : 0;
            parent->flags |= str8_equal(token->raw, str8_literal("]")) ? Nat_NodeFlag_HasBracketRight : 0;
            pop_work();
            ++token;
            goto end;
        }

        // NOTE(simon): [main, main implicit] label -> consume, push child
        if ((work_top->kind == WorkKind_Main || work_top->kind == WorkKind_MainImplicit) && (token->flags& Nat_TokenFlag_Label)) {
            Nat_Node *parent = work_top->parent;
            Nat_NodeFlags flags = work_top->accumulated_flags;
            flags |= (token->flags & Nat_TokenFlag_SingleQuoted) ? Nat_NodeFlag_StringSingleQuoted : 0;
            flags |= (token->flags & Nat_TokenFlag_DoubleQuoted) ? Nat_NodeFlag_StringDoubleQuoted : 0;
            flags |= (token->flags & Nat_TokenFlag_Ticked)       ? Nat_NodeFlag_StringTicked       : 0;
            flags |= (token->flags & Nat_TokenFlag_Multiline)    ? Nat_NodeFlag_StringMultiline    : 0;
            flags |= (token->flags & Nat_TokenFlag_Punctuation)  ? Nat_NodeFlag_Punctuation        : 0;
            flags |= (token->flags & Nat_TokenFlag_Identifier)   ? Nat_NodeFlag_Identifier         : 0;
            flags |= (token->flags & Nat_TokenFlag_String)       ? Nat_NodeFlag_String             : 0;
            flags |= (token->flags & Nat_TokenFlag_Number)       ? Nat_NodeFlag_Number             : 0;
            Nat_Node *node = nat_create_node(arena, token->raw, token->raw, flags);
            work_top->accumulated_flags = 0;
            nat_node_push_child(parent, node);
            push_work(WorkKind_OptionalChildren, node);

            ++token;
            goto end;
        }

end:;
    }
#undef push_work
#undef pop_work

    arena_end_temporary(scratch);
    return root;
}

internal Void nat_test(Void) {
    Arena *arena = arena_create();

    Str8 path = str8_literal("feeds.json");
    Str8 source = { 0 };
    os_file_read(arena, path, &source);
    Nat_TokenArray tokens = nat_token_array_from_string(arena, source);
    for (U64 i = 0; i < tokens.count; ++i) {
        Nat_TokenFlags flags = tokens.tokens[i].flags;
        if (flags & Nat_TokenFlag_Unknown)      printf("Unknown ");
        if (flags & Nat_TokenFlag_Newline)      printf("Newline ");
        if (flags & Nat_TokenFlag_Whitespace)   printf("Whitespace ");
        if (flags & Nat_TokenFlag_Comment)      printf("Comment ");
        if (flags & Nat_TokenFlag_Punctuation)  printf("Punctuation ");
        if (flags & Nat_TokenFlag_Identifier)   printf("Identifier ");
        if (flags & Nat_TokenFlag_String)       printf("String ");
        if (flags & Nat_TokenFlag_Number)       printf("Number ");
        if (flags & Nat_TokenFlag_SingleQuoted) printf("SingleQuoted ");
        if (flags & Nat_TokenFlag_DoubleQuoted) printf("DoubleQuoted ");
        if (flags & Nat_TokenFlag_Ticked)       printf("Ticked ");
        if (flags & Nat_TokenFlag_Multiline)    printf("Multiline ");
        if (flags & Nat_TokenFlag_Unclosed)     printf("Unclosed ");
        printf("%.*s\n", str8_expand(tokens.tokens[i].raw));
    }

    Nat_Node *root = nat_parse_from_tokens(arena, path, source, tokens);

    S32 depth = 0;
    for (Nat_Node *node = root; !nat_node_is_nil(node); ) {
        printf("%*s%.*s: ", 2 * depth, "", str8_expand(node->string));
        if (node->flags & Nat_NodeFlag_IsBeforeComma)      printf("IsBeforeComma ");
        if (node->flags & Nat_NodeFlag_IsAfterComma)       printf("IsAfterComma ");
        if (node->flags & Nat_NodeFlag_IsBeforeSemicolon)  printf("IsBeforeSemicolon ");
        if (node->flags & Nat_NodeFlag_IsAfterSemicolon)   printf("IsAfterSemicolon ");
        if (node->flags & Nat_NodeFlag_HasParenLeft)       printf("HasParenLeft ");
        if (node->flags & Nat_NodeFlag_HasParenRight)      printf("HasParenRight ");
        if (node->flags & Nat_NodeFlag_HasBraceLeft)       printf("HasBraceLeft ");
        if (node->flags & Nat_NodeFlag_HasBraceRight)      printf("HasBraceRight ");
        if (node->flags & Nat_NodeFlag_HasBracketLeft)     printf("HasBracketLeft ");
        if (node->flags & Nat_NodeFlag_HasBracketRight)    printf("HasBracketRight ");
        if (node->flags & Nat_NodeFlag_StringSingleQuoted) printf("StringSingleQuoted ");
        if (node->flags & Nat_NodeFlag_StringDoubleQuoted) printf("StringDoubleQuoted ");
        if (node->flags & Nat_NodeFlag_StringTicked)       printf("StringTicked ");
        if (node->flags & Nat_NodeFlag_StringMultiline)    printf("StringMultiline ");
        if (node->flags & Nat_NodeFlag_Punctuation)        printf("Punctuation ");
        if (node->flags & Nat_NodeFlag_Identifier)         printf("Identifier ");
        if (node->flags & Nat_NodeFlag_String)             printf("String ");
        if (node->flags & Nat_NodeFlag_Number)             printf("Number ");
        printf("\n");

        Nat_Node *next = &nat_nil_node;
        if (!nat_node_is_nil(node->first)) {
            next = node->first;
            ++depth;
        } else {
            for (Nat_Node *parent = node; !nat_node_is_nil(parent); parent = parent->parent) {
                if (!nat_node_is_nil(parent->next)) {
                    next = parent->next;
                    break;
                }
                --depth;
            }
        }
        node = next;
    }

    arena_destroy(arena);
}
