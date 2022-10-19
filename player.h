#pragma once

#include "physics.h"
#include "gun.h"

enum PlayerAction
{
    PLAYER_ACTION_UP               = 1<<0,
    PLAYER_ACTION_DOWN             = 1<<1,
    PLAYER_ACTION_LEFT             = 1<<2,
    PLAYER_ACTION_RIGHT            = 1<<3,
    PLAYER_ACTION_RUN              = 1<<4,
    PLAYER_ACTION_JUMP             = 1<<5,
    PLAYER_ACTION_INTERACT         = 1<<6,
    PLAYER_ACTION_PRIMARY_ACTION   = 1<<7,
    PLAYER_ACTION_SECONDARY_ACTION = 1<<8,
    PLAYER_ACTION_TOGGLE_FIRE      = 1<<9,
    PLAYER_ACTION_TOGGLE_DEBUG     = 1<<10,
    PLAYER_ACTION_TOGGLE_GUN       = 1<<11,
};

typedef struct
{
    Physics phys;
    Rect visible_rect;

    float w,h;

    float speed;
    float max_base_speed;
    float angle;
    float scale;

    int image;
    int sprite_index;
    uint16_t keys;

    Gun gun;
    bool gun_ready;
} Player;

extern Player player;
extern bool debug_enabled;

void player_init();
void player_update(double delta_t);
void player_draw();
