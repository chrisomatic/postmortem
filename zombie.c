#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "window.h"
#include "math2d.h"
#include "player.h"
#include "gfx.h"
#include "world.h"
#include "log.h"

#include "zombie.h"

Zombie zombies[MAX_ZOMBIES] = {0};
int num_zombies = 0;

static int zombie_image;

static void zombie_remove(int index)
{
    memcpy(&zombies[index], &zombies[num_zombies-1], sizeof(Zombie));
    num_zombies--;
}

static void sort_zombies(Zombie arr[], int n)
{
    // insertion sort
    int i, j;
    Zombie key;
    for (i = 1; i < n; i++) 
    {
        memcpy(&key,&arr[i],sizeof(Zombie));
        j = i - 1;

        while (j >= 0 && arr[j].phys.pos.y >= arr[i].phys.pos.y)
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
        zom->action_timer_max = (rand() % 100)/50.0 + 0.5; // 0.5 to 2.0 seconds
    }


}

void zombie_init()
{
    zombie_image = gfx_load_image("img/zombie_f1.png", true, false);

    num_zombies = 100;

    for(int i = 0; i < num_zombies; ++i)
    {
        zombies[i].phys.pos.x = rand() % view_width;
        zombies[i].phys.pos.y = rand() % view_height;
        zombies[i].phys.max_linear_vel = 128.0;
        zombies[i].push_vel.x = 0.0;
        zombies[i].push_vel.y = 0.0;
        zombies[i].w = gfx_images[zombie_image].w;
        zombies[i].h = gfx_images[zombie_image].h;
        memcpy(&zombies[i].visible_rect, &gfx_images[zombie_image].visible_rect, sizeof(Rect));
        zombies[i].hp_max = 3;
        zombies[i].hp = zombies[i].hp_max;
        zombies[i].action = ZOMBIE_ACTION_NONE;
        zombies[i].action = rand() % ZOMBIE_ACTION_MAX;
        zombies[i].action_timer = 0;
        zombies[i].action_timer_max = (rand() % 100)/20.0 + 0.5;
        zombies[i].speed = 16.0;
    }

}

static void update_zombie_boxes(Zombie* zom)
{
    const float shrink_factor = 0.80;

    Rect* vr = &zom->visible_rect;

    float x0 = zom->phys.pos.x + vr->x;
    float y0 = zom->phys.pos.y + vr->y;
    float w = vr->w;
    float h = vr->h;

    float collision_height = h*0.4;
    zom->collision_box.x = x0;
    zom->collision_box.y = y0+h-collision_height;
    zom->collision_box.w = w;
    zom->collision_box.h = collision_height;

    float hit_box_height = h*0.5;
    zom->hit_box.x = x0;
    zom->hit_box.y = y0;
    zom->hit_box.w = w;
    zom->hit_box.h = hit_box_height;

    // zom->collision_box.y = zom->phys.pos.y + (2.0*zom->h / 3.0);
    // zom->collision_box.w = zom->w;
    // zom->collision_box.h = (zom->h / 3.0);

    // zom->collision_box.x = zom->phys.pos.x;
    // zom->collision_box.y = zom->phys.pos.y + (2.0*zom->h / 3.0);
    // zom->collision_box.w = zom->w;
    // zom->collision_box.h = (zom->h / 3.0);

    // zom->collision_box.x += 0.5*zom->collision_box.w*(1.00 - shrink_factor);
    // zom->collision_box.y += 0.5*zom->collision_box.h*(1.00 - shrink_factor);
    // zom->collision_box.w *= shrink_factor;
    // zom->collision_box.h *= shrink_factor;

    // zom->hit_box.x = zom->phys.pos.x+zom->visible_rect.x;
    // zom->hit_box.y = zom->phys.pos.y+zom->visible_rect.y;
    // zom->hit_box.w = zom->visible_rect.w;
    // zom->hit_box.h = zom->visible_rect.h;



    float x = zom->collision_box.x + zom->collision_box.w/2.0;
    float y = zom->collision_box.y + zom->collision_box.h;
    coords_to_map_grid(x, y, &zom->map_row, &zom->map_col);
    coords_to_map_grid(x, y, &zom->world_row, &zom->world_col);

    // zom->hit_box.x = zom->phys.pos.x;
    // zom->hit_box.y = zom->phys.pos.y;
    // zom->hit_box.w = zom->w;
    // zom->hit_box.h = (zom->h / 1.5);

    // zom->hit_box.x += 0.5*zom->hit_box.w*(1.00 - shrink_factor);
    // zom->hit_box.y += 0.5*zom->hit_box.h*(1.00 - shrink_factor);
    // zom->hit_box.w *= shrink_factor;
    // zom->hit_box.h *= shrink_factor;
}

static void zombie_die(int index)
{
    zombie_remove(index);
}

void zombie_update(float delta_t)
{
    for(int i = 0; i < num_zombies; ++i)
    {
        Zombie* zom = &zombies[i];
        wander(zom, delta_t);

        Vector2f accel = {0.0,0.0};
        float amt = zom->speed;

        switch(zom->action)
        {
            case ZOMBIE_ACTION_NONE:
                break;
            case ZOMBIE_ACTION_MOVE_UP:
                accel.y -= amt;
                break;
            case ZOMBIE_ACTION_MOVE_UP_RIGHT:
                accel.x += amt;
                accel.y -= amt;
                break;
            case ZOMBIE_ACTION_MOVE_RIGHT:
                accel.x += amt;
                break;
            case ZOMBIE_ACTION_MOVE_DOWN_RIGHT:
                accel.x += amt;
                accel.y += amt;
                break;
            case ZOMBIE_ACTION_MOVE_DOWN:
                accel.y += amt;
                break;
            case ZOMBIE_ACTION_MOVE_DOWN_LEFT:
                accel.x -= amt;
                accel.y += amt;
                break;
            case ZOMBIE_ACTION_MOVE_LEFT:
                accel.x -= amt;
                break;
            case ZOMBIE_ACTION_MOVE_UP_LEFT:
                accel.x -= amt;
                accel.y -= amt;
                break;
        }

        if(zom->push_vel.x > 0.0)
        {
            accel.x += zom->push_vel.x;
        }
        if(zom->push_vel.y > 0.0)
        {
            accel.y += zom->push_vel.y;
        }

        physics_begin(&zom->phys);
        physics_add_friction(&zom->phys, 16.0);
        physics_add_force(&zom->phys, accel.x, accel.y);
        physics_simulate(&zom->phys, delta_t);

        //zombies[i].pos.x = MAX(zombies[i].pos.x, 0.0);
        //zombies[i].pos.y = MAX(zombies[i].pos.y, 0.0);

        update_zombie_boxes(&zombies[i]);
    }

    sort_zombies(zombies,num_zombies);
}

void zombie_draw()
{
    for(int i = 0; i < num_zombies; ++i)
    {
        Zombie* zom = &zombies[i];

        Rect r = {
            .x = zom->phys.pos.x,
            .y = zom->phys.pos.y,
            .w = zom->w,
            .h = zom->h
        };

        if(is_in_camera_view(&r))
        {
            gfx_draw_image(zombie_image,(int)zom->phys.pos.x,(int)zom->phys.pos.y, COLOR_TINT_NONE,1.0,0.0,1.0);

            if(debug_enabled)
            {
                Rect* cbox  = &zom->collision_box;
                Rect* hbox  = &zom->hit_box;

                gfx_draw_rect(cbox, 0x0000FF00, 1.0,1.0);
                gfx_draw_rect(hbox, 0x00FFFF00, 1.0,1.0);
            }
        }

    }
}

void zombie_push(int index, Vector2f* force)
{
    if(index < 0 || index >= num_zombies)
    {
        LOGW("Zombie index %d is out of range",index);
        return;
    }

    Zombie* zom = &zombies[index];

    zom->push_vel.x = force->x;
    zom->push_vel.y = force->y;
}

void zombie_hurt(int index, float val)
{
    if(index < 0 || index >= num_zombies)
    {
        LOGW("Zombie index %d is out of range",index);
        return;
    }

    Zombie* zom = &zombies[index];

    zom->hp -= val;
    if(zom->hp < 0.0)
    {
        zombie_die(index);
    }
}
