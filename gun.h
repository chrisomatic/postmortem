#pragma once

#include "math2d.h"

typedef enum
{
    GUN_TYPE_HANDGUN = 0,
    GUN_TYPE_MACHINEGUN = 1,
    GUN_TYPE_SHOTGUN = 2,
    GUN_TYPE_MAX,
} GunType;

typedef struct
{
    void* owner;

    Vector2f pos;
    // Rect visible_rect;
    float angle;
    RectXY rectxy;  //rotated

    int sprite_index;
    int type;
    int projectile_type;

    float power;

    float recoil_spread;
    float fire_range;
    float fire_speed;
    float fire_period;
    float fire_cooldown;
    float fire_spread;
    int fire_count;

    int bullets;
    int bullets_max;

} Gun;

extern Gun gun_arsenal[GUN_TYPE_MAX];
extern int gun_image_set;

void gun_init();
void gun_update(Gun* gun, float delta_t);
void gun_draw();
void gun_fire(Gun* gun, bool held);
Gun gun_get(void* p, GunType type);
