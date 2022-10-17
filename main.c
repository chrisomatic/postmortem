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
#include "math2d.h"
#include "world.h"
#include "camera.h"
#include "player.h"
#include "gun.h"
#include "projectile.h"
#include "zombie.h"
#include "gui.h"
#include "main.h"

// Settings
#define VIEW_WIDTH   800
#define VIEW_HEIGHT  600

// =========================
// Global Vars
// =========================

Timer game_timer = {0};

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
    // float a;
    // float deg = 0.0;
    // deg = 0.0; a = calc_angle_rad(0,0, cosf(RAD(deg)), -sin(RAD(deg))); printf("Angle: %.2f, Calc: %.2f\n", deg, DEG(a));
    // deg = 45.0; a = calc_angle_rad(0,0, cosf(RAD(deg)), -sin(RAD(deg))); printf("Angle: %.2f, Calc: %.2f\n", deg, DEG(a));
    // deg = 90.0; a = calc_angle_rad(0,0, cosf(RAD(deg)), -sin(RAD(deg))); printf("Angle: %.2f, Calc: %.2f\n", deg, DEG(a));
    // deg = 180.0; a = calc_angle_rad(0,0, cosf(RAD(deg)), -sin(RAD(deg))); printf("Angle: %.2f, Calc: %.2f\n", deg, DEG(a));
    // deg = 270.0; a = calc_angle_rad(0,0, cosf(RAD(deg)), -sin(RAD(deg))); printf("Angle: %.2f, Calc: %.2f\n", deg, DEG(a));
    // exit(0);


    // angle_sector(0.0, 2);
    // angle_sector(0.0, 2);
    // exit(0);




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

    printf(" - Camera.\n");
    camera_init();

    printf(" - World.\n");
    world_init();

    printf(" - Guns.\n");
    gun_init();

    printf(" - Player.\n");
    player_init();

    printf(" - Zombies.\n");
    zombie_init();

    printf(" - Projectiles.\n");
    projectile_init();
}

void deinit()
{
    shader_deinit();
    window_deinit();
}

int mx, my;
void update(double delta_t)
{
    gfx_clear_lines();

    Vector2f offset = {player.w/2.0,player.h/2.0};
    if(player.gun_ready)
    {
        window_get_mouse_view_coords(&mx, &my);

        mx = (mx - view_width/2.0);
        my = (my - view_height/2.0);

        mx = 200.0*((float)mx/view_width);
        my = 200.0*((float)my/view_height);

        offset.x += mx;
        offset.y += my;
    }

    camera_move(player.phys.pos.x + offset.x, player.phys.pos.y + offset.y);

    world_update();
    zombie_update(delta_t);
    player_update(delta_t);
    projectile_update(delta_t);
}

void draw()
{
    gfx_clear_buffer(50,50,50);

    world_draw();
    zombie_draw();
    player_draw();

    // gfx_draw_sub_image(player.image, 5, player.phys.pos.x, player.phys.pos.y, 0, 1.0, 0, 1.0);



    projectile_draw();
    gfx_draw_lines();
    gui_draw();
    gfx_draw_stringf(2.0,200.0,0x00FFFFFF,0.16,0.0, 1.0, false, true,"cam offset: %d %d", mx, my);

}

