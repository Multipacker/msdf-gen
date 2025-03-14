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
};

internal Void unicode_range_property_list_insert(Arena *arena, Unicode_RangePropertyList *list, U32 min, U32 max, Str8List properties) {
    if (min >= max) {
        return;
    }

    Unicode_RangePropertyNode *node = arena_push_struct_zero(arena, Unicode_RangePropertyNode);
    node->min = min;
    node->max = max;
    node->properties = properties;

    // TODO(simon): Find the correct insertion point.
    // TODO(simon): Shrink and remove ranges that are covered by this range.
}

internal Void unicode_test(Void) {
    Arena *arena = arena_create();
    Str8 contents = { 0 };
    os_file_read(arena, str8_literal("ucd/Blocks.txt"), &contents);

    Unicode_RangePropertyList defaults_properties = { 0 };
    Unicode_RangePropertyList explicit_properties = { 0 };

    Str8List lines = str8_split_by_codepoints(arena, contents, str8_literal("\n"));
    for (Str8Node *line_node = lines.first; line_node; line_node = line_node->next) {
        Arena_Temporary scratch = arena_get_scratch(0, 0);
        Str8 whole_line = line_node->string;

        // NOTE(simon): Strip comments.
        U64 comment_start = str8_first_index_of(whole_line, '#');
        Str8 line    = str8_prefix(whole_line, comment_start);
        Str8 comment = str8_skip(whole_line, comment_start + 1);

        // NOTE(simon): Extract default values.
        Str8 missing_text  = str8_literal("@missing:");
        U64  missing_index = str8_find(0, comment, missing_text);
        Str8 missing       = str8_skip(comment, missgin_index + missing_text.size);
        Str8List default_values = str8_split_by_codepoints(scratch.arena, missing, str8_literal(";"));
        // TODO(simon): Parse default values.

        // NOTE(simon): Skip blank lines.

        // NOTE(simon): Does this line define a segment?
        if (line.data.size >= 1 && line.data[0] == '@') {
        }

        // NOTE(simon): Extract fields.
        Str8List fields = str8_split_by_codepoints(scratch.arena, line, str8_literal(";"));

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

        // NOTE(simon): Effected codepoints.
        Str8Node *codepoint_range = fields.first;
        dll_remove(fields.first, fields.last, codepoint_range);
        //U32 start_codepoint = ...;
        //U32 end_codepoint = start_codepoint;
        //if (is_range) {
            //end_codepoint = ...;
        //}

        for (Str8Node *field_node = fields.first; field_node; field_node = field_node->next) {
            os_console_print(field_node->string);
            os_console_print(str8_literal("\n"));
        }

        arena_end_temporary(scratch);
    }

    arena_destroy(arena);
}
