#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "math2d.h"
#include "gfx.h"
#include "window.h"
#include "imgui.h"

#define NOMINAL_FONT_SIZE 64.0 // px for 1.0 scale

typedef struct
{
    int curr_x;
    int curr_y;
    int padding;

    int text_size_px;
    uint32_t text_color;
    float text_scale;
    int text_padding;

    uint32_t button_color_background;
    uint32_t button_color_foreground;
    float button_opacity;

} ImGuiContext;

static ImGuiContext ctx;

void imgui_begin(int x, int y)
{
    ctx.curr_x = x;
    ctx.curr_y = y;

    // defaults
    ctx.padding = 3;
    ctx.text_padding = 3;
    ctx.text_size_px = 20;
    ctx.text_color = 0xFFFFFFFF;
    ctx.text_scale = ctx.text_size_px / NOMINAL_FONT_SIZE;

    ctx.button_color_background = 0xAAAAAAAA;
    ctx.button_color_foreground = 0xFFFFFFFF;
    ctx.button_opacity = 0.8;
}

void imgui_set_text_size(int pxsize)
{
    ctx.text_size_px = pxsize;
    ctx.text_scale = ctx.text_size_px / NOMINAL_FONT_SIZE;
}

void imgui_set_text_color(uint32_t color)
{
    ctx.text_color = color;
}

void imgui_text(char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    Vector2f size = gfx_draw_string(ctx.curr_x, ctx.curr_y, ctx.text_color, ctx.text_scale, 0.0, 1.0, false, true, str);
    ctx.curr_y += size.y + ctx.padding;
}

bool imgui_button(char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    Vector2f text_size = gfx_string_get_size(ctx.text_scale, str);

    Vector2f button_size = {
        text_size.x + 2*ctx.text_padding,
        text_size.y + 2*ctx.text_padding,
    };

    gfx_draw_rect_xywh(ctx.curr_x + button_size.x/2.0, ctx.curr_y + button_size.y/2.0, button_size.x, button_size.y, ctx.button_color_background, 1.0, ctx.button_opacity, true,false);

    gfx_draw_string(ctx.curr_x + ctx.text_padding, ctx.curr_y + ctx.text_padding, ctx.button_color_foreground, ctx.text_scale, 0.0, 1.0, false, false, str);

    ctx.curr_y += button_size.y + ctx.padding;

    return false;
}
void imgui_end()
{

}
