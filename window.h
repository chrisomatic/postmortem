#pragma once

#include <GLFW/glfw3.h>

extern int view_width;
extern int view_height;

bool window_init();
void window_deinit();
void window_poll_events();
bool window_should_close();
void window_swap_buffers();
