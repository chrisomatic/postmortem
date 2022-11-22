#pragma once

#include "gfx.h"
#include "physics.h"
#include "net.h"

// debug box colors
#define COLOR_POS        COLOR_BLUE
#define COLOR_HIT        COLOR_RED
#define COLOR_COLLISON   COLOR_CYAN
#define COLOR_MAXSIZE    COLOR_YELLOW

#define PLAYER_TEXTURES_MAX     5
#define MAX_CLIENT_PREDICTED_STATES 8
#define PLAYER_NAME_MAX 32
#define PLAYER_HEIGHT   50

//TEMP: blocks
#define MAX_BLOCKS  1000
typedef enum
{
    BLOCK_0,
    BLOCK_1,

    BLOCK_MAX
} BlockType;
typedef struct
{
    BlockType type;
    float hp;
    int image;
    int sprite_index;
    uint32_t color;
} BlockProp;
typedef struct
{
    int row;
    int col;
    float hp;
    BlockType type;
} block_t;



typedef enum
{
    ITEM_TYPE_NONE,
    ITEM_TYPE_MELEE,
    ITEM_TYPE_GUN,
    ITEM_TYPE_BLOCK,
    ITEM_TYPE_OBJECT,
} PlayerItemType;

typedef struct
{
    PlayerItemType item_type;
    void* props;
    bool drawable;
    bool mouse_aim;
    Rect pos;
} PlayerItem;

// animation states
typedef enum
{
    ANIM_IDLE,
    ANIM_WALK,
    ANIM_ATTACK1, // swing

    ANIM_MAX,
    ANIM_NONE    // keep this after MAX (affects image loading/lookup)
} PlayerAnimState;

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
    GUN_TYPE_HANDGUN,
    GUN_TYPE_RIFLE,
    GUN_TYPE_BOW,

    GUN_TYPE_MAX
} GunType;

typedef enum
{
    GUN_PISTOL1,
    GUN_MACHINEGUN1,
    GUN_SHOTGUN1,

    GUN_MAX
} GunIndex;



typedef struct
{
    GunIndex index;
    GunType type;
    const char* name;

    PlayerAnimState anim_state;
    Rect pos;

    // should get rid of this probably
    int projectile_type; // sprite index

    float power;

    float recoil_spread;
    float fire_range;
    float fire_speed;
    float fire_period;
    float fire_spread;
    int fire_count;


    int bullets;
    int bullets_max;
    float reload_time;
} Gun;


//TODO
typedef enum
{
    MELEE_TYPE0,
    MELEE_TYPE1,

    MELEE_TYPE_MAX
} MeleeType;

typedef enum
{
    MELEE_KNIFE1,

    MELEE_MAX
} MeleeIndex;

typedef struct
{
    MeleeIndex index;
    MeleeType type;
    const char* name;

    PlayerAnimState anim_state;
    Rect pos;

    float period;
    float power;
    float range;
} Melee;

enum PlayerActions
{
    PLAYER_ACTION_UP,
    PLAYER_ACTION_DOWN,
    PLAYER_ACTION_LEFT,
    PLAYER_ACTION_RIGHT,
    PLAYER_ACTION_RUN,
    PLAYER_ACTION_JUMP,
    PLAYER_ACTION_INTERACT,
    PLAYER_ACTION_PRIMARY_ACTION,//lmouse
    PLAYER_ACTION_SECONDARY_ACTION,//rmouse
    PLAYER_ACTION_RELOAD,
    PLAYER_ACTION_EQUIP,
    PLAYER_ACTION_DEBUG,
    PLAYER_ACTION_EDITOR,
    PLAYER_ACTION_CYCLE_EQUIP_DOWN, //TEMP
    PLAYER_ACTION_CYCLE_EQUIP_UP,   //TEMP

    PLAYER_ACTION_MAX
};

typedef struct
{
    bool state;
    bool prior_state;
    bool toggled_on;
    // bool toggled_off;    //not needed currently
} PlayerAction;

typedef struct
{
    Vector2f pos;
    float angle;
} PlayerServerState;


typedef enum
{
    MOUSE_TRIGGER_NONE,
    MOUSE_TRIGGER_PRESS,
    MOUSE_TRIGGER_HOLD,
    MOUSE_TRIGGER_RELEASE
} MouseTrigger;

typedef void (*mouse_trigger_cb_t)(void* player, MouseTrigger trigger);

typedef struct
{
    bool triggered;
    float period;
    float cooldown;
    bool held;
    bool trigger_on_release;
    bool trigger_on_press;
    bool trigger_on_held;
    mouse_trigger_cb_t cb;
} MouseData;

typedef enum
{
    PSTATE_NONE,
    PSTATE_ATTACKING,
    PSTATE_RELOADING
} PlayerState;

typedef struct
{
    int index;
    bool active;
    char name[PLAYER_NAME_MAX+1];


    // physical/graphical properties of the player
    PlayerModelIndex model_index;
    int model_texture;

    GFXAnimation anim;
    PlayerAnimState anim_state;
    PlayerAnimState attacking_state;

    int image;
    uint8_t sprite_index;
    uint8_t sprite_index_direction; // 0-7

    Rect standard_size; // only used for scaling
    Rect max_size;      // used for player name mostly
    Rect hit_box;
    Rect collision_box;
    Rect pos;       // actual position of the player
    Physics phys;
    float angle;
    float scale;
    float speed;
    float max_base_speed;

    PlayerState state;
    bool busy;
    bool moving;
    bool running;
    float reload_timer;

    uint8_t melee_hit_count;

    bool item_equipped;
    PlayerItem item;    //equipped item

    //TEMP
    int item_index;



    // mouse stuff
    MouseData lmouse;
    MouseData rmouse;
    int mouse_x;
    int mouse_y;
    int mouse_r;
    int mouse_c;

    // keys/actions
    PlayerAction actions[PLAYER_ACTION_MAX];


    int point_light;

    // net stuff
    NetPlayerInput input;
    NetPlayerInput input_prior;
    // for client-side interpolation
    float lerp_t;
    PlayerServerState server_state_target;
    PlayerServerState server_state_prior;

    PlayerNetState predicted_states[MAX_CLIENT_PREDICTED_STATES];
    int predicted_state_index;



} Player;


#define MOVING_PLAYER(p) (p->actions[PLAYER_ACTION_UP].state || p->actions[PLAYER_ACTION_DOWN].state || p->actions[PLAYER_ACTION_LEFT].state || p->actions[PLAYER_ACTION_RIGHT].state)

extern Player players[MAX_CLIENTS];
extern Player* player;
extern int player_count;
extern uint32_t player_colors[MAX_CLIENTS];
extern PlayerModel player_models[PLAYER_MODELS_MAX];

extern Gun guns[GUN_MAX];
extern Melee melees[MELEE_MAX];


void player_init_images();
void player_init_controls(Player* p);
void players_init();
const char* player_state_str(PlayerState state);
const char* player_anim_state_str(PlayerAnimState anim_state);
int player_get_image_index(Player* p);
int players_get_count();
void player_get_maxwh(Player* p, float* w, float* h);

void player_equip_gun(Player* p, GunIndex index);
void player_equip_melee(Player* p, MeleeIndex index);
void player_equip_block(Player* p, BlockType index);
void player_set_equipped_item(Player* p, int idx);
void player_equip_item(Player* p, PlayerItemType itype, void* props, bool drawable, bool mouse_aim);

void player_set_mouse_nothing(MouseData* mouse_data);
void player_set_mouse(MouseData* mouse_data, bool held, bool press, bool release, float period, mouse_trigger_cb_t cb);
void player_update_mouse_click(Player* p, bool active, bool toggled, MouseData* mouse, float delta_t);

void player_update_anim_timing(Player* p);
void player_update_anim_state(Player* p);
void player_update_image(Player* p);
void player_update_boxes(Player* p);
void player_update_sprite_index(Player* p);
void player_update(Player* p, double delta_t);
void player_update_other(Player* p, double delta_t);
void player_handle_net_inputs(Player* p, double delta_t);
void player_draw(Player* p);

const char* player_item_type_str(PlayerItemType item_type);


void weapons_init();
void weapons_init_images();

int gun_get_image_index(PlayerModelIndex model_index, PlayerAnimState anim_state, GunType gtype);
int melee_get_image_index(PlayerModelIndex model_index, PlayerAnimState anim_state, MeleeType mtype);
const char* gun_type_str(GunType gtype);
const char* melee_type_str(MeleeType mtype);
void gun_fire(Player* p, Gun* gun, bool held);
void player_weapon_melee_check_collision(Player* p);

