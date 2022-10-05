#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include "window.h"
#include "shader.h"
#include "timer.h"
#include "gfx.h"
#include "rat_math.h"
#include "world.h"

#include "player.h"

// Settings
#define NUM_RATS   10

// =========================
// Global Vars
// =========================

// =========================
// Function Prototypes
// =========================

void start_game();
void init();
void deinit();
void update(double);
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

    Timer game_timer = {0};
    timer_set_fps(&game_timer,TARGET_FPS);
    timer_begin(&game_timer);

    double t0=timer_get_time();
    double t1=0.0;

    // main game loop
    for(;;)
    {
        window_poll_events();
        if(window_should_close())
            break;

        t1 = timer_get_time();

        update(t1-t0);
        draw();

        timer_wait_for_frame(&game_timer);
        //printf("fps: %f\n",timer_get_prior_frame_fps(&game_timer));
        window_swap_buffers();
        t0 = t1;

    }

    deinit();
}

void init()
{
    bool success;

    printf("resolution: %d %d\n",VIEW_WIDTH, VIEW_HEIGHT);
    success = window_init(VIEW_WIDTH, VIEW_HEIGHT);

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

    printf(" - World.\n");
    world_init();

    printf(" - Player.\n");
    player_init();
}

void deinit()
{
    shader_deinit();
    window_deinit();
}

void update(double delta_t)
{
    world_update();
    player_update(delta_t);
}

void draw()
{
    gfx_clear_buffer(0,0,0);

    world_draw();
    player_draw();
}

