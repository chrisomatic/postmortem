#pragma once

#include "player.h"
#include "glist.h"
// #include "gun.h"
// #include "weapon.h"

typedef enum
{
    PROJECTILE_TYPE_BULLET = 0,
    PROJECTILE_TYPE_MAX,
} ProjectileType;

extern glist* plist;

void projectile_init();
void projectile_add(int sprite_index, Gun* gun, int mx, int my, float angle_offset);
void projectile_update(float delta_t);
void projectile_draw();
