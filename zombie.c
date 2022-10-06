#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "window.h"
#include "rat_math.h"
#include "gfx.h"

#include "zombie.h"

#define MAX_ZOMBIES 1024

static Zombie zombies[MAX_ZOMBIES] = {0};
static int num_zombies = 0;

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
        zombies[i].action = ZOMBIE_ACTION_NONE;
        zombies[i].action_timer = 0;
        zombies[i].action_timer_max = (rand() % 100)/20.0 + 0.5;
        zombies[i].speed = 32.0;
    }

    num_zombies = 100;
}

void zombie_update(float delta_t)
{
    for(int i = 0; i < num_zombies; ++i)
    {
        wander(&zombies[i], delta_t);

        zombies[i].pos.x = MAX(zombies[i].pos.x, 0.0);
        zombies[i].pos.y = MAX(zombies[i].pos.y, 0.0);
    }

    sort_zombies(zombies,num_zombies);
}

void zombie_draw()
{
    for(int i = 0; i < num_zombies; ++i)
    {
        gfx_draw_image(zombie_image,(int)zombies[i].pos.x,(int)zombies[i].pos.y, COLOR_TINT_NONE,1.0,0.0,1.0);
    }
}
