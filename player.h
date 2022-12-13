#pragma once

#include "main.h"

#include "gfx.h"
#include "physics.h"
#include "net.h"
#include "item.h"
#include "glist.h"


// debug box colors
#define COLOR_POS        COLOR_BLUE
#define COLOR_HIT        COLOR_RED
#define COLOR_COLLISON   COLOR_CYAN
#define COLOR_MAXSIZE    COLOR_YELLOW

#define PLAYER_TEXTURES_MAX     5
#define MAX_CLIENT_PREDICTED_STATES 8
#define PLAYER_NAME_MAX 32
#define PLAYER_HEIGHT   50

#define PLAYER_BLOCK_PLACEMENT_RADIUS   10

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
    PLAYER_ACTION_MENU,

    PLAYER_ACTION_1,
    PLAYER_ACTION_2,
    PLAYER_ACTION_3,
    PLAYER_ACTION_4,
    PLAYER_ACTION_5,
    PLAYER_ACTION_6,

    PLAYER_ACTION_SPAWN_ZOMBIE,     //TEMP

    PLAYER_ACTION_MAX
};

typedef struct
{
    bool state;
    bool prior_state;
    bool toggled_on;
    bool toggled_off;
    int key; // gets set when state is true
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
    Physics phys;
    Vector2i grid_pos;

    float angle;
    float scale;
    float speed;
    float max_base_speed;

    float hp;
    float hp_max;

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

    float detect_radius;    // unit is 1 map grid space

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

// extern Gun guns[GUN_MAX];
// extern Melee melees[MELEE_MAX];

extern bool moving_zombie;



void player_init_images();
void player_init_controls(Player* p);
void players_init();
void player_set_pos(Player* p, float x, float y);

const char* player_state_str(PlayerState state);
const char* player_anim_state_str(PlayerAnimState anim_state);
int player_get_image_index(Player* p);
int players_get_count();
void player_get_maxwh(Player* p, float* w, float* h);

Rect* player_get_equipped_item_pos(Player* p);
int player_get_equipped_item_img(Player* p);

void player_set_mouse_nothing(MouseData* mouse_data);
void player_set_mouse(MouseData* mouse_data, bool held, bool press, bool release, float period, mouse_trigger_cb_t cb);
void player_update_mouse_click(Player* p, bool active, bool toggled, MouseData* mouse, float delta_t);

void player_add_detect_radius(Player* p, float add);

void player_update_anim_timing(Player* p);
void player_update_anim_state(Player* p);
void player_update_image(Player* p);

void player_update_pos_offset(Player* p);
void player_update_boxes(Player* p);
void player_update_static_boxes(Player* p);

void player_update_sprite_index(Player* p);
void player_update(Player* p, double delta_t);
void player_update_other(Player* p, double delta_t);
void player_handle_net_inputs(Player* p, double delta_t);
void player_draw(Player* p, bool add_to_existing_batch);
void player_draw_debug(Player* p);

void player_draw_all();
void player_draw_offscreen();
void player_draw_crosshair(Player* p);

void player_hurt(Player* p, float damage);

void player_weapon_melee_check_collision(Player* p);

