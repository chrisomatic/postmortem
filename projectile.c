#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "math2d.h"
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
    // Rect hurt_box_temp;  //TODO
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
    GFXSubImageData* sid = gfx_images[projectile_image_set].sub_img_data;
    float img_w = sid->element_width;
    float img_h = sid->element_height;
    Rect* vr = &sid->visible_rects[proj->sprite_index];
    // printf("%.0f, %.0f, %.0f, %.0f\n", vr->x, vr->y, vr->w, vr->h);

    memcpy(&proj->hurt_box_prior,&proj->hurt_box,sizeof(Rect));

    // proj->hurt_box.x = proj->pos.x;
    // proj->hurt_box.y = proj->pos.y;
    // proj->hurt_box.w = vr->w;
    // proj->hurt_box.h = vr->h;


    //TODO: calculate this stuff once and then translate the box in here
    //TODO: refactor this for 

    // proj->angle_deg = 45.0;
    // proj->angle_deg = 90.0;

    // top left, bottom left, top right, bottom right
    float xcoords[4] = {proj->pos.x, proj->pos.x,       proj->pos.x+vr->w,  proj->pos.x+vr->w};
    float ycoords[4] = {proj->pos.y, proj->pos.y+vr->h, proj->pos.y,        proj->pos.y+vr->h};
    float xmin,xmax,ymin,ymax;
    float xrange = rangef(xcoords, 4, &xmin, &xmax);
    float yrange = rangef(ycoords, 4, &ymin, &ymax);
    float xcenter = xcoords[0]+xrange/2.0;
    float ycenter = ycoords[0]+yrange/2.0;

    float xrcoords[4] = {0};
    float yrcoords[4] = {0};

    float a = RAD(360-proj->angle_deg);
    float xa = cos(a);
    float ya = sin(a);

    for(int i = 0; i < 4; ++i)
    {
        xrcoords[i] = (xa * (xcoords[i] - xcenter) - ya * (ycoords[i] - ycenter)) + xcenter;
        yrcoords[i] = (ya * (xcoords[i] - xcenter) - xa * (ycoords[i] - ycenter)) + ycenter;
    }

    float xrmin,xrmax,yrmin,yrmax;
    float xrrange = rangef(xrcoords, 4, &xrmin, &xrmax);
    float yrrange = rangef(yrcoords, 4, &yrmin, &yrmax);

    proj->hurt_box.x = xrmin;
    proj->hurt_box.y = yrmin;
    proj->hurt_box.w = xrrange;
    proj->hurt_box.h = yrrange;


    // if(&projectiles[projectile_count-1] == proj)
    // {
    //     // printf("x0 %.2f\n", (-vr->w/2.0)*xa);
    //     // printf("deg: %.2f (%.2f), xa: %.2f, ya: %.2f\n", proj->angle_deg, a, xa, ya);
    //     printf("center: %.1f, %.1f\n", xcenter, ycenter);
    //     printf("p: %.1f, %.1f, %.0f, %.0f\n", proj->pos.x, proj->pos.y, vr->w, vr->h);
    //     printf("xcoords: %.1f, %.1f, %.1f, %.1f\n", xcoords[0], xcoords[1],xcoords[2],xcoords[3]);
    //     printf("ycoords: %.1f, %.1f, %.1f, %.1f\n", ycoords[0], ycoords[1],ycoords[2],ycoords[3]);
    //     printf("xrcoords: %.1f, %.1f, %.1f, %.1f\n", xrcoords[0], xrcoords[1],xrcoords[2],xrcoords[3]);
    //     printf("yrcoords: %.1f, %.1f, %.1f, %.1f\n", yrcoords[0], yrcoords[1],yrcoords[2],yrcoords[3]);
    //     printf("x: %.2f, %.2f, %.2f\n", xrmin,xrmax,xrrange);
    //     printf("y: %.2f, %.2f, %.2f\n", yrmin,yrmax,yrrange);
    //     // printf("xrmin: %.2f\n", floor(xrmin));
    // }


}

void projectile_init()
{
    projectile_image_set = gfx_load_image_set("img/projectile_set.png",32,32);
}

void projectile_add(int sprite_index, Gun* gun, float x, float y, float angle)
{
    // angle += PI;
    if(projectile_count >= MAX_PROJECTILES)
    {
        LOGW("Too many projectiles!");
        return;
    }

    Projectile* proj = &projectiles[projectile_count];
    projectile_count++;

    memset(proj,0, sizeof(Projectile));

    float speed = gun->fire_speed;
    // speed = 0.1;

    proj->sprite_index = sprite_index;
    proj->damage = gun->power + proj->power;
    proj->pos.x = x;
    proj->pos.y = y;
    proj->w = 4;
    proj->h = 4;
    proj->angle_deg = DEG(angle);
    proj->vel.x = speed*cosf(angle);
    proj->vel.y = speed*sinf(angle);
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
        proj->pos.y -= delta_t*proj->vel.y; // @minus

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

                Vector2f force = {
                    100.0*cosf(RAD(proj->angle_deg)),
                    100.0*sinf(RAD(proj->angle_deg))
                };
                //zombie_push(j,&force);

                zombie_hurt(j,proj->damage);
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
            // gfx_draw_sub_image(projectile_image_set,proj->sprite_index,proj->pos.x,proj->pos.y, COLOR_TINT_NONE,1.0,proj->angle_deg-180,1.0);
            gfx_draw_sub_image(projectile_image_set,proj->sprite_index,proj->pos.x,proj->pos.y, COLOR_TINT_NONE,1.0, proj->angle_deg, 1.0);

            if(debug_enabled)
            {
                gfx_draw_rect(&proj->hurt_box, 0x0000FFFF, 1.0,1.0);
            }
        }
    }
}
