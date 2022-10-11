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
#include "player.h"
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
    float power;
    int damage;
    int sprite_index;
    Rect hurt_box;
    Rect hurt_box_prior;
    float time;
    float ttl;
    bool dead;
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

static void update_hurt_box(Projectile* proj)
{
    float img_w = gfx_images[projectile_image_set].element_width;
    float img_h = gfx_images[projectile_image_set].element_height;

    memcpy(&proj->hurt_box_prior,&proj->hurt_box,sizeof(Rect));

    proj->hurt_box.x = proj->pos.x + (img_w - proj->w)/2.0;
    proj->hurt_box.y = proj->pos.y + (img_h - proj->h)/2.0;
    proj->hurt_box.w = proj->w;
    proj->hurt_box.h = proj->h;
}

static bool is_colliding(Rect* a, Rect* b)
{
    bool overlap = (
        a->x < (b->x+b->w) && (a->x+a->w) > b->x &&
        a->y < (b->y+b->h) && (a->y+a->h) > b->y
    );

    return overlap;
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

    float speed = gun->fire_speed;

    proj->sprite_index = sprite_index;
    proj->damage = gun->power + proj->power;
    proj->pos.x = x;
    proj->pos.y = y;
    proj->w = 4;
    proj->h = 4;
    proj->angle_deg = DEG(angle-PI);
    proj->vel.x = speed*cos(angle-PI);
    proj->vel.y = speed*sin(angle-PI);
    proj->dead = false;

    float vel = sqrt(proj->vel.x*proj->vel.x + proj->vel.y*proj->vel.y);

    proj->time = 0.0;
    proj->ttl  = 1.0 / (vel / gun->fire_range);

    update_hurt_box(proj);
}

void projectile_update(float delta_t)
{
    for(int i = projectile_count - 1; i >= 0; --i)
    {
        Projectile* proj = &projectiles[i];

        if(proj->dead)
            continue;

        proj->time += delta_t;
        if(proj->time >= proj->ttl)
        {
            proj->dead = true;
            continue;
        }

        proj->pos.x += delta_t*proj->vel.x;
        proj->pos.y += delta_t*proj->vel.y;

        update_hurt_box(proj);

        for(int j = 0; j < num_zombies; ++j) // num_zombies
        {
#if 0
            if(is_colliding(&proj->hurt_box, &zombies[j].hit_box))
            {
                projectile_remove(i);
            }
#else
            if(are_rects_colliding(&proj->hurt_box_prior, &proj->hurt_box, &zombies[j].hit_box))
            {
                //printf("Bullet collided!\n");
                proj->dead = true;
            }
#endif
        }
    }

    for(int i = projectile_count - 1; i >= 0; --i)
    {
        if(projectiles[i].dead)
        {
            projectile_remove(i);
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

            if(debug_enabled)
            {
                gfx_draw_rect(&proj->hurt_box, 0x0000FFFF, 1.0,1.0);
            }
        }
    }
}
