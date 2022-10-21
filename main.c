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
Vector2f aim_camera_offset = {0};

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


// Zombie zombies2[MAX_ZOMBIES] = {0};
int main(int argc, char* argv[])
{
    // Zombie x = {0};

    // glist* l = list_create(NULL, 2, sizeof(Zombie));
    // list_add(l, (void*)&x);
    // list_add(l, (void*)&x);
    // list_add(l, (void*)&x);
    // list_add(l, (void*)&x);

    // printf("zombies: %p\n", zombies2);


    // glist* zlist = list_create((void*)zombies2, MAX_ZOMBIES, sizeof(Zombie));
    // printf("item count: %d\n", zlist->count);
    // printf("item size: %d\n", zlist->item_size);
    // printf("zlist buf: %p\n", zlist->buf);

    // Zombie z = {0};
    // z.map_row = 1;
    // list_add(zlist, (void*)&z);
    // z.map_row = 2;
    // list_add(zlist, (void*)&z);
    // z.map_row = 100;
    // list_add(zlist, (void*)&z);
    // printf("item count: %d\n", zlist->count);


    // printf("z0: %d\n", zombies2[0].map_row);
    // printf("zn: %d\n", zombies2[zlist->count-1].map_row);

    // // alternative for getting from list, not really neeeded though
    // Zombie* p = (Zombie*)list_get(zlist, 0);
    // printf("z0: %d\n", p->map_row);
    // p = (Zombie*)list_get(zlist, zlist->count-1);
    // printf("zn: %d\n", p->map_row);

    // list_remove(zlist, 0);
    // printf("z0: %d\n", zombies2[0].map_row);



    // exit(0);

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

    // float w = map.cols*MAP_GRID_PXL_SIZE;
    // float h = map.rows*MAP_GRID_PXL_SIZE;
    // physics_set_pos_limits()
}

void deinit()
{
    shader_deinit();
    window_deinit();
}

// also checks if the mouse is off the screen
void camera_set()
{
    int mx, my;
    window_get_mouse_view_coords(&mx, &my);

    if(player.gun_ready)
    {
        float r = 0.2;  //should be <= 0.5 to make sense otherwise player will end up off of the screen
        float ox = (mx - view_width/2.0);
        float oy = (my - view_height/2.0);
        float xr = view_width*r;
        float yr = view_height*r;
        ox = 2.0*xr*(ox/view_width);
        oy = 2.0*yr*(oy/view_height);
        ox = RANGE(ox, -1.0*xr, xr);
        oy = RANGE(oy, -1.0*yr, yr);
        aim_camera_offset.x = ox;
        aim_camera_offset.y = oy;
    }
    else
    {
        aim_camera_offset.x = 0.0;
        aim_camera_offset.y = 0.0;
    }

    if(!window_is_cursor_enabled())
    {
        if(mx >= view_width || mx <= 0 || my >= view_height || my <= 0)
        {
            int new_mx = RANGE(mx, 0, view_width);
            int new_my = RANGE(my, 0, view_height);
            window_set_mouse_view_coords(new_mx, new_my);
        }
    }

    float cam_pos_x = player.phys.pos.x + aim_camera_offset.x;
    float cam_pos_y = player.phys.pos.y + aim_camera_offset.y;
    Rect cam_rect = {0};
    cam_rect.x = cam_pos_x;
    cam_rect.y = cam_pos_y;
    cam_rect.w = view_width;
    cam_rect.h = view_height;
    limit_pos(&map.rect, &cam_rect);
    camera_move(cam_rect.x, cam_rect.y, false);
}

void update(double delta_t)
{
    gfx_clear_lines();

    camera_set();
    camera_update();

    world_update();
    zombie_update(delta_t);
    player_update(delta_t);
    projectile_update(delta_t);

    // if(!window_is_cursor_enabled())
    //     window_set_mouse_world_coords(400.0,1000.0);

}

void draw()
{
    gfx_clear_buffer(50,50,50);

    world_draw();
    zombie_draw();
    projectile_draw();
    player_draw();

    gfx_draw_lines();
    gui_draw();

}


// lists
// -------------------------------------------------------------------------------

glist* list_create(void* buf, int max_count, int item_size)
{
    if(item_size <= 0 || max_count <= 1)
    {
        LOGE("Invalid item_size (%d) or max_count (%d) for list", item_size, max_count);
        return NULL;
    }
    if(buf == NULL)
    {
        LOGE("List buffer is NULL");
        return NULL;
    }

    glist* list = calloc(1, sizeof(glist));
    list->count = 0;
    list->max_count = max_count;
    list->item_size = item_size;
    list->buf = buf;
    // if(list->buf == NULL)
    // {
    //     LOGI("Allocating %d bytes for list %p", max_count*item_size, list);
    //     list->buf = calloc(max_count, item_size);
    // }
    return list;
}

void list_delete(glist* list)
{
    if(list != NULL) free(list);
}

bool list_add(glist* list, void* item)
{
    if(list == NULL)
        return false;

    if(list->count >= list->max_count)
        return false;

    memcpy(list->buf + list->count*list->item_size, item, list->item_size);
    list->count++;
    return true;
}

bool list_remove(glist* list, int index)
{
    if(list == NULL)
        return false;

    if(index >= list->count)
        return false;

    memcpy(list->buf + index*list->item_size, list->buf+(list->count-1)*list->item_size, list->item_size);
    list->count--;
}

void* list_get(glist* list, int index)
{
    if(list == NULL)
        return NULL;

    return list->buf + index*list->item_size;
}






void limit_pos(Rect* limit, Rect* pos)
{
    // printf("-------------------------------------\n");
    // printf("map: "); print_rect(limit);
    // printf("before: "); print_rect(pos);
    float lx0 = limit->x - limit->w/2.0;
    float lx1 = lx0 + limit->w;
    float ly0 = limit->y - limit->h/2.0;
    float ly1 = ly0 + limit->h;

    float px0 = pos->x - pos->w/2.0;
    float px1 = px0 + pos->w;
    float py0 = pos->y - pos->h/2.0;
    float py1 = py0 + pos->h;

    if(px0 < lx0)
        pos->x = lx0+pos->w/2.0;
    if(px1 > lx1)
        pos->x = lx1-pos->w/2.0;
    if(py0 < ly0)
        pos->y = ly0+pos->h/2.0;
    if(py1 > ly1)
        pos->y = ly1-pos->h/2.0;
    // printf("after: "); print_rect(pos);
}