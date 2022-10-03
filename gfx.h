#pragma once

#define COLOR_TINT_NONE (0xFFFFFFFF)

void gfx_init(int width, int height);
void gfx_clear_buffer(uint8_t r, uint8_t g, uint8_t b);

// Image
int gfx_load_image(const char* image_path);
bool gfx_draw_image(int img_index, float x, float y, uint32_t color, float scale, float rotation, float opacity);
void gfx_free_image(int img_index);

uint32_t gfx_rgb_to_color(uint8_t r,uint8_t g,uint8_t b);
