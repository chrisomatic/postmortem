#pragma once

#include "player.h"
#include "item.h"
#include "glist.h"

#define MAX_PROJECTILES 1024


typedef enum
{
    PROJECTILE_TYPE_BULLET = 0,
    PROJECTILE_TYPE_MAX,
} ProjectileType;

typedef struct
{
    ProjectileType type;
    Physics phys;
    Vector2i grid_pos;
    Vector2i grid_pos_prior;
    Vector2f vel;
    float angle_deg;
    float power;
    int damage;
    int sprite_index;
    float time;
    float ttl;
    bool dead;
} Projectile;

extern Projectile projectiles[MAX_PROJECTILES];
extern glist* plist;

void projectile_init();
void projectile_add(Player* p, Gun* gun, float angle_offset);

void projectile_update(float delta_t);
void projectile_draw();
void projectile_draw_debug(Projectile* proj);
