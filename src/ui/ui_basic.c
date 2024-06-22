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

internal UI_Box *ui_row(UI_Context *ui) {
    ui_layout_axis_next(ui, Axis2_X);
    UI_Box *row = ui_box_create(ui, 0);
    return row;
}

internal UI_Box *ui_column(UI_Context *ui) {
    ui_layout_axis_next(ui, Axis2_Y);
    UI_Box *column = ui_box_create(ui, 0);
    return column;
}
