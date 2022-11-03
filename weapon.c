#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "gfx.h"
#include "player.h"
#include "projectile.h"
#include "window.h"
#include "main.h"
#include "weapon.h"

Weapon weapons[WEAPON_MAX] = {0};

static int weapon_image_sets[PLAYER_MODELS_MAX][PSTATE_MAX][WEAPON_MAX];


const char* weapon_type_str(WeaponType wtype)
{
    switch(wtype)
    {
        case WEAPON_HANDGUN: return "handgun";
        case WEAPON_MELEE: return "melee";
        default: return "";
    }
}





void weapons_init()
{

    int idx = WEAPON_PISTOL1;
    weapons[idx].index = idx;
    weapons[idx].name = "pistol1";
    weapons[idx].type = WEAPON_HANDGUN;

    weapons[idx].primary_attack = ATTACK_SHOOT;
    weapons[idx].primary_state = PSTATE_MAX;    // no change in state

    weapons[idx].secondary_attack = ATTACK_MELEE;
    weapons[idx].secondary_state = PSTATE_ATTACK1;    // no change in state

    weapons[idx].gun.power = 1.0;
    weapons[idx].gun.recoil_spread = 2.0;
    weapons[idx].gun.fire_range = 500.0;
    weapons[idx].gun.fire_speed = 1000.0;
    weapons[idx].gun.fire_period = 500.0; // milliseconds
    weapons[idx].gun.fire_cooldown = 0.0;
    weapons[idx].gun.fire_spread = 0.0;
    weapons[idx].gun.fire_count = 1;
    weapons[idx].gun.bullets = 100;
    weapons[idx].gun.bullets_max = 100;
    weapons[idx].gun.projectile_type = PROJECTILE_TYPE_BULLET;

    // must call this last
    weapons_init_images();

}

void weapons_init_images()
{
    int ew = 128;
    int eh = 128;

    for(int pm = 0; pm < PLAYER_MODELS_MAX; ++pm)
    {
        for(int ps = 0; ps < PSTATE_MAX; ++ps)
        {
            for(int w = 0; w < WEAPON_MAX; ++w)
            {
                char fname[100] = {0};
                sprintf(fname, "img/blender_output/%s-%s_%s_%s.png", player_models[pm].name, player_state_str(ps), weapon_type_str(weapons[w].type), weapons[w].name);
                weapon_image_sets[pm][ps][w] = gfx_load_image(fname, false, false, ew, eh, NULL);
            }
        }

    }
}

int weapons_get_image_index(PlayerModelIndex model_index, PlayerState pstate, WeaponType wtype)
{
    return weapon_image_sets[model_index][pstate][wtype];
}

// void weapon_draw()
// {
//     gfx_draw_image(gun_image_set,gun->sprite_index,gun->pos.x,gun->pos.y, COLOR_TINT_NONE,1.0,DEG(gun->angle),1.0);

// }