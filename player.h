#pragma once

#include "gfx.h"
#include "physics.h"
#include "net.h"

#define PLAYER_TEXTURES_MAX     5
#define MAX_CLIENT_PREDICTED_STATES 8
#define PLAYER_NAME_MAX 32
#define PLAYER_HEIGHT   64

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


typedef enum
{
    WEAPON_TYPE_HANDGUN,
    WEAPON_TYPE_MELEE,

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
    WEAPON_MACHINEGUN1,
    WEAPON_SHOTGUN1,

    WEAPON_NONE,
    WEAPON_MAX
} WeaponIndex;


typedef struct
{

    int projectile_type;

    float power;

    float recoil_spread;
    float fire_range;
    float fire_speed;
    float fire_period;
    float fire_spread;
    int fire_count;

    int bullets;
    int bullets_max;
} Gun2;

typedef struct
{
    float range;
    float power;
    float period;
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

    Gun2 gun;
    Melee melee;

} Weapon;

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
    PLAYER_ACTION_TOGGLE_EQUIP_WEAPON = 1<<9,
    PLAYER_ACTION_TOGGLE_DEBUG     = 1<<10,
    PLAYER_ACTION_TOGGLE_GUN       = 1<<11,
};

typedef struct
{
    bool up, down, left, right;
    bool run, jump, interact;
    bool primary_action, secondary_action;
    bool toggle_equip_weapon, toggle_debug, toggle_gun;
} PlayerActions;

typedef struct
{
    Vector2f pos;
    float angle;
} PlayerServerState;

typedef struct
{
    bool trigger;
    float period;
    float cooldown;
    bool held;
    bool trigger_on_release;
    bool trigger_on_press;
    bool trigger_on_held;
} MouseClick;

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

    Weapon* weapon;
    bool attacking; //melee
    PlayerState attacking_state;


    int image;
    uint8_t sprite_index;
    uint8_t sprite_index_direction;

    Physics phys;
    float speed;
    float max_base_speed;
    float angle;
    float scale;
    bool running;

    MouseClick lmouse;
    MouseClick rmouse;

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

    // for client-side interpolation
    float lerp_t;
    PlayerServerState state_target;
    PlayerServerState state_prior;

    GFXAnimation anim;

} Player;

#define MOVING_PLAYER(p) (p->actions.up || p->actions.down || p->actions.left || p->actions.right)

extern Player players[MAX_CLIENTS];
extern Player* player;
extern int player_count;
extern uint32_t player_colors[MAX_CLIENTS];
extern int player_image_sets[PLAYER_MODELS_MAX][PLAYER_TEXTURES_MAX][PSTATE_MAX][WEAPON_TYPE_MAX];
extern PlayerModel player_models[PLAYER_MODELS_MAX];

extern Weapon weapons[WEAPON_MAX];


void player_init_images();
void player_init_controls(Player* p);
void players_init();
const char* player_state_str(PlayerState pstate);
int player_get_image_index(PlayerModelIndex model_index, int texture, PlayerState pstate, WeaponType wtype);
int players_get_count();
void player_set_weapon(Player* p, WeaponIndex weapon_index);
void player_update_mouse_click(bool active, bool toggled, MouseClick* mouse, float delta_t);
void player_update_anim_timing(Player* p);
void player_update_state(Player* p);
void player_update_image(Player* p);
void player_update_sprite_index(Player* p);
void player_update(Player* p, double delta_t);
void player_update_other(Player* p, double delta_t);
void player_handle_net_inputs(Player* p, double delta_t);
void player_draw(Player* p);

void weapons_init();
void weapons_init_images();
int weapons_get_image_index(PlayerModelIndex model_index, PlayerState pstate, WeaponType wtype);
const char* weapon_type_str(WeaponType wtype);
void weapon_fire(int mx, int my, Weapon* weapon, bool held);

