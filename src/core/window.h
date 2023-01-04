#pragma once

#include <GLFW/glfw3.h>

#define ASPECT_NUM  3.0f
#define ASPECT_DEM  2.0f
#define ASPECT_RATIO (ASPECT_NUM / ASPECT_DEM)

#define TARGET_FPS 60.0f

typedef enum
{
    KEY_MODE_NONE,
    KEY_MODE_NORMAL,
    KEY_MODE_TEXT
} KeyMode;

typedef void (*key_cb_t)(GLFWwindow* window, int key, int scan_code, int action, int mods);

extern int window_width;
extern int window_height;
extern int view_width;
extern int view_height;

bool window_init(int _view_width, int _view_height);
void window_deinit();

float window_scale_view_to_world(float distance);
void window_translate_view_to_world(int* x, int* y);
void window_translate_world_to_view(int* x, int* y);

void window_get_mouse_coords(int* x, int* y);
void window_get_mouse_view_coords(int* x, int* y);
void window_set_mouse_view_coords(int x, int y);
void window_get_mouse_world_coords(int* x, int* y);
void window_set_mouse_world_coords(float x, float y);
void window_poll_events();
bool window_should_close();
void window_set_close(int value);
void window_swap_buffers();

bool window_controls_is_key_state(int key, int state);
void window_controls_set_cb(key_cb_t cb);
void window_controls_set_text_buf(char* buf, int max_len);
void window_controls_set_key_mode(KeyMode mode);
KeyMode window_controls_get_key_mode();
void window_controls_clear_keys();
void window_controls_add_key(bool* state, int key);

void window_controls_add_mouse_button(bool* state, int key);

bool window_is_cursor_enabled();
void window_enable_cursor();
void window_disable_cursor();

void windows_text_mode_buf_insert(char c, int index);
void window_text_mode_buf_remove(int index, bool backspace);

bool window_mouse_left_went_down();
bool window_mouse_left_went_up();
void window_mouse_update_actions();

void window_mouse_set_cursor_ibeam();
void window_mouse_set_cursor_normal();
