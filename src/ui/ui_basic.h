#ifndef UI_BASIC_H
#define UI_BASIC_H

typedef struct UI_DrawLineEdit UI_DrawLineEdit;
struct UI_DrawLineEdit {
    Str8 text;
    U64 cursor;
    U64 mark;
};

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
internal UI_Input ui_checkbox_format(B32 is_checked, CStr format, ...);
internal UI_Input ui_checkbox_b32(B32 *is_checked, Str8 label);
internal UI_Input ui_checkbox_b32_format(B32 *is_checked, CStr format, ...);

#define ui_padding(size) defer_loop(ui_spacer_sized(size), ui_spacer_sized(size))
#define ui_center()      ui_padding(ui_size_parent_percent(1.0f, 0.0f))

// NOTE(simon): Line edit
UI_BOX_DRAW_FUNCTION(ui_draw_line_edit);
internal B32 ui_is_word(U32 codepoint);
internal Void ui_line_edit(U8 *buffer, U64 *buffer_size, U64 buffer_capacity, U64 *cursor, U64 *mark, UI_Key key);

#endif //UI_BASIC_H
