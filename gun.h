#pragma once

#include "math2d.h"

typedef enum
{
    GUN_TYPE_HANDGUN = 0,
    GUN_TYPE_MAX,
} GunType;

typedef struct
{
    Vector2f pos;
    Rect visible_rect;
    float angle;
    RectXY rectxy;  //rotated

    int type;
    int projectile_type;

    float power;

    float fire_range;
    float fire_speed;
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
