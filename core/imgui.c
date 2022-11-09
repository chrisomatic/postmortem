#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "math2d.h"
#include "gfx.h"
#include "window.h"
#include "hash.h"
#include "imgui.h"

#define NOMINAL_FONT_SIZE 64.0 // px for 1.0 scale
#define MAX_CONTEXTS 32
#define MAX_INT_LOOKUPS 32

typedef struct
{
    uint32_t hash;
    int val;
} IntLookup;

typedef struct
{
    uint32_t hash;

    // ids
    uint32_t highlighted_id;
    uint32_t active_id;

    bool is_horizontal;

    // formatting
    int curr_x, curr_y;
    int curr_w, curr_h;

    int start_x, start_y;
    int total_width, total_height;
    int indent_amount;

    IntLookup int_lookups[MAX_INT_LOOKUPS];

    int mouse_x, mouse_y;

} ImGuiContext;

typedef struct
{
    // general
    int vert_spacing;

    // text
    int text_size_px;
    uint32_t text_color;
    float text_scale;
    int text_padding;

    // button
    uint32_t button_color_background;
    uint32_t button_color_background_highlighted;
    uint32_t button_color_background_active;
    uint32_t button_color_foreground;
    float button_opacity;

    // checkbox
    int checkbox_size;

    // slider
    uint32_t slider_color_foreground;
    uint32_t slider_color_background;
    uint32_t slider_color_handle;
    uint32_t slider_color_handle_highlighted;
    uint32_t slider_color_handle_active;
    float slider_width;
    float slider_handle_width;
    float slider_opacity;

    // panel
    uint32_t panel_color;
    float panel_opacity;

} ImGuiTheme;

static ImGuiContext contexts[MAX_CONTEXTS] = {0};
static ImGuiContext default_context = {0};
static ImGuiTheme theme;

static ImGuiContext* ctx;

static inline bool is_mouse_inside(int x, int y, int width, int height);
static inline bool is_highlighted(uint32_t hash);
static inline bool is_active(uint32_t hash);
static inline bool is_inside();
static inline void set_highlighted(uint32_t hash);
static inline void set_active(uint32_t hash);
static inline void clear_highlighted();
static inline void clear_active();
static void assign_context(uint32_t hash);
static void handle_highlighting(uint32_t hash);
static void imgui_slider_float_internal(char* label, float min, float max, float* result, char* format);
static void progress_pos();

static void draw_button(uint32_t hash, char* str);
static void draw_checkbox(uint32_t hash, char* label, bool result);
static void draw_slider(uint32_t hash, char* str, int slider_x, char* val_format, float val);
static void draw_color_picker(uint32_t hash, char* label, bool result);
static void draw_panel();

void imgui_begin(char* name, int x, int y)
{
    uint32_t hash = hash_str(name,strlen(name),0x0);
    assign_context(hash);

    ctx->start_x = x;
    ctx->start_y = y;
    
    ctx->curr_x = x;
    ctx->curr_y = y;

    ctx->indent_amount = 0;

    // set theme
    // text
    theme.vert_spacing = 4;
    theme.text_size_px = 20;
    theme.text_scale = theme.text_size_px / NOMINAL_FONT_SIZE;
    theme.text_color = 0xFFFFFFFF;
    theme.text_padding = 5;

    // checkbox
    theme.checkbox_size = 10;

    // button
    theme.button_color_background = 0x55555555;
    theme.button_color_background_highlighted = 0xAAAAAAAA;
    theme.button_color_background_active = 0x0000AAAA;
    theme.button_color_foreground = 0xFFFFFFFF;
    theme.button_opacity = 0.8;

    // slider
    theme.slider_color_foreground = 0xFFFFFFFF;
    theme.slider_color_background = 0x55555555;
    theme.slider_color_handle = 0x8888888;
    theme.slider_color_handle_highlighted = 0xAAAAAAAA;
    theme.slider_color_handle_active = 0x0000AAAA;
    theme.slider_width = 150.0;
    theme.slider_handle_width = 16;
    theme.slider_opacity = 0.8;

    // panel
    theme.panel_color = 0x20202020;
    theme.panel_opacity = 0.75;

    window_get_mouse_view_coords(&ctx->mouse_x, &ctx->mouse_y);
}

void imgui_begin_panel(char* name, int x, int y)
{
    imgui_begin(name, x,y);
    draw_panel();
    ctx->curr_x += 4;
}

void imgui_set_text_size(int pxsize)
{
    theme.text_size_px = pxsize;
    theme.text_scale = theme.text_size_px / NOMINAL_FONT_SIZE;
}

void imgui_set_text_color(uint32_t color)
{
    theme.text_color = color;
}

void imgui_text(char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    imgui_text_colored(theme.text_color, str);
}

void imgui_text_sized(int pxsize, char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    Vector2f size = gfx_draw_string(ctx->curr_x, ctx->curr_y, theme.text_color, (pxsize / NOMINAL_FONT_SIZE), 0.0, 1.0, false, true, str);
    ctx->curr_w = size.x;
    ctx->curr_h = size.y;
    progress_pos();
}

void imgui_text_colored(uint32_t color, char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    Vector2f size = gfx_draw_string(ctx->curr_x, ctx->curr_y, color, theme.text_scale, 0.0, 1.0, false, true, str);
    ctx->curr_w = size.x;
    ctx->curr_h = size.y;
    progress_pos();
}

void imgui_indent_begin(int indentpx)
{
    ctx->indent_amount = indentpx;
    ctx->curr_x += indentpx;
}

void imgui_indent_end()
{
    ctx->curr_x -= ctx->indent_amount;
    ctx->indent_amount = 0;
}

void imgui_newline()
{
    ctx->curr_y += theme.text_size_px;
}

bool imgui_button(char* label, ...)
{
    bool result = false;
    uint32_t hash = hash_str(label,strlen(label),0x0);

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            if(is_highlighted(hash))
            {
                result = true;
            }
            clear_active();
        }
    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    va_list args;
    va_start(args, label);
    char str[256] = {0};
    vsprintf(str,label, args);
    va_end(args);

    Vector2f text_size = gfx_string_get_size(theme.text_scale, str);

    ctx->curr_w = text_size.x + 2*theme.text_padding;
    ctx->curr_h = text_size.y + 2*theme.text_padding;

    handle_highlighting(hash);

    draw_button(hash, str);
    progress_pos();

    return result;
}

void imgui_checkbox(char* label, bool* result)
{
    uint32_t hash = hash_str(label,strlen(label),0x0);

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            if(is_highlighted(hash))
            {
                bool curr_result = *result;
                *result = !curr_result;
            }
            clear_active();
        }
    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    ctx->curr_w = theme.checkbox_size;
    ctx->curr_h = theme.checkbox_size;

    handle_highlighting(hash);

    Vector2f text_size = gfx_string_get_size(theme.text_scale, label);

    draw_checkbox(hash, label, *result);

    ctx->curr_w = theme.checkbox_size + text_size.x + theme.text_padding;
    ctx->curr_h = MAX(theme.checkbox_size, text_size.y) + theme.text_padding;

    progress_pos();
}

void imgui_color_picker(char* label, uint32_t* result)
{
    uint32_t hash = hash_str(label,strlen(label),0x0);

    Vector2f text_size = gfx_string_get_size(theme.text_scale, label);
    ctx->curr_w = text_size.x + 2*theme.text_padding;
    ctx->curr_h = text_size.y + 2*theme.text_padding;

    draw_color_picker(hash, label, *result);
    progress_pos();
}

void imgui_slider_float(char* label, float min, float max, float* result)
{
    imgui_slider_float_internal(label, min, max, result, "%.2f");
    progress_pos();
}

void imgui_slider_int(char* label, int min, int max, int* result)
{
    imgui_slider_float_internal(label, min, max, (float*)result, "%d");
    progress_pos();
}

void imgui_end()
{
    ctx->total_width = 210; // @TODO
    ctx->total_height = (ctx->curr_y-ctx->start_y);
}

// ============================
// Static Functions
// ============================

static inline bool is_mouse_inside(int x, int y, int width, int height)
{
    return (ctx->mouse_x >= x && ctx->mouse_x <= (x+width) &&
                   ctx->mouse_y >= y && ctx->mouse_y <= (y+height));
}
static inline bool is_highlighted(uint32_t hash)
{
    return (ctx->highlighted_id == hash);
}
static inline bool is_active(uint32_t hash)
{
    return (ctx->active_id == hash);
}
static inline bool is_inside()
{
    return is_mouse_inside(ctx->curr_x, ctx->curr_y, ctx->curr_w, ctx->curr_h);
}
static inline void set_highlighted(uint32_t hash)
{
    if(ctx->active_id == 0x0)
    {
        ctx->highlighted_id = hash;
    }
}
static inline void set_active(uint32_t hash)
{
    ctx->active_id = hash;
}

static inline void clear_highlighted()
{
    ctx->highlighted_id = 0x0;
}

static inline void clear_active()
{
    ctx->active_id = 0x0;
}

static void progress_pos()
{
    if(ctx->is_horizontal)
    {
        ctx->curr_x += ctx->curr_h;
    }
    else
    {
        ctx->curr_y += 1.3*ctx->curr_h;
    }
}

static void assign_context(uint32_t hash)
{
    for(int i = 0; i < MAX_CONTEXTS;++i)
    {
        if(contexts[i].hash == hash || contexts[i].hash == 0x0)
        {
            ctx = &contexts[i];
            ctx->hash = hash;
            return;
        }
    }
    ctx = &default_context;
}

static IntLookup* get_int_lookup(uint32_t hash)
{
    for(int i = 0; i < MAX_INT_LOOKUPS; ++i)
    {
        if(ctx->int_lookups[i].hash == 0x0 || ctx->int_lookups[i].hash == hash)
        {
            IntLookup* lookup = &ctx->int_lookups[i];
            lookup->hash = hash;
            return lookup;
        }
    }
}

static void imgui_slider_float_internal(char* label, float min, float max, float* result, char* format)
{
    if(min > max)
        return;

    Vector2f text_size = gfx_string_get_size(theme.text_scale, label);

    ctx->curr_w = theme.slider_width;
    ctx->curr_h = text_size.y + 2*theme.text_padding;

    uint32_t hash = hash_str(label,strlen(label),0x0);

    // get slider index
    int first_empty = -1;
    int slider_index = -1;

    IntLookup* lookup = get_int_lookup(hash);

    int *slider_x = &lookup->val;

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            clear_active();
        }

        *slider_x = ctx->mouse_x-ctx->curr_x-theme.slider_handle_width/2.0;
        *slider_x = RANGE(*slider_x, 0, ctx->curr_w - theme.slider_handle_width);

    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    handle_highlighting(hash);

    float slider_val = (*slider_x / (ctx->curr_w - theme.slider_handle_width));
    slider_val *= (max-min);
    slider_val += min;
    draw_slider(hash,label, *slider_x, format, slider_val);

    *result = slider_val;
}

static void handle_highlighting(uint32_t hash)
{
    if(is_inside())
    {
        set_highlighted(hash);
    }
    else
    {
        if(is_highlighted(hash))
        {
            clear_highlighted();
        }
    }
}

static void draw_button(uint32_t hash, char* str)
{
    // draw button
    uint32_t button_color = theme.button_color_background;

    if(is_highlighted(hash))
    {
        button_color = theme.button_color_background_highlighted;
    }
    if(is_active(hash))
    {
        button_color = theme.button_color_background_active;
    }

    gfx_draw_rect_xywh(ctx->curr_x + ctx->curr_w/2.0, ctx->curr_y + (ctx->curr_h+theme.text_padding)/2.0, ctx->curr_w, ctx->curr_h, button_color, 1.0, theme.button_opacity, true,false);

    gfx_draw_string(ctx->curr_x + theme.text_padding, ctx->curr_y + theme.text_padding, theme.button_color_foreground, theme.text_scale, 0.0, 1.0, false, false, str);
}

static void draw_slider(uint32_t hash, char* str, int slider_x, char* val_format, float val)
{
    // draw bar
    gfx_draw_rect_xywh(ctx->curr_x + ctx->curr_w/2.0, ctx->curr_y + ctx->curr_h/2.0, ctx->curr_w, ctx->curr_h, theme.slider_color_background, 1.0, theme.slider_opacity, true,false);

    // draw handle
    uint32_t handle_color = theme.slider_color_handle;

    if(is_highlighted(hash))
    {
        handle_color = theme.slider_color_handle_highlighted;
    }
    if(is_active(hash))
    {
        handle_color = theme.slider_color_handle_active;
    }

    gfx_draw_rect_xywh(ctx->curr_x + theme.slider_handle_width/2.0 + slider_x, ctx->curr_y + ctx->curr_h/2.0, theme.slider_handle_width-4, ctx->curr_h-4, handle_color, 1.0, theme.slider_opacity, true,false);

    // draw value
    char val_str[16] = {0};
    snprintf(val_str,15,val_format,val);
    Vector2f val_size = gfx_string_get_size(theme.text_scale, val_str);
    gfx_draw_string(ctx->curr_x + (ctx->curr_w-val_size.x)/2.0, ctx->curr_y + theme.text_padding, theme.slider_color_foreground, theme.text_scale, 0.0, 1.0, false, false, val_str);

    // draw label
    gfx_draw_string(ctx->curr_x + 2.0*ctx->curr_w/2.0 + theme.text_padding, ctx->curr_y + theme.text_padding, theme.slider_color_foreground, theme.text_scale, 0.0, 1.0, false, false, str);
}

static void draw_panel()
{
    gfx_draw_rect_xywh(ctx->start_x+ctx->total_width/2.0, ctx->start_y+ctx->total_height/2.0, ctx->total_width, ctx->total_height, theme.panel_color, 1.0, theme.panel_opacity,true,false);
}

static void draw_checkbox(uint32_t hash, char* label, bool result)
{
    // draw button
    uint32_t check_color = theme.button_color_background;

    if(is_highlighted(hash))
    {
        check_color = theme.button_color_background_highlighted;
    }
    if(is_active(hash))
    {
        check_color = theme.button_color_background_active;
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, label);

    // draw check
    float x = ctx->curr_x + theme.text_padding;
    float y = ctx->curr_y + ctx->curr_h/2.0;

    gfx_draw_rect_xywh(x,y, theme.checkbox_size, theme.checkbox_size, check_color, 1.0, 1.0, false,false);
    gfx_draw_rect_xywh(x,y, theme.checkbox_size-4, theme.checkbox_size-4, check_color, 1.0, 1.0, result,false);

    gfx_draw_string(ctx->curr_x + theme.checkbox_size + theme.text_padding, ctx->curr_y-text_size.y/2.0, theme.text_color, theme.text_scale, 0.0, 1.0, false, false, label);
}

static void draw_color_picker(uint32_t hash, char* label, bool result)
{
    // draw r,g,b rects

    /*
    gfx_draw_rect_xywh(ctx->curr_x + theme.text_padding, ctx->curr_y + ctx->curr_h/2.0, theme.checkbox_size, theme.checkbox_size, check_color, 1.0, 1.0, false,false);

    gfx_draw_string(ctx->curr_x + theme.checkbox_size + theme.text_padding, ctx->curr_y + theme.text_padding, theme.text_color, theme.text_scale, 0.0, 1.0, false, false, "R");
    
    */
}
