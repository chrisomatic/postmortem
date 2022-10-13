#include <stdint.h>
#include <string.h>
#include "gfx.h"
#include "projectile.h"
#include "gun.h"

Gun gun_arsenal[GUN_TYPE_MAX] = {0};
int gun_image_set;

void gun_init()
{

    gun_image_set = gfx_load_image_set("img/gun_set.png",32,32);

    gun_arsenal[GUN_TYPE_HANDGUN].power = 1.0;
    gun_arsenal[GUN_TYPE_HANDGUN].fire_range = 500.0;
    gun_arsenal[GUN_TYPE_HANDGUN].fire_speed = 1000.0;
    gun_arsenal[GUN_TYPE_HANDGUN].fire_period = 100.0; // milliseconds
    gun_arsenal[GUN_TYPE_HANDGUN].fire_cooldown = 0.0;
    gun_arsenal[GUN_TYPE_HANDGUN].bullets = 100;
    gun_arsenal[GUN_TYPE_HANDGUN].bullets_max = 100;
    gun_arsenal[GUN_TYPE_HANDGUN].projectile_type = PROJECTILE_TYPE_BULLET;
    gun_arsenal[GUN_TYPE_HANDGUN].type = GUN_TYPE_HANDGUN;

    GFXSubImageData* sid = gfx_images[gun_image_set].sub_img_data;
    Rect* vr = &sid->visible_rects[GUN_TYPE_HANDGUN];
    memcpy(&gun_arsenal[GUN_TYPE_HANDGUN].visible_rect, vr, sizeof(Rect));
}

Gun gun_get(GunType type)
{
    Gun gun;
    memcpy(&gun, &gun_arsenal[type], sizeof(Gun));
    return gun;
}

void gun_fire(Gun* gun)
{
    if(gun->fire_cooldown == 0)
    {
        projectile_add(PROJECTILE_TYPE_BULLET, gun, gun->pos.x, gun->pos.y, gun->angle);
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
    // gfx_draw_sub_image(gun_image_set,gun->type,gun->pos.x,gun->pos.y, COLOR_TINT_NONE,1.0,DEG(gun->angle+PI),1.0);
    gfx_draw_sub_image(gun_image_set,gun->type,gun->pos.x,gun->pos.y, COLOR_TINT_NONE,1.0,DEG(gun->angle),1.0);
}
