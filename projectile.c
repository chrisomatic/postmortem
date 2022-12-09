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
    memcpy(&proj->hurt_box_prior,&proj->hurt_box,sizeof(Rect));
    proj->hurt_box.x = proj->pos.x;
    proj->hurt_box.y = proj->pos.y;
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
    proj.pos.x = _x;
    proj.pos.y = _y;

    coords_to_map_grid(proj.pos.x, proj.pos.y, &proj.grid_pos.x, &proj.grid_pos.y);
    memcpy(&proj.grid_pos_prior, &proj.grid_pos, sizeof(Vector2i));

    Rect* vr = &gfx_images[projectile_image_set].visible_rects[proj.sprite_index];
    proj.hurt_box.w = vr->w;
    proj.hurt_box.h = vr->h;
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
        angle_deg = calc_angle_deg(proj.pos.x, proj.pos.y, mx, my);
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

        proj->pos.x += delta_t*proj->vel.x;
        proj->pos.y -= delta_t*proj->vel.y; // @minus

        memcpy(&proj->grid_pos_prior, &proj->grid_pos, sizeof(Vector2i));
        coords_to_map_grid(proj->pos.x, proj->pos.y, &proj->grid_pos.x, &proj->grid_pos.y);

        update_hurt_box(proj);

        #define HITS_MAX 100
        int hits[HITS_MAX] = {0};
        int num_hits = 0;
        for(int j = zlist->count - 1; j >= 0; --j)
        {
            if(num_hits >= HITS_MAX) break;

            if(zombies[j].dead)
                continue;

            /*
            int x0 = proj->grid_pos_prior.x;
            int y0 = proj->grid_pos_prior.y;
            int x1 = proj->grid_pos.x;
            int y1 = proj->grid_pos.y;

            int dx = x1 - x0;
            int dy = y1 - y0;

            int cx = x0 + dx/2.0;
            int cy = y0 + dy/2.0;

            int radius = (ABS(dx) + ABS(dy))/2.0;
            printf("center %d %d, radius: %d\n",cx, cy, radius);

            if(!is_grid_within_radius(cx,cy,zombies[j].grid_pos.x, zombies[j].grid_pos.y,radius))
                continue;
            */

            if(are_rects_colliding(&proj->hurt_box_prior, &proj->hurt_box, &zombies[j].phys.hit))
            {
                hits[num_hits++] = j;
            }
        }

        if(num_hits > 0)
        {
            int j_min = 0;
            float min_d = INFINITY;
            for(int _j = 0; _j < num_hits; ++_j)
            {
                int j = hits[_j];
                float d = dist(proj->hurt_box_prior.x, proj->hurt_box_prior.y, zombies[j].phys.hit.x, zombies[j].phys.hit.y);
                if(d < min_d)
                {
                    min_d = d;
                    j_min = j;
                }
            }

            proj->dead = true;

            Vector2f force = {
                100.0*cosf(RAD(proj->angle_deg)),
                100.0*sinf(RAD(proj->angle_deg))
            };
            //zombie_push(j_min,&force);

            if(role == ROLE_SERVER)
            {
                printf("Zombie %d hurt for %d damage!\n",j_min,proj->damage);
            }

            // correct projectile pos back to zombie hitbox
            {
                Vector2f p0 = {proj->hurt_box.x, proj->hurt_box.y};
                Vector2f p1 = {proj->hurt_box_prior.x, proj->hurt_box_prior.y};
                Vector2f p2 = {zombies[j_min].phys.hit.x, zombies[j_min].phys.hit.y};

                float d = dist(p0.x,p0.y,p2.x,p2.y);

                Vector2f v = {p0.x - p1.x, p0.y - p1.y};
                normalize(&v);

                Vector2f correction = {-d*v.x, -d*v.y};

                proj->pos.x += correction.x;
                proj->pos.y += correction.y;

                update_hurt_box(proj);
            }

            ParticleEffect pe;
            memcpy(&pe,&particle_effects[EFFECT_BLOOD1],sizeof(ParticleEffect));

            pe.scale.init_min *= 0.5;
            pe.scale.init_max *= 0.5;
            pe.velocity_x.init_min = (proj->vel.x*0.03);
            pe.velocity_x.init_max = (proj->vel.x*0.03);
            pe.velocity_x.rate = -0.02;
            pe.velocity_y.init_min = -(proj->vel.y*0.03);
            pe.velocity_y.init_max = 0.0;
            pe.velocity_y.rate = -0.02;

            particles_spawn_effect(zombies[j_min].phys.pos.x, zombies[j_min].phys.pos.y, &pe, 0.6, true, false);


            zombie_hurt(j_min,proj->damage);
        }

        float x0 = proj->hurt_box.x;
        float y0 = proj->hurt_box.y;
        float x1 = proj->hurt_box_prior.x;
        float y1 = proj->hurt_box_prior.y;

        //printf("p0 (%f %f) -> p1 (%f %f)\n",x0,y0,x1,y1);
        gfx_add_line(x0,y0,x1,y1, 0x00FFFF00);
        gfx_add_line(x0+1,y0+1,x1+1,y1+1, 0x00555555);
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
            //gfx_add_line(proj->hurt_box.x + proj->hurt_box.w/2.0, proj->hurt_box.y + proj->hurt_box.h/2.0, proj->hurt_box_prior.x + proj->hurt_box_prior.w/2.0, proj->hurt_box_prior.y + proj->hurt_box_prior.h/2.0, 0x00FFFF00);

            //gfx_draw_image(projectile_image_set,proj->sprite_index,proj->pos.x,proj->pos.y, COLOR_TINT_NONE,1.0, proj->angle_deg, 1.0, false,true);

            if(debug_enabled)
            {
                gfx_draw_rect(&proj->hurt_box, 0x0000FFFF, 0.0, 1.0,1.0, false, true);
            }
        }
    }
}
