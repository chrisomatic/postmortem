#pragma once

#include "physics.h"
#include "gun.h"
#include "net.h"

#define MAX_CLIENT_PREDICTED_STATES 8

#define PLAYER_NAME_MAX 32

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
    bool up, down, left, right;
    bool run, jump, interact;
    bool primary_action, secondary_action;
    bool toggle_fire, toggle_debug, toggle_gun;
} PlayerActions;

typedef struct
{
    Vector2f pos;
    float angle;
} PlayerState;

typedef struct
{
    char name[PLAYER_NAME_MAX+1];

    bool active;
    int index;

    Physics phys;
    float speed;
    float max_base_speed;
    float angle;
    float scale;
    bool running;

    int image;
    int sprite_index;
    uint16_t keys;

    Vector2f mouse_pos;

    PlayerActions actions_prior;
    PlayerActions actions;

    NetPlayerInput input_prior;
    NetPlayerInput input;

    NetPlayerState predicted_states[MAX_CLIENT_PREDICTED_STATES];
    int predicted_state_index;

    Gun gun;
    bool gun_ready;

    // for client-side interpolation
    float lerp_t;
    PlayerState state_target;
    PlayerState state_prior;

} Player;

#define PLAYER_MOVING(p) (p->actions.up || p->actions.down || p->actions.left || p->actions.right)


extern Player* player;
extern uint32_t player_colors[MAX_CLIENTS];
extern Player players[MAX_CLIENTS];
extern int player_count;

void player_init_images();
void player_init_controls(Player* p);
void players_init();
void player_update(Player* p, double delta_t);
void player_update_other(Player* p, double delta_t);
void player_draw(Player* p);
