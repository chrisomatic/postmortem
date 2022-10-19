#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "gfx.h"
#include "projectile.h"
#include "gun.h"

Gun gun_arsenal[GUN_TYPE_MAX] = {0};
int gun_image_set;

void gun_init()
{

    gun_image_set = gfx_load_image_set("img/gun_set.png",32,32);
    GFXSubImageData* sid = gfx_images[gun_image_set].sub_img_data;

    int idx = GUN_TYPE_HANDGUN;
    gun_arsenal[idx].power = 1.0;
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
    gun_arsenal[idx].sprite_index = GUN_TYPE_HANDGUN;
    memcpy(&gun_arsenal[idx].visible_rect, &sid->visible_rects[gun_arsenal[idx].sprite_index], sizeof(Rect));

    idx = GUN_TYPE_MACHINEGUN;
    gun_arsenal[idx].power = 1.0;
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
    gun_arsenal[idx].sprite_index = GUN_TYPE_HANDGUN;
    memcpy(&gun_arsenal[idx].visible_rect, &sid->visible_rects[gun_arsenal[idx].sprite_index], sizeof(Rect));

    idx = GUN_TYPE_SHOTGUN;
    gun_arsenal[idx].power = 1.0;
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
    gun_arsenal[idx].sprite_index = idx;
    memcpy(&gun_arsenal[idx].visible_rect, &sid->visible_rects[gun_arsenal[idx].sprite_index], sizeof(Rect));

}

Gun gun_get(GunType type)
{
    if(type >= GUN_TYPE_MAX) type--;
    Gun gun;
    memcpy(&gun, &gun_arsenal[type], sizeof(Gun));
    gun.fire_cooldown = 0.0;    //reset cooldown
    return gun;
}

void gun_fire(Gun* gun)
{
    if(gun->fire_cooldown == 0)
    {

        if(gun->fire_count > 1)
        {
            for(int i = 0; i < gun->fire_count; ++i)
            {
                int direction = rand()%2 == 0 ? -1 : 1;
                float angle_offset = ((float)rand()/(float)(RAND_MAX)) * (gun->fire_spread/2.0) * direction;
                projectile_add(gun->projectile_type, gun, angle_offset);
            }
        }
        else
        {
            projectile_add(gun->projectile_type, gun, 0.0);
        }

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
    gfx_draw_sub_image(gun_image_set,gun->sprite_index,gun->pos.x,gun->pos.y, COLOR_TINT_NONE,1.0,DEG(gun->angle),1.0);
}
