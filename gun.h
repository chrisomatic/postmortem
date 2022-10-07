#pragma once

#include "rat_math.h"

typedef enum
{
    GUN_TYPE_HANDGUN = 0,
    GUN_TYPE_MAX,
} GunType;

typedef struct
{
    Vector2f pos;
    float angle;

    int projectile_type;

    float fire_power;
    float fire_period;
    float fire_cooldown;

    int bullets;
    int bullets_max;
} Gun;

void gun_init();
void gun_update(Gun* gun, float delta_t);
void gun_draw();
void gun_fire(Gun* gun);
Gun gun_get(GunType type);
