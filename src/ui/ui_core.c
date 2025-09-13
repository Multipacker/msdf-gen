global UI_Context *global_ui_state;

global UI_Key global_ui_null_key = { 0 };

global UI_Box global_ui_null_box = {
    .parent   = &global_ui_null_box,
    .next     = &global_ui_null_box,
    .previous = &global_ui_null_box,
    .first    = &global_ui_null_box,
    .last     = &global_ui_null_box,
};

internal Void ui_select_state(UI_Context *ui_state) {
    global_ui_state = ui_state;
}

internal Arena *ui_frame_arena(Void) {
    UI_Context *ui = global_ui_state;
    Arena *result = ui->frame_arenas[ui->frame_index % array_count(ui->frame_arenas)];
    return result;
}

internal UI_Key ui_active_seed_key(Void) {
    UI_Box *parent_seed = &global_ui_null_box;

    for (UI_Box *parent = ui_parent_top(); !ui_box_is_null(parent); parent = parent->parent) {
        if (!ui_key_is_null(parent->key)) {
            parent_seed = parent;
            break;
        }
    }

    return parent_seed->key;
}

internal V2F32 ui_mouse(Void) {
    UI_Context *ui = global_ui_state;
    return ui->mouse;
}

internal FontCache_Font *ui_icon_font(Void) {
    UI_Context *ui = global_ui_state;
    return ui->icon_info.icon_font;
}

internal Str8 ui_icon_string_from_kind(UI_IconKind icon) {
    UI_Context *ui = global_ui_state;
    return ui->icon_info.icon_kind_text[icon];
}



// NOTE(simon): Event lists
internal Void ui_event_list_push_event(UI_EventList *list, UI_Event *event) {
    dll_push_back(list->first, list->last, event);
}

internal Void ui_event_list_consume_event(UI_EventList *list, UI_Event *event) {
    dll_remove(list->first, list->last, event);
}



// NOTE(simon): Events
internal B32 ui_next_event(UI_Event **event) {
    UI_Context *ui = global_ui_state;

    UI_Event *next_event = ui->events->first;

    if (*event) {
        next_event = (*event)->next;
    }

    *event = next_event;
    B32 result = !!next_event;
    return result;
}

internal Void ui_consume_event(UI_Event *event) {
    UI_Context *ui = global_ui_state;
    ui_event_list_consume_event(ui->events, event);
}

internal UI_Event *ui_consume_event_kind(UI_EventKind kind) {
    UI_Event *result = 0;
    for (UI_Event *event = 0; ui_next_event(&event);) {
        if (event->kind == kind) {
            result = event;
            ui_consume_event(event);
            break;
        }
    }
    return result;
}

internal UI_Event *ui_consume_key_press(Gfx_Key key, Gfx_KeyModifier modifiers) {
    UI_Event *result = 0;
    for (UI_Event *event = 0; ui_next_event(&event);) {
        if (event->kind == UI_EventKind_KeyPress && event->key == key && event->modifiers == modifiers) {
            result = event;
            ui_consume_event(event);
            break;
        }
    }
    return result;
}



// NOTE(simon): Either everything is hashed or only the part after '###'.
internal Str8 ui_hash_part_from_string(Str8 string) {
    Str8 result = string;

    if (string.size > 2) {
        for (U64 i = 0; i < string.size - 2; ++i) {
            if (string.data[i] == '#' && string.data[i + 1] == '#' && string.data[i + 2] == '#') {
                result = str8_skip(string, i + 3);
                break;
            }
        }
    }

    return result;
}

// NOTE(simon): Either everything is displayed or only the part before '##'.
internal Str8 ui_display_part_from_string(Str8 string) {
    Str8 result = string;

    if (string.size > 1) {
        for (U64 i = 0; i < string.size - 1; ++i) {
            if (string.data[i] == '#' && string.data[i + 1] == '#') {
                result = str8_prefix(string, i);
                break;
            }
        }
    }

    return result;
}



// NOTE(simon): Keys
internal B32 ui_keys_match(UI_Key a, UI_Key b) {
    B32 result = a == b;
    return result;
}

internal B32 ui_key_is_null(UI_Key key) {
    B32 result = ui_keys_match(key, global_ui_null_key);
    return result;
}

internal UI_Key ui_key_from_string(UI_Key seed, Str8 string) {
    UI_Key result = seed;
    for (U64 i = 0; i < string.size; ++i) {
        result ^= string.data[i];
        result *= 1111111111111111111;
    }
    return (result ^ result >> 32) | 1;
}

internal UI_Key ui_key_from_string_format(UI_Key seed, CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    UI_Key result = ui_key_from_string(seed, string);

    arena_end_temporary(scratch);
    return result;
}



// NOTE(simon): Focus
internal B32 ui_is_focus_hot(Void) {
    UI_Context *ui = global_ui_state;
    B32 result = ui_focus_hot_top() == UI_Focus_Active;
    if (result) {
        for (UI_FocusStackNode *node = ui->focus_hot_stack.top; node; node = node->next) {
            if (node->item == UI_Focus_Root) {
                break;
            } else if (node->item == UI_Focus_Inactive) {
                result = false;
                break;
            }
        }
    }

    return result;
}

internal B32 ui_is_focus_active(Void) {
    UI_Context *ui = global_ui_state;
    B32 result = ui_focus_active_top() == UI_Focus_Active;
    if (result) {
        for (UI_FocusStackNode *node = ui->focus_active_stack.top; node; node = node->next) {
            if (node->item == UI_Focus_Root) {
                break;
            } else if (node->item == UI_Focus_Inactive) {
                result = false;
                break;
            }
        }
    }

    return result;
}

internal B32 ui_is_key_auto_focus_hot(UI_Key key) {
    B32 result = false;

    if (!ui_key_is_null(key)) {
        for (UI_Box *parent = ui_parent_top(); !ui_box_is_null(parent); parent = parent->parent) {
            if (parent->flags & UI_BoxFlag_DefaultNavigation && ui_keys_match(parent->default_navigation_focus_hot_key, key)) {
                result = true;
                break;
            }
        }
    }

    return result;
}

internal B32 ui_is_key_auto_focus_active(UI_Key key) {
    B32 result = false;

    if (!ui_key_is_null(key)) {
        for (UI_Box *parent = ui_parent_top(); !ui_box_is_null(parent); parent = parent->parent) {
            if (parent->flags & UI_BoxFlag_DefaultNavigation && ui_keys_match(parent->default_navigation_focus_active_key, key)) {
                result = true;
                break;
            }
        }
    }

    return result;
}

internal Void ui_set_auto_focus_hot_key(UI_Key key) {
    for (UI_Box *parent = ui_parent_top(); !ui_box_is_null(parent); parent = parent->parent) {
        if (parent->flags & UI_BoxFlag_DefaultNavigation) {
            parent->default_navigation_focus_hot_key_next = key;
            break;
        }
    }
}

internal Void ui_set_auto_focus_active_key(UI_Key key) {
    for (UI_Box *parent = ui_parent_top(); !ui_box_is_null(parent); parent = parent->parent) {
        if (parent->flags & UI_BoxFlag_DefaultNavigation) {
            parent->default_navigation_focus_active_key_next = key;
            break;
        }
    }
}



// NOTE(simon): Sizes
internal UI_Size ui_size_pixels(F32 pixels, F32 strictness) {
    UI_Size result = { 0 };
    result.kind = UI_Size_Pixels;
    result.value = pixels;
    result.strictness = strictness;
    return result;
}

internal UI_Size ui_size_ems(F32 ems, F32 strictness) {
    UI_Size result = { 0 };
    result.kind = UI_Size_Pixels;
    result.value = ems * (F32) ui_font_size_top();
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

internal UI_Size ui_size_aspect_ratio(F32 ratio, F32 strictness) {
    UI_Size result = { 0 };
    result.kind = UI_Size_AspectRatio;
    result.value = ratio;
    result.strictness = strictness;
    return result;
}



internal UI_Context *ui_create(Void) {
    Arena *arena = arena_create();
    UI_Context *ui = arena_push_struct(arena, UI_Context);
    ui->drag_arena = arena_create();

    ui->permanent_arena = arena;
    for (U32 i = 0; i < array_count(ui->frame_arenas); ++i) {
        ui->frame_arenas[i] = arena_create();
    }
    ui->root = &global_ui_null_box;

    ui->box_table       = arena_push_array(ui->permanent_arena, UI_BoxList,       UI_BOX_TABLE_SIZE);
    ui->animation_table = arena_push_array(ui->permanent_arena, UI_AnimationList, UI_ANIMATION_TABLE_SIZE);

    return ui;
}



internal Void ui_begin(Gfx_Window window, UI_EventList *events, UI_IconInfo *icon_info, F32 dt) {
    prof_function_begin();

    UI_Context *ui = global_ui_state;

    ui->box_count = 0;

    // NOTE(simon): Reset stacks
    memory_zero_struct(&ui->parent_stack);
    memory_zero_struct(&ui->palette_stack);
    memory_zero_struct(&ui->size_stacks);
    memory_zero_struct(&ui->layout_axis_stack);
    memory_zero_struct(&ui->extra_box_flags_stack);
    memory_zero_struct(&ui->fixed_x_stack);
    memory_zero_struct(&ui->fixed_y_stack);
    memory_zero_struct(&ui->font_stack);
    memory_zero_struct(&ui->font_size_stack);
    memory_zero_struct(&ui->text_align_stack);
    memory_zero_struct(&ui->text_x_padding_stack);
    memory_zero_struct(&ui->text_y_padding_stack);
    memory_zero_struct(&ui->hover_cursor_stack);
    memory_zero_struct(&ui->draw_function_stack);
    memory_zero_struct(&ui->draw_data_stack);
    memory_zero_struct(&ui->corner_radius_stacks);
    memory_zero_struct(&ui->focus_hot_stack);
    memory_zero_struct(&ui->focus_active_stack);

    memory_zero_struct(&ui->icon_info);
    ui->icon_info.icon_font = icon_info->icon_font;
    for (UI_IconKind icon = 0; icon < UI_IconKind_COUNT; ++icon) {
        ui->icon_info.icon_kind_text[icon] = str8_copy(ui_frame_arena(), icon_info->icon_kind_text[icon]);
    }
    ui->mouse = gfx_mouse_position_from_window(window);
    ui->events = events;
    ui->dt = dt;
    ui->fast_rate = 1.0f - f32_pow(2, -ui->dt / (1.0f / 60.0f));
    ui->slow_rate = 1.0f - f32_pow(2, -ui->dt / (1.0f / 30.0f));
    ui->super_slow_rate = 1.0f - f32_pow(2, -ui->dt / (1.0f / 10.0f));

    ui->is_tooltip_active = false;
    ui->context_menu_used_this_frame = false;
    ui->is_animating = false;

    // NOTE(simon): Give default values to all stacks
    ui_parent_next(&global_ui_null_box);
    ui_palette_push((UI_Palette) { 0 });
    ui_width_push(ui_size_pixels(0.0f, 0.0f));
    ui_height_push(ui_size_pixels(0.0f, 0.0f));
    ui_layout_axis_push(Axis2_X);
    ui_extra_box_flags_push(0);
    ui_font_push(&global_font_cache_null_font);
    ui_font_size_push(14);
    ui_text_align_push(UI_TextAlign_Left);
    ui_text_padding_push(v2f32(0.0f, 0.0f));
    ui_hover_cursor_push(Gfx_Cursor_Pointer);
    ui_draw_function_push(0);
    ui_draw_data_push(0);
    ui_corner_radius_00_push(0.0f);
    ui_corner_radius_01_push(0.0f);
    ui_corner_radius_10_push(0.0f);
    ui_corner_radius_11_push(0.0f);
    ui_focus_hot_push(UI_Focus_None);
    ui_focus_active_push(UI_Focus_None);

    // NOTE(simon): Default navigation.
    if (!ui_key_is_null(ui->default_navigation_root_key)) {
        UI_Box *default_navigation_root_box = ui_box_from_key(ui->default_navigation_root_key);
        if (!ui_box_is_null(default_navigation_root_box)) {
            if (ui_key_is_null(default_navigation_root_box->default_navigation_focus_active_key)) {
                // NOTE(simon): No child has focus, do navigation at this
                // layer.
                for (;;) {
                    UI_Box *focus_box = ui_box_from_key(default_navigation_root_box->default_navigation_focus_hot_key_next);

                    B32 move_next     = ui_consume_key_press(Gfx_Key_Tab, 0) != 0;
                    B32 move_previous = ui_consume_key_press(Gfx_Key_Tab, Gfx_KeyModifier_Shift) != 0;

                    UI_Box *next_focus_box = &global_ui_null_box;

                    if (move_next) {
                        UI_Box *start = ui_box_is_null(focus_box) ? default_navigation_root_box : focus_box;
                        for (UI_Box *box = start;;) {
                            if (box != start && !(box->flags & UI_BoxFlag_DefaultNavigationSkip) && box->flags & UI_BoxFlag_Clickable) {
                                next_focus_box = box;
                                break;
                            }

                            // NOTE(simon): Find next candidate.
                            UI_Box *last_box = box;
                            if (!ui_box_is_null(box->first)) {
                                box = box->first;
                            } else {
                                for (UI_Box *parent = box; !ui_box_is_null(parent) && parent != default_navigation_root_box; parent = parent->parent) {
                                    if (!ui_box_is_null(parent->next)) {
                                        box = parent->next;
                                        break;
                                    }
                                }
                            }

                            if (last_box == box) {
                                break;
                            }
                        }
                    }

                    if (move_previous) {
                        UI_Box *start = ui_box_is_null(focus_box) ? default_navigation_root_box : focus_box;
                        for (UI_Box *box = start;;) {
                            if (box != start && !(box->flags & UI_BoxFlag_DefaultNavigationSkip) && box->flags & UI_BoxFlag_Clickable) {
                                next_focus_box = box;
                                break;
                            }

                            // NOTE(simon): Find next candidate.
                            UI_Box *last_box = box;
                            if (!ui_box_is_null(box->last)) {
                                box = box->last;
                            } else {
                                for (UI_Box *parent = box; !ui_box_is_null(parent) && parent != default_navigation_root_box; parent = parent->parent) {
                                    if (!ui_box_is_null(parent->previous)) {
                                        box = parent->previous;
                                        break;
                                    }
                                }
                            }

                            if (last_box == box) {
                                break;
                            }
                        }
                    }

                    if (ui_box_is_null(next_focus_box)) {
                        break;
                    } else {
                        default_navigation_root_box->default_navigation_focus_hot_key_next = next_focus_box->key;
                    }
                }
            } else {
                // NOTE(simon): Some child has focus, potentially pop from the
                // active key stack.
                while (ui_consume_event_kind(UI_EventKind_Cancel)) {
                    UI_Box *previous_focus_root_box = default_navigation_root_box;
                    for (UI_Box *focus_root_box = ui_box_from_key(previous_focus_root_box->default_navigation_focus_active_key); !ui_box_is_null(focus_root_box);) {
                        UI_Box *next_focus_root_box = ui_box_from_key(focus_root_box->default_navigation_focus_active_key);
                        if (ui_box_is_null(next_focus_root_box)) {
                            previous_focus_root_box->default_navigation_focus_active_key_next = global_ui_null_key;
                            break;
                        } else {
                            previous_focus_root_box = focus_root_box;
                            focus_root_box = next_focus_root_box;
                        }
                    }
                }
            }
        }

        ui->default_navigation_root_key = global_ui_null_key;
    }

    // NOTE(simon): Update focus hot and focus active.
    // TODO(simon): Maybe move to ui_end
    for (U32 i = 0; i < UI_BOX_TABLE_SIZE; ++i) {
        UI_BoxList *boxes = &ui->box_table[i];
        for (UI_Box *box = boxes->first; box; box = box->hash_next) {
            box->default_navigation_focus_hot_key    = box->default_navigation_focus_hot_key_next;
            box->default_navigation_focus_active_key = box->default_navigation_focus_active_key_next;
        }
    }

    // NOTE(simon): Build root
    {
        V2U32 window_size = gfx_client_area_from_window(window);
        ui_width_next(ui_size_pixels((F32) window_size.width, 1.0f));
        ui_height_next(ui_size_pixels((F32) window_size.height, 1.0f));
        ui->root = ui_create_box(0);
        ui_parent_push(ui->root);
    }

    // NOTE(simon): Build tooltip root
    {
        ui_fixed_x_next(ui->mouse.x + 5.0f);
        ui_fixed_y_next(ui->mouse.y + 5.0f);
        ui_width_next(ui_size_children_sum(1.0f));
        ui_height_next(ui_size_children_sum(1.0f));
        ui_layout_axis_next(Axis2_Y);
        ui->tooltip_root = ui_create_box_from_string(UI_BoxFlag_FloatingPosition, str8_literal("##tooltip"));
    }

    // NOTE(simon): Build context menu root
    {
        ui->context_menu_key           = ui->context_menu_key_next;
        ui->context_menu_anchor_key    = ui->context_menu_anchor_key_next;
        ui->context_menu_anchor_offset = ui->context_menu_anchor_offset_next;
        ui_width_next(ui_size_children_sum(1.0f));
        ui_height_next(ui_size_children_sum(1.0f));
        ui_layout_axis_next(Axis2_Y);
        ui->context_menu_root = ui_create_box_from_string(UI_BoxFlag_Clickable | UI_BoxFlag_Scrollable | UI_BoxFlag_FloatingPosition, str8_literal("##context_menu"));
    }

    // NOTE(simon): Reset active key if the active box is disabled or pruned.
    for (UI_MouseButton button = 0; button < UI_MouseButton_COUNT; ++button) {
        // TODO(simon): This first check could probably be removed because of null values.
        if (!ui_key_is_null(ui->active_key[button])) {
            UI_Box *box = ui_box_from_key(ui->active_key[button]);
            if (ui_box_is_null(box) || box->flags & UI_BoxFlag_Disabled) {
                ui->active_key[button] = global_ui_null_key;
            }
        }
    }

    // NOTE(simon): Reset hot key if there is no active key.
    {
        B32 is_active = false;
        for (UI_MouseButton button = 0; button < UI_MouseButton_COUNT; ++button) {
            is_active |= !ui_key_is_null(ui->active_key[button]);
        }
        if (!is_active) {
            ui->hot_key = global_ui_null_key;
        }
    }

    ui->drop_hot_key = global_ui_null_key;

    prof_function_end();
}

internal Void ui_layout_independent_sizes(UI_Box **boxes, Axis2 axis) {
    UI_Context *ui = global_ui_state;
    for (U64 i = 0; i < ui->box_count; ++i) {
        UI_Box *box = boxes[i];
        if (box->size[axis].kind == UI_Size_Pixels) {
            box->calculated_size.values[axis] = box->size[axis].value;
        } else if (box->size[axis].kind == UI_Size_TextContent) {
            box->calculated_size.values[axis] = box->text.size.values[axis] + 2.0f * box->size[axis].value + 2.0f * box->text_padding.values[axis];
        }
    }
}

internal Void ui_layout_upwards_dependent_sizes_no_recurse(UI_Box *box, Axis2 axis) {
    if (box->size[axis].kind == UI_Size_ParentPercent) {
        UI_Box *parent = box->parent;
        while (
            !ui_box_is_null(parent) && !(
                parent->size[axis].kind == UI_Size_Pixels ||
                parent->size[axis].kind == UI_Size_TextContent ||
                parent->size[axis].kind == UI_Size_ParentPercent
            )
        ) {
            parent = parent->parent;
        }

        box->calculated_size.values[axis] = parent->calculated_size.values[axis] * box->size[axis].value;
    }
}

internal Void ui_layout_upwards_dependent_sizes(UI_Box **boxes, Axis2 axis) {
    UI_Context *ui = global_ui_state;
    for (U64 i = 0; i < ui->box_count; ++i) {
        UI_Box *box = boxes[i];
        ui_layout_upwards_dependent_sizes_no_recurse(box, axis);
    }
}

internal Void ui_layout_self_dependent_sizes(UI_Box **boxes, Axis2 axis) {
    UI_Context *ui = global_ui_state;
    for (U64 i = 0; i < ui->box_count; ++i) {
        UI_Box *box = boxes[i];
        if (box->size[axis].kind == UI_Size_AspectRatio) {
            box->calculated_size.values[axis] = box->calculated_size.values[axis2_flip(axis)] * box->size[axis].value;
        }
    }
}

internal Void ui_layout_downwards_dependent_sizes(UI_Box **boxes, Axis2 axis) {
    UI_Context *ui = global_ui_state;
    for (S64 i = (S64) ui->box_count - 1; i >= 0; --i) {
        UI_Box *box = boxes[i];

        if (box->size[axis].kind == UI_Size_ChildrenSum) {
            F32 sum = 0.0f;
            if (axis == box->layout_axis) {
                for (UI_Box *child = box->first; !ui_box_is_null(child); child = child->next) {
                    if (!(child->flags & (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis))) {
                        sum += child->calculated_size.values[axis];
                    }
                }
            } else {
                for (UI_Box *child = box->first; !ui_box_is_null(child); child = child->next) {
                    if (!(child->flags & (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis))) {
                        sum = f32_max(sum, child->calculated_size.values[axis]);
                    }
                }
            }

            box->calculated_size.values[axis] = sum;
        }
    }
}

internal Void ui_layout_position(UI_Box *box, Axis2 axis) {
    // NOTE(simon): Calculate final rectangle.
    if (box->flags & (UI_BoxFlags) (UI_BoxFlag_AnimateX << axis)) {
        if (box->create_index == box->last_used_index) {
            box->animated_position.values[axis] = box->calculated_position.values[axis];
        }
        box->calculated_rectangle.min.values[axis] = box->parent->calculated_rectangle.min.values[axis] + box->animated_position.values[axis] - box->parent->view_offset.values[axis];
    } else {
        box->calculated_rectangle.min.values[axis] = box->parent->calculated_rectangle.min.values[axis] + box->calculated_position.values[axis] - box->parent->view_offset.values[axis];
    }
    box->calculated_rectangle.max.values[axis] = box->calculated_rectangle.min.values[axis] + box->calculated_size.values[axis];

    // NOTE(simon): Position children
    if (axis == box->layout_axis) {
        F32 position = 0.0f;
        for (UI_Box *child = box->first; !ui_box_is_null(child); child = child->next) {
            if (!(child->flags & (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis))) {
                child->calculated_position.values[axis] = position;
                position += child->calculated_size.values[axis];
            }
        }
    } else {
        for (UI_Box *child = box->first; !ui_box_is_null(child); child = child->next) {
            if (!(child->flags & (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis))) {
                child->calculated_position.values[axis] = 0.0f;
            }
        }
    }

    // NOTE(simon): Recurse
    for (UI_Box *child = box->first; !ui_box_is_null(child); child = child->next) {
        ui_layout_position(child, axis);
    }

    // NOTE(simon): Truncate to integer coordinates.
    box->calculated_rectangle.min.values[axis] = f32_floor(box->calculated_rectangle.min.values[axis]);
    box->calculated_rectangle.max.values[axis] = f32_floor(box->calculated_rectangle.max.values[axis]);
}

internal Void ui_layout_resolve_violations(UI_Box **boxes, Axis2 axis) {
    UI_Context *ui = global_ui_state;
    for (U64 i = 0; i < ui->box_count; ++i) {
        UI_Box *box = boxes[i];

        if (box->flags & (UI_BoxFlags) (UI_BoxFlag_OverflowX << axis)) {
            for (UI_Box *child = box->first; !ui_box_is_null(child); child = child->next) {
                ui_layout_upwards_dependent_sizes_no_recurse(child, axis);
            }
        } else {
            if (axis == box->layout_axis) {
                F32 total_size = 0.0f;
                F32 total_adjustable_size = 0.0f;
                for (UI_Box *child = box->first; !ui_box_is_null(child); child = child->next) {
                    if (!(child->flags & (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis))) {
                        total_size += child->calculated_size.values[axis];
                        total_adjustable_size += child->calculated_size.values[axis] * (1.0f - child->size[axis].strictness);
                    }
                }

                F32 violation = total_size - box->calculated_size.values[axis];
                if (violation > 0.0f && total_adjustable_size > 0.0f) {
                    // NOTE(simon): Adjust children
                    F32 adjust_percent = violation / total_adjustable_size;
                    for (UI_Box *child = box->first; !ui_box_is_null(child); child = child->next) {
                        if (!(child->flags & (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis))) {
                            F32 child_size = child->calculated_size.values[axis];
                            F32 adjustable_size = child_size * (1.0f - child->size[axis].strictness);

                            child->calculated_size.values[axis] -= f32_min(adjustable_size * adjust_percent, child_size);
                        }
                    }
                }
            } else {
                for (UI_Box *child = box->first; !ui_box_is_null(child); child = child->next) {
                    if (!(child->flags & (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis))) {
                        F32 violation = f32_max(0.0f, child->calculated_size.values[axis] - box->calculated_size.values[axis]);
                        child->calculated_size.values[axis] -= violation;
                    }
                }
            }
        }
    }
}

internal Void ui_end(Void) {
    prof_function_begin();

    UI_Context *ui = global_ui_state;

    if (!ui_key_is_null(ui->context_menu_key)) {
        if (ui_consume_event_kind(UI_EventKind_Cancel)) {
            ui_context_menu_close();
        }
    }

    // NOTE(simon): Remove untouched boxes.
    for (U32 i = 0; i < UI_BOX_TABLE_SIZE; ++i) {
        UI_BoxList *boxes = &ui->box_table[i];
        for (UI_Box *box = boxes->first, *next = 0; box; box = next) {
            next = box->hash_next;

            if (box->last_used_index != ui->frame_index) {
                dll_remove_next_previous_zero(boxes->first, boxes->last, box, hash_next, hash_previous, 0);
                sll_stack_push(ui->box_freelist, box);
            }
        }
    }

    // NOTE(simon): Remove untouched animations.
    for (U64 i = 0; i < UI_ANIMATION_TABLE_SIZE; ++i) {
        UI_AnimationList *animations = &ui->animation_table[i];
        for (UI_Animation *animation = animations->first, *next = 0; animation; animation = next) {
            next = animation->next;

            if (animation->last_used_index != ui->frame_index) {
                dll_remove(animations->first, animations->last, animation);
                sll_stack_push(ui->animation_freelist, animation);
            }
        }
    }

    if (!ui->context_menu_used_this_frame) {
        ui_context_menu_close();
    }

    prof_zone_begin(prof_collect, "collect");
    UI_Box **box_array = arena_push_array_no_zero(ui_frame_arena(), UI_Box *, ui->box_count);
    for (UI_Box *box = ui->root, **ptr = box_array; !ui_box_is_null(box); box = ui_box_iterator_depth_first_pre_order(box).next) {
        *ptr++ = box;
    }
    prof_zone_end(prof_collect);

    // NOTE(simon): Layout
    {
        prof_zone_begin(prof_layout, "layout");
        for (Axis2 axis = 0; axis < Axis2_COUNT; ++axis) {
            ui_layout_independent_sizes(box_array, axis);
            ui_layout_upwards_dependent_sizes(box_array, axis);
        }
        for (Axis2 axis = 0; axis < Axis2_COUNT; ++axis) {
            ui_layout_self_dependent_sizes(box_array, axis);
            ui_layout_downwards_dependent_sizes(box_array, axis);
            ui_layout_resolve_violations(box_array, axis);
            ui_layout_position(ui->root, axis);
        }
        prof_zone_end(prof_layout);
    }

    // NOTE(simon): Move context menu to anchor.
    if (!ui_key_is_null(ui->context_menu_key)) {
        if (ui_key_is_null(ui->context_menu_anchor_key)) {
            ui->context_menu_root->calculated_position = ui->context_menu_anchor_offset;
        } else {
            UI_Box *anchor = ui_box_from_key(ui->context_menu_anchor_key);
            V2F32 offset = v2f32(0.0f, anchor->calculated_size.height);
            ui->context_menu_root->calculated_position = v2f32_add(anchor->calculated_rectangle.min, offset);
        }
        ui_layout_position(ui->context_menu_root, Axis2_X);
        ui_layout_position(ui->context_menu_root, Axis2_Y);
    }

    // NOTE(simon): Move tooltip to anchor or mouse.
    {
        if (ui_key_is_null(ui->tooltip_anchor_key)) {
            ui->tooltip_root->calculated_position = ui->mouse;
        } else {
            // TODO(simon): Think about how to anchor to boxes that are
            // partially outside of clip regions. Currently the tooltip stays
            // in the same place as before, which looks odd as it isn't cut off
            // by the clip region.
            UI_Box *anchor = ui_box_from_key(ui->tooltip_anchor_key);
            V2F32 offset = { 0 };
            if (anchor->calculated_size.width >= anchor->calculated_size.height) {
                offset = v2f32(0.5f * (anchor->calculated_size.width - ui->tooltip_root->calculated_size.width), anchor->calculated_size.height);
            } else {
                offset = v2f32(anchor->calculated_size.width, 0.5f * (anchor->calculated_size.height - ui->tooltip_root->calculated_size.height));
            }
            ui->tooltip_root->calculated_position = v2f32_add(anchor->calculated_rectangle.min, offset);
        }
        ui_layout_position(ui->tooltip_root, Axis2_X);
        ui_layout_position(ui->tooltip_root, Axis2_Y);
    }

    // NOTE(simon): Redo layout for tooltip and context menu.
    {
        UI_Box *update_roots[] = { ui->tooltip_root, ui->context_menu_root, };
        B32 force_contain[] = {
            ui_key_is_null(ui->active_key[UI_MouseButton_Left]) &&
            ui_key_is_null(ui->active_key[UI_MouseButton_Middle]) &&
            ui_key_is_null(ui->active_key[UI_MouseButton_Right]),
            1,
        };
        for (U32 i = 0; i < array_count(update_roots); ++i) {
            UI_Box *root = update_roots[i];

            for (Axis2 axis = 0; axis < Axis2_COUNT; ++axis) {
                // NOTE(simon): Move the root to always be on screen.
                F32 max_coordinate = ui->root->calculated_size.values[axis];
                F32 size = root->calculated_size.values[axis];
                if (force_contain[i]) {
                    if (root->calculated_position.values[axis] + size > max_coordinate) {
                        root->calculated_position.values[axis] = max_coordinate - size;
                    }
                    if (root->calculated_position.values[axis] < 0.0f) {
                        root->calculated_position.values[axis] = 0.0f;
                    }
                }

                // NOTE(simon): Recalculate child positions.
                ui_layout_position(root, axis);
            }
        }
    }

    // NOTE(simon): Animate
    {
        prof_zone_begin(prof_animate, "animate");

        for (U64 i = 0; i < ui->box_count; ++i) {
            UI_Box *box = box_array[i];

            // NOTE(simon): Skip transient boxes.
            if (ui_key_is_null(box->key)) {
                continue;
            }

            B32 is_hot                   = ui_keys_match(ui->hot_key, box->key);
            B32 is_active                = ui_keys_match(ui->active_key[UI_MouseButton_Left], box->key);
            B32 is_disabled              = !!(box->flags & UI_BoxFlag_Disabled);
            B32 is_focus_hot             = !!(box->flags & UI_BoxFlag_FocusHot);
            B32 is_focus_hot_disabled    = !!(box->flags & UI_BoxFlag_FocusHotDisabled);
            B32 is_focus_active          = !!(box->flags & UI_BoxFlag_FocusActive);
            B32 is_focus_active_disabled = !!(box->flags & UI_BoxFlag_FocusActiveDisabled);

            F32 position_x_delta              = (box->calculated_position.x     - box->animated_position.x)     * ui->fast_rate;
            F32 position_y_delta              = (box->calculated_position.y     - box->animated_position.y)     * ui->fast_rate;
            F32 hot_t_delta                   = ((F32) is_hot                   - box->hot_t)                   * ui->fast_rate;
            F32 active_t_delta                = ((F32) is_active                - box->active_t)                * ui->fast_rate;
            F32 disabled_t_delta              = ((F32) is_disabled              - box->disabled_t)              * ui->slow_rate;
            F32 focus_hot_t_delta             = ((F32) is_focus_hot             - box->focus_hot_t)             * ui->fast_rate;
            F32 focus_active_t_delta          = ((F32) is_focus_active          - box->focus_active_t)          * ui->fast_rate;
            F32 focus_active_disabled_t_delta = ((F32) is_focus_active_disabled - box->focus_active_disabled_t) * ui->fast_rate;

            B32 is_animating = false;
            is_animating |= f32_abs(position_x_delta)              >= 1.0f;
            is_animating |= f32_abs(position_y_delta)              >= 1.0f;
            is_animating |= f32_abs(hot_t_delta)                   >= 0.001f;
            is_animating |= f32_abs(active_t_delta)                >= 0.001f;
            is_animating |= f32_abs(disabled_t_delta)              >= 0.001f;
            is_animating |= f32_abs(focus_active_t_delta)          >= 0.001f;
            is_animating |= f32_abs(focus_hot_t_delta)             >= 0.001f;
            is_animating |= f32_abs(focus_active_t_delta)          >= 0.001f;
            is_animating |= f32_abs(focus_active_disabled_t_delta) >= 0.001f;

            if (is_animating) {
                box->animated_position.x     += position_x_delta;
                box->animated_position.y     += position_y_delta;
                box->hot_t                   += hot_t_delta;
                box->active_t                += active_t_delta;
                box->disabled_t              += disabled_t_delta;
                box->focus_hot_t             += focus_hot_t_delta;
                box->focus_active_t          += focus_active_t_delta;
                box->focus_active_disabled_t += focus_active_disabled_t_delta;
            } else {
                box->animated_position.x     = box->calculated_position.x;
                box->animated_position.y     = box->calculated_position.y;
                box->hot_t                   = (F32) is_hot;
                box->active_t                = (F32) is_active;
                box->disabled_t              = (F32) is_disabled;
                box->focus_hot_t             = (F32) is_focus_hot;
                box->focus_active_t          = (F32) is_focus_active;
                box->focus_active_disabled_t = (F32) is_focus_active_disabled;
            }

            ui->is_animating |= is_animating;
        }

        for (U64 i = 0; i < UI_ANIMATION_TABLE_SIZE; ++i) {
            UI_AnimationList *animations = &ui->animation_table[i];
            for (UI_Animation *animation = animations->first; animation; animation = animation->next) {
                F32 delta = animation->parameters.target - animation->current;
                if (f32_abs(delta) >= animation->parameters.epsilon) {
                    animation->current += delta * animation->parameters.rate;
                    ui->is_animating = true;
                } else {
                    animation->current = animation->parameters.target;
                }
            }
        }

        F32 tooltip_t_delta = ((F32) ui->is_tooltip_active - ui->tooltip_t) * ui->fast_rate;
        if (f32_abs(tooltip_t_delta) >= 0.001f) {
            ui->tooltip_t += tooltip_t_delta;
            ui->is_animating = true;
        } else {
            ui->tooltip_t = (F32) ui->is_tooltip_active;
        }

        prof_zone_end(prof_animate);
    }

    // NOTE(simon): Make sure events don't go through the context menu.
    if (!ui_key_is_null(ui->context_menu_anchor_key)) {
        ui_input_from_box(ui->context_menu_root);
    }

    // NOTE(simon): Close the context menu if there were unconsumed click events.
    for (UI_Event *event = ui->events->first; event; event = event->next) {
        if (
            event->kind == UI_EventKind_KeyPress && (
                event->key == Gfx_Key_MouseLeft ||
                event->key == Gfx_Key_MouseMiddle ||
                event->key == Gfx_Key_MouseRight
            )
        ) {
            ui_context_menu_close();
        }
    }

    // NOTE(simon): Update cursor
    {
        UI_Box *hot = ui_box_from_key(ui->hot_key);
        Gfx_Cursor cursor = hot->hover_cursor;
        if (hot->flags & UI_BoxFlag_Disabled) {
            cursor = Gfx_Cursor_Disabled;
        }
        gfx_set_cursor(cursor);
    }

    ++ui->frame_index;
    arena_pop_to(ui_frame_arena(), 0);

    prof_function_end();
}



internal UI_BoxIterator ui_box_iterator_depth_first_pre_order(UI_Box *box) {
    UI_BoxIterator iterator = { 0 };
    iterator.next = &global_ui_null_box;

    if (!ui_box_is_null(box->last)) {
        iterator.next = box->last;
        iterator.push_count = 1;
    } else {
        for (UI_Box *parent = box; !ui_box_is_null(parent); parent = parent->parent) {
            if (!ui_box_is_null(parent->previous)) {
                iterator.next = parent->previous;
                break;
            }
            ++iterator.pop_count;
        }
    }

    return iterator;
}



// NOTE(simon): Boxes
internal B32 ui_box_is_null(UI_Box *box) {
    B32 result = box == &global_ui_null_box;
    return result;
}

internal UI_Box *ui_box_from_key(UI_Key key) {
    UI_Context *ui = global_ui_state;
    UI_Box *result = &global_ui_null_box;

    if (!ui_key_is_null(key)) {
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

internal UI_Box *ui_create_box_from_key(UI_BoxFlags flags, UI_Key key) {
    prof_function_begin();
    UI_Context *ui = global_ui_state;
    ++ui->box_count;
    UI_Box *box = ui_box_from_key(key);

    // NOTE(simon): Zero the box if it was already used this frame.
    if (!ui_box_is_null(box) && box->last_used_index == ui->frame_index) {
        box = &global_ui_null_box;
        key = global_ui_null_key;
    }

    B32 is_transient = ui_key_is_null(key);

    if (ui_box_is_null(box)) {
        if (is_transient) {
            box = arena_push_struct(ui_frame_arena(), UI_Box);
        } else {
            box = ui->box_freelist;
            if (box) {
                sll_stack_pop(ui->box_freelist);
                memory_zero_struct(box);
            } else {
                box = arena_push_struct(ui->permanent_arena, UI_Box);
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
    if (!ui_box_is_null(box->parent)) {
        dll_insert_next_previous_zero(box->parent->first, box->parent->last, box->parent->last, box, next, previous, &global_ui_null_box);
    }

    box->key = key;

    box->size[Axis2_X] = ui_width_top();
    box->size[Axis2_Y] = ui_height_top();
    box->flags         = flags | ui_extra_box_flags_top();
    box->palette       = ui_palette_top();
    box->layout_axis   = ui_layout_axis_top();
    box->font          = ui_font_top();
    box->font_size     = ui_font_size_top();
    box->text_align    = ui_text_align_top();
    box->text_padding  = ui_text_padding_top();
    box->hover_cursor  = ui_hover_cursor_top();
    box->draw_list     = 0;
    box->draw_function = ui_draw_function_top();
    box->draw_data     = ui_draw_data_top();
    box->corner_radies[Corner_00] = ui_corner_radius_00_top();
    box->corner_radies[Corner_01] = ui_corner_radius_01_top();
    box->corner_radies[Corner_10] = ui_corner_radius_10_top();
    box->corner_radies[Corner_11] = ui_corner_radius_11_top();

    B32 is_auto_focus_hot    = ui_is_key_auto_focus_hot(key);
    B32 is_auto_focus_active = ui_is_key_auto_focus_active(key);

    if (is_auto_focus_hot) {
        ui_focus_hot_next(UI_Focus_Active);
    }

    if (is_auto_focus_active) {
        ui_focus_active_next(UI_Focus_Active);
    }

    if (box->flags & UI_BoxFlag_DefaultNavigation && ui_is_focus_active() && ui_key_is_null(ui->default_navigation_root_key)) {
        ui->default_navigation_root_key = key;
    }

    if (ui_focus_hot_top() == UI_Focus_Active) {
        box->flags |= UI_BoxFlag_FocusHot;
    }

    if (ui_focus_active_top() == UI_Focus_Active) {
        box->flags |= UI_BoxFlag_FocusActive;
    }

    if (ui_focus_hot_top() == UI_Focus_Active && !ui_is_focus_hot()) {
        box->flags |= UI_BoxFlag_FocusHotDisabled;
    }

    if (ui_focus_active_top() == UI_Focus_Active && !ui_is_focus_active()) {
        box->flags |= UI_BoxFlag_FocusActiveDisabled;
    }

    if (ui->fixed_x_stack.top) {
        box->flags |= UI_BoxFlag_FloatingX;
        box->calculated_position.x = ui_fixed_x_top();
    }
    if (ui->fixed_y_stack.top) {
        box->flags |= UI_BoxFlag_FloatingY;
        box->calculated_position.y = ui_fixed_y_top();
    }

    box->last_used_index = ui->frame_index;

    // NOTE(simon): Handle autopops
    ui_parent_auto_pop();
    ui_palette_auto_pop();
    ui_width_auto_pop();
    ui_height_auto_pop();
    ui_layout_axis_auto_pop();
    ui_extra_box_flags_auto_pop();
    ui_fixed_x_auto_pop();
    ui_fixed_y_auto_pop();
    ui_font_auto_pop();
    ui_font_size_auto_pop();
    ui_text_align_auto_pop();
    ui_text_x_padding_auto_pop();
    ui_text_y_padding_auto_pop();
    ui_hover_cursor_auto_pop();
    ui_draw_function_auto_pop();
    ui_draw_data_auto_pop();
    ui_corner_radius_00_auto_pop();
    ui_corner_radius_01_auto_pop();
    ui_corner_radius_10_auto_pop();
    ui_corner_radius_11_auto_pop();
    ui_focus_hot_auto_pop();
    ui_focus_active_auto_pop();

    prof_function_end();
    return box;
}

internal UI_Box *ui_create_box(UI_BoxFlags flags) {
    UI_Box *result = ui_create_box_from_key(flags, global_ui_null_key);
    return result;
}

internal UI_Box *ui_create_box_from_string(UI_BoxFlags flags, Str8 string) {
    UI_Key key = ui_key_from_string(ui_active_seed_key(), ui_hash_part_from_string(string));
    UI_Box *result = ui_create_box_from_key(flags, key);
    ui_box_set_string(result, ui_display_part_from_string(string));
    return result;
}

internal UI_Box *ui_create_box_from_string_format(UI_BoxFlags flags, CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    UI_Box *result = ui_create_box_from_string(flags, string);

    arena_end_temporary(scratch);
    return result;
}

internal Void ui_box_set_string(UI_Box *box, Str8 string) {
    box->string = str8_copy(ui_frame_arena(), string);
    if (box->flags & UI_BoxFlag_DrawText) {
        box->text = font_cache_text(ui_frame_arena(), box->font, box->string, box->font_size);
    }
}

internal Void ui_box_set_fuzzy_match_list(UI_Box *box, FuzzyMatchList fuzzy_matches) {
    box->fuzzy_matches = fuzzy_match_list_copy(ui_frame_arena(), fuzzy_matches);
    box->flags |= UI_BoxFlag_DrawFuzzyMatches;
}

internal Void ui_box_set_draw_list(UI_Box *box, Draw_List *list) {
    box->draw_list = list;
}

internal V2F32 ui_box_text_location(UI_Box *box) {
    V2F32 result = { 0 };

    V2F32 offset = box->text_padding;
    for (Axis2 axis = Axis2_X; axis < Axis2_COUNT; ++axis) {
        if (box->size[axis].kind == UI_Size_TextContent) {
            offset.values[axis] += box->size[axis].value;
        }
    }

    result.y = (box->calculated_rectangle.min.y + box->calculated_rectangle.max.y) * 0.5f + (box->text.ascent + box->text.descent) * 0.5f;

    switch (box->text_align) {
        case UI_TextAlign_Left: {
            result.x = box->calculated_rectangle.min.x + offset.x;
        } break;
        case UI_TextAlign_Center: {
            result.x = (box->calculated_rectangle.max.x + box->calculated_rectangle.min.x) * 0.5f - box->text.size.x * 0.5f;
            result.x = f32_max(box->calculated_rectangle.min.x + offset.x, result.x);
            result.x = f32_floor(result.x);
        } break;
        case UI_TextAlign_Right: {
            result.x = box->calculated_rectangle.max.x - box->text.size.x - offset.x;
            result.x = f32_max(box->calculated_rectangle.min.x + offset.x, result.x);
            result.x = f32_floor(result.x);
        } break;
        case UI_TextAlign_COUNT: {
        } break;
    }

    return result;
}

internal UI_Input ui_input_from_box(UI_Box *box) {
    UI_Context *ui = global_ui_state;
    UI_Input result = { 0 };
    result.box = box;

    B32 is_focus_hot = box->flags & UI_BoxFlag_FocusHot && !(box->flags & UI_BoxFlag_FocusHotDisabled);

    R2F32 bounds = box->calculated_rectangle;
    for (UI_Box *parent = box; !ui_box_is_null(parent); parent = parent->parent) {
        if (parent->flags & UI_BoxFlag_Clip) {
            bounds = r2f32_intersect(bounds, parent->calculated_rectangle);
        }
    }

    // NOTE(simon): Are we part of the context menu?
    B32 is_context_menu = false;
    for (UI_Box *parent = box; !ui_box_is_null(parent); parent = parent->parent) {
        if (parent == ui->context_menu_root) {
            is_context_menu = true;
            break;
        }
    }

    R2F32 exclude_bounds = { 0 };
    if (!is_context_menu && !ui_keys_match(ui->context_menu_key, global_ui_null_key)) {
        exclude_bounds = ui->context_menu_root->calculated_rectangle;
    }

    for (UI_Event *event = 0; ui_next_event(&event);) {
        B32 consumed = false;

        B32 is_in_bounds = r2f32_contains_v2f32(bounds, event->position) && !r2f32_contains_v2f32(exclude_bounds, event->position);
        UI_MouseButton mouse_key = UI_MouseButton_Left;
        B32 is_mouse_key = false;
        switch (event->key) {
            case Gfx_Key_MouseLeft:   is_mouse_key = true; mouse_key = UI_MouseButton_Left;   break;
            case Gfx_Key_MouseMiddle: is_mouse_key = true; mouse_key = UI_MouseButton_Middle; break;
            case Gfx_Key_MouseRight:  is_mouse_key = true; mouse_key = UI_MouseButton_Right;  break;
            default:                  is_mouse_key = false;                                   break;
        }

        // NOTE(simon): Clicked in bounds.
        if (box->flags & UI_BoxFlag_Clickable && is_mouse_key && event->kind == UI_EventKind_KeyPress && is_in_bounds) {
            result.flags |= (UI_InputFlag) (UI_InputFlag_LeftPressed << mouse_key);
            ui->active_key[mouse_key] = box->key;
            ui->hot_key = box->key;
            ui->drag_start = event->position;
            consumed = true;
        }

        // NOTE(simon): Release in bounds of active box.
        if (
            box->flags & UI_BoxFlag_Clickable &&
            is_mouse_key &&
            event->kind == UI_EventKind_KeyRelease &&
            is_in_bounds &&
            ui_keys_match(ui->active_key[mouse_key], box->key)
        ) {
            result.flags |= (UI_InputFlag) (UI_InputFlag_LeftReleased << mouse_key);
            result.flags |= (UI_InputFlag) (UI_InputFlag_LeftClicked << mouse_key);
            ui->active_key[mouse_key] = global_ui_null_key;
            consumed = true;
        }

        // NOTE(simon): Release out of bounds of active box.
        if (
            box->flags & UI_BoxFlag_Clickable &&
            is_mouse_key &&
            event->kind == UI_EventKind_KeyRelease &&
            !is_in_bounds &&
            ui_keys_match(ui->active_key[mouse_key], box->key)
        ) {
            result.flags |= (UI_InputFlag) (UI_InputFlag_LeftReleased << mouse_key);
            ui->active_key[mouse_key] = global_ui_null_key;
            ui->hot_key = global_ui_null_key;
            consumed = true;
        }

        if ((box->flags & UI_BoxFlag_KeyboardClickable) && is_focus_hot && event->kind == UI_EventKind_Accept) {
            result.flags |= UI_InputFlag_KeyboardPressed;
            consumed = true;
        }

        if (box->flags & UI_BoxFlag_Scrollable && event->kind == UI_EventKind_Scroll && is_in_bounds) {
            result.scroll = v2f32_add(result.scroll, event->scroll);
            consumed = true;
        }

        if (consumed) {
            ui_consume_event(event);
        }
    }

    // NOTE(simon): If hovering over a drop target, set us as the hot drop
    // target.
    if (
        box->flags & UI_BoxFlag_DropTarget &&
        r2f32_contains_v2f32(bounds, ui->mouse) &&
        !r2f32_contains_v2f32(exclude_bounds, ui->mouse) &&
        ui_keys_match(ui->drop_hot_key, global_ui_null_key)
       ) {
        ui->drop_hot_key = box->key;
    }

    if (box->flags & UI_BoxFlag_Clickable) {
        for (UI_MouseButton button = 0; button < UI_MouseButton_COUNT; ++button) {
            if (result.flags & (UI_InputFlag) (UI_InputFlag_LeftPressed << button) || ui_keys_match(ui->active_key[button], box->key)) {
                result.flags |= (UI_InputFlag) (UI_InputFlag_LeftDragging << button);
            }
        }
    }

    if (
        r2f32_contains_v2f32(bounds, ui->mouse) &&
        !r2f32_contains_v2f32(exclude_bounds, ui->mouse) &&
        box->flags & UI_BoxFlag_Clickable &&
        (ui_keys_match(ui->hot_key, global_ui_null_key) || ui_keys_match(ui->hot_key, box->key)) &&
        (ui_keys_match(ui->active_key[UI_MouseButton_Left],   global_ui_null_key) || ui_keys_match(ui->active_key[UI_MouseButton_Left],   box->key)) &&
        (ui_keys_match(ui->active_key[UI_MouseButton_Middle], global_ui_null_key) || ui_keys_match(ui->active_key[UI_MouseButton_Middle], box->key)) &&
        (ui_keys_match(ui->active_key[UI_MouseButton_Right],  global_ui_null_key) || ui_keys_match(ui->active_key[UI_MouseButton_Right],  box->key))
    ) {
        ui->hot_key = box->key;
        result.flags |= UI_InputFlag_Hovering;
    }

    // NOTE(simon): Pressing on something that isn't the context menu closes it.
    if (!is_context_menu && result.flags & UI_InputFlag_Pressed) {
        ui_context_menu_close();
    }

    UI_Box *default_navigation_parent = &global_ui_null_box;
    for (UI_Box *parent = box->parent; !ui_box_is_null(parent); parent = parent->parent) {
        if (parent->flags & UI_BoxFlag_DefaultNavigation) {
            default_navigation_parent = parent;
            break;
        }
    }

    if (box->flags & UI_BoxFlag_ClickToFocus && result.flags & UI_InputFlag_Pressed && !ui_box_is_null(default_navigation_parent)) {
        default_navigation_parent->default_navigation_focus_hot_key_next = box->key;
        if (!ui_keys_match(default_navigation_parent->default_navigation_focus_active_key, box->key)) {
            default_navigation_parent->default_navigation_focus_active_key_next = global_ui_null_key;
        }
    }

    return result;
}



// NOTE(simon): Tooltips
internal Void ui_tooltip_begin(UI_Key anchor_key) {
    UI_Context *ui = global_ui_state;
    ui->tooltip_anchor_key = anchor_key;
    ui->is_tooltip_active = true;
    ui_parent_push(ui->tooltip_root);
}

internal Void ui_tooltip_end(Void) {
    ui_parent_pop();
}



// NOTE(simon): Context menus
internal Void ui_context_menu_open(UI_Key context_key, UI_Key anchor_key, V2F32 anchor_offset) {
    UI_Context *ui = global_ui_state;
    ui->context_menu_key_next           = context_key;
    ui->context_menu_anchor_key_next    = anchor_key;
    ui->context_menu_anchor_offset_next = anchor_offset;
    ui->context_menu_used_this_frame    = true;
}

internal Void ui_context_menu_close(Void) {
    UI_Context *ui = global_ui_state;
    ui->context_menu_key_next = global_ui_null_key;
}

internal B32 ui_context_menu_begin(UI_Key context_key) {
    UI_Context *ui = global_ui_state;
    ui_parent_push(ui->context_menu_root);
    B32 result = ui_keys_match(context_key, ui->context_menu_key);
    if (result) {
        ui->context_menu_root->palette = ui_palette_top();
        ui->context_menu_used_this_frame    = true;
    }
    return result;
}

internal Void ui_context_menu_end(Void) {
    ui_parent_pop();
}

internal B32 ui_context_menu_is_open(UI_Key context_key) {
    UI_Context *ui = global_ui_state;
    B32 result = ui_keys_match(ui->context_menu_key, context_key);
    return result;
}



// NOTE(simon): Drag and drop
internal UI_Key ui_drop_hot_key(Void) {
    UI_Context *ui = global_ui_state;
    return ui->drop_hot_key;
}

internal V2F32 ui_drag_delta(Void) {
    UI_Context *ui = global_ui_state;
    V2F32 result = v2f32_subtract(ui->mouse, ui->drag_start);
    return result;
}

internal Str8 ui_get_drag_data_str8(U64 min_size) {
    UI_Context *ui = global_ui_state;

    if (ui->drag_data.size < min_size) {
        Arena_Temporary scratch = arena_get_scratch(0, 0);
        Str8 data = {
            .data = arena_push_array_no_zero(scratch.arena, U8, min_size),
            .size = min_size,
        };
        ui_set_drag_data_str8(data);
        arena_end_temporary(scratch);
    }

    return ui->drag_data;
}

internal Void ui_set_drag_data_str8(Str8 data) {
    UI_Context *ui = global_ui_state;
    arena_pop_to(ui->drag_arena, 0);
    ui->drag_data = str8_copy(ui->drag_arena, data);
}



// NOTE(simon): Animation
internal F32 ui_animation_super_slow_rate(Void) {
    F32 result = global_ui_state->super_slow_rate;
    return result;
}

internal F32 ui_animation_slow_rate(Void) {
    F32 result = global_ui_state->slow_rate;
    return result;
}

internal F32 ui_animation_fast_rate(Void) {
    F32 result = global_ui_state->fast_rate;
    return result;
}

internal B32 ui_is_animating_from_context(UI_Context *ui) {
    B32 result = ui->is_animating;
    return result;
}

internal F32 ui_animate_internal(UI_Key key, UI_AnimationParameters *parameters) {
    UI_Context *ui = global_ui_state;

    // NOTE(simon): Get animation.
    UI_Animation *animation = 0;
    {
        UI_AnimationList *animations = &ui->animation_table[key % (UI_ANIMATION_TABLE_SIZE - 1)];
        for (UI_Animation *node = animations->first; node; node = node->next) {
            if (node->key == key) {
                animation = node;
                break;
            }
        }

        if (!animation) {
            animation = ui->animation_freelist;
            if (animation) {
                sll_stack_pop(ui->animation_freelist);
                memory_zero_struct(animation);
            } else {
                animation = arena_push_struct(ui->permanent_arena, UI_Animation);
            }

            animation->key        = key;
            animation->parameters = *parameters;
            animation->current    = parameters->initial;
            dll_push_back(animations->first, animations->last, animation);
        }
    }

    // NOTE(simon): Mark as used and update parameters and current.
    F32 result = parameters->target;
    if (animation) {
        animation->last_used_index = ui->frame_index;

        if (parameters->reset) {
            animation->current = parameters->initial;
        }

        animation->parameters = *parameters;
        if (animation->parameters.epsilon == 0.0f) {
            animation->parameters.epsilon = 0.005f;
        }

        result = animation->current;
    }

    return result;
}
