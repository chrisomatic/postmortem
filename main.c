#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include "common.h"
#include "settings.h"
#include "window.h"
#include "shader.h"
#include "timer.h"
#include "gfx.h"

// =========================
// Global Vars
// =========================

Timer game_timer = {0};

int rat_img = 0;
int mouse_x = 0;
int mouse_y = 0;

// =========================
// Function Prototypes
// =========================

void start_game();
void init();
void deinit();
void update();
void draw();

// =========================
// Main Loop
// =========================

int main(int argc, char* argv[])
{
    start_game();
    return 0;
}

// =========================
// Functions
// =========================

void start_game()
{
    init();

    timer_set_fps(&game_timer,TARGET_FPS);
    timer_begin(&game_timer);

    // main game loop
    for(;;)
    {
        window_poll_events();
        if(window_should_close())
            break;

        update();
        draw();

        timer_wait_for_frame(&game_timer);
        //printf("fps: %f\n",timer_get_prior_frame_fps(&game_timer));
    }

    deinit();
}

void init()
{
    bool success;

    success = window_init();

    if(!success)
    {
        fprintf(stderr,"Failed to initialize window!\n");
        exit(1);
    }

    time_t t;
    srand((unsigned) time(&t));

    printf("Initializing...\n");

    printf(" - Shaders.\n");
    shader_load_all();

    printf(" - Graphics.\n");
    gfx_init(VIEW_WIDTH, VIEW_HEIGHT);

    rat_img = gfx_load_image("img/rat.png");
}

void deinit()
{
    shader_deinit();
    window_deinit();
}

void update()
{
    //gfx_draw_line(100,100,300,300,COLOR_BLUE);

    //gfx_draw_circle(500,500,20, main_color, true);
    //gfx_draw_ellipse(500,550,20,20,main_color, true);

    int x,y;
    window_get_mouse_coords(&x, &y);
    if(x != mouse_x || y != mouse_y)
    {
        mouse_x = x;
        mouse_y = y;
        printf("X: %d, Y: %d\n",x,y);
    }
}

void draw()
{
    uint32_t bkg_color =  gfx_rgb_to_color(100,100,100);
    uint32_t red = gfx_rgb_to_color(255,0,0);
    uint32_t green = gfx_rgb_to_color(0,255,0);
    uint32_t black = gfx_rgb_to_color(0,0,0);


    gfx_clear_buffer(bkg_color);

    int x,y;

    // center
    x = CENTER_X; y = CENTER_Y;
    gfx_draw_rect(x, y, 100, 100, red, true);
    gfx_draw_rect(x, y, 100, 100, black, false);
    for(int i = 0; i < 4; ++i)
        for(int j = 0; j < 4; ++j)
            gfx_draw_pixela(i+x, j+y, green, 1.0);

    bool filled = false;
    // right
    x = VIEW_WIDTH - 50; y = CENTER_Y;
    gfx_draw_rect(x, y, 100, 100, red, filled);

    // left
    x = - 50; y = CENTER_Y;
    gfx_draw_rect(x, y, 100, 100, red, filled);

    // bottom
    x = CENTER_X; y = 50;
    gfx_draw_rect(x, y, 100, 100, red, filled);

    // top
    x = CENTER_X; y = VIEW_HEIGHT+50;
    gfx_draw_rect(x, y, 100, 100, red, filled);

    // bottom right
    x = VIEW_WIDTH - 50; y = 50;
    gfx_draw_rect(x, y, 100, 100, red, filled);

    x = VIEW_WIDTH-1; y = 0;
    gfx_draw_rect(x, y, 100, 100, green, filled);

    x = -99; y = 0;
    gfx_draw_rect(x, y, 100, 100, red, filled);


    // gfx_draw_circle_wu(100, 100, 30, red);


    // gfx_draw_pixela(0, VIEW_HEIGHT-1, green, 1.0);
    // gfx_draw_pixela(10, window_height-10, green, 1.0);

    // gfx_draw_image(rat_img,200,200,0.2,0.4);
    // for(int i = 0; i < 4; ++i)
    //     for(int j = 0; j < 4; ++j)
    //         gfx_draw_pixela(i+200, j+200, red, 1.0);

    gfx_draw();
    window_swap_buffers();
}

