#pragma once

#include "gfx.h"
#include "physics.h"
#include "gun.h"
#include "net.h"

#define PLAYER_TEXTURES_MAX     5

typedef enum
{
    PSTATE_IDLE,
    PSTATE_WALK,
    PSTATE_ATTACK1, // swing

    PSTATE_MAX,
    PSTATE_NONE    // keep this after MAX
} PlayerState;

typedef enum
{
    HUMAN1,

    PLAYER_MODELS_MAX
} PlayerModelIndex;


typedef struct
{
    const char* name;
    PlayerModelIndex index;
    int textures;
} PlayerModel;



// must include underneath PlayerState
#include "weapon.h"


extern int player_image_sets[PLAYER_MODELS_MAX][PLAYER_TEXTURES_MAX][PSTATE_MAX][WEAPON_TYPE_MAX];
extern PlayerModel player_models[PLAYER_MODELS_MAX];





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
} PlayerServerState;


typedef struct
{
    char name[PLAYER_NAME_MAX+1];

    bool active;
    int index;

    PlayerModelIndex model_index;
    int model_texture;
    PlayerState state;
    bool moving;
    bool weapon_ready;
    bool attacking;
    Weapon weapon;




    int image;
    uint8_t sprite_index;



    Physics phys;
    float speed;
    float max_base_speed;
    float angle;
    float scale;
    bool running;


    uint16_t keys;

    int mouse_x;
    int mouse_y;

    PlayerActions actions_prior;
    PlayerActions actions;

    NetPlayerInput input_prior;
    NetPlayerInput input;

    NetPlayerState predicted_states[MAX_CLIENT_PREDICTED_STATES];
    int predicted_state_index;

    int point_light;

    // remove
    Gun gun;
    bool gun_ready;
    bool gun_front;

    // for client-side interpolation
    float lerp_t;
    PlayerServerState state_target;
    PlayerServerState state_prior;

    GFXAnimation anim;

} Player;

#define MOVING_PLAYER(p) (p->actions.up || p->actions.down || p->actions.left || p->actions.right)


extern Player* player;
extern uint32_t player_colors[MAX_CLIENTS];
extern Player players[MAX_CLIENTS];
extern int player_count;

void player_init_images();
void player_init_controls(Player* p);
void players_init();

const char* player_state_str(PlayerState pstate);
int player_get_image_index(PlayerModelIndex model_index, int texture, PlayerState pstate, WeaponType wtype);

int players_get_count();

void player_update_state(Player* p);
void player_update_image(Player* p);

void player_update(Player* p, double delta_t);
void player_update_other(Player* p, double delta_t);
void player_handle_net_inputs(Player* p, double delta_t);
void player_draw(Player* p);
