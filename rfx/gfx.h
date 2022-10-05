#pragma once

#define COLOR_TINT_NONE (0xFFFFFFFF)

typedef struct
{
    //unsigned char* data;
    int w,h,n;
    GLuint texture;
} GFXImage;

void gfx_init(int width, int height);
void gfx_clear_buffer(uint8_t r, uint8_t g, uint8_t b);
// Image
int gfx_load_image(const char* image_path);
bool gfx_draw_image(int img_index, float x, float y, uint32_t color, float scale, float rotation, float opacity);
bool gfx_draw_sub_image(int img_index, int sprite_index, float w, float h, float x, float y, uint32_t color, float scale, float rotation, float opacity);
void gfx_free_image(int img_index);
GFXImage* gfx_get_image_data(int img_index);
