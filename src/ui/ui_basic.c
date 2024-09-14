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



internal Void ui_label(Str8 string) {
    ui_create_box_from_string(UI_BoxFlag_DrawText, string);
}

internal Void ui_label_format(CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    ui_label(string);

    arena_end_temporary(scratch);
}



internal UI_Input ui_button(Str8 string) {
    ui_hover_cursor_next(Gfx_Cursor_Hand);
    UI_Box *box = ui_create_box_from_string(
        UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText | UI_BoxFlag_DrawBorder |
        UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
        UI_BoxFlag_Clickable,
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

// TODO(simon): Update to use the font height for the checkbox.
internal UI_Input ui_checkbox(B32 is_checked, Str8 label) {
    ui_width_next(ui_size_children_sum(1.0f));
    ui_height_next(ui_size_children_sum(1.0f));
    ui_row_begin();

    ui_width_next(ui_size_pixels(20.0f, 1.0f));
    ui_height_next(ui_size_pixels(20.0f, 1.0f));
    UI_Box *check = ui_create_box_from_string_format(
        UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | (is_checked ? UI_BoxFlag_DrawText : 0) |
        UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
        UI_BoxFlag_Clickable,
        "X###%.*s", str8_expand(label)
    );

    ui_spacer_sized(ui_size_pixels(10.0f, 1.0f));

    ui_label(label);
    ui_row_end();

    UI_Input check_input = ui_input_from_box(check);
    return check_input;
}

internal Void ui_checkbox_b32(B32 *is_checked, Str8 label) {
    UI_Input input = ui_checkbox(*is_checked, label);
    if (input.input_flags & UI_InputFlag_LeftClicked) {
        *is_checked = !(*is_checked);
    }
}
