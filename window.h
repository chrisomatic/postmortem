#pragma once

#include <GLFW/glfw3.h>

#define VIEW_WIDTH   1200
#define VIEW_HEIGHT  800

#define ASPECT_NUM 16.0f
#define ASPECT_DEM  9.0f
#define ASPECT_RATIO (ASPECT_NUM / ASPECT_DEM)


extern int window_width;
extern int window_height;
extern int window_center_x;
extern int window_center_y;

bool window_init();
void window_deinit();
void window_get_mouse_coords(int* x, int* y);
void window_poll_events();
bool window_should_close();
void window_swap_buffers();
