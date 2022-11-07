#pragma once

void imgui_begin(int x, int y);
void imgui_end();

// widgets
void imgui_text(char* text, ...);
void imgui_text_colored(uint32_t color, char* text, ...);
bool imgui_button(char* text, ...);
void imgui_slider_float(char* label, float min, float max, float* result);
void imgui_slider_int(char* label, int min, int max, int* result);

// properties
void imgui_set_text_size(int pxsize);
void imgui_set_text_color(uint32_t color);

