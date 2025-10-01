internal UI_Size ui_size_fill(Void) {
    UI_Size result = ui_size_parent_percent(1.0f, 0.0f);
    return result;
}

internal UI_Box *ui_spacer(Void) {
    UI_Box *spacer = ui_create_box(0);
    return spacer;
}

internal UI_Box *ui_spacer_sized(UI_Size size) {
    Axis2 axis = ui_parent_top()->layout_axis;
    ui_size_next(size, axis);
    UI_Box *spacer = ui_spacer();
    return spacer;
}

internal UI_Box *ui_row_begin(Void) {
    ui_layout_axis_next(Axis2_X);
    UI_Box *row = ui_create_box(0);
    ui_parent_push(row);
    return row;
}

internal UI_Box *ui_row_string_begin(Str8 string) {
    ui_layout_axis_next(Axis2_X);
    UI_Box *row = ui_create_box_from_string(0, string);
    ui_parent_push(row);
    return row;
}

internal UI_Box *ui_row_end(Void) {
    UI_Box *result = ui_parent_pop();
    return result;
}

internal UI_Box *ui_column_begin(Void) {
    ui_layout_axis_next(Axis2_Y);
    UI_Box *column = ui_create_box(0);
    ui_parent_push(column);
    return column;
}

internal UI_Box *ui_column_string_begin(Str8 string) {
    ui_layout_axis_next(Axis2_Y);
    UI_Box *column = ui_create_box_from_string(0, string);
    ui_parent_push(column);
    return column;
}

internal UI_Box *ui_column_end(Void) {
    UI_Box *result = ui_parent_pop();
    return result;
}



internal UI_Box *ui_label(Str8 string) {
    UI_Box *box = ui_create_box_from_string(UI_BoxFlag_DrawText, string);
    return box;
}

internal UI_Box *ui_label_format(CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    UI_Box *box = ui_label(string);

    arena_end_temporary(scratch);
    return box;
}



internal UI_Input ui_button(Str8 string) {
    ui_hover_cursor_next(Gfx_Cursor_Hand);
    UI_Box *box = ui_create_box_from_string(
        UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText | UI_BoxFlag_DrawBorder |
        UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
        UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable,
        string
    );
    UI_Input result = ui_input_from_box(box);
    return result;
}

internal UI_Input ui_button_format(CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    UI_Input result = ui_button(string);

    arena_end_temporary(scratch);
    return result;
}

internal UI_Input ui_checkbox(B32 is_checked, Str8 label) {
    ui_width_next(ui_size_children_sum(1.0f));
    ui_height_next(ui_size_children_sum(1.0f));
    ui_row_begin();

    ui_width_next(ui_size_ems(1.2f, 1.0f));
    ui_height_next(ui_size_ems(1.2f, 1.0f));
    ui_hover_cursor_next(Gfx_Cursor_Hand);
    ui_text_align_next(UI_TextAlign_Center);
    ui_font_next(ui_icon_font());
    UI_Box *check = ui_create_box_from_string_format(
        UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | (is_checked ? UI_BoxFlag_DrawText : 0) |
        UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
        UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable,
        "%.*s###check_%.*s", str8_expand(ui_icon_string_from_kind(UI_IconKind_Check)), str8_expand(label)
    );

    ui_spacer_sized(ui_size_ems(0.5f, 1.0f));

    ui_width_next(ui_size_text_content(0.0f, 1.0f));
    ui_height_next(ui_size_ems(1.2f, 1.0f));
    ui_label(label);
    ui_row_end();

    UI_Input check_input = ui_input_from_box(check);
    return check_input;
}

internal UI_Input ui_checkbox_format(B32 is_checked, CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    UI_Input result = ui_checkbox(is_checked, string);
    arena_end_temporary(scratch);
    return result;
}

internal UI_Input ui_checkbox_b32(B32 *is_checked, Str8 label) {
    UI_Input input = ui_checkbox(*is_checked, label);
    if (input.flags & UI_InputFlag_Clicked) {
        *is_checked = !(*is_checked);
    }
    return input;
}

internal UI_Input ui_checkbox_b32_format(B32 *is_checked, CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    UI_Input result = ui_checkbox_b32(is_checked, string);
    arena_end_temporary(scratch);
    return result;
}



// TODO(simon): This visualization doesn't support bidirectional text layout.
UI_BOX_DRAW_FUNCTION(ui_draw_line_edit) {
    UI_DrawLineEdit *draw_data = (UI_DrawLineEdit *) data;
    FontCache_Font *font = box->font;
    U32 font_size = box->font_size;
    F32 offset_to_cursor = font_cache_text_prefix(box->text, draw_data->cursor).size.width;
    F32 offset_to_mark = font_cache_text_prefix(box->text, draw_data->mark).size.width;
    V2F32 text_position = ui_box_text_location(box);
    F32 cursor_width = f32_max(2.0f, (F32) box->font_size / 4.0f);

    V4F32 selection_color = box->palette.selection;
    V4F32 cursor_color = box->palette.cursor;

    if (draw_data->mark != draw_data->cursor) {
        draw_rectangle(
            r2f32(
                text_position.x + f32_min(offset_to_cursor, offset_to_mark) - 0.5f * cursor_width,
                text_position.y - font->ascent * (F32) font_size / font->units_per_em,
                text_position.x + f32_max(offset_to_cursor, offset_to_mark) + 0.5f * cursor_width,
                text_position.y - font->descent * (F32) font_size / font->units_per_em
            ),
            selection_color,
            0.0f,
            0.0f,
            1.0f
        );
    }
    draw_rectangle(
        r2f32(
            text_position.x + offset_to_cursor - 0.5f * cursor_width,
            text_position.y - font->ascent * (F32) font_size / font->units_per_em + 2.0f,
            text_position.x + offset_to_cursor + 0.5f * cursor_width,
            text_position.y - font->descent * (F32) font_size / font->units_per_em - 2.0f
        ),
        cursor_color,
        0.0f,
        0.0f,
        1.0f
    );
}

// NOTE(simon): Helper function for the line edit to figure out where word
// boundaries are.
// TODO(simon): This function doesn't handle unicode at all, fix it!
internal B32 ui_is_word(U32 codepoint) {
    B32 is_alpha      = ('a' <= codepoint && codepoint <= 'z') || ('A' <= codepoint && codepoint <= 'Z');
    B32 is_numeric    = ('0' <= codepoint && codepoint <= '9');
    B32 is_underscore = codepoint == '_';
    B32 result        = is_alpha || is_numeric || is_underscore;
    return result;
}

internal UI_Input ui_line_edit(U8 *buffer, U64 *buffer_size, U64 buffer_capacity, U64 *cursor, U64 *mark, UI_Key key) {
    prof_function_begin();
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    // NOTE(simon): Handle auto focus.
    B32 is_auto_focus_hot    = ui_is_key_auto_focus_hot(key);
    B32 is_auto_focus_active = ui_is_key_auto_focus_active(key);
    if (is_auto_focus_hot) {
        ui_focus_hot_push(UI_Focus_Active);
    }
    if (is_auto_focus_active) {
        ui_focus_active_push(UI_Focus_Active);
    }

    // NOTE(simon): Acquire focus information.
    B32 is_focus_hot    = ui_is_focus_hot();
    B32 is_focus_active = ui_is_focus_active();
    B32 is_focus_hot_disabled    = !is_focus_hot    && ui_focus_hot_top()    == UI_Focus_Active;
    B32 is_focus_active_disabled = !is_focus_active && ui_focus_active_top() == UI_Focus_Active;

    // NOTE(simon): Build box.
    ui_hover_cursor_next(Gfx_Cursor_Beam);
    UI_Box *text_container_box = ui_create_box_from_key(
        UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
        UI_BoxFlag_OverflowX | UI_BoxFlag_Clip |
        UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable | UI_BoxFlag_ClickToFocus,
        key
    );

    // NOTE(simon): Input handling
    UI_Input input = ui_input_from_box(text_container_box);

    B32 start_edit = false;

    if (is_focus_hot) {
        for (UI_Event *event = 0; ui_next_event(&event);) {
            if (event->kind == UI_EventKind_Text) {
                ui_set_auto_focus_active_key(key);
                start_edit = true;
                break;
            }
        }
    }

    if (!is_focus_active && input.flags & UI_InputFlag_KeyboardPressed) {
        ui_set_auto_focus_active_key(key);
        start_edit = true;
    } else if (is_focus_active && input.flags & UI_InputFlag_KeyboardPressed) {
        ui_set_auto_focus_active_key(global_ui_null_key);
        input.flags |= UI_InputFlag_Commit;
    }

    if (start_edit || is_focus_active) {
        prof_zone_begin(prof_events, "events");
        for (UI_Event *event = 0; ui_next_event(&event);) {
            if (!(event->kind == UI_EventKind_Text || event->kind == UI_EventKind_Edit || event->kind == UI_EventKind_Navigation)) {
                continue;
            }

            if (event->delta.y != 0) {
                continue;
            }

            Str8 edit_string = str8(buffer, *buffer_size);

            U64 new_cursor = *cursor;
            U64 new_mark   = *mark;
            U64 replace_min = 0;
            U64 replace_max = 0;
            Str8 replace = { 0 };
            S64 cursor_delta = 0;
            Str8 copy_string = { 0 };

            // NOTE(simon): Build edit
            switch (event->unit) {
                case UI_EventDeltaUnit_Null: {
                } break;
                case UI_EventDeltaUnit_Character: {
                    if (event->delta.x < 0) {
                        cursor_delta = (S64) str8_next_codepoint_offset(edit_string, *cursor, Side_Min) - (S64) *cursor;
                    } else if (0 < event->delta.x) {
                        cursor_delta = (S64) str8_next_codepoint_offset(edit_string, *cursor, Side_Max) - (S64) *cursor;
                    }
                } break;
                case UI_EventDeltaUnit_Word: {
                    U8 *start = edit_string.data;
                    U8 *opl   = edit_string.data + edit_string.size;
                    U8 *ptr   = edit_string.data + *cursor;
                    if (event->delta.x < 0) {
                        while (start < ptr && !ui_is_word(ptr[-1])) {
                            --ptr;
                        }
                        while (start < ptr && ui_is_word(ptr[-1])) {
                            --ptr;
                        }
                    } else if (0 < event->delta.x) {
                        while (ptr < opl && ui_is_word(*ptr)) {
                            ++ptr;
                        }
                        while (ptr < opl && !ui_is_word(*ptr)) {
                            ++ptr;
                        }
                    }
                    cursor_delta = (S64) (ptr - start) - (S64) *cursor;
                } break;
                case UI_EventDeltaUnit_Line: {
                    if (event->delta.x < 0) {
                        cursor_delta = -(S64) *cursor;
                    } else {
                        cursor_delta = (S64) edit_string.size - (S64) *cursor;
                    }
                } break;
                case UI_EventDeltaUnit_Page: {
                } break;
                case UI_EventDeltaUnit_Whole: {
                    if (event->delta.x < 0) {
                        cursor_delta = -(S64) *cursor;
                    } else {
                        cursor_delta = (S64) edit_string.size - (S64) *cursor;
                    }
                } break;
                case UI_EventDeltaUnit_COUNT: {
                } break;
            }

            if (*cursor != *mark && (event->flags & UI_EventFlag_PickSelectSide)) {
                if (event->delta.x < 0) {
                    new_cursor = u64_min(*cursor, *mark);
                } else if (0 < event->delta.x) {
                    new_cursor = u64_max(*cursor, *mark);
                }
            }

            if ((event->flags & UI_EventFlag_ZeroDeltaOnSelection) && *cursor != *mark) {
                cursor_delta = 0;
            }

            new_cursor = (U64) s64_min(s64_max(0, (S64) new_cursor + cursor_delta), (S64) edit_string.size);

            if (event->flags & UI_EventFlag_Delete) {
                replace_min = u64_min(new_cursor, new_mark);
                replace_max = u64_max(new_cursor, new_mark);
                new_cursor = new_mark = replace_min;
            }

            if (!(event->flags & UI_EventFlag_KeepMark)) {
                new_mark = new_cursor;
            }

            if (event->text.size) {
                replace_min = u64_min(*cursor, *mark);
                replace_max = u64_max(*cursor, *mark);
                replace = event->text;
                new_cursor = new_mark = replace_min + replace.size;
            }

            if ((event->flags & UI_EventFlag_Copy) && *cursor != *mark) {
                U64 min = u64_min(*cursor, *mark);
                U64 max = u64_max(*cursor, *mark);
                copy_string = str8_skip(str8_prefix(edit_string, max), min);
            }



            // NOTE(simon): Apply edit
            *cursor = u64_min(new_cursor, buffer_capacity);
            *mark   = u64_min(new_mark,   buffer_capacity);

            if (copy_string.size) {
                gfx_set_clipboard_text(copy_string);
            }

            // NOTE(simon): Filter out newlines.
            // TODO(simon): Handle \r\n
            Str8List lines = { 0 };
            for (U64 offset = 0; offset < replace.size; ) {
                U64 newline_offset = str8_first_index_of(str8_skip(replace, offset), '\n');
                Str8 line = str8_substring(replace, offset, newline_offset);

                str8_list_push(scratch.arena, &lines, line);

                // NOTE(simon): Add one more to skip the newline itself.
                offset += newline_offset + 1;
            }

            {
                U64 to_remove = replace_max - replace_min;
                // TODO(simon): This should round down to the previous codepoint, at least!
                U64 to_insert = u64_min(lines.total_size, buffer_capacity - (*buffer_size - to_remove));
                U64 to_move = u64_min(*buffer_size - replace_max, buffer_capacity - (replace_min + to_insert));

                memory_move(&buffer[replace_min + to_insert], &buffer[replace_max], to_move);
                U64 offset = 0;
                for (Str8Node *node = lines.first; node && offset < to_insert; node = node->next) {
                    U64 to_insert_line = u64_min(node->string.size, to_insert - offset);
                    memory_copy(&buffer[replace_min + offset], node->string.data, to_insert_line);
                    offset += to_insert_line;
                }
                *buffer_size -= to_remove;
                *buffer_size += to_insert;
            }

            ui_consume_event(event);
        }
        prof_zone_end(prof_events);
    }

    FontCache_Font *font = ui_font_top();
    U32 font_size = ui_font_size_top();

    Str8 edit_string = str8(buffer, *buffer_size);

    ui_parent_push(text_container_box);

    UI_DrawLineEdit *draw_data = arena_push_struct(ui_frame_arena(), UI_DrawLineEdit);
    draw_data->cursor = *cursor;
    draw_data->mark   = *mark;

    ui_width_next(ui_size_text_content(0.0f, 1.0f));
    ui_draw_function_next(ui_draw_line_edit);
    ui_draw_data_next(draw_data);
    UI_Box *text_box = ui_create_box_from_string(UI_BoxFlag_DrawText, str8_literal("###edit_string"));
    ui_box_set_string(text_box, edit_string);

    F32 mouse = ui_mouse().x;
    F32 text_mouse = mouse - ui_box_text_location(text_box).x;
    U64 mouse_position = font_cache_offset_from_text_position(text_box->text, text_mouse);

    if (input.flags & UI_InputFlag_LeftDragging) {
        if (input.flags & UI_InputFlag_LeftPressed) {
            *mark = mouse_position;
        }
        *cursor = mouse_position;
    }

    ui_parent_pop();

    // NOTE(simon): Focus the cursor
    F32 cursor_position = font_cache_text_prefix(text_box->text, *cursor).size.width;
    F32 cursor_position_min = f32_max(0.0f, cursor_position - 2.0f * (F32) font_size);
    F32 cursor_position_max = f32_max(0.0f, cursor_position + 2.0f * (F32) font_size);
    V2F32 box_size = r2f32_size(text_container_box->calculated_rectangle);
    F32 view_min = text_container_box->view_offset.x;
    F32 view_max = text_container_box->view_offset.x + box_size.width;
    F32 min_delta = f32_min(0.0f, cursor_position_min - view_min);
    F32 max_delta = f32_max(0.0f, cursor_position_max - view_max);
    text_container_box->view_offset.x += min_delta;
    text_container_box->view_offset.x += max_delta;

    if (is_auto_focus_hot) {
        ui_focus_hot_pop();
    }
    if (is_auto_focus_active) {
        ui_focus_active_pop();
    }

    arena_end_temporary(scratch);
    prof_function_end();
    return input;
}

UI_BOX_DRAW_FUNCTION(ui_draw_slider) {
    F32 percentage_filled = *(F32 *) data;
    R2F32 rectangle = box->calculated_rectangle;
    rectangle.max.x = rectangle.min.x + percentage_filled * r2f32_size(rectangle).width;
    V4F32 selection_color = box->palette.selection;
    Render_Shape *shape = draw_rectangle(
        rectangle,
        selection_color,
        0.0f, 0.0f, 1.0f
    );
    memory_copy(shape->radies, box->corner_radies, sizeof(shape->radies));
}

internal UI_Input ui_slider(F32 min, F32 *value, F32 max, UI_Key key) {
    // NOTE(simon): Handle auto focus.
    B32 is_auto_focus_hot    = ui_is_key_auto_focus_hot(key);
    B32 is_auto_focus_active = ui_is_key_auto_focus_active(key);
    if (is_auto_focus_hot) {
        ui_focus_hot_push(UI_Focus_Active);
    }
    if (is_auto_focus_active) {
        ui_focus_active_push(UI_Focus_Active);
    }

    // NOTE(simon): Acquire focus information.
    B32 is_focus_hot    = ui_is_focus_hot();
    B32 is_focus_active = ui_is_focus_active();
    B32 is_focus_hot_disabled    = !is_focus_hot    && ui_focus_hot_top()    == UI_Focus_Active;
    B32 is_focus_active_disabled = !is_focus_active && ui_focus_active_top() == UI_Focus_Active;

    F32 *percentage_filled = arena_push_struct(ui_frame_arena(), F32);

    // NOTE(simon): Build box.
    ui_draw_data_next(percentage_filled);
    ui_draw_function_next(ui_draw_slider);
    ui_text_align_next(UI_TextAlign_Center);
    ui_hover_cursor_next(Gfx_Cursor_Hand);
    UI_Box *box = ui_create_box_from_key(
        UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawText |
        UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
        UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable,
        key
    );
    ui_box_set_string(box, str8_format(ui_frame_arena(), "%.2f", *value));

    UI_Input input = ui_input_from_box(box);

    // NOTE(simon): Take and release focus from the keyboard.
    if (!is_focus_active && input.flags & UI_InputFlag_KeyboardPressed) {
        ui_set_auto_focus_active_key(key);
    } else if (is_focus_active && input.flags & UI_InputFlag_KeyboardPressed) {
        ui_set_auto_focus_active_key(global_ui_null_key);
        input.flags |= UI_InputFlag_Commit;
    }

    // NOTE(simon): Changing the value from with the keyboard.
    if (is_focus_active) {
        F32 percentage = (*value - min) / (max - min);

        for (UI_Event *event = 0; ui_next_event(&event);) {
            if (event->kind != UI_EventKind_Navigation) {
                continue;
            }

            if (event->delta.y != 0) {
                continue;
            }

            switch (event->unit) {
                case UI_EventDeltaUnit_Null: {
                } break;
                case UI_EventDeltaUnit_Character: {
                    percentage += (F32) event->delta.x * 0.01f;
                } break;
                case UI_EventDeltaUnit_Word: {
                    percentage += (F32) event->delta.x * 0.1f;
                } break;
                case UI_EventDeltaUnit_Line: {
                    percentage = event->delta.x < 0 ? 0.0f : 1.0f;
                } break;
                case UI_EventDeltaUnit_Page: {
                } break;
                case UI_EventDeltaUnit_Whole: {
                    percentage = event->delta.x < 0 ? 0.0f : 1.0f;
                } break;
                case UI_EventDeltaUnit_COUNT: {
                } break;
            }

            ui_consume_event(event);
        }

        F32 value_post_change         = min + percentage * (max - min);
        F32 clamped_value_post_change = f32_clamp(value_post_change, min, max);
        *value = clamped_value_post_change;
    }

    // NOTE(simon): Changing the value by dragging with the mouse.
    if (input.flags & UI_InputFlag_LeftDragging) {
        if (input.flags & UI_InputFlag_LeftPressed) {
            F32 drag_data = *value;
            ui_set_drag_data(&drag_data);
        }

        F32 value_pre_drag          = *ui_get_drag_data(F32);
        F32 percentage_pre_drag     = (value_pre_drag - min) / (max - min);
        F32 pixels_pre_drag         = percentage_pre_drag * r2f32_size(box->calculated_rectangle).width;
        F32 drag_delta              = ui_drag_delta().x;
        F32 pixels_post_drag        = pixels_pre_drag + drag_delta;
        F32 percentage_post_drag    = pixels_post_drag / r2f32_size(box->calculated_rectangle).width;
        F32 value_post_drag         = min + (percentage_post_drag) * (max - min);
        F32 clamped_value_post_drag = f32_min(f32_max(min, value_post_drag), max);
        *value = clamped_value_post_drag;
    }

    // NOTE(simon): Fill draw data.
    *percentage_filled = (*value - min) / (max - min);

    // NOTE(simon): Clear auto focus.
    if (is_auto_focus_hot) {
        ui_focus_hot_pop();
    }
    if (is_auto_focus_active) {
        ui_focus_active_pop();
    }

    return input;
}



// NOTE(simon): Scrolling
internal UI_ScrollPosition ui_scroll_bar(UI_ScrollPosition position, S64 first_row, S64 last_row, S64 visible_rows) {
    UI_Box *scroll = &global_ui_null_box;

    S64 rows_above = position.index - first_row;
    S64 row_count  = s64_max(0, last_row - first_row - 1) + visible_rows;
    S64 rows_below = s64_max(0, last_row - position.index - 1);

    UI_Input up_input     = { 0 };
    UI_Input before_input = { 0 };
    UI_Input scroll_input = { 0 };
    UI_Input after_input  = { 0 };
    UI_Input down_input   = { 0 };

    // NOTE(simon): Build
    ui_extra_box_flags_next(UI_BoxFlag_DrawBorder);
    ui_column_string(str8_literal("##scroll"))
    ui_hover_cursor(Gfx_Cursor_Hand)
    ui_text_align(UI_TextAlign_Center)
    ui_font(ui_icon_font()) {
        ui_height_next(ui_size_aspect_ratio(1.0f, 1.0f));
        up_input = ui_button(ui_icon_string_from_kind(UI_IconKind_UpArrow));

        ui_height_next(ui_size_fill());
        ui_column_string(str8_literal("##container")) {
            ui_height_next(ui_size_parent_percent(((F32) rows_above + position.offset) / (F32) row_count, 0.0f));
            UI_Box *scroll_before = ui_create_box_from_string(UI_BoxFlag_Clickable, str8_literal("##before"));
            before_input = ui_input_from_box(scroll_before);

            ui_height_next(ui_size_parent_percent(f32_max(0.01f, (F32) visible_rows / (F32) row_count), 0.0f));
            scroll = ui_create_box_from_string(
                UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
                UI_BoxFlag_Clickable,
                str8_literal("##scrollbar")
            );
            scroll_input = ui_input_from_box(scroll);

            ui_height_next(ui_size_parent_percent(((F32) rows_below - position.offset) / (F32) row_count, 0.0f));
            UI_Box *scroll_after = ui_create_box_from_string(UI_BoxFlag_Clickable, str8_literal("##after"));
            after_input = ui_input_from_box(scroll_after);
        }

        ui_height_next(ui_size_aspect_ratio(1.0f, 1.0f));
        down_input = ui_button(ui_icon_string_from_kind(UI_IconKind_DownArrow));
    }

    // NOTE(simon): Input
    UI_ScrollPosition result = position;

    if (up_input.flags & UI_InputFlag_LeftClicked) {
        result.index  -= visible_rows;
        result.offset += (F32) visible_rows;
    }

    if (before_input.flags & UI_InputFlag_LeftDragging) {
        result.index  -= 1;
        result.offset += 1;
    }

    if (scroll_input.flags & UI_InputFlag_LeftDragging) {
        if (scroll_input.flags & UI_InputFlag_LeftPressed) {
            ui_set_drag_data(&position.index);
        }

        S64 start_row = *ui_get_drag_data(S64);

        F32 scroll_size  = scroll->parent->calculated_size.height - scroll->calculated_size.height;
        F32 drag_percent = ui_drag_delta().y / scroll_size;
        S64 previous_index = result.index;
        result.index   = start_row + (S64) f32_round(drag_percent * (F32) (row_count - visible_rows));
        result.index   = s64_clamp(result.index, first_row, last_row - 1);
        result.offset += (F32) (previous_index - result.index);
    }

    if (after_input.flags & UI_InputFlag_LeftDragging) {
        result.index  += 1;
        result.offset -= 1;
    }

    if (down_input.flags & UI_InputFlag_LeftClicked) {
        result.index  += visible_rows;
        result.offset -= (F32) visible_rows;
    }

    // NOTE(simon): Clamp scrolling.
    if (result.index < 0) {
        result.offset += (F32) result.index;
        result.index = 0;
    } else if (last_row <= result.index) {
        result.offset -= (F32) (s64_max(0, last_row - 1) - result.index);
        result.index = s64_max(0, last_row - 1);
    }

    return result;
}

internal Void ui_scroll_region_begin(V2F32 size, F32 row_height, S64 item_count, S64 *cursor, R1S64 *visible_range_out, UI_ScrollPosition *scroll_position) {
    F32 scrollbar_width = (F32) ui_font_size_top();
    F32 container_width = size.width - scrollbar_width;

    // NOTE(simon): Properties of the data begin viewed.
    S64 visible_rows = (S64) f32_ceil(size.height / row_height);

    // NOTE(simon): Properties of the current view.
    S64 top_row    = scroll_position->index + (S64) f32_floor(scroll_position->offset);
    S64 bottom_row = s64_min(top_row + (scroll_position->offset != 0.0f) + visible_rows, item_count);

    // NOTE(simon): Fill out paramters.
    visible_range_out->min = top_row;
    visible_range_out->max = bottom_row;

    ui_width_next(ui_size_pixels(size.width, 1.0f));
    ui_height_next(ui_size_pixels(size.height, 1.0f));
    ui_layout_axis_next(Axis2_X);
    UI_Box *region = ui_create_box(0);

    ui_parent_next(region);
    ui_width_next(ui_size_pixels(container_width, 1.0f));
    ui_height_next(ui_size_pixels(size.height, 1.0f));
    ui_layout_axis_next(Axis2_Y);
    UI_Box *container = ui_create_box_from_string(UI_BoxFlag_OverflowY | UI_BoxFlag_Clip | UI_BoxFlag_Scrollable, str8_literal("##container"));
    container->view_offset.y = row_height * (f32_mod(scroll_position->offset, 1.0f) + (scroll_position->offset < 0.0f));

    ui_parent(region)
    ui_focus(UI_Focus_None) {
        ui_width(ui_size_pixels(scrollbar_width, 1.0f))
        ui_height(ui_size_pixels(size.height, 1.0f)) {
            *scroll_position = ui_scroll_bar(*scroll_position, 0, item_count, visible_rows);
        }
    }

    ui_parent_push(region);
    ui_parent_push(container);
    ui_height_push(ui_size_pixels(row_height, 1.0f));
}

internal Void ui_scroll_region_end(V2F32 size, F32 row_height, S64 item_count, S64 *cursor, UI_ScrollPosition *scroll_position) {
    // NOTE(simon): Pop UI stacks.
    ui_height_pop();
    UI_Box *container = ui_parent_pop();
    UI_Box *region    = ui_parent_pop();

    // NOTE(simon): Compuate focus.
    B32 is_focus_hot    = ui_is_focus_hot();
    B32 is_focus_active = ui_is_focus_active();

    // NOTE(simon): Properties of the data begin viewed.
    S64 visible_rows = (S64) f32_ceil(size.height / row_height);

    // NOTE(simon): Properties of the current view.
    S64 top_row    = scroll_position->index + (S64) f32_floor(scroll_position->offset);
    S64 bottom_row = s64_min(top_row + (scroll_position->offset != 0.0f) + visible_rows, item_count);

    B32 snap_to_cursor = false;

    for (UI_Event *event = 0; is_focus_active && ui_next_event(&event);) {
        if (event->kind != UI_EventKind_Navigation) {
            continue;
        }

        S64 delta = 0;
        switch (event->unit) {
            case UI_EventDeltaUnit_Null: {
            } break;
            case UI_EventDeltaUnit_Character: {
                delta += event->delta.y;
                snap_to_cursor = true;
            } break;
            case UI_EventDeltaUnit_Word: {
            } break;
            case UI_EventDeltaUnit_Line: {
            } break;
            case UI_EventDeltaUnit_Page: {
                delta += event->delta.y * visible_rows;
                snap_to_cursor = true;
            } break;
            case UI_EventDeltaUnit_Whole: {
            } break;
            case UI_EventDeltaUnit_COUNT: {
            } break;
        }

        *cursor = s64_clamp(*cursor + delta, 0, item_count - 1);

        ui_consume_event(event);
    }

    // NOTE(simon): Scrolling
    UI_Input region_input = ui_input_from_box(container);
    S64 scroll_delta = (S64) f32_round(region_input.scroll.y);
    scroll_position->index  -= scroll_delta;
    scroll_position->offset += (F32) scroll_delta;

    // NOTE(simon): Snap to cursor.
    if (snap_to_cursor) {
        *cursor = s64_clamp(*cursor, 0, item_count - 1);

        if (!(top_row <= *cursor && *cursor < top_row + visible_rows)) {
            S64 target_row = *cursor - visible_rows / 2;
            S64 delta = target_row - scroll_position->index;
            scroll_position->index  += delta;
            scroll_position->offset -= (F32) delta;
        }
    }

    // NOTE(simon): Clamp scrolling.
    if (scroll_position->index < 0) {
        scroll_position->offset += (F32) scroll_position->index;
        scroll_position->index = 0;
    } else if (item_count <= scroll_position->index) {
        scroll_position->offset -= (F32) (s64_max(0, item_count - 1) - scroll_position->index);
        scroll_position->index = s64_max(0, item_count - 1);
    }

    // NOTE(simon): Animation
    scroll_position->offset += -scroll_position->offset * ui_animation_slow_rate();
    if (f32_abs(scroll_position->offset) < 0.001f) {
        scroll_position->offset = 0.0f;
    } else {
        // TODO(simon): Mark the UI as animating.
    }
}



// NOTE(simon): Color picking
UI_BOX_DRAW_FUNCTION(ui_draw_saturation_value) {
    V4F32 *color = (V4F32 *) data;
    V4F32 hsva = hsva_from_srgba(srgba_from_color(*color));
    V4F32 full_color = color_from_srgba(srgba_from_hsva(v4f32(hsva.x, 1.0f, 1.0f, 1.0f)));
    Render_Shape *shape = draw_rectangle(
        box->calculated_rectangle,
        v4f32(0.0f, 0.0f, 0.0f, 1.0f),
        0.0f, 0.0f, 0.0f
    );
    shape->colors[Corner_00] = v4f32(1.0f, 1.0f, 1.0f, 1.0f);
    shape->colors[Corner_01] = full_color;
    memory_copy(shape->radies, box->corner_radies, sizeof(shape->radies));

    F32 min_x = box->calculated_rectangle.min.x;
    F32 max_y = box->calculated_rectangle.max.y;
    F32 saturation_pixels = hsva.y * r2f32_size(box->calculated_rectangle).width;
    F32 value_pixels = hsva.z * r2f32_size(box->calculated_rectangle).height;
    draw_rectangle(
        r2f32(
            min_x + saturation_pixels - 3.0f, max_y - value_pixels - 3.0f,
            min_x + saturation_pixels + 3.0f, max_y - value_pixels + 3.0f
        ),
        box->palette.cursor,
        0.0f, 1.0f, 0.0f
    );
}

UI_BOX_DRAW_FUNCTION(ui_draw_hue) {
    V4F32 *color = (V4F32 *) data;
    V4F32 hsva = hsva_from_srgba(srgba_from_color(*color));

    F32 min_x = box->calculated_rectangle.min.x;
    F32 max_x = box->calculated_rectangle.max.x;
    F32 min_y = box->calculated_rectangle.min.y;
    F32 segment_height = r2f32_size(box->calculated_rectangle).height / 6.0f;

    V4F32 colors[] = {
        v4f32(1.0f, 0.0f, 0.0f, 1.0f),
        v4f32(1.0f, 1.0f, 0.0f, 1.0f),
        v4f32(0.0f, 1.0f, 0.0f, 1.0f),
        v4f32(0.0f, 1.0f, 1.0f, 1.0f),
        v4f32(0.0f, 0.0f, 1.0f, 1.0f),
        v4f32(1.0f, 0.0f, 1.0f, 1.0f),
        v4f32(1.0f, 0.0f, 0.0f, 1.0f),
    };

    for (U64 i = 0; i < 6; ++i) {
        Render_Shape *shape = draw_rectangle(
            r2f32(
                min_x, min_y + (F32) (i + 0) * segment_height,
                max_x, min_y + (F32) (i + 1) * segment_height
            ),
            v4f32(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f, 0.0f, 0.0f
        );
        shape->colors[Corner_00] = shape->colors[Corner_01] = colors[i + 0];
        shape->colors[Corner_10] = shape->colors[Corner_11] = colors[i + 1];

        if (i == 0) {
            shape->radies[Corner_00] = box->corner_radies[Corner_00];
            shape->radies[Corner_01] = box->corner_radies[Corner_01];
        } else if (i == 5) {
            shape->radies[Corner_10] = box->corner_radies[Corner_10];
            shape->radies[Corner_11] = box->corner_radies[Corner_11];
        }
    }

    F32 hue_percentage = hsva.x / 360.0f;
    F32 hue_pixels = hue_percentage * r2f32_size(box->calculated_rectangle).height;
    draw_rectangle(
        r2f32(
            min_x + 1.0f, min_y + hue_pixels - 3.0f,
            max_x - 1.0f, min_y + hue_pixels + 3.0f
        ),
        box->palette.cursor,
        0.0f, 1.0f, 0.0f
    );
}

UI_BOX_DRAW_FUNCTION(ui_draw_alpha) {
    V4F32 *color = (V4F32 *) data;
    Render_Shape *shape = draw_rectangle(
        box->calculated_rectangle,
        v4f32(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f, 0.0f, 0.0f
    );
    shape->colors[Corner_00] = shape->colors[Corner_01] = v4f32(color->r, color->g, color->b, 0.0f);
    shape->colors[Corner_10] = shape->colors[Corner_11] = v4f32(color->r, color->g, color->b, 1.0f);
    memory_copy(shape->radies, box->corner_radies, sizeof(shape->radies));

    F32 min_x = box->calculated_rectangle.min.x;
    F32 max_x = box->calculated_rectangle.max.x;
    F32 min_y = box->calculated_rectangle.min.y;
    F32 alpha_pixels = color->a * r2f32_size(box->calculated_rectangle).height;
    draw_rectangle(
        r2f32(
            min_x + 1.0f, min_y + alpha_pixels - 3.0f,
            max_x - 1.0f, min_y + alpha_pixels + 3.0f
        ),
        box->palette.cursor,
        0.0f, 1.0f, 0.0f
    );
    draw_rectangle(
        r2f32(
            min_x + 1.0f, min_y + alpha_pixels - 3.0f,
            max_x - 1.0f, min_y + alpha_pixels + 3.0f
        ),
        box->palette.cursor,
        0.0f, 1.0f, 0.0f
    );
}

internal UI_Input ui_saturation_value_picker(V4F32 *color) {
    V4F32 *color_data = arena_push_struct(ui_frame_arena(), V4F32);
    *color_data = *color;
    ui_draw_data_next(color_data);
    ui_draw_function_next(ui_draw_saturation_value);
    ui_draw_data_next(color_data);
    ui_hover_cursor_next(Gfx_Cursor_Hand);
    UI_Box *box = ui_create_box_from_string(UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable, str8_literal("##saturation_value_picker"));

    UI_Input input = ui_input_from_box(box);
    if (input.flags & UI_InputFlag_LeftDragging) {
        if (input.flags & UI_InputFlag_LeftPressed) {
            V4F32 hsva = hsva_from_srgba(srgba_from_color(*color));
            ui_set_drag_data(&hsva);
        }

        V2F32 size                       = r2f32_size(box->calculated_rectangle);
        V4F32 hsva_pre_drag              = *ui_get_drag_data(V4F32);
        F32 saturation_pixels_pre_drag   = hsva_pre_drag.y * size.width;
        F32 value_pixels_pre_drag        = (1.0f - hsva_pre_drag.z) * size.height;
        V2F32 drag_delta                 = ui_drag_delta();
        F32 saturation_pixels_post_drag  = saturation_pixels_pre_drag + drag_delta.x;
        F32 value_pixels_post_drag       = value_pixels_pre_drag + drag_delta.y;
        F32 saturation_post_drag         = saturation_pixels_post_drag / size.width;
        F32 value_post_drag              = 1.0f - value_pixels_post_drag / size.height;
        F32 clamped_saturation_post_drag = f32_min(f32_max(0.0f, saturation_post_drag), 1.0f);
        F32 clamped_value_post_drag      = f32_min(f32_max(0.0f, value_post_drag), 1.0f);
        V4F32 hsva_post_drag             = v4f32(hsva_pre_drag.x, clamped_saturation_post_drag, clamped_value_post_drag, hsva_pre_drag.a);
        V4F32 srgba_post_drag            = srgba_from_hsva(hsva_post_drag);
        V4F32 color_post_drag            = color_from_srgba(srgba_post_drag);

        *color = color_post_drag;
    }

    return input;
}

internal UI_Input ui_hue_picker(V4F32 *color) {
    V4F32 *color_data = arena_push_struct(ui_frame_arena(), V4F32);
    *color_data = *color;
    ui_draw_data_next(color_data);
    ui_draw_function_next(ui_draw_hue);
    ui_hover_cursor_next(Gfx_Cursor_Hand);
    UI_Box *box = ui_create_box_from_string(UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable, str8_literal("##hue_picker"));

    UI_Input input = ui_input_from_box(box);
    if (input.flags & UI_InputFlag_LeftDragging) {
        if (input.flags & UI_InputFlag_LeftPressed) {
            V4F32 hsva = hsva_from_srgba(srgba_from_color(*color));
            ui_set_drag_data(&hsva);
        }

        V2F32 size                       = r2f32_size(box->calculated_rectangle);
        V4F32 hsva_pre_drag              = *ui_get_drag_data(V4F32);
        F32 hue_percentage_pre_drag      = hsva_pre_drag.x / 360.0f;
        F32 hue_pixels_pre_drag          = hue_percentage_pre_drag * size.height;
        F32 drag_delta                   = ui_drag_delta().y;
        F32 hue_pixels_post_drag         = hue_pixels_pre_drag + drag_delta;
        F32 hue_percentage_post_drag     = hue_pixels_post_drag / size.height;
        F32 hue_post_drag                = hue_percentage_post_drag * 360.0f;
        F32 clamped_hue_post_drag        = f32_min(f32_max(0.0f, hue_post_drag), 360.0f);
        V4F32 hsva_post_drag             = v4f32(clamped_hue_post_drag, hsva_pre_drag.y, hsva_pre_drag.z, hsva_pre_drag.a);
        V4F32 srgba_post_drag            = srgba_from_hsva(hsva_post_drag);
        V4F32 color_post_drag            = color_from_srgba(srgba_post_drag);

        *color = color_post_drag;
    }

    return input;
}

internal UI_Input ui_alpha_picker(V4F32 *color) {
    V4F32 *color_data = arena_push_struct(ui_frame_arena(), V4F32);
    *color_data = *color;
    ui_draw_data_next(color_data);
    ui_draw_function_next(ui_draw_alpha);
    ui_hover_cursor_next(Gfx_Cursor_Hand);
    UI_Box *box = ui_create_box_from_string(UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable, str8_literal("##alpha_picker"));

    UI_Input input = ui_input_from_box(box);
    if (input.flags & UI_InputFlag_LeftDragging) {
        if (input.flags & UI_InputFlag_LeftPressed) {
            ui_set_drag_data(color);
        }

        V2F32 size                  = r2f32_size(box->calculated_rectangle);
        V4F32 color_pre_drag        = *ui_get_drag_data(V4F32);
        F32 alpha_pixels_pre_drag   = color_pre_drag.a * size.height;
        F32 drag_delta              = ui_drag_delta().y;
        F32 alpha_pixels_post_drag  = alpha_pixels_pre_drag + drag_delta;
        F32 alpha_post_drag         = alpha_pixels_post_drag / size.height;
        F32 clamped_alpha_post_drag = f32_min(f32_max(0.0f, alpha_post_drag), 1.0f);
        V4F32 color_post_drag       = v4f32(color_pre_drag.r, color_pre_drag.g, color_pre_drag.b, clamped_alpha_post_drag);

        *color = color_post_drag;
    }

    return input;
}

internal B32 ui_color_picker(V4F32 *color, UI_Size size, UI_Size bar_width, UI_Size spacing) {
    B32 changed = false;

    ui_row()
    ui_height(size) {
        ui_width_next(size);
        UI_Input saturation_value_input = ui_saturation_value_picker(color);

        ui_spacer_sized(spacing);

        ui_width_next(bar_width);
        UI_Input hue_input = ui_hue_picker(color);

        ui_spacer_sized(spacing);

        ui_width_next(bar_width);
        UI_Input alpha_input = ui_alpha_picker(color);

        changed |= saturation_value_input.flags & UI_InputFlag_LeftDragging;
        changed |= hue_input.flags & UI_InputFlag_LeftDragging;
        changed |= alpha_input.flags & UI_InputFlag_LeftDragging;
    }

    return changed;
}
