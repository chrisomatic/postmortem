#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "camera.h"
#include "world.h"
#include "gfx.h"
#include "log.h"
#include "player.h"
#include "zombie.h"
#include "effects.h"
#include "particles.h"
#include "projectile.h"




Projectile projectiles[MAX_PROJECTILES];
glist* plist = NULL;

static int projectile_image_set;



static void projectile_remove(int index)
{
    list_remove(plist, index);
}

static void update_hurt_box(Projectile* proj)
{
    memcpy(&proj->phys.prior_collision,&proj->phys.collision,sizeof(Rect));
    proj->phys.collision.x = proj->phys.pos.x;
    proj->phys.collision.y = proj->phys.pos.y;
}

void projectile_init()
{
    plist = list_create((void*)projectiles, MAX_PROJECTILES, sizeof(Projectile));
    projectile_image_set = gfx_load_image("img/projectile_set.png", false, false, 32, 32, NULL);
}

void projectile_add(Player* p, Gun* gun, float angle_offset)
{
    Projectile proj = {0};

    float speed = gun->fire_speed;
    // speed = 10.0;

    proj.sprite_index = gun->projectile_type;
    proj.damage = gun->power + proj.power;
    proj.dead = false;

    float _x = gun->pos.x;
    float _y = gun->pos.y;
    proj.phys.pos.x = _x;
    proj.phys.pos.y = _y;

    coords_to_map_grid(proj.phys.pos.x, proj.phys.pos.y, &proj.grid_pos.x, &proj.grid_pos.y);
    memcpy(&proj.grid_pos_prior, &proj.grid_pos, sizeof(Vector2i));

    Rect* vr = &gfx_images[projectile_image_set].visible_rects[proj.sprite_index];
    proj.phys.collision.w = vr->w;
    proj.phys.collision.h = vr->h;
    update_hurt_box(&proj);

    int mx = p->mouse_x;
    int my = p->mouse_y;
    int px = p->phys.actual_pos.x;
    int py = p->phys.actual_pos.y;

    Rect mouse_r = {0};
    mouse_r.x = mx;
    mouse_r.y = my;
    mouse_r.w = 1;
    mouse_r.h = 1;

    // float d = dist(proj.pos.x, proj.pos.y, mx, my);

    float angle_deg = 0.0;
    // if(rectangles_colliding(&mouse_r, &p->phys.actual_pos) || dist(px, py, mx, my) <= 40.0)
    // if(rectangles_colliding(&mouse_r, &p->phys.actual_pos))
    if(dist(px, py, mx, my) <= 40.0)
    {
        // player_colors[p->index] = COLOR_RED;
        angle_deg = DEG(p->angle);
    }
    else
    {
        // player_colors[p->index] = COLOR_BLUE;
        angle_deg = calc_angle_deg(proj.phys.pos.x, proj.phys.pos.y, mx, my);
    }
    angle_deg += angle_offset;

    float angle = RAD(angle_deg);

    proj.angle_deg = angle_deg;
    proj.vel.x = speed*cosf(angle);
    proj.vel.y = speed*sinf(angle);
    proj.time = 0.0;
    float vel = sqrt(proj.vel.x*proj.vel.x + proj.vel.y*proj.vel.y);
    proj.ttl  = 1.0 / (vel / gun->fire_range);

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

        proj->phys.pos.x += delta_t*proj->vel.x;
        proj->phys.pos.y -= delta_t*proj->vel.y; // @minus

        memcpy(&proj->grid_pos_prior, &proj->grid_pos, sizeof(Vector2i));
        coords_to_map_grid(proj->phys.pos.x, proj->phys.pos.y, &proj->grid_pos.x, &proj->grid_pos.y);

        update_hurt_box(proj);

        /*
        float x0 = proj->phys.collision.x;
        float y0 = proj->phys.collision.y;
        float x1 = proj->phys.prior_collision.x;
        float y1 = proj->phys.prior_collision.y;

        //printf("p0 (%f %f) -> p1 (%f %f)\n",x0,y0,x1,y1);
        gfx_add_line(x0,y0,x1,y1, 0x00FFFF00);
        gfx_add_line(x0+1,y0+1,x1+1,y1+1, 0x00555555);
        */
        ParticleEffect pe;
        memcpy(&pe,&particle_effects[EFFECT_BULLET_TRAIL],sizeof(ParticleEffect));

        pe.velocity_x.init_min = proj->phys.vel.x;
        pe.velocity_x.init_max = proj->phys.vel.x;

        pe.velocity_y.init_min = proj->phys.vel.y;
        pe.velocity_y.init_max = proj->phys.vel.y;

        //particles_spawn_effect(proj->phys.pos.x, proj->phys.pos.y, &pe, 0.6, true, false);
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

        if(is_in_camera_view(&proj->phys.collision))
        {
            //gfx_add_line(proj->phys.collision.x + proj->phys.collision.w/2.0, proj->phys.collision.y + proj->phys.collision.h/2.0, proj->phys.prior_collision.x + proj->phys.prior_collision.w/2.0, proj->phys.prior_collision.y + proj->phys.prior_collision.h/2.0, 0x00FFFF00);

            //gfx_draw_image(projectile_image_set,proj->sprite_index,proj->phys.pos.x,proj->phys.pos.y, COLOR_TINT_NONE,1.0, proj->angle_deg, 1.0, false,true);

            if(debug_enabled)
            {
                gfx_draw_rect(&proj->phys.collision, 0x0000FFFF, 0.0, 1.0,1.0, false, true);
            }
        }
    }
}
