#pragma once

void imgui_begin(char* name, int x, int y);
void imgui_begin_panel(char* name, int x, int y);
void imgui_end();

// widgets
void imgui_text(char* text, ...);
void imgui_text_colored(uint32_t color, char* text, ...);
void imgui_text_sized(int pxsize, char* text, ...);
bool imgui_button(char* text, ...);
void imgui_checkbox(char* label, bool* result);
void imgui_color_picker(char* label, uint32_t* result);
void imgui_slider_float(char* label, float min, float max, float* result);
void imgui_slider_int(char* label, int min, int max, int* result);

// properties
void imgui_set_text_size(int pxsize);
void imgui_set_text_color(uint32_t color);

// formatting
void imgui_indent_begin(int indentpx);
void imgui_indent_end();
