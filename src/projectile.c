#include "headers.h"
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
    projectile_image_set = gfx_load_image("src/img/projectile_set.png", false, false, 32, 32);
}

void projectile_add(Player* p, Gun* gun, float angle_offset)
{
    Projectile proj = {0};

    proj.player = p;

    // float speed = gun->fire_speed;
    float speed = gun->fire_speed*10000.0;
    //speed = 600.0;

    proj.sprite_index = 26;//gun->projectile_type;
    proj.damage = gun->power + proj.power;
    proj.dead = false;

    float _x = gun->pos.x;
    float _y = gun->pos.y;
    proj.phys.pos.x = _x;
    proj.phys.pos.y = _y;

    coords_to_map_grid(proj.phys.pos.x, proj.phys.pos.y, &proj.grid_pos.x, &proj.grid_pos.y);
    memcpy(&proj.grid_pos_prior, &proj.grid_pos, sizeof(Vector2i));

    // Rect* vr = &gfx_images[projectile_image_set].visible_rects[proj.sprite_index];
    // proj.phys.collision.w = vr->w;
    // proj.phys.collision.h = vr->h;
    proj.phys.collision.w = 4.0;
    proj.phys.collision.h = 4.0;
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



        if(proj->time >= proj->ttl)
        {
            proj->dead = true;
            continue;
        }

        float _dt = RANGE(proj->ttl - proj->time, 0.0, delta_t);

        proj->time += _dt;

        proj->phys.pos.x += _dt*proj->vel.x;
        proj->phys.pos.y -= _dt*proj->vel.y; // @minus

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
        /*
        ParticleEffect pe;
        memcpy(&pe,&particle_effects[EFFECT_BULLET_TRAIL],sizeof(ParticleEffect));

        pe.velocity_x.init_min = proj->phys.vel.x;
        pe.velocity_x.init_max = proj->phys.vel.x;

        pe.velocity_y.init_min = proj->phys.vel.y;
        pe.velocity_y.init_max = proj->phys.vel.y;
        */

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

void projectile_draw(Projectile* proj,bool add_to_existing_batch)
{
    if(add_to_existing_batch)
    {
        gfx_sprite_batch_add(particles_image,proj->sprite_index,proj->phys.pos.x,proj->phys.pos.y, 0x00FFCC00,1.0, proj->angle_deg-90.0, 1.0, false,true,false);
    }
    else
    {
        gfx_draw_image(particles_image,proj->sprite_index,proj->phys.pos.x,proj->phys.pos.y, 0x00FFCC00,1.0, proj->angle_deg-90.0, 1.0, false,true);
    }
}

void projectile_draw_debug(Projectile* proj)
{
    if(is_in_camera_view(&proj->phys.collision))
    {
        gfx_draw_rect(&proj->phys.prior_collision, COLOR_ORANGE, 0.0, 1.0,1.0, false, true);
        gfx_draw_rect(&proj->phys.collision, COLOR_COLLISON, 0.0, 1.0,1.0, false, true);
    }
}
