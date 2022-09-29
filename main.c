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
        printf("fps: %f\n",timer_get_prior_frame_fps(&game_timer));
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

    int rat_img = gfx_load_image("img/rat.png");
}

void deinit()
{
    shader_deinit();
    window_deinit();
}

int x,y;
void update()
{
    //gfx_draw_line(100,100,300,300,COLOR_BLUE);

    //gfx_draw_circle(500,500,20, main_color, true);
    //gfx_draw_ellipse(500,550,20,20,main_color, true);

    window_get_mouse_coords(&x, &y);
    printf("X: %d, Y: %d\n",x,y);
}

void draw()
{
    uint32_t bkg_color =  gfx_rgb_to_color(0,0,255);
    uint32_t main_color = gfx_rgb_to_color(255,0,0);

    gfx_clear_buffer(bkg_color);

    gfx_draw_image(rat_img,200,200,1.0);

    for(int i = x; i < 10; ++i)
        for(int j = y; j < 10; ++j)
            gfx_draw_pixela(i, j, main_color, 0.5);

    gfx_draw();
    window_swap_buffers();
}

