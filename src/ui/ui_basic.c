internal UI_Size ui_size_fill(Void) {
    UI_Size result = ui_size_parent_percent(1.0f, 0.0f);
    return result;
}

internal UI_Box *ui_spacer(UI_Context *ui) {
    UI_Box *spacer = ui_box_create(ui, 0);
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
    UI_Box *row = ui_box_create(ui, 0);
    ui_parent_push(ui, row);
    return row;
}

internal UI_Box *ui_row_end(UI_Context *ui) {
    UI_Box *result = ui_parent_pop(ui);
    return result;
}

internal UI_Box *ui_column_begin(UI_Context *ui) {
    ui_layout_axis_next(ui, Axis2_Y);
    UI_Box *column = ui_box_create(ui, 0);
    ui_parent_push(ui, column);
    return column;
}

internal UI_Box *ui_column_end(UI_Context *ui) {
    UI_Box *result = ui_parent_pop(ui);
    return result;
}
