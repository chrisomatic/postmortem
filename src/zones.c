#include "headers.h"
#include "main.h"
#include "gfx.h"
#include "player.h"
#include "math2d.h"
#include "window.h"
#include "zones.h"

#define ZONE_NAME_MAX 32
#define ZONE_DISPLAY_NAME_MAX_TIME 3.0
#define ZONE_COUNT 3

typedef struct
{
    int id;
    char name[ZONE_NAME_MAX+1];
    Rect rect;
} Zone;

Zone zones[ZONE_COUNT] = {
    {0, "Bunker",   {100.0,100.0,100.0,100.0}},
    {1, "Wasteland",{1200.0,1200.0,1000.0,1000.0}},
    {2, "Survivor Outpost",{300.0,500.0,300.0,300.0}}
};

static bool display_zone_name;
static int display_zone_id;
static float display_zone_name_time;
static float display_zone_name_max_time;

static void zones_check_for_player(Player* p);
static Zone* get_zone_by_id(int id);

void zones_init()
{
    display_zone_name = false;
    display_zone_name_time = 0.0;
    display_zone_name_max_time = ZONE_DISPLAY_NAME_MAX_TIME;
}

void zones_update(double delta_t)
{
    zones_check_for_player(player);

    if(display_zone_name)
    {
        display_zone_name_time += delta_t;
        if(display_zone_name_time >= display_zone_name_max_time)
        {
            display_zone_name = false;
            display_zone_name_time = 0.0;
        }
    }
}

void zones_draw()
{
    if(display_zone_name)
    {
        Zone* z = get_zone_by_id(display_zone_id);
        char* str = z == NULL ? "Unknown" : z->name;
        const float scale = 0.8;

        float opacity = 1.0;
        if(display_zone_name_time < 0.5)
        {
            opacity = (display_zone_name_time / 0.5);
        }
        else if(display_zone_name_time >= 0.5 && display_zone_name_time < 2.0)
        {
            opacity = 1.0;
        }
        else if(display_zone_name_time >= 2.0)
        {
            opacity = 1.0 - ((display_zone_name_time - 2.0) / (display_zone_name_max_time - 2.0));
        }

        Vector2f size = gfx_string_get_size(scale,str);
        gfx_draw_string((view_width - size.x)/2.0, 0.25*(view_height - size.y) , 0xFFFFFFFF, scale, 0.0, opacity, false, true, str);
    }

    if(debug_enabled)
    {
        for(int i = 0; i < ZONE_COUNT; ++i)
        {
            Zone* z = &zones[i];
            gfx_draw_rect(&z->rect, 0x00AAAAAA, 0.0,1.0,1.0, false, true);
        }
    }
}

static Zone* get_zone_by_id(int id)
{
    for(int i = 0; i < ZONE_COUNT; ++i)
    {
        if(zones[i].id == id)
        {
            return &zones[i];
        }
    }

    return NULL;
}

static void zones_check_for_player(Player* p)
{
    bool in_zone = false;
    for(int i = 0; i < ZONE_COUNT; ++i)
    {
        Zone* z = &zones[i];

        if(rectangles_colliding(&p->phys.pos,&z->rect))
        {
            in_zone = true;

            if(p->zone_id == z->id)
                continue;

            p->zone_id = z->id;

            if(display_zone_name && display_zone_id == z->id)
                continue;

            display_zone_id = z->id;
            display_zone_name_time = 0.0;
            display_zone_name = true;
        }
    }

    if(!in_zone)
    {
        p->zone_id = -1;
    }
}
