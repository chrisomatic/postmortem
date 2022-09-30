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

#define NUM_RATS 100

// =========================
// Global Vars
// =========================

Timer game_timer = {0};

int rat_img = 0;

typedef struct
{
    float x,y;
    float angle;
} Rat;

Rat rats[NUM_RATS] = {0};

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
    gfx_init(VIEW_WIDTH, VIEW_HEIGHT,20);

    rat_img = gfx_load_image("img/rat_small.png");

    for(int i = 0; i < NUM_RATS; ++i)
    {
        rats[i].x = rand() % VIEW_WIDTH;
        rats[i].y = rand() % VIEW_HEIGHT;
        rats[i].angle = RAD(rand() % 360);
    }

}

void deinit()
{
    shader_deinit();
    window_deinit();
}

void update()
{
    for(int i = 0; i < NUM_RATS; ++i)
    {
        rats[i].x += cos(rats[i].angle);
        rats[i].y += sin(rats[i].angle);
    }

    int x,y;
    window_get_mouse_coords(&x, &y);
    //printf("X: %d, Y: %d\n",x,y);
}

void draw()
{
    uint32_t bkg_color =  gfx_rgb_to_color(50,50,50);
    uint32_t main_color = gfx_rgb_to_color(0,200,200);

    gfx_clear_buffer(bkg_color);

    for(int i = 0; i < NUM_RATS; ++i)
    {
        gfx_draw_image_scaled(rat_img,(int)rats[i].x,(int)rats[i].y,1.0,1.0);
        //gfx_draw_image(rat_img,(int)rats[i].x,(int)rats[i].y);
        //gfx_draw_pixel((int)rats[i].x,(int)rats[i].y,main_color);
    }

    //gfx_draw_circle(200, 200, 100, main_color);

    gfx_draw();
    window_swap_buffers();
}

