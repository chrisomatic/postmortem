#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "gfx.h"
#include "player.h"
#include "projectile.h"
#include "window.h"
#include "main.h"
#include "gun.h"

Gun gun_arsenal[GUN_TYPE_MAX] = {0};
int gun_image_set;

void gun_init()
{
    gun_image_set = gfx_load_image("img/gun_set.png",false, false, 32, 32, NULL);


    int idx = GUN_TYPE_HANDGUN;
    gun_arsenal[idx].power = 1.0;
    gun_arsenal[idx].recoil_spread = 2.0;
    gun_arsenal[idx].fire_range = 500.0;
    gun_arsenal[idx].fire_speed = 1000.0;
    gun_arsenal[idx].fire_period = 500.0; // milliseconds
    gun_arsenal[idx].fire_cooldown = 0.0;
    gun_arsenal[idx].fire_spread = 0.0;
    gun_arsenal[idx].fire_count = 1;
    gun_arsenal[idx].bullets = 100;
    gun_arsenal[idx].bullets_max = 100;
    gun_arsenal[idx].projectile_type = PROJECTILE_TYPE_BULLET;
    gun_arsenal[idx].type = idx;
    gun_arsenal[idx].sprite_index = 0;
    // memcpy(&gun_arsenal[idx].visible_rect, &gfx_images[gun_image_set].visible_rects[gun_arsenal[idx].sprite_index], sizeof(Rect));

    idx = GUN_TYPE_MACHINEGUN;
    gun_arsenal[idx].power = 1.0;
    gun_arsenal[idx].recoil_spread = 4.0;
    gun_arsenal[idx].fire_range = 500.0;
    gun_arsenal[idx].fire_speed = 1000.0;
    gun_arsenal[idx].fire_period = 100.0; // milliseconds
    gun_arsenal[idx].fire_cooldown = 0.0;
    gun_arsenal[idx].fire_spread = 0.0;
    gun_arsenal[idx].fire_count = 1;
    gun_arsenal[idx].bullets = 100;
    gun_arsenal[idx].bullets_max = 100;
    gun_arsenal[idx].projectile_type = PROJECTILE_TYPE_BULLET;
    gun_arsenal[idx].type = idx;
    gun_arsenal[idx].sprite_index = 0;
    // memcpy(&gun_arsenal[idx].visible_rect, &gfx_images[gun_image_set].visible_rects[gun_arsenal[idx].sprite_index], sizeof(Rect));

    idx = GUN_TYPE_SHOTGUN;
    gun_arsenal[idx].power = 1.0;
    gun_arsenal[idx].recoil_spread = 0.0;
    gun_arsenal[idx].fire_range = 200.0;
    gun_arsenal[idx].fire_speed = 1000.0;
    gun_arsenal[idx].fire_period = 400.0; // milliseconds
    gun_arsenal[idx].fire_cooldown = 0.0;
    gun_arsenal[idx].fire_spread = 30.0;
    gun_arsenal[idx].fire_count = 5;
    gun_arsenal[idx].bullets = 100;
    gun_arsenal[idx].bullets_max = 100;
    gun_arsenal[idx].projectile_type = PROJECTILE_TYPE_BULLET;
    gun_arsenal[idx].type = idx;
    gun_arsenal[idx].sprite_index = 1;
    // memcpy(&gun_arsenal[idx].visible_rect, &gfx_images[gun_image_set].visible_rects[gun_arsenal[idx].sprite_index], sizeof(Rect));
}

Gun gun_get(void* p, GunType type)
{
    if(type >= GUN_TYPE_MAX) type--;
    Gun gun;
    memcpy(&gun, &gun_arsenal[type], sizeof(Gun));
    gun.owner = p;

    gun.fire_cooldown = 0.0;    //reset cooldown
    return gun;
}

void gun_fire(Gun* gun, bool held)
{
    if(gun->fire_cooldown == 0)
    {

        if(gun->fire_count > 1)
        {
            for(int i = 0; i < gun->fire_count; ++i)
            {
                int direction = rand()%2 == 0 ? -1 : 1;
                float angle_offset = rand_float_between(0.0, gun->fire_spread/2.0) * direction;
                projectile_add(gun->projectile_type, gun, angle_offset);
            }
        }
        else
        {
            float angle_offset = 0.0;
            if(held && !FEQ(gun->recoil_spread,0.0))
            {
                int direction = rand()%2 == 0 ? -1 : 1;
                angle_offset = rand_float_between(0.0, gun->recoil_spread/2.0) * direction;
            }
            projectile_add(gun->projectile_type, gun, angle_offset);
        }

        // // //recoil
        // float mouse_x, mouse_y;
        // window_get_mouse_world_coords(&mouse_x, &mouse_y);
        // // float d = dist(player.phys.pos.x, player.phys.pos.y, mouse_x, mouse_y);
        // // float angle = calc_angle_rad(player.phys.pos.x, player.phys.pos.y, mouse_x, mouse_y);
        // // angle += RAD(1.0);
        // // float mouse_x_new = player.phys.pos.x+d*cosf(angle);
        // // float mouse_y_new = player.phys.pos.y+d*sinf(angle);
        // // window_set_mouse_world_coords(mouse_x_new, mouse_y_new);
        // // window_set_mouse_world_coords(mouse_x+.5, mouse_y+.5);
        // window_set_mouse_world_coords(mouse_x+10, mouse_y+10);


        // // //recoil
        // float mouse_x, mouse_y;
        // window_get_mouse_world_coords(&mouse_x, &mouse_y);
        // // float d = dist(player.phys.pos.x, player.phys.pos.y, mouse_x, mouse_y);
        // float angle = calc_angle_rad(player.phys.pos.x, player.phys.pos.y, mouse_x, mouse_y);
        // angle += RAD(5.0);
        // // window_enable_cursor();
        // window_set_mouse_world_coords(mouse_x+1.0*cosf(angle), mouse_y+1.0*sinf(angle));
        // // window_disable_cursor();


        gun->fire_cooldown = gun->fire_period;
    }
}

void gun_update(Gun* gun, float delta_t)
{
    if(gun->fire_cooldown > 0.0)
    {
        gun->fire_cooldown -= (delta_t*1000);
        if(gun->fire_cooldown < 0.0)
            gun->fire_cooldown = 0.0;
    }
}

void gun_draw(Gun* gun)
{
    gfx_draw_image(gun_image_set,gun->sprite_index,gun->pos.x,gun->pos.y, COLOR_TINT_NONE,1.0,DEG(gun->angle),1.0);

    // gfx_draw_sub_image(gun_image_set,gun->sprite_index,gun->pos.x,gun->pos.y, COLOR_TINT_NONE,1.0,DEG(gun->angle),1.0);
}
