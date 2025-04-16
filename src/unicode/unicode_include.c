typedef struct Unicode_RangePropertyNode Unicode_RangePropertyNode;
struct Unicode_RangePropertyNode {
    Unicode_RangePropertyNode *next;
    Unicode_RangePropertyNode *previous;

    U32 min;
    U32 max;
    Str8List properties;
};

typedef struct Unicode_RangePropertyList Unicode_RangePropertyList;
struct Unicode_RangePropertyList {
    Unicode_RangePropertyNode *first;
    Unicode_RangePropertyNode *last;
    U64 count;
};

internal U64Decode u64_from_str8_hex(Str8 string) {
    U8 *ptr = string.data;
    U8 *opl = string.data + string.size;
    U64 value = 0;

    for (; ptr < opl; ++ptr) {
        U8 character = *ptr;
        if ('0' <= character && character <= '9') {
            value = 16 * value + (character - '0');
        } else if ('a' <= character && character <= 'f') {
            value = 16 * value + 10 + (character - 'a');
        } else if ('A' <= character && character <= 'F') {
            value = 16 * value + 10 + (character - 'A');
        } else {
            break;
        }
    }

    U64Decode result = { 0 };
    result.value     = value;
    result.size      = (U64) (ptr - string.data);
    return result;
}

internal Unicode_RangePropertyList unicode_range_property_list_from_file(Arena *arena, Str8 path) {
    Str8 contents = { 0 };
    os_file_read(arena, path, &contents);

    Unicode_RangePropertyList properties = { 0 };

    Str8List lines = str8_split_by_codepoints(arena, contents, str8_literal("\n"));
    for (Str8Node *line_node = lines.first; line_node; line_node = line_node->next) {
        Str8 whole_line = line_node->string;

        // NOTE(simon): Strip comments.
        U64 comment_start = str8_first_index_of(whole_line, '#');
        Str8 line    = str8_prefix(whole_line, comment_start);
        Str8 comment = str8_skip(whole_line, comment_start + 1);

        // NOTE(simon): Extract default values.
        Str8 missing_text  = str8_literal("@missing:");
        U64  missing_index = str8_find(0, comment, missing_text);
        Str8 missing       = str8_skip(comment, missing_index + missing_text.size);
        Str8List default_values = str8_split_by_codepoints(arena, missing, str8_literal(";"));
        // TODO(simon): Store somewhere

        // NOTE(simon): Does this line define a segment?
        // TODO(simon): Parse and store segment (UAX #44 4.2.14)

        // NOTE(simon): Extract fields.
        Str8List fields = str8_split_by_codepoints(arena, line, str8_literal(";"));

        // NOTE(simon): Trim leading and trailing spaces.
        for (Str8Node *field_node = fields.first; field_node; field_node = field_node->next) {
            Str8 field = field_node->string;

            U8 *ptr = field.data;
            U8 *opl = field.data + field.size;

            while (ptr < opl && ptr[0] == ' ') {
                ++ptr;
            }

            while (ptr < opl && opl[-1] == ' ') {
                --opl;
            }

            field_node->string = str8_range(ptr, opl);
        }

        // NOTE(simon): Get effected codepoints.
        U32 min_codepoint = 0;
        U32 max_codepoint = 0;
        if (fields.first) {
            Str8 codepoint_range = fields.first->string;
            dll_remove(fields.first, fields.last, fields.first);

            U64Decode min_decode = u64_from_str8_hex(codepoint_range);
            U64Decode max_decode = u64_from_str8_hex(str8_skip(codepoint_range, min_decode.size + 2));

            min_codepoint = (U32) min_decode.value;
            if (max_decode.size) {
                max_codepoint = (U32) max_decode.value + 1;
            } else {
                max_codepoint = min_codepoint + 1;
            }
        }

        // NOTE(simon): Insert.
        if (min_codepoint < max_codepoint) {
            Unicode_RangePropertyNode *node = arena_push_struct(arena, Unicode_RangePropertyNode);
            node->min = min_codepoint;
            node->max = max_codepoint;
            node->properties = fields;

            dll_push_back(properties.first, properties.last, node);
            ++properties.count;
        }
    }

    return properties;
}

internal Str8 unicode_name_to_identifier(Arena *arena, Str8 name) {
    U8 *buffer = arena_push_array(arena, U8, name.size);
    U64 size = 0;

    for (U64 i = 0; i < name.size; ++i) {
        U8 character = name.data[i];

        if (character == ' ' || character == '-') {
            character = 0;
        }

        if (character != 0) {
            buffer[size] = character;
            ++size;
        }
    }

    Str8 result = str8(buffer, size);
    return result;
}

internal Void unicode_test(Void) {
    Arena *arena = arena_create();

    Unicode_RangePropertyList blocks = unicode_range_property_list_from_file(arena, str8_literal("ucd/Blocks.txt"));

    typedef struct BlockRange BlockRange;
    struct BlockRange {
        U32 min;
        U32 max;
        U32 name_index;
    };

    Str8 *names = arena_push_array(arena, Str8, blocks.count);
    U64 name_count = 0;
    U32 *all_name_indicies = arena_push_array(arena, U32, 0x110000);
    for (Unicode_RangePropertyNode *range = blocks.first; range; range = range->next) {
        if (!range->properties.first) {
            continue;
        }

        // NOTE(simon): Deduplicate block names.
        Str8 name = unicode_name_to_identifier(arena, range->properties.first->string);
        U64 name_index = 0;
        while (name_index < name_count && !str8_equal(names[name_index], name)) {
            ++name_index;
        }
        if (name_index == name_count) {
            names[name_index] = name;
            ++name_count;
        }

        for (U64 i = range->min; i < range->max; ++i) {
            all_name_indicies[i] = (U32) name_index;
        }
    }

    // NOTE(simon): Generate block enum.
    os_console_print(str8_literal("typedef enum {\n"));
    for (U64 i = 0; i < name_count; ++i) {
        os_console_print(str8_format(arena, "    Unicode_Block_%.*s,\n", str8_expand(names[i])));
    }
    os_console_print(str8_literal("} Unicode_Block;\n"));

    U64 items_per_level = 16;
    U32 *offsets = arena_push_array(arena, U32, 0x110000 / items_per_level);
    U64 offset_count = 0;
    typedef struct Item Item;
    struct Item {
        Item *next;
        U64 hash;
        U32 offset;
    };
    typedef struct Bucket Bucket;
    struct Bucket {
        Item *first;
        Item *last;
    };
    U64 bucket_count = 256;
    Bucket *buckets = arena_push_array(arena, Bucket, bucket_count);
    U64 items = 0;
    for (U32 i = 0; i < 0x110000; i += items_per_level) {
        U64 hash = str8_hash(str8((U8 *) &all_name_indicies[i], items_per_level * sizeof(*all_name_indicies)));

        Bucket *bucket = &buckets[hash % bucket_count];
        Item *item = bucket->first;
        while (item && !(item->hash == hash && memory_equal(&all_name_indicies[item->offset], &all_name_indicies[i], items_per_level * sizeof(*all_name_indicies)))) {
            item = item->next;
        }

        if (!item) {
            item = arena_push_struct(arena, Item);
            item->offset = i;
            item->hash = hash;
            sll_stack_push(bucket->first, item);
            ++items;
        }

        offsets[offset_count] = item->offset;
        ++offset_count;
    }

    for (U64 i = 0; i < offset_count; ++i) {
        os_console_print(str8_format(arena, "%u, ", offsets[i]));
    }
    os_console_print(str8_literal("\n"));
    os_console_print(str8_format(arena, "%u\n", items));

    //os_console_print(str8_format(arena, "global Unicode_Block unicode_block_level0[%u][] = { ", items_per_level));
    //os_console_print(str8_literal("};\n"));

    arena_destroy(arena);
}
