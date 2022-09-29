#pragma once

void gfx_init(int width, int height);
void gfx_draw();
void gfx_clear_buffer(uint32_t color);

// Image
int gfx_load_image(const char* image_path);
bool gfx_draw_image(int img_index, int x, int y,float scale);
void gfx_free_image(int img_index);

void gfx_draw_rect(int x, int y, int w, int h, uint32_t color, bool filled);
void gfx_draw_pixel(int x, int y, uint32_t color);
void gfx_draw_pixela(int x, int y, uint32_t color, float alpha);
void gfx_draw_line(int x, int y, int x2, int y2, uint32_t color);
void gfx_draw_circle(int x0, int y0, int r, uint32_t color, bool filled);
void gfx_draw_ellipse(int origin_x,int origin_y, int w, int h, uint32_t color, bool filled);

uint32_t gfx_rgb_to_color(uint8_t r,uint8_t g,uint8_t b);
void gfx_color_to_rgb(uint32_t color, uint8_t* r,uint8_t* g,uint8_t* b);
