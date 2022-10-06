#pragma once

#include <GLFW/glfw3.h>

#define ASPECT_NUM 16.0f
#define ASPECT_DEM  9.0f
#define ASPECT_RATIO (ASPECT_NUM / ASPECT_DEM)

#define TARGET_FPS 60.0f

extern int window_width;
extern int window_height;
extern int view_width;
extern int view_height;

bool window_init(int _view_width, int _view_height);
void window_deinit();
void window_get_mouse_coords(int* x, int* y);
void window_get_mouse_view_coords(int* x, int* y);
void window_poll_events();
bool window_should_close();
void window_swap_buffers();

void window_controls_clear_keys();
void window_controls_add_key(uint32_t* keys, int key, int bit_num);
void window_controls_add_mouse_button(uint32_t* keys, int key, int bit_num);
