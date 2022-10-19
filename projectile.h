#pragma once

#include "gun.h"

typedef enum
{
    PROJECTILE_TYPE_BULLET = 0,
    PROJECTILE_TYPE_MAX,
} ProjectileType;


void projectile_init();
void projectile_add(int sprite_index, Gun* gun, float angle_offset);
void projectile_update(float delta_t);
void projectile_draw();
