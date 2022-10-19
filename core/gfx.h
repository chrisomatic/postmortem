#pragma once

#include "math2d.h"

#define MAX_GFX_IMAGES 256
#define COLOR_TINT_NONE (0xFFFFFFFF)

typedef struct
{
    int element_count;
    int element_width, element_height;
    Rect* visible_rects;
    Rect* sprite_rects;
} GFXSubImageData;

typedef struct
{
    int w,h,n;
    Rect visible_rect;
    Rect sprite_rect;
    uint32_t texture;
    GFXSubImageData* sub_img_data;
} GFXImage;

extern GFXImage gfx_images[MAX_GFX_IMAGES];
extern int font_image;

void gfx_init(int width, int height);
void gfx_clear_buffer(uint8_t r, uint8_t g, uint8_t b);

// Rects
void gfx_draw_rect(Rect* r, uint32_t color, float scale, float opacity, bool filled, bool in_world);
void gfx_draw_rect_xywh(float x, float y, float w, float h, uint32_t color, float scale, float opacity, bool filled, bool in_world);

// Images
int gfx_load_image(const char* image_path, bool flip, bool linear_filter);
void gfx_get_image_visible_rect(int img_w, int img_h, int img_n, unsigned char* img_data, Rect* ret);
int gfx_load_image_set(const char* image_path, int element_width, int element_height);
bool gfx_draw_image(int img_index, float x, float y, uint32_t color, float scale, float rotation, float opacity);
bool gfx_draw_sub_image(int img_index, int sprite_index, float x, float y, uint32_t color, float scale, float rotation, float opacity);
void gfx_free_image(int img_index);
GFXImage* gfx_get_image_data(int img_index);

// Strings
//void gfx_draw_string(char* str, float x, float y, uint32_t color, float scale, float rotation, float opacity, bool in_world);
void gfx_draw_string(char* str, float x, float y, uint32_t color, float scale, float rotation, float opacity, bool in_world, bool drop_shadow);
void gfx_draw_stringf(float x, float y, uint32_t color, float scale, float rotation, float opacity, bool in_world, bool drop_shadow, char* fmt, ...);
void gfx_string_get_size(char* str, float scale, float* w, float* h);
void gfx_stringf_get_size(float scale, float* w, float* h, char* fmt, ...);

// Lines
void gfx_clear_lines();
void gfx_add_line(float x0, float y0, float x1, float y1, uint32_t color);
void gfx_draw_lines();

