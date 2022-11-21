#pragma once

#include "player.h"
#include "glist.h"

typedef enum
{
    PROJECTILE_TYPE_BULLET = 0,
    PROJECTILE_TYPE_MAX,
} ProjectileType;

extern glist* plist;

void projectile_init();
void projectile_add(Player* p, Gun* gun, float angle_offset);

void projectile_update(float delta_t);
void projectile_draw();
