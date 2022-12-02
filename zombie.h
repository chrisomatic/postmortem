#pragma once

#include "glist.h"

#define ZOMBIE_HEIGHT   50
#define MAX_ZOMBIES 2000
#define ZOMBIE_TEXTURES_MAX     5

typedef enum
{
    ZANIM_IDLE,
    ZANIM_WALK,
    ZANIM_ATTACK1, // swing

    ZANIM_MAX,
    ZANIM_NONE    // keep this after MAX (affects image loading/lookup)
} ZombieAnimState;


typedef enum
{
    ZOMBIE1,

    ZOMBIE_MODELS_MAX
} ZombieModelIndex;


typedef struct
{
    const char* name;
    ZombieModelIndex index;
    int textures;
} ZombieModel;


typedef enum
{
    ZOMBIE_ACTION_NONE = 0,
    ZOMBIE_ACTION_MOVE_UP,
    ZOMBIE_ACTION_MOVE_UP_RIGHT,
    ZOMBIE_ACTION_MOVE_RIGHT,
    ZOMBIE_ACTION_MOVE_DOWN_RIGHT,
    ZOMBIE_ACTION_MOVE_DOWN,
    ZOMBIE_ACTION_MOVE_DOWN_LEFT,
    ZOMBIE_ACTION_MOVE_LEFT,
    ZOMBIE_ACTION_MOVE_UP_LEFT,
    ZOMBIE_ACTION_MAX,
} ZombieAction;

typedef struct
{
    uint32_t id;
    Physics phys;
    Vector2i grid_pos;
    Vector2f push_vel;
    float speed;
    float scale;
    float hp;
    float hp_max;
    ZombieAction action;
    float action_timer;
    float action_timer_max;
    Rect hit_box;
    Rect collision_box;

    bool moving;

    // // based on collision_box
    // int map_row;
    // int map_col;
    // int world_row;
    // int world_col;

    // physical/graphical properties of the player
    GFXAnimation anim;
    ZombieAnimState anim_state;
    ZombieModelIndex model_index;
    int model_texture;
    int image;
    uint8_t sprite_index;
    uint8_t sprite_index_direction; // 0-7

} Zombie;

typedef struct
{
    Vector2f pos;
    float speed;
    float max_linear_vel;
    float scale;
    float hp_max;
    ZombieAction action;
    float action_timer_max;

    ZombieModelIndex model_index;
    int model_texture;
} ZombieSpawn;

extern uint32_t zombie_info_id;
extern bool zombies_idle;
extern ZombieModel zombie_models[ZOMBIE_MODELS_MAX];
extern Zombie zombies[MAX_ZOMBIES];
extern glist* zlist;

void zombie_init();
bool zombie_add(ZombieSpawn* spawn);
bool zombie_add_to_world_grid(ZombieSpawn* spawn, int world_row, int world_col);
void zombie_push(int index, Vector2f* force);
void zombie_hurt(int index, float val);

void zombie_update_image(Zombie* z);
void zombie_update_anim_state(Zombie* z);
void zombie_update_sprite_index(Zombie* z);

void zombie_update_boxes(Zombie* z);
void zombie_update(Zombie* z, float delta_t);
bool zombie_check_block_collision(Zombie* z, Rect prior_pos, Rect prior_collision_box);
void zombie_draw(Zombie* z, bool batch);
void zombies_update(float delta_t);
void zombies_draw();
Zombie* zombie_get_by_id(uint32_t id);

const char* zombie_anim_state_str(ZombieAnimState anim_state);
