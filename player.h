#pragma once

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
};

typedef struct
{
    Vector2f pos;
    Vector2f vel;

    float speed;
    float max_base_speed;
    float max_speed;

    int image;
    uint32_t keys;
} Player;

extern Player player;

void player_init();
void player_update(double delta_t);
void player_draw();
