#include <string.h>
#include "projectile.h"
#include "gun.h"

Gun gun_arsenal[GUN_TYPE_MAX] = {0};

void gun_init()
{
    gun_arsenal[GUN_TYPE_HANDGUN].fire_power = 1000.0;
    gun_arsenal[GUN_TYPE_HANDGUN].fire_period = 100.0; // milliseconds
    gun_arsenal[GUN_TYPE_HANDGUN].fire_cooldown = 0.0;
    gun_arsenal[GUN_TYPE_HANDGUN].bullets = 100;
    gun_arsenal[GUN_TYPE_HANDGUN].bullets_max = 100;
    gun_arsenal[GUN_TYPE_HANDGUN].projectile_type = PROJECTILE_TYPE_BULLET;
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

void gun_draw()
{

}
