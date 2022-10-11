#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "rat_math.h"
#include "window.h"
#include "world.h"
#include "gfx.h"
#include "log.h"
#include "gun.h"
#include "zombie.h"
#include "projectile.h"

#define MAX_PROJECTILES 1024

typedef struct
{
    ProjectileType type;
    Vector2f pos;
    Vector2f vel;
    float w,h;
    float angle_deg;
    int damage;
    int sprite_index;
} Projectile;

static int projectile_image_set;

static Projectile projectiles[MAX_PROJECTILES];
static int projectile_count = 0;

static void projectile_remove(int index)
{
    if(index < 0 || index >= projectile_count)
    {
        LOGE("Projectile index out of range (%d)", index);
        return;
    }

    memcpy(&projectiles[index], &projectiles[projectile_count-1], sizeof(Projectile));

    projectile_count--;
}

void projectile_init()
{
    projectile_image_set = gfx_load_image_set("img/projectile_set.png",32,32);
}

void projectile_add(int sprite_index, Gun* gun, float x, float y, float angle)
{
    if(projectile_count >= MAX_PROJECTILES)
    {
        LOGW("Too many projectiles!");
        return;
    }

    Projectile* proj = &projectiles[projectile_count];
    projectile_count++;

    memset(proj,0, sizeof(Projectile));

    float speed = gun->fire_power;

    proj->sprite_index = sprite_index;
    proj->damage = gun->fire_power;
    proj->pos.x = x;
    proj->pos.y = y;
    proj->w = 2;
    proj->h = 2;
    proj->angle_deg = DEG(angle-PI);
    proj->vel.x = speed*cos(angle-PI);
    proj->vel.y = speed*sin(angle-PI);


}

void projectile_update(float delta_t)
{
    for(int i = 0; i < projectile_count; ++i)
    {
        Projectile* proj = &projectiles[i];

        proj->pos.x += delta_t*proj->vel.x;
        proj->pos.y += delta_t*proj->vel.y;

        for(int j = 0; j < num_zombies; ++j)
        {
            Rect p = {
                .x = proj->pos.x,
                .y = proj->pos.y,
                .w = proj->w,
                .h = proj->h
            };

            if(rectangles_colliding(&p, &zombies[j].hit_box))
            {
                projectile_remove(i);
            }
        }

    }

}

void projectile_draw()
{
    for(int i = 0; i < projectile_count; ++i)
    {
        Projectile* proj = &projectiles[i];

        Rect p = {
            .x = proj->pos.x,
            .y = proj->pos.y,
            .w = proj->w,
            .h = proj->h
        };

        if(is_in_camera_view(&p))
        {
            gfx_draw_sub_image(projectile_image_set,proj->sprite_index,proj->pos.x,proj->pos.y, COLOR_TINT_NONE,1.0,proj->angle_deg,1.0);
        }
    }
}
