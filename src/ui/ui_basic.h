#ifndef UI_BASIC_H
#define UI_BASIC_H

internal UI_Size ui_size_fill(Void);

internal UI_Box *ui_spacer(UI_Context *ui);
internal UI_Box *ui_spacer_sized(UI_Context *ui, UI_Size size);

internal UI_Box *ui_row_begin(UI_Context *ui);
internal UI_Box *ui_row_end(UI_Context *ui);
#define ui_row(ui) defer_loop(ui_row_begin(ui), ui_row_end(ui))

internal UI_Box *ui_column_begin(UI_Context *ui);
internal UI_Box *ui_column_end(UI_Context *ui);
#define ui_column(ui) defer_loop(ui_column_begin(ui), ui_column_end(ui))

#endif //UI_BASIC_H
