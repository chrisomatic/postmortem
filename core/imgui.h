#pragma once

void imgui_begin(int x, int y);
void imgui_end();

// widgets
void imgui_text(char* text, ...);
bool imgui_button(char* text, ...);

// properties
void imgui_set_text_size(int pxsize);
void imgui_set_text_color(uint32_t color);

