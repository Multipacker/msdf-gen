internal UI_Size ui_size_fill(Void) {
    UI_Size result = ui_size_parent_percent(1.0f, 0.0f);
    return result;
}

internal UI_Box *ui_spacer(UI_Context *ui) {
    UI_Box *spacer = ui_create_box(ui, 0);
    return spacer;
}

internal UI_Box *ui_spacer_sized(UI_Context *ui, UI_Size size) {
    Axis2 axis = ui_parent_top(ui)->layout_axis;
    ui_size_next(ui, size, axis);
    UI_Box *spacer = ui_spacer(ui);
    return spacer;
}

internal UI_Box *ui_row_begin(UI_Context *ui) {
    ui_layout_axis_next(ui, Axis2_X);
    UI_Box *row = ui_create_box(ui, 0);
    ui_parent_push(ui, row);
    return row;
}

internal UI_Box *ui_row_string_begin(UI_Context *ui, Str8 string) {
    ui_layout_axis_next(ui, Axis2_X);
    UI_Box *row = ui_create_box_from_string(ui, 0, string);
    ui_parent_push(ui, row);
    return row;
}

internal UI_Box *ui_row_end(UI_Context *ui) {
    UI_Box *result = ui_parent_pop(ui);
    return result;
}

internal UI_Box *ui_column_begin(UI_Context *ui) {
    ui_layout_axis_next(ui, Axis2_Y);
    UI_Box *column = ui_create_box(ui, 0);
    ui_parent_push(ui, column);
    return column;
}

internal UI_Box *ui_column_string_begin(UI_Context *ui, Str8 string) {
    ui_layout_axis_next(ui, Axis2_Y);
    UI_Box *column = ui_create_box_from_string(ui, 0, string);
    ui_parent_push(ui, column);
    return column;
}

internal UI_Box *ui_column_end(UI_Context *ui) {
    UI_Box *result = ui_parent_pop(ui);
    return result;
}



internal Void ui_label(UI_Context *ui, Str8 string) {
    ui_create_box_from_string(ui, UI_BoxFlags_DrawText, string);
}

internal Void ui_label_format(UI_Context *ui, CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    ui_label(ui, string);

    arena_end_temporary(scratch);
}



internal UI_Input ui_button(UI_Context *ui, Str8 string) {
    UI_Box *box = ui_create_box_from_string(
        ui,
        UI_BoxFlags_DrawBackground | UI_BoxFlags_DrawText | UI_BoxFlags_DrawBorder |
        UI_BoxFlags_DrawHot | UI_BoxFlags_DrawActive |
        UI_BoxFlags_Clickable,
        string
    );
    UI_Input result = ui_input_from_box(ui, box);
    return result;
}

internal UI_Input ui_button_format(UI_Context *ui, CStr format, ...) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    va_list arguments;
    va_start(arguments, format);
    Str8 string = str8_format_list(scratch.arena, format, arguments);
    va_end(arguments);

    UI_Input result = ui_button(ui, string);

    arena_end_temporary(scratch);
    return result;
}
