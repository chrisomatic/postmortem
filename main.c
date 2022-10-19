#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include "main.h"
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
#include "net.h"
#include "log.h"

// Settings
#define VIEW_WIDTH   800
#define VIEW_HEIGHT  600

// =========================
// Global Vars
// =========================

Timer game_timer = {0};
GameRole role;

// =========================
// Function Prototypes
// =========================

void parse_args(int argc, char* argv[]);
void start_local();
void start_client();
void start_server();
void init();
void deinit();
void update(double);
void draw();

// =========================
// Main Loop
// =========================

int main(int argc, char* argv[])
{
    parse_args(argc, argv);

    switch(role)
    {
        case ROLE_LOCAL:
            start_local();
            break;
        case ROLE_CLIENT:
            start_client();
            break;
        case ROLE_SERVER:
            start_server();
            break;
    }

    return 0;
}

// =========================
// Functions
// =========================

void parse_args(int argc, char* argv[])
{
    role = ROLE_LOCAL;

    if(argc > 1)
    {
        for(int i = 1; i < argc; ++i)
        {
            if(argv[i][0] == '-' && argv[i][1] == '-')
            {
                // server
                if(strncmp(argv[i]+2,"server",6) == 0)
                    role = ROLE_SERVER;

                // client
                else if(strncmp(argv[i]+2,"client",6) == 0)
                    role = ROLE_CLIENT;
            }
            else
            {
                net_client_set_server_ip(argv[i]);
            }
        }
    }
}

const char* game_role_to_str(GameRole _role)
{
    switch(_role)
    {
        case ROLE_LOCAL:  return "Local";
        case ROLE_CLIENT: return "Client";
        case ROLE_SERVER: return "Server";
    }
    return "Unknown";
}

void start_local()
{
    LOGI("--------------");
    LOGI("Starting Local");
    LOGI("--------------");

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

void start_client()
{
    LOGI("---------------");
    LOGI("Starting Client");
    LOGI("---------------");

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

void start_server()
{
    LOGI("---------------");
    LOGI("Starting Server");
    LOGI("---------------");

    time_t t;
    srand((unsigned) time(&t));

    net_server_start();
}

void init()
{
    bool success;

    LOGI("resolution: %d %d",VIEW_WIDTH, VIEW_HEIGHT);
    success = window_init(VIEW_WIDTH, VIEW_HEIGHT);

    if(!success)
    {
        fprintf(stderr,"Failed to initialize window!\n");
        exit(1);
    }

    time_t t;
    srand((unsigned) time(&t));

    LOGI("Initializing...");

    LOGI(" - Shaders.");
    shader_load_all();

    LOGI(" - Graphics.");
    gfx_init(VIEW_WIDTH, VIEW_HEIGHT);

    LOGI(" - Camera.");
    camera_init();

    LOGI(" - World.");
    world_init();

    LOGI(" - Guns.");
    gun_init();

    LOGI(" - Player.");
    player_init();

    LOGI(" - Zombies.");
    zombie_init();

    LOGI(" - Projectiles.");
    projectile_init();
}

void deinit()
{
    shader_deinit();
    window_deinit();
}

int mx, my; //TODO
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
    projectile_draw();
    player_draw();

    // gfx_draw_sub_image(player.image, 5, player.phys.pos.x, player.phys.pos.y, 0, 1.0, 0, 1.0);

    gfx_draw_lines();
    gui_draw();
    gfx_draw_stringf(2.0,200.0,0x00FFFFFF,0.16,0.0, 1.0, false, true,"cam offset: %d %d", mx, my);
}

