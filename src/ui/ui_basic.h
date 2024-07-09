#ifndef UI_BASIC_H
#define UI_BASIC_H

internal UI_Size ui_size_fill(Void);

internal UI_Box *ui_spacer(UI_Context *ui);
internal UI_Box *ui_spacer_sized(UI_Context *ui, UI_Size size);

internal UI_Box *ui_row_begin(UI_Context *ui);
internal UI_Box *ui_row_string_begin(UI_Context *ui, Str8 string);
internal UI_Box *ui_row_end(UI_Context *ui);
#define ui_row(ui) defer_loop(ui_row_begin(ui), ui_row_end(ui))
#define ui_row_string(ui, string) defer_loop(ui_row_string_begin(ui, string), ui_row_end(ui))

internal UI_Box *ui_column_begin(UI_Context *ui);
internal UI_Box *ui_column_string_begin(UI_Context *ui, Str8 string);
internal UI_Box *ui_column_end(UI_Context *ui);
#define ui_column(ui) defer_loop(ui_column_begin(ui), ui_column_end(ui))
#define ui_column_string(ui, string) defer_loop(ui_column_string_begin(ui, string), ui_column_end(ui))

internal Void ui_label(UI_Context *ui, Str8 string);
internal Void ui_label_format(UI_Context *ui, CStr format, ...);

internal UI_Input ui_button(UI_Context *ui, Str8 string);
internal UI_Input ui_button_format(UI_Context *ui, CStr format, ...);

#endif //UI_BASIC_H
