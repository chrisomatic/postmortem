#pragma once

#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*(_ARR))))

void imgui_begin(char* name, int x, int y);
void imgui_begin_panel(char* name, int x, int y);
Vector2f imgui_end(); // returns size of imgui area

// widgets
void imgui_text(char* text, ...);
void imgui_text_colored(uint32_t color, char* text, ...);
void imgui_text_sized(int pxsize, char* text, ...);
bool imgui_button(char* text, ...);
void imgui_toggle_button(bool* toggle, char* text, ...);
void imgui_checkbox(char* label, bool* result);
void imgui_color_picker(char* label, uint32_t* result);
void imgui_slider_float(char* label, float min, float max, float* result);
void imgui_number_box(char* label, int min, int max, int* result);
void imgui_inputtext(char* label, char* buf, int bufsize);
int imgui_button_select(int num_buttons, char* button_labels[], char* label);

Vector2f imgui_draw_demo(int x, int y); // for showcasing widgets
void imgui_theme_editor(); // for editing theme properties

void imgui_store_theme();
void imgui_restore_theme();

// properties
void imgui_set_text_size(int pxsize);
void imgui_set_text_color(uint32_t color);
void imgui_set_text_padding(int padding);
void imgui_set_spacing(int spacing);
void imgui_set_slider_width(int width);

// formatting
void imgui_indent_begin(int indentpx);
void imgui_indent_end();
void imgui_newline();
void imgui_horizontal_begin();
void imgui_horizontal_end();
void imgui_deselect_text_box();
