#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "window.h"
#include "rat_math.h"
#include "gfx.h"
#include "world.h"

#include "zombie.h"

Zombie zombies[MAX_ZOMBIES] = {0};
int num_zombies = 0;

static int zombie_image;

static void sort_zombies(Zombie arr[], int n)
{
    // insertion sort
    int i, j;
    Zombie key;
    for (i = 1; i < n; i++) 
    {
        memcpy(&key,&arr[i],sizeof(Zombie));
        j = i - 1;

        while (j >= 0 && arr[j].pos.y >= arr[i].pos.y)
        {
            memcpy(&arr[j+1],&arr[j], sizeof(Zombie));
            j = j - 1;
        }
        memcpy(&arr[j+1],&key, sizeof(Zombie));
    }
}

static void wander(Zombie* zom, float delta_t)
{
    zom->action_timer+=delta_t;

    if(zom->action_timer >= zom->action_timer_max)
    {
        zom->action_timer = 0.0;//zom->action_timer_max;
        zom->action = rand() % ZOMBIE_ACTION_MAX;
        zom->action_timer_max = (rand() % 100)/20.0 + 0.5; // 0.5 to 5.0 seconds
    }

    float amt = zom->speed*delta_t;

    switch(zom->action)
    {
        case ZOMBIE_ACTION_NONE:
            break;
        case ZOMBIE_ACTION_MOVE_UP:
            zom->pos.y -= amt;//zom->speed;
            break;
        case ZOMBIE_ACTION_MOVE_DOWN:
            zom->pos.y += amt; //zom->speed;
            break;
        case ZOMBIE_ACTION_MOVE_LEFT:
            zom->pos.x -= amt; //zom->speed;
            break;
        case ZOMBIE_ACTION_MOVE_RIGHT:
            zom->pos.x += amt; //zom->speed;
            break;
    }
}

void zombie_init()
{
    zombie_image = gfx_load_image("img/zombie_f1.png");

    for(int i = 0; i < 100; ++i)
    {
        zombies[i].pos.x = rand() % 800;
        zombies[i].pos.y = rand() % 600;
        zombies[i].w = gfx_images[zombie_image].w;
        zombies[i].h = gfx_images[zombie_image].h;
        zombies[i].action = ZOMBIE_ACTION_NONE;
        zombies[i].action_timer = 0;
        zombies[i].action_timer_max = (rand() % 100)/20.0 + 0.5;
        zombies[i].speed = 32.0;
    }

    num_zombies = 100;
}

static void update_zombie_boxes(Zombie* zom)
{
    const float shrink_factor = 0.80;

    zom->collision_box.x = zom->pos.x;
    zom->collision_box.y = zom->pos.y + (2.0*zom->h / 3.0);
    zom->collision_box.w = zom->w;
    zom->collision_box.h = (zom->h / 3.0);

    zom->collision_box.x += 0.5*zom->collision_box.w*(1.00 - shrink_factor);
    zom->collision_box.y += 0.5*zom->collision_box.h*(1.00 - shrink_factor);
    zom->collision_box.w *= shrink_factor;
    zom->collision_box.h *= shrink_factor;

    zom->hit_box.x = zom->pos.x;
    zom->hit_box.y = zom->pos.y;
    zom->hit_box.w = zom->w;
    zom->hit_box.h = (zom->h / 1.5);

    zom->hit_box.x += 0.5*zom->hit_box.w*(1.00 - shrink_factor);
    zom->hit_box.y += 0.5*zom->hit_box.h*(1.00 - shrink_factor);
    zom->hit_box.w *= shrink_factor;
    zom->hit_box.h *= shrink_factor;
}

void zombie_update(float delta_t)
{
    for(int i = 0; i < num_zombies; ++i)
    {
        wander(&zombies[i], delta_t);

        zombies[i].pos.x = MAX(zombies[i].pos.x, 0.0);
        zombies[i].pos.y = MAX(zombies[i].pos.y, 0.0);

        update_zombie_boxes(&zombies[i]);
    }

    sort_zombies(zombies,num_zombies);
}

void zombie_draw()
{
    for(int i = 0; i < num_zombies; ++i)
    {
        Zombie* zom = &zombies[i];

        Rect* cbox  = &zom->collision_box;
        Rect* hbox  = &zom->hit_box;

        Rect r = {
            .x = zom->pos.x,
            .y = zom->pos.y,
            .w = zom->w,
            .h = zom->h
        };

        if(is_in_camera_view(&r))
        {
            gfx_draw_image(zombie_image,(int)zom->pos.x,(int)zom->pos.y, COLOR_TINT_NONE,1.0,0.0,1.0);
            gfx_draw_rect(cbox->x, cbox->y, cbox->w, cbox->h, 0x0000FF00, 1.0,1.0);
            gfx_draw_rect(hbox->x, hbox->y, hbox->w, hbox->h, 0x00FFFF00, 1.0,1.0);
        }

    }
}
