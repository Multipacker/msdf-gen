#ifndef UI_BASIC_H
#define UI_BASIC_H

internal UI_Size ui_size_fill(Void);

internal UI_Box *ui_spacer(Void);
internal UI_Box *ui_spacer_sized(UI_Size size);

internal UI_Box *ui_row_begin(Void);
internal UI_Box *ui_row_string_begin(Str8 string);
internal UI_Box *ui_row_end(Void);
#define ui_row() defer_loop(ui_row_begin(), ui_row_end())
#define ui_row_string(string) defer_loop(ui_row_string_begin(string), ui_row_end())

internal UI_Box *ui_column_begin(Void);
internal UI_Box *ui_column_string_begin(Str8 string);
internal UI_Box *ui_column_end(Void);
#define ui_column() defer_loop(ui_column_begin(), ui_column_end())
#define ui_column_string(string) defer_loop(ui_column_string_begin(, string), ui_column_end())

internal Void ui_label(Str8 string);
internal Void ui_label_format(CStr format, ...);

internal UI_Input ui_button(Str8 string);
internal UI_Input ui_button_format(CStr format, ...);

internal UI_Input ui_checkbox(B32 is_checked, Str8 label);
internal Void     ui_checkbox_b32(B32 *is_checked, Str8 label);

#define ui_padding(size) defer_loop(ui_spacer_sized(size), ui_spacer_sized(size))
#define ui_center()      ui_padding(ui_size_parent_percent(1.0f, 0.0f))

#endif //UI_BASIC_H
