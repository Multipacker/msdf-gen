// TODO(simon): Allow specifying fonts directly from `FontCache_Font`s.
// TODO(simon): Allow padding for text.

global UI_Key global_ui_null_key = { 0 };

global UI_Box global_ui_null_box = {
    .parent   = &global_ui_null_box,
    .next     = &global_ui_null_box,
    .previous = &global_ui_null_box,
    .first    = &global_ui_null_box,
    .last     = &global_ui_null_box,
};

internal B32 ui_keys_match(UI_Key a, UI_Key b) {
    B32 result = a == b;
    return result;
}

internal Arena *ui_frame_arena(UI_Context *ui) {
    Arena *result = ui->frame_arenas[ui->frame_index % array_count(ui->frame_arenas)];
    return result;
}

// NOTE(simon): Either everything is hashed or only the part after '###'.
internal Str8 ui_hash_part_from_string(Str8 string) {
    Str8 result = string;

    for (U64 i = 0; i < string.size - 2; ++i) {
        if (string.data[i] == '#' && string.data[i + 1] == '#' && string.data[i + 2] == '#') {
            result = str8_skip(string, i + 3);
            break;
        }
    }

    return result;
}

// NOTE(simon): Either everything is displayed or only the part before '##'.
internal Str8 ui_display_part_from_string(Str8 string) {
    Str8 result = string;

    for (U64 i = 0; i < string.size - 1; ++i) {
        if (string.data[i] == '#' && string.data[i + 1] == '#') {
            result = str8_prefix(string, i);
            break;
        }
    }

    return result;
}

internal UI_Key ui_key_from_string(Str8 string) {
    UI_Key result = 6180339887498948482;
    for (U64 i = 0; i < string.size; ++i) {
        result ^= string.data[i];
        result *= 1111111111111111111;
    }
    return (result ^ result >> 32) | 1;
}

internal UI_Key ui_key_from_string_format(CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    UI_Key result = ui_key_from_string(string);

    arena_end_temporary(scratch);
    return result;
}

internal UI_Size ui_size_pixels(F32 pixels, F32 strictness) {
    UI_Size result = { 0 };
    result.kind = UI_Size_Pixels;
    result.value = pixels;
    result.strictness = strictness;
    return result;
}

internal UI_Size ui_size_parent_percent(F32 percent, F32 strictness) {
    UI_Size result = { 0 };
    result.kind = UI_Size_ParentPercent;
    result.value = percent;
    result.strictness = strictness;
    return result;
}

internal UI_Size ui_size_children_sum(F32 strictness) {
    UI_Size result = { 0 };
    result.kind = UI_Size_ChildrenSum;
    result.strictness = strictness;
    return result;
}

internal UI_Size ui_size_text_content(F32 padding, F32 strictness) {
    UI_Size result = { 0 };
    result.kind = UI_Size_TextContent;
    result.value = padding;
    result.strictness = strictness;
    return result;
}



internal UI_Context *ui_create(Void) {
    Arena *arena = arena_create();
    UI_Context *ui = arena_push_struct_zero(arena, UI_Context);

    ui->permanent_arena = arena;
    for (U32 i = 0; i < array_count(ui->frame_arenas); ++i) {
        ui->frame_arenas[i] = arena_create();
    }
    ui->root = &global_ui_null_box;

    ui->box_table = arena_push_array_zero(ui->permanent_arena, UI_BoxList, UI_BOX_TABLE_SIZE);

    return ui;
}



internal Void ui_begin(Gfx_Context *gfx, UI_Context *ui, Gfx_EventList *events, F32 dt) {
    // NOTE(simon): Reset stacks
    ui->parent_stack.top      = 0;
    ui->parent_stack.freelist = 0;
    ui->parent_stack.auto_pop = false;
    ui->color_stack.top      = 0;
    ui->color_stack.freelist = 0;
    ui->color_stack.auto_pop = false;
    ui->border_color_stack.top      = 0;
    ui->border_color_stack.freelist = 0;
    ui->border_color_stack.auto_pop = false;
    ui->text_color_stack.top      = 0;
    ui->text_color_stack.freelist = 0;
    ui->text_color_stack.auto_pop = false;
    ui->size_stacks[Axis2_X].top      = 0;
    ui->size_stacks[Axis2_X].freelist = 0;
    ui->size_stacks[Axis2_X].auto_pop = false;
    ui->size_stacks[Axis2_Y].top      = 0;
    ui->size_stacks[Axis2_Y].freelist = 0;
    ui->size_stacks[Axis2_Y].auto_pop = false;
    ui->layout_axis_stack.top      = 0;
    ui->layout_axis_stack.freelist = 0;
    ui->layout_axis_stack.auto_pop = false;
    ui->extra_box_flags_stack.top      = 0;
    ui->extra_box_flags_stack.freelist = 0;
    ui->extra_box_flags_stack.auto_pop = false;
    ui->fixed_x_stack.top      = 0;
    ui->fixed_x_stack.freelist = 0;
    ui->fixed_x_stack.auto_pop = false;
    ui->fixed_y_stack.top      = 0;
    ui->fixed_y_stack.freelist = 0;
    ui->fixed_y_stack.auto_pop = false;
    ui->font_stack.top      = 0;
    ui->font_stack.freelist = 0;
    ui->font_stack.auto_pop = false;
    ui->font_size_stack.top      = 0;
    ui->font_size_stack.freelist = 0;
    ui->font_size_stack.auto_pop = false;
    ui->hover_cursor_stack.top      = 0;
    ui->hover_cursor_stack.freelist = 0;
    ui->hover_cursor_stack.auto_pop = false;

    ui->mouse = gfx_get_mouse_position(gfx);
    ui->events = events;
    ui->dt = dt;

    ui->is_tooltip_active = false;
    ui->context_menu_used_this_frame = false;

    // NOTE(simon): Give default values to all stacks
    ui_parent_next(ui, &global_ui_null_box);
    ui_color_push(ui, v4f32(0.0f, 0.0f, 0.0f, 0.0f));
    ui_border_color_push(ui, v4f32(0.0f, 0.0f, 0.0f, 0.0f));
    ui_text_color_push(ui, v4f32(1.0f, 1.0f, 1.0f, 1.0f));
    ui_width_push(ui, ui_size_pixels(0.0f, 0.0f));
    ui_height_push(ui, ui_size_pixels(0.0f, 0.0f));
    ui_layout_axis_push(ui, Axis2_X);
    ui_extra_box_flags_push(ui, 0);
    ui_font_push(ui, str8_literal("data/NotoSans-Regular.ttf"));
    ui_font_size_push(ui, 14);
    ui_hover_cursor_push(ui, Gfx_Cursor_Pointer);

    // NOTE(simon): Build root
    {
        V2U32 window_size = gfx_get_window_client_area(gfx);
        ui_width_next(ui, ui_size_pixels(window_size.width, 1.0f));
        ui_height_next(ui, ui_size_pixels(window_size.height, 1.0f));
        ui->root = ui_create_box(ui, 0);
        ui_parent_push(ui, ui->root);
    }

    // NOTE(simon): Build tooltip root
    {
        ui_fixed_x_next(ui, ui->mouse.x + 5.0f);
        ui_fixed_y_next(ui, ui->mouse.y + 5.0f);
        ui_width_next(ui, ui_size_children_sum(1.0f));
        ui_height_next(ui, ui_size_children_sum(1.0f));
        ui_layout_axis_next(ui, Axis2_Y);
        ui->tooltip_root = ui_create_box_from_string(ui, UI_BoxFlags_FloatingPosition, str8_literal("tooltip"));
    }

    // NOTE(simon): Build context menu root
    {
        ui->context_menu_key           = ui->context_menu_key_next;
        ui->context_menu_anchor_key    = ui->context_menu_anchor_key_next;
        ui->context_menu_anchor_offset = ui->context_menu_anchor_offset_next;
        ui_width_next(ui, ui_size_children_sum(1.0f));
        ui_height_next(ui, ui_size_children_sum(1.0f));
        ui_layout_axis_next(ui, Axis2_Y);
        ui->context_menu_root = ui_create_box_from_string(ui, UI_BoxFlags_DrawBackground | UI_BoxFlags_DrawBorder | UI_BoxFlags_Clickable | UI_BoxFlags_Scrollable | UI_BoxFlags_FloatingPosition, str8_literal("context_menu"));
    }

    // NOTE(simon): Reset active key if the active box is disabled or pruned.
    if (!ui_keys_match(ui->active_key, global_ui_null_key)) {
        UI_Box *box = ui_box_from_key(ui, ui->active_key);
        if (box == &global_ui_null_box || box->flags & UI_BoxFlags_Disabled) {
            ui->active_key = global_ui_null_key;
        }
    }

    // NOTE(simon): Reset hot key if there is no active key.
    if (ui_keys_match(ui->active_key, global_ui_null_key)) {
        ui->hot_key = global_ui_null_key;
    }
}

internal Void ui_layout_independent_sizes(UI_Box *box, Axis2 axis) {
    if (box->size[axis].kind == UI_Size_Pixels) {
        box->calculated_size.values[axis] = box->size[axis].value;
    } else if (box->size[axis].kind == UI_Size_TextContent) {
        box->calculated_size.values[axis] = box->text.size.values[axis] + 2.0f * box->size[axis].value;
    }

    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        ui_layout_independent_sizes(child, axis);
    }
}

internal Void ui_layout_upwards_dependent_sizes_no_recurse(UI_Box *box, Axis2 axis) {
    if (box->size[axis].kind == UI_Size_ParentPercent) {
        UI_Box *parent = box->parent;
        while (parent != &global_ui_null_box && parent->size[axis].kind != UI_Size_Pixels) {
            parent = parent->parent;
        }

        box->calculated_size.values[axis] = parent->calculated_size.values[axis] * box->size[axis].value;
    }
}

internal Void ui_layout_upwards_dependent_sizes(UI_Box *box, Axis2 axis) {
    ui_layout_upwards_dependent_sizes_no_recurse(box, axis);

    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        ui_layout_upwards_dependent_sizes(child, axis);
    }
}

internal Void ui_layout_downwards_dependent_sizes(UI_Box *box, Axis2 axis) {
    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        ui_layout_downwards_dependent_sizes(child, axis);
    }

    if (box->size[axis].kind == UI_Size_ChildrenSum) {
        F32 sum = 0.0f;
        if (axis == box->layout_axis) {
            for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
                if (!(child->flags & (UI_BoxFlags_FloatingX << axis))) {
                    sum += child->calculated_size.values[axis];
                }
            }
        } else {
            for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
                if (!(child->flags & (UI_BoxFlags_FloatingX << axis))) {
                    sum = f32_max(sum, child->calculated_size.values[axis]);
                }
            }
        }

        box->calculated_size.values[axis] = sum;
    }
}

internal Void ui_layout_position(UI_Box *box, Axis2 axis) {
    // NOTE(simon): Calculate final rectangle.
    if (box->flags & (UI_BoxFlags_AnimateX << axis)) {
        if (box->create_index == box->last_used_index) {
            box->animated_position.values[axis] = box->calculated_position.values[axis];
        }
        box->calculated_rectangle.min.values[axis] = box->parent->calculated_rectangle.min.values[axis] + box->animated_position.values[axis];
    } else {
        box->calculated_rectangle.min.values[axis] = box->parent->calculated_rectangle.min.values[axis] + box->calculated_position.values[axis];
    }
    box->calculated_rectangle.max.values[axis] = box->calculated_rectangle.min.values[axis] + box->calculated_size.values[axis];

    // NOTE(simon): Position children
    if (axis == box->layout_axis) {
        F32 position = 0.0f;
        for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
            if (!(child->flags & (UI_BoxFlags_FloatingX << axis))) {
                child->calculated_position.values[axis] = position;
                position += child->calculated_size.values[axis];
            }
        }
    } else {
        for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
            if (!(child->flags & (UI_BoxFlags_FloatingX << axis))) {
                child->calculated_position.values[axis] = 0.0f;
            }
        }
    }

    // NOTE(simon): Recurse
    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        ui_layout_position(child, axis);
    }

    // NOTE(simon): Truncate to integer coordinates.
    box->calculated_rectangle.min.values[axis] = f32_floor(box->calculated_rectangle.min.values[axis]);
    box->calculated_rectangle.max.values[axis] = f32_floor(box->calculated_rectangle.max.values[axis]);
}

internal Void ui_layout_resolve_violations(UI_Box *box, Axis2 axis) {
    if (box->flags & (UI_BoxFlags_OverflowX << axis)) {
        for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
            ui_layout_upwards_dependent_sizes_no_recurse(child, axis);
        }
    } else {
        if (axis == box->layout_axis) {
            F32 total_size = 0.0f;
            F32 total_adjustable_size = 0.0f;
            for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
                if (!(child->flags & (UI_BoxFlags_FloatingX << axis))) {
                    total_size += child->calculated_size.values[axis];
                    total_adjustable_size += child->calculated_size.values[axis] * (1.0f - child->size[axis].strictness);
                }
            }

            F32 violation = total_size - box->calculated_size.values[axis];
            if (violation > 0.0f && total_adjustable_size > 0.0f) {
                // NOTE(simon): Adjust children
                F32 adjust_percent = violation / total_adjustable_size;
                for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
                    if (!(child->flags & (UI_BoxFlags_FloatingX << axis))) {
                        F32 child_size = child->calculated_size.values[axis];
                        F32 adjustable_size = child_size * (1.0f - child->size[axis].strictness);

                        child->calculated_size.values[axis] -= f32_min(adjustable_size * adjust_percent, child_size);
                    }
                }
            }
        } else {
            for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
                if (!(child->flags & (UI_BoxFlags_FloatingX << axis))) {
                    F32 violation = f32_max(0.0f, child->calculated_size.values[axis] - box->calculated_size.values[axis]);
                    child->calculated_size.values[axis] -= violation;
                }
            }
        }
    }

    // NOTE(simon): Recurse
    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        ui_layout_resolve_violations(child, axis);
    }
}

internal Void ui_end(Gfx_Context *gfx, UI_Context *ui) {
    // NOTE(simon): Remove untouched boxes.
    for (U32 i = 0; i < UI_BOX_TABLE_SIZE; ++i) {
        UI_BoxList *boxes = &ui->box_table[i];
        for (UI_Box *box = boxes->first, *next; box; box = next) {
            next = box->hash_next;

            if (box->last_used_index != ui->frame_index) {
                dll_remove_next_previous_zero(boxes->first, boxes->last, box, hash_next, hash_previous, 0);
                sll_stack_push(ui->box_freelist, box);
            }
        }
    }

    if (!ui->context_menu_used_this_frame) {
        ui_context_menu_close(ui);
    }

    // NOTE(simon): Layout
    for (Axis2 axis = 0; axis < Axis2_COUNT; ++axis) {
        ui_layout_independent_sizes(ui->root, axis);
        ui_layout_upwards_dependent_sizes(ui->root, axis);
        ui_layout_downwards_dependent_sizes(ui->root, axis);
        ui_layout_resolve_violations(ui->root, axis);
        ui_layout_position(ui->root, axis);
    }

    // NOTE(simon): Move context menu to anchor.
    if (!ui_keys_match(ui->context_menu_key, global_ui_null_key)) {
        if (ui_keys_match(ui->context_menu_anchor_key, global_ui_null_key)) {
            ui->context_menu_root->calculated_position = ui->context_menu_anchor_offset;
        } else {
            UI_Box *anchor = ui_box_from_key(ui, ui->context_menu_anchor_key);
            V2F32 offset = v2f32(0.0f, anchor->calculated_size.height);
            ui->context_menu_root->calculated_position = v2f32_add(anchor->calculated_position, offset);
        }
    }

    // NOTE(simon): Redo layout for tooltip and context menu.
    {
        UI_Box *update_roots[] = { ui->tooltip_root, ui->context_menu_root, };
        for (U32 i = 0; i < array_count(update_roots); ++i) {
            UI_Box *root = update_roots[i];

            for (Axis2 axis = 0; axis < Axis2_COUNT; ++axis) {
                // NOTE(simon): Move the root to always be on screen.
                F32 max_coordinate = ui->root->calculated_size.values[axis];
                F32 size = root->calculated_size.values[axis];
                if (root->calculated_position.values[axis] + size > max_coordinate) {
                    root->calculated_position.values[axis] = max_coordinate - size;
                }
                if (root->calculated_position.values[axis] < 0.0f) {
                    root->calculated_position.values[axis] = 0.0f;
                }

                // NOTE(simon): Redo layout.
                ui_layout_independent_sizes(root, axis);
                ui_layout_upwards_dependent_sizes(root, axis);
                ui_layout_downwards_dependent_sizes(root, axis);
                ui_layout_resolve_violations(root, axis);
                ui_layout_position(root, axis);
            }
        }
    }

    // NOTE(simon): Animate
    F32 fast_rate = 1.0f - f32_pow(2, -ui->dt / (1.0f / 60.0f));
    F32 slow_rate = 1.0f - f32_pow(2, -ui->dt / (1.0f / 30.0f));

    for (U32 i = 0; i < UI_BOX_TABLE_SIZE; ++i) {
        UI_BoxList boxes = ui->box_table[i];
        for (UI_Box *box = boxes.first; box; box = box->hash_next) {
            B32 is_hot      = ui_keys_match(ui->hot_key,    box->key);
            B32 is_active   = ui_keys_match(ui->active_key, box->key);
            B32 is_disabled = box->flags & UI_BoxFlags_Disabled;

            box->animated_position.x += (box->calculated_position.x - box->animated_position.x) * fast_rate;
            box->animated_position.y += (box->calculated_position.y - box->animated_position.y) * fast_rate;
            if (f32_abs(box->calculated_position.x - box->animated_position.x) < 1.0f) {
                box->animated_position.x = box->calculated_position.x;
            }
            if (f32_abs(box->calculated_position.y - box->animated_position.y) < 1.0f) {
                box->animated_position.y = box->calculated_position.y;
            }

            box->hot_t      += (is_hot    - box->hot_t)        * fast_rate;
            box->active_t   += (is_active - box->active_t)     * fast_rate;
            box->disabled_t += (is_disabled - box->disabled_t) * slow_rate;
        }
    }
    ui->tooltip_t += ((F32) ui->is_tooltip_active - ui->tooltip_t) * fast_rate;

    // NOTE(simon): Make sure events don't go through the context menu.
    if (!ui_keys_match(ui->context_menu_anchor_key, global_ui_null_key)) {
        ui_input_from_box(ui, ui->context_menu_root);
    }

    // NOTE(simon): Close the context menu if there were unconsumed click events.
    for (Gfx_Event *event = ui->events->first; event; event = event->next) {
        if (
            event->kind == Gfx_EventKind_KeyPress && (
                event->key == Gfx_Key_MouseLeft ||
                event->key == Gfx_Key_MouseMiddle ||
                event->key == Gfx_Key_MouseRight
            )
        ) {
            ui_context_menu_close(ui);
        }
    }

    // NOTE(simon): Update cursor
    {
        UI_Box *hot = ui_box_from_key(ui, ui->hot_key);
        Gfx_Cursor cursor = hot->hover_cursor;
        if (hot->flags & UI_BoxFlags_Disabled) {
            cursor = Gfx_Cursor_Disabled;
        }
        gfx_set_cursor(gfx, cursor);
    }

    ++ui->frame_index;
    arena_pop_to(ui_frame_arena(ui), 0);
}



internal UI_Box *ui_box_from_key(UI_Context *ui, UI_Key key) {
    UI_Box *result = &global_ui_null_box;

    if (key != global_ui_null_key) {
        UI_BoxList boxes = ui->box_table[key & (UI_BOX_TABLE_SIZE - 1)];
        for (UI_Box *box = boxes.first; box; box = box->hash_next) {
            if (ui_keys_match(box->key, key)) {
                result = box;
                break;
            }
        }
    }

    return result;
}

internal UI_Box *ui_create_box_from_key(UI_Context *ui, UI_BoxFlags flags, UI_Key key) {
    UI_Box *box = ui_box_from_key(ui, key);

    B32 is_transient = ui_keys_match(key, global_ui_null_key);

    if (box == &global_ui_null_box) {
        if (is_transient) {
            box = arena_push_struct_zero(ui_frame_arena(ui), UI_Box);
        } else {
            box = ui->box_freelist;
            if (box) {
                sll_stack_pop(ui->box_freelist);
            } else {
                box = arena_push_struct_zero(ui->permanent_arena, UI_Box);
            }

            UI_BoxList *boxes = &ui->box_table[key & (UI_BOX_TABLE_SIZE - 1)];
            dll_insert_next_previous_zero(boxes->first, boxes->last, boxes->last, box, hash_next, hash_previous, 0);
        }

        box->create_index = ui->frame_index;
    }

    // NOTE(simon): Clear state
    box->next     = &global_ui_null_box;
    box->previous = &global_ui_null_box;
    box->first    = &global_ui_null_box;
    box->last     = &global_ui_null_box;

    // NOTE(simon): Set links
    box->parent = ui->parent_stack.top->item;
    if (box->parent != &global_ui_null_box) {
        dll_insert_next_previous_zero(box->parent->first, box->parent->last, box->parent->last, box, next, previous, &global_ui_null_box);
    }

    box->key = key;
    box->size[Axis2_X] = ui_width_top(ui);
    box->size[Axis2_Y] = ui_height_top(ui);
    box->flags         = flags | ui_extra_box_flags_top(ui);
    box->color         = ui_color_top(ui);
    box->border_color  = ui_border_color_top(ui);
    box->text_color    = ui_text_color_top(ui);
    box->layout_axis   = ui_layout_axis_top(ui);
    box->font          = font_cache_font_from_path(ui_font_top(ui));
    box->font_size     = ui_font_size_top(ui);
    box->hover_cursor  = ui_hover_cursor_top(ui);

    if (ui->fixed_x_stack.top) {
        box->flags |= UI_BoxFlags_FloatingX;
        box->calculated_position.x = ui_fixed_x_top(ui);
    }
    if (ui->fixed_y_stack.top) {
        box->flags |= UI_BoxFlags_FloatingY;
        box->calculated_position.y = ui_fixed_y_top(ui);
    }

    box->last_used_index = ui->frame_index;

    // NOTE(simon): Handle autopops
    ui_parent_auto_pop(ui);
    ui_color_auto_pop(ui);
    ui_border_color_auto_pop(ui);
    ui_text_color_auto_pop(ui);
    ui_width_auto_pop(ui);
    ui_height_auto_pop(ui);
    ui_layout_axis_auto_pop(ui);
    ui_extra_box_flags_auto_pop(ui);
    ui_fixed_x_auto_pop(ui);
    ui_fixed_y_auto_pop(ui);
    ui_font_auto_pop(ui);
    ui_font_size_auto_pop(ui);

    return box;
}

internal UI_Box *ui_create_box(UI_Context *ui, UI_BoxFlags flags) {
    UI_Box *result = ui_create_box_from_key(ui, flags, global_ui_null_key);
    return result;
}

internal UI_Box *ui_create_box_from_string(UI_Context *ui, UI_BoxFlags flags, Str8 string) {
    UI_Key key = ui_key_from_string(ui_hash_part_from_string(string));
    UI_Box *result = ui_create_box_from_key(ui, flags, key);
    ui_box_set_string(ui, result, ui_display_part_from_string(string));
    return result;
}

internal UI_Box *ui_create_box_from_string_format(UI_Context *ui, UI_Key key, CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    UI_Box *result = ui_create_box_from_string(ui, key, string);

    arena_end_temporary(scratch);
    return result;
}

internal Void ui_box_set_string(UI_Context *ui, UI_Box *box, Str8 string) {
    box->string = str8_copy(ui_frame_arena(ui), string);
    if (box->flags & UI_BoxFlags_DrawText) {
        box->text = font_cache_text(ui_frame_arena(ui), box->font, box->string, box->font_size);
    }
}

internal UI_Input ui_input_from_box(UI_Context *ui, UI_Box *box) {
    UI_Input result = { 0 };
    result.box = box;

    R2F32 bounds = box->calculated_rectangle;

    // NOTE(simon): Are we part of the context menu?
    B32 is_context_menu = false;
    for (UI_Box *parent = box; parent != &global_ui_null_box; parent = parent->parent) {
        if (parent == ui->context_menu_root) {
            is_context_menu = true;
            break;
        }
    }

    R2F32 exclude_bounds = { 0 };
    if (!is_context_menu && !ui_keys_match(ui->context_menu_key, global_ui_null_key)) {
        exclude_bounds = ui->context_menu_root->calculated_rectangle;
    }

    for (Gfx_Event *event = ui->events->first, *next; event; event = next) {
        next = event->next;
        B32 consumed = false;

        B32 is_in_bounds = r2f32_contains(bounds, event->position) && !r2f32_contains(exclude_bounds, event->position);
        UI_MouseButtonKind mouse_key = UI_MouseButtonKind_Left;
        B32 is_mouse_key = false;
        switch (event->key) {
            case Gfx_Key_MouseLeft:   is_mouse_key = true; mouse_key = UI_MouseButtonKind_Left;   break;
            case Gfx_Key_MouseMiddle: is_mouse_key = true; mouse_key = UI_MouseButtonKind_Middle; break;
            case Gfx_Key_MouseRight:  is_mouse_key = true; mouse_key = UI_MouseButtonKind_Right;  break;
            default:                  is_mouse_key = false;                                       break;
        }

        // NOTE(simon): Clicked in bounds.
        if (box->flags & UI_BoxFlags_Clickable && is_mouse_key && event->kind == Gfx_EventKind_KeyPress && is_in_bounds) {
            result.input_flags |= UI_InputFlag_LeftPressed << mouse_key;
            ui->active_key = box->key;
            ui->hot_key = box->key;
            consumed = true;
        }

        // NOTE(simon): Release in bounds of active box.
        if (
            box->flags & UI_BoxFlags_Clickable &&
            is_mouse_key &&
            event->kind == Gfx_EventKind_KeyRelease &&
            is_in_bounds &&
            ui_keys_match(ui->active_key, box->key)
        ) {
            result.input_flags |= UI_InputFlag_LeftReleased << mouse_key;
            result.input_flags |= UI_InputFlag_LeftClicked << mouse_key;
            ui->active_key = global_ui_null_key;
            consumed = true;
        }

        // NOTE(simon): Release out of bounds of active box.
        if (
            box->flags & UI_BoxFlags_Clickable &&
            is_mouse_key &&
            event->kind == Gfx_EventKind_KeyRelease &&
            !is_in_bounds &&
            ui_keys_match(ui->active_key, box->key)
        ) {
            result.input_flags |= UI_InputFlag_LeftReleased << mouse_key;
            ui->active_key = global_ui_null_key;
            ui->hot_key = global_ui_null_key;
            consumed = true;
        }

        if (box->flags & UI_BoxFlags_Scrollable && event->kind == Gfx_EventKind_Scroll && is_in_bounds) {
            consumed = true;
        }

        if (consumed) {
            dll_remove(ui->events->first, ui->events->last, event);
        }
    }

    if (
        r2f32_contains(bounds, ui->mouse) &&
        !r2f32_contains(exclude_bounds, ui->mouse) &&
        box->flags & UI_BoxFlags_Clickable &&
        (ui_keys_match(ui->hot_key, global_ui_null_key) || ui_keys_match(ui->hot_key, box->key)) &&
        (ui_keys_match(ui->active_key, global_ui_null_key) || ui_keys_match(ui->active_key, box->key))
    ) {
        ui->hot_key = box->key;
        result.input_flags |= UI_InputFlag_Hovering;
    }

    // NOTE(simon): Pressing on something that isn't the context menu closes it.
    if (!is_context_menu && result.input_flags & UI_InputFlag_Pressed) {
        ui_context_menu_close(ui);
    }

    return result;
}

internal Void ui_tooltip_begin(UI_Context *ui) {
    ui->is_tooltip_active = true;
    ui_parent_push(ui, ui->tooltip_root);
}

internal Void ui_tooltip_end(UI_Context *ui) {
    ui_parent_pop(ui);
}

internal Void ui_context_menu_open(UI_Context *ui, UI_Key context_key, UI_Key anchor_key, V2F32 anchor_offset) {
    ui->context_menu_key_next           = context_key;
    ui->context_menu_anchor_key_next    = anchor_key;
    ui->context_menu_anchor_offset_next = anchor_offset;
    ui->context_menu_used_this_frame    = true;
}

internal Void ui_context_menu_close(UI_Context *ui) {
    ui->context_menu_key_next = global_ui_null_key;
}

internal B32 ui_context_menu_begin(UI_Context *ui, UI_Key context_key) {
    ui_parent_push(ui, ui->context_menu_root);
    B32 result = ui_keys_match(context_key, ui->context_menu_key);
    if (result) {
        ui->context_menu_root->color        = ui_color_top(ui);
        ui->context_menu_root->border_color = ui_border_color_top(ui);
        ui->context_menu_used_this_frame    = true;
    }
    return result;
}

internal Void ui_context_menu_end(UI_Context *ui) {
    ui_parent_pop(ui);
}
