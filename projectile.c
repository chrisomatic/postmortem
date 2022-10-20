#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "math2d.h"
#include "window.h"
#include "camera.h"
#include "world.h"
#include "gfx.h"
#include "log.h"
#include "gun.h"
#include "player.h"
#include "zombie.h"
#include "projectile.h"

typedef struct
{
    ProjectileType type;
    Vector2f pos;
    Vector2f vel;
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


#define MAX_PROJECTILES 1024
static Projectile projectiles[MAX_PROJECTILES];
glist* plist = NULL;

static int projectile_image_set;



static void projectile_remove(int index)
{
    list_remove(plist, index);
    // if(index < 0 || index >= plist->count)
    // {
    //     LOGE("Projectile index out of range (%d)", index);
    //     return;
    // }

    // memcpy(&projectiles[index], &projectiles[plist->count-1], sizeof(Projectile));

    // plist->count--;
}

static void update_hurt_box(Projectile* proj)
{
    memcpy(&proj->hurt_box_prior,&proj->hurt_box,sizeof(Rect));
    proj->hurt_box.x = proj->pos.x;
    proj->hurt_box.y = proj->pos.y;
}

void projectile_init()
{
    plist = list_create((void*)projectiles, MAX_PROJECTILES, sizeof(Projectile));

    projectile_image_set = gfx_load_image_set("img/projectile_set.png",32,32);
}


void projectile_add(int sprite_index, Gun* gun, float angle_offset)
{
    // if(plist->count >= MAX_PROJECTILES)
    // {
    //     LOGW("Too many projectiles!");
    //     return;
    // }

    // Projectile* proj = &projectiles[plist->count];
    // plist->count++;


    Projectile proj = {0};

    // memset(proj,0, sizeof(Projectile));

    float speed = gun->fire_speed;
    // speed = 10.0;

    proj.sprite_index = sprite_index;
    proj.damage = gun->power + proj.power;
    proj.dead = false;

    // //spawn at the end of the gun, (could subtract bullet width/height to make spawn inside of gun)
    // float _x = gun->pos.x + (gun->visible_rect.w/2.0)*cosf(angle);
    // float _y = gun->pos.y + (gun->visible_rect.h/2.0)*sinf(PI*2-angle);
    float _x = gun->pos.x;
    float _y = gun->pos.y;
    proj.pos.x = _x;
    proj.pos.y = _y;

    float mx, my;
    window_get_mouse_world_coords(&mx, &my);
    float angle_deg = angle_offset + calc_angle_deg(proj.pos.x, proj.pos.y, mx, my);
    float angle = RAD(angle_deg);

    proj.angle_deg = angle_deg;
    proj.vel.x = speed*cosf(angle);
    proj.vel.y = speed*sinf(angle);
    proj.time = 0.0;
    float vel = sqrt(proj.vel.x*proj.vel.x + proj.vel.y*proj.vel.y);
    proj.ttl  = 1.0 / (vel / gun->fire_range);

    GFXSubImageData* sid = gfx_images[projectile_image_set].sub_img_data;
    Rect* vr = &sid->visible_rects[proj.sprite_index];

    //TODO: refactor this for reusable axis aligned boxes
    Rect r = {0};
    Rect r_rot = {0};
    RectXY rxy_rot = {0};
    r.x = proj.pos.x;
    r.y = proj.pos.y;
    r.w = vr->w;
    r.h = vr->h;

    rotate_rect(&r, proj.angle_deg, r.x, r.y, &rxy_rot);
    rectxy_to_rect(&rxy_rot, &proj.hurt_box);
    // memcpy(&proj.hurt_box_prior, &proj.hurt_box, sizeof(proj.hurt_box));
    update_hurt_box(&proj);

    list_add(plist, (void*)&proj);
}

void projectile_update(float delta_t)
{
    for(int i = plist->count - 1; i >= 0; --i)
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

        for(int j = 0; j < zlist->count; ++j) // num_zombies
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

    for(int i = plist->count - 1; i >= 0; --i)
    {
        if(projectiles[i].dead)
        {
            projectile_remove(i);
        }
    }

}

void projectile_draw()
{
    for(int i = 0; i < plist->count; ++i)
    {
        Projectile* proj = &projectiles[i];

        if(is_in_camera_view(&proj->hurt_box))
        {
            gfx_draw_sub_image(projectile_image_set,proj->sprite_index,proj->pos.x,proj->pos.y, COLOR_TINT_NONE,1.0, proj->angle_deg, 1.0);

            if(debug_enabled)
            {
                gfx_draw_rect(&proj->hurt_box, 0x0000FFFF, 1.0,1.0, false, true);
            }
        }
    }
}
