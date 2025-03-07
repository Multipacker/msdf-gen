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
    UI_Box *check = ui_create_box_from_string_format(
        UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | (is_checked ? UI_BoxFlag_DrawText : 0) |
        UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
        UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable,
        "X###check_%.*s", str8_expand(label)
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
    if (input.input_flags & UI_InputFlag_Clicked) {
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
    F32 cursor_width = f32_max(2.0f, (F32) box->font_size / 5.0f);

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
            0.0f
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
        0.0f
    );
}

// NOTE(simon): Helper function for the line edit to figure out where word
// boundaries are.
// TODO(simon): This function doesn't handle unicode at all, fix it!
internal B32 ui_is_word(U32 codepoint) {
    B32 is_alpha   = ('a' <= codepoint && codepoint <= 'z') || ('A' <= codepoint && codepoint <= 'Z');
    B32 is_numeric = ('0' <= codepoint && codepoint <= '9');
    B32 result     = is_alpha || is_numeric;
    return result;
}

internal UI_Input ui_line_edit(U8 *buffer, U64 *buffer_size, U64 buffer_capacity, U64 *cursor, U64 *mark, UI_Key key) {
    prof_function_begin();
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    ui_hover_cursor_next(Gfx_Cursor_Beam);
    UI_Box *text_container_box = ui_create_box_from_key(
        UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
        UI_BoxFlag_OverflowX | UI_BoxFlag_Clip |
        UI_BoxFlag_Clickable,
        key
    );

    // NOTE(simon): Input handling
    if (ui_is_focus_active()) {
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
                    *cursor = u64_min(*cursor, *mark);
                } else if (0 < event->delta.x) {
                    *cursor = u64_max(*cursor, *mark);
                }
            }

            if ((event->flags & UI_EventFlag_ZeroDeltaOnSelection) && *cursor != *mark) {
                cursor_delta = 0;
            }

            new_cursor = (U64) s64_min(s64_max(0, (S64) *cursor + cursor_delta), (S64) edit_string.size);

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

            {
                U64 to_remove = replace_max - replace_min;
                // TODO(simon): This should round down to the previous codepoint, at least!
                U64 to_insert = u64_min(replace.size, buffer_capacity - (*buffer_size - to_remove));
                U64 to_move = u64_min(*buffer_size - replace_max, buffer_capacity - (replace_min + to_insert));

                memory_move(&buffer[replace_min + to_insert], &buffer[replace_max], to_move);
                memory_copy(&buffer[replace_min], replace.data, to_insert);
                *buffer_size -= to_remove;
                *buffer_size += to_insert;
            }

            ui_consume_event(event);
        }
        prof_zone_end(prof_events);
    }

    FontCache_Font *font = ui_font_top();
    U32 font_size = ui_font_size_top();

    U64 mouse_position = 0;
    Str8 edit_string = str8(buffer, *buffer_size);

    ui_parent_push(text_container_box);

    UI_DrawLineEdit *draw_data = arena_push_struct_zero(ui_frame_arena(), UI_DrawLineEdit);
    draw_data->cursor = *cursor;
    draw_data->mark   = *mark;

    ui_width_next(ui_size_text_content(0.0f, 1.0f));
    ui_draw_function_next(ui_draw_line_edit);
    ui_draw_data_next(draw_data);
    UI_Box *text_box = ui_create_box_from_string(UI_BoxFlag_DrawText, str8_literal("###edit_string"));
    ui_box_set_string(text_box, edit_string);

    F32 mouse = ui_mouse().x;
    F32 text_mouse = mouse - ui_box_text_location(text_box).x;
    mouse_position = font_cache_offset_from_text_position(text_box->text, text_mouse);

    ui_parent_pop();

    UI_Input input = ui_input_from_box(text_container_box);
    if (input.input_flags & UI_InputFlag_LeftDragging) {
        if (input.input_flags & UI_InputFlag_LeftPressed) {
            *mark = mouse_position;
        }
        *cursor = mouse_position;
    }

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
        0, 0, 0
    );
    memory_copy(shape->radies, box->corner_radies, sizeof(shape->radies));
}

internal UI_Input ui_slider(F32 min, F32 *value, F32 max, UI_Key key) {
    F32 *percentage_filled = arena_push_struct_zero(ui_frame_arena(), F32);

    ui_draw_data_next(percentage_filled);
    ui_draw_function_next(ui_draw_slider);
    ui_text_align_next(UI_TextAlign_Center);
    ui_hover_cursor_next(Gfx_Cursor_Hand);
    UI_Box *box = ui_create_box_from_key(
        UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawText |
        UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
        UI_BoxFlag_Clickable,
        key
    );
    ui_box_set_string(box, str8_format(ui_frame_arena(), "%.2f", *value));

    UI_Input input = ui_input_from_box(box);
    if (input.input_flags & UI_InputFlag_LeftDragging) {
        if (input.input_flags & UI_InputFlag_LeftPressed) {
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

    *percentage_filled = (*value - min) / (max - min);

    return input;
}

internal UI_ScrollPosition ui_scroll_bar(UI_ScrollPosition position, S64 first_row, S64 last_row, S64 visible_rows) {
    UI_Box *scroll = &global_ui_null_box;

    S64 rows_above = position.index - first_row;
    S64 row_count  = s64_max(0, last_row - first_row - 1) + visible_rows;
    S64 rows_below = last_row - 1 - position.index;

    UI_Input before_input = { 0 };
    UI_Input scroll_input = { 0 };
    UI_Input after_input  = { 0 };

    // NOTE(simon): Build
    ui_layout_axis_next(Axis2_Y);
    UI_Box *scroll_container = ui_create_box_from_string(0, str8_literal("##scroll_container"));

    ui_parent(scroll_container)
    ui_hover_cursor(Gfx_Cursor_Hand) {
        ui_height_next(ui_size_parent_percent(((F32) rows_above + position.offset) / (F32) row_count, 0.0f));
        UI_Box *scroll_before = ui_create_box_from_string(UI_BoxFlag_Clickable, str8_literal("##before"));
        before_input = ui_input_from_box(scroll_before);

        ui_height_next(ui_size_parent_percent(f32_max(0.01f, (F32) visible_rows / (F32) row_count), 1.0f));
        ui_corner_radius_next(ui_parent_top()->calculated_size.width / 2.0f);
        scroll = ui_create_box_from_string(
            UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
            UI_BoxFlag_Clickable,
            str8_literal("##scrollbar")
        );
        scroll_input = ui_input_from_box(scroll);

        ui_height_next(ui_size_parent_percent(((F32) rows_below - position.offset) / (F32) row_count, 0.0f));
        UI_Box *scroll_after = ui_create_box_from_string(UI_BoxFlag_Clickable, str8_literal("##after"));
        after_input = ui_input_from_box(scroll_after);
    }

    // NOTE(simon): Input
    UI_ScrollPosition result = position;

    if (before_input.input_flags & UI_InputFlag_LeftClicked) {
        result.index  -= visible_rows;
        result.offset += (F32) visible_rows;
    }

    if (scroll_input.input_flags & UI_InputFlag_LeftDragging) {
        if (scroll_input.input_flags & UI_InputFlag_LeftPressed) {
            ui_set_drag_data(&position.index);
        }

        S64 start_row = *ui_get_drag_data(S64);

        F32 scroll_size  = scroll_container->calculated_size.height - scroll->calculated_size.height;
        F32 drag_percent = ui_drag_delta().y / scroll_size;
        result.index  = start_row + (S64) f32_round(drag_percent * (F32) (row_count - visible_rows));
        result.index  = s64_min(s64_max(first_row, result.index), last_row - 1);
        result.offset = 0.0f;
    }

    if (after_input.input_flags & UI_InputFlag_LeftClicked) {
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
