#pragma once

#include "math2d.h"


// defines
// --------------------------------------------------------

#define MAX_GFX_IMAGES 256

#define COLOR(r,g,b)  (uint32_t)(r<<16|g<<8|b)
#define COLOR2(r,g,b) (uint32_t)((uint8_t)(r*255.0)<<16|(uint8_t)(g*255.0)<<8|(uint8_t)(b*255.0))
#define COLOR_RED       COLOR(0xff,0x00,0x00)
#define COLOR_GREEN     COLOR(0x00,0xff,0x00)
#define COLOR_BLUE      COLOR(0x00,0x00,0xff)
#define COLOR_ORANGE    COLOR(0xff,0x80,0x00)
#define COLOR_CYAN      COLOR(0x00,0xff,0xff)
#define COLOR_PURPLE    COLOR(0x7f,0x00,0xff)
#define COLOR_PINK      COLOR(0xff,0x00,0xff)
#define COLOR_YELLOW    COLOR(0xff,0xff,0x00)
#define COLOR_WHITE     COLOR(0xff,0xff,0xff)
#define COLOR_BLACK     COLOR(0,0,0)
#define COLOR_TINT_NONE (0xFFFFFFFF)

// types
// --------------------------------------------------------

typedef struct
{
    int w,h,n;
    unsigned char* data;
} GFXImageData;

typedef struct
{
    uint32_t texture;
    int w,h,n;

    int element_count;
    int elements_per_row;
    int elements_per_col;
    int element_width, element_height;

    Rect* visible_rects;
    Rect* sprite_visible_rects;
    Rect* sprite_rects;

    int node_sets;
    Vector2f** nodes;   // node positions are interpreted as offsets from center of image visible rectangle
    uint32_t* node_colors;
} GFXImage;

typedef struct
{
    const char* image_path;
    uint32_t colors[10];
    int num_sets;
} GFXNodeDataInput;

typedef struct
{
    int curr_frame;
    int max_frames;
    int frame_sequence[16];
    float curr_frame_time;
    float max_frame_time;
    bool finite;
    int curr_loop;
    int max_loops;
} GFXAnimation;

// global vars
// --------------------------------------------------------

extern GFXImage gfx_images[MAX_GFX_IMAGES];
extern int font_image;


// global functions
// --------------------------------------------------------

// Graphics
void gfx_init(int width, int height);
void gfx_clear_buffer(uint8_t r, uint8_t g, uint8_t b);

// Images
void gfx_image_init();
bool gfx_load_image_data(const char* image_path, GFXImageData* image, bool flip);
int gfx_load_image(const char* image_path, bool flip, bool linear_filter, int element_width, int element_height, GFXNodeDataInput* node_input);
bool gfx_draw_image(int img_index, int sprite_index, float x, float y, uint32_t color, float scale, float rotation, float opacity, bool full_image);
GFXImage* gfx_get_image_data(int img_index);
bool gfx_get_image_node_point(int img_index, int sprite_index, uint32_t node_color, Vector2f* node);

// Lines
void gfx_clear_lines();
void gfx_add_line(float x0, float y0, float x1, float y1, uint32_t color);
void gfx_draw_lines();

// Rects
void gfx_draw_rect(Rect* r, uint32_t color, float scale, float opacity, bool filled, bool in_world);
void gfx_draw_rect_xywh(float x, float y, float w, float h, uint32_t color, float scale, float opacity, bool filled, bool in_world);

// Strings
Vector2f gfx_draw_string(float x, float y, uint32_t color, float scale, float rotation, float opacity, bool in_world, bool drop_shadow, char* fmt, ...);
Vector2f gfx_string_get_size(float scale, char* fmt, ...);

// Animation
void gfx_anim_update(GFXAnimation* anim, double delta_t);

