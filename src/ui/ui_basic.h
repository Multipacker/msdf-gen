#ifndef UI_BASIC_H
#define UI_BASIC_H

typedef struct UI_DrawLineEdit UI_DrawLineEdit;
struct UI_DrawLineEdit {
    U64 cursor;
    U64 mark;
};

typedef struct UI_ScrollPosition UI_ScrollPosition;
struct UI_ScrollPosition {
    S64 index;
    F32 offset;
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
#define ui_column_string(string) defer_loop(ui_column_string_begin(string), ui_column_end())

internal UI_Box *ui_label(Str8 string);
internal UI_Box *ui_label_format(CStr format, ...);

internal UI_Input ui_button(Str8 string);
internal UI_Input ui_button_format(CStr format, ...);

internal UI_Input ui_checkbox(B32 is_checked, Str8 label);
internal UI_Input ui_checkbox_format(B32 is_checked, CStr format, ...);
internal UI_Input ui_checkbox_b32(B32 *is_checked, Str8 label);
internal UI_Input ui_checkbox_b32_format(B32 *is_checked, CStr format, ...);

#define ui_padding(size) defer_loop(ui_spacer_sized(size), ui_spacer_sized(size))
#define ui_center()      ui_padding(ui_size_fill())

// NOTE(simon): Line edit
UI_BOX_DRAW_FUNCTION(ui_draw_line_edit);
internal B32      ui_is_word(U32 codepoint);
internal UI_Input ui_line_edit(U8 *buffer, U64 *buffer_size, U64 buffer_capacity, U64 *cursor, U64 *mark, UI_Key key);

internal UI_Input ui_slider(F32 min, F32 *value, F32 max, UI_Key key);

// NOTE(simon): Scrolling
internal UI_ScrollPosition ui_scroll_bar(UI_ScrollPosition position, S64 first_row, S64 last_row, S64 visible_rows);

internal Void ui_scroll_region_begin(V2F32 size, F32 row_height, S64 item_count, S64 *cursor, R1S64 *visible_range_out, UI_ScrollPosition *scroll_position);
internal Void ui_scroll_region_end(V2F32 size, F32 row_height, S64 item_count, S64 *cursor, UI_ScrollPosition *scroll_position);
#define ui_scroll_region(size, row_height, item_count, visible_range_out, cursor, scroll_position) defer_loop(ui_scroll_region_begin(size, row_height, item_count, cursor, visible_range_out, scroll_position), ui_scroll_region_end(size, row_height, item_count, cursor, scroll_position))

// NOTE(simon): Color picking
internal UI_Input ui_saturation_value_picker(V4F32 *color);
internal UI_Input ui_hue_picker(V4F32 *color);
internal UI_Input ui_alpha_picker(V4F32 *color);
internal B32 ui_color_picker(V4F32 *color, UI_Size size, UI_Size bar_width, UI_Size spacing);

#endif //UI_BASIC_H
