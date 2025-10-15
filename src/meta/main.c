#include "src/base/base_include.h"
#include "src/base/base_include.c"

#include "object.h"
#include "elf.h"
#include "coff.h"
#include "lexer.h"
#include "meta.h"

#include "object.c"
#include "elf.c"
#include "coff.c"
#include "lexer.c"

internal Token *parser_next(Token **ptr, Token *opl) {
    Token *token = *ptr;
    // NOTE(simon): Skip pre-processor tokens and comments.
    for (; token < opl; ++token) {
        if (!(token->flags & Token_Flag_PreProcessor) && token->kind != Token_Comment) {
            break;
        }
    }

    if (token < opl) {
        *ptr = token + 1;
    } else {
        *ptr = opl;
        token = 0;
    }

    return token;
}

internal Token *parser_accept_kind(Token **ptr, Token *opl, TokenKind kind) {
    Token *token = *ptr;
    // NOTE(simon): Skip pre-processor tokens and comments.
    for (; token < opl; ++token) {
        if (!(token->flags & Token_Flag_PreProcessor) && token->kind != Token_Comment) {
            break;
        }
    }

    if (token < opl && token->kind == kind) {
        *ptr = token + 1;
    } else {
        token = 0;
    }

    if (token >= opl) {
        *ptr = opl;
    }

    return token;
}

internal Token *parser_accept_string(Token **ptr, Token *opl, Str8 string) {
    Token *token = *ptr;
    // NOTE(simon): Skip pre-processor tokens and comments.
    for (; token < opl; ++token) {
        if (!(token->flags & Token_Flag_PreProcessor) && token->kind != Token_Comment) {
            break;
        }
    }

    if (token < opl && str8_equal(token->string, string)) {
        *ptr = token + 1;
    } else {
        token = 0;
    }

    if (token >= opl) {
        *ptr = opl;
    }

    return token;
}

// TODO(simon): Resolve escape sequences.
internal Str8 string_from_literal(Arena *arena, Str8 raw) {
    Str8 result = str8_copy(arena, str8_skip(str8_chop(raw, 1), 1));
    return result;
}

internal S32 os_run(Str8List arguments) {
    Arena *arena = arena_create();

    Str8 binary_path  = os_file_path(arena, OS_SYSTEM_PATH_BINARY);
    Str8 project_path = str8_chop_last_slash(binary_path);
    Str8 code_path    = str8_format(arena, "%.*s/src", str8_expand(project_path));

    // NOTE(simon): Collect files.
    Str8List files = { 0 };
    {
        Arena_Temporary scratch = arena_get_scratch(&arena, 1);

        Str8Node *directories = { 0 };
        Str8Node *start = arena_push_struct(scratch.arena, Str8Node);
        start->string = code_path;
        sll_stack_push(directories, start);
        
        while (directories) {
            Str8Node *directory = directories;
            sll_stack_pop(directories);

            OS_FileIterator *file_iterator = os_file_iterator_begin(scratch.arena, directory->string);
            for (OS_FileInfo info = { 0 }; os_file_iterator_next(scratch.arena, file_iterator, &info); ) {
                Str8 name = str8_format(scratch.arena, "%.*s/%.*s", str8_expand(directory->string), str8_expand(info.name));

                if (info.properties.flags & FilePropertyFlags_IsFolder) {
                    Str8Node *new_directory = arena_push_struct(scratch.arena, Str8Node);
                    new_directory->string = name;
                    sll_stack_push(directories, new_directory);
                } else {
                    str8_list_push(arena, &files, str8_copy(arena, name));
                }
            }
            os_file_iterator_end(file_iterator);
        }

        arena_end_temporary(scratch);
    }

    typedef struct FileEmbed FileEmbed;
    struct FileEmbed {
        FileEmbed *next;
        FileEmbed *previous;
        Str8   source_file;
        Str8   identifier;
        Str8   file;
    };

    FileEmbed *first_embed = 0;
    FileEmbed *last_embed = 0;

    // NOTE(simon): Collect embeds.
    Log *log = log_create();
    log_select(log);
    log_scope_begin();
    for (Str8Node *file = files.first; file; file = file->next) {
        Arena_Temporary scratch = arena_get_scratch(&arena, 1);

        Str8 contents = { 0 };
        os_file_read(scratch.arena, file->string, &contents);

        TokenArray tokens = tokens_from_string(scratch.arena, file->string, contents);

        Token *ptr = tokens.tokens;
        Token *opl = tokens.tokens + tokens.count;

        while (ptr < opl) {
            if (parser_accept_string(&ptr, opl, str8_literal("embed_file")) && parser_accept_string(&ptr, opl, str8_literal("("))) {
                Token *identifier = parser_accept_kind(&ptr, opl, Token_Identifier);
                if (identifier && parser_accept_string(&ptr, opl, str8_literal(","))) {
                    Token *path = parser_accept_kind(&ptr, opl, Token_StringLiteral);
                    if (path && parser_accept_string(&ptr, opl, str8_literal(")"))) {
                        FileEmbed *embed = arena_push_struct(arena, FileEmbed);
                        embed->source_file = file->string;
                        embed->identifier  = str8_copy(arena, identifier->string);
                        embed->file        = string_from_literal(arena, path->string);
                        dll_push_back(first_embed, last_embed, embed);
                    }
                }
            } else {
                parser_next(&ptr, opl);
            }
        }

        arena_end_temporary(scratch);
    }
    LogScopeResult parsing_log = log_scope_end(arena);
    if (parsing_log.strings[Log_MessageKind_Warning].data) {
        os_console_print(parsing_log.strings[Log_MessageKind_Warning]);
    }
    if (parsing_log.strings[Log_MessageKind_Error].data) {
        os_console_print(parsing_log.strings[Log_MessageKind_Error]);
    }

    Meta_Layer *first_layer = 0;
    Meta_Layer *last_layer = 0;

    for (FileEmbed *embed = first_embed; embed; embed = embed->next) {
        Str8 directory = str8_chop_last_slash(embed->source_file);

        Meta_Layer *layer = 0;
        for (Meta_Layer *old_layer = first_layer; old_layer; old_layer = old_layer->next) {
            if (str8_equal(old_layer->path, directory)) {
                layer = old_layer;
                break;
            }
        }

        if (!layer) {
            layer = arena_push_struct(arena, Meta_Layer);
            layer->path = directory;
            str8_list_push(arena, &layer->header_lines, str8_literal("#undef embed_file\n"));
            str8_list_push(arena, &layer->header_lines, str8_literal("#define embed_file(...)\n"));

            Object_Section *data_section = object_add_section(arena, &layer->object, str8_literal(".data"));
            data_section->flags = Object_SectionFlag_Write;

            dll_push_back(first_layer, last_layer, layer);
        }

        Str8 embed_file = { 0 };
        if (embed->file.size >= 1 && embed->file.data[0] == '/') {
            // NOTE(simon): Path is relative to project directory.
            embed_file = str8_concatenate(arena, project_path, embed->file);
        } else {
            // NOTE(simon): Path is relative to the file it is referenced from.
            embed_file = str8_format(arena, "%.*s/%.*s", str8_expand(directory), str8_expand(embed->file));
        }

        Str8 data_symbol_name = str8_format(arena, "%.*s_data", str8_expand(embed->identifier));
        Str8 contents = { 0 };
        os_file_read(arena, embed_file, &contents);

        Object_Symbol *data_symbol = object_add_symbol(arena, &layer->object, data_symbol_name);
        data_symbol->section_name = str8_literal(".data");
        data_symbol->data         = contents;
        data_symbol->align        = 1;

        str8_list_push_format(arena, &layer->header_lines, "extern U8 %.*s[%lu];\n", str8_expand(data_symbol_name), contents.size);
        str8_list_push_format(arena, &layer->header_lines, "global Str8 %.*s = { %.*s, %lu, };\n", str8_expand(embed->identifier), str8_expand(data_symbol_name), contents.size);
    }

    // NOTE(simon): Write layers.
    for (Meta_Layer *layer = first_layer; layer; layer = layer->next) {
        Str8 layer_name  = str8_skip_last_slash(layer->path);
        Str8 object_path = str8_format(arena, "build/%.*s.o",     str8_expand(layer_name));
        Str8 header_path = str8_format(arena, "%.*s/generated.h", str8_expand(layer->path));

        os_file_write(header_path, layer->header_lines);
        Str8List output = platform_binary_from_object(arena, layer->object);
        os_file_write(object_path, output);
    }

    arena_destroy(arena);
    return 0;
}
