#pragma once

#include "main.h"
#include "player.h"

typedef enum
{
    WEAPON_HANDGUN,
    WEAPON_MELEE,

    WEAPON_TYPE_NONE,
    WEAPON_TYPE_MAX
} WeaponType;

typedef enum
{
    ATTACK_NONE,
    ATTACK_SHOOT,
    ATTACK_MELEE,
    ATTACK_POWER_MELEE,

    ATTACK_MAX
} WeaponAttack;

typedef enum
{
    WEAPON_PISTOL1,

    WEAPON_MAX
} WeaponIndex;


typedef struct
{
    // Vector2f pos;
    // // Rect visible_rect;
    // float angle;

    // RectXY rectxy;  //rotated

    // int sprite_index;
    // int type;

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
} Gun2;

typedef struct
{
    float range;
    float power;
    float speed;
    float period;
    float cooldown;
} Melee;

typedef struct
{

    const char* name;
    WeaponType type;
    WeaponIndex index;
    Vector2f pos;

    WeaponAttack primary_attack;
    PlayerState primary_state;

    WeaponAttack secondary_attack;
    PlayerState secondary_state;

    // union
    Gun2 gun;
    Melee melee;

} Weapon;



extern Weapon weapons[WEAPON_MAX];




void weapons_init();
void weapons_init_images();
int weapons_get_image_index(PlayerModelIndex model, PlayerState pstate, WeaponType wtype);

const char* weapon_type_str(WeaponType wtype);
// const char* weapon_name_str(WeaponIndex wtype);
