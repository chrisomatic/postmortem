#pragma once

#include <GLFW/glfw3.h>

#define ASPECT_NUM  4.0f
#define ASPECT_DEM  3.0f
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
void window_get_mouse_world_coords(float* x, float* y);
void window_poll_events();
bool window_should_close();
void window_swap_buffers();

void window_controls_clear_keys();
void window_controls_add_key(uint16_t* keys, int key, int bit_num);
void window_controls_add_mouse_button(uint16_t* keys, int key, int bit_num);

bool window_is_cursor_enabled();
void window_enable_cursor();
void window_disable_cursor();
