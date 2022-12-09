#pragma once

#include "glist.h"

#define ZOMBIE_HEIGHT   50
#define MAX_ZOMBIES 2000
#define ZOMBIE_TEXTURES_MAX     5
#define ZOMBIE_DEAD_MAX_TIME 10.0

typedef enum
{
    ZANIM_IDLE,
    ZANIM_WALK,
    ZANIM_HURT,
    ZANIM_ATTACK1, // swing
    ZANIM_DEAD,

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


typedef struct
{
    uint32_t id;
    Physics phys;
    Vector2i world_grid_pos;
    Vector2i map_grid_pos;
    Vector2f push_vel;
    float speed;
    float scale;
    float hp;
    float hp_max;
    // Rect hit_box;
    // Rect collision_box;

    uint32_t color;
    float opacity;

    float damage_min;
    float damage_max;
    float angle;
    bool action_none;
    float action_timer;
    float action_timer_max;

    //states

    bool moving;
    bool hurt;

    bool pursuing;
    Vector2f pursue_target;
    Player* pursue_player;

    bool attacking;

    float attack_range;
    float attack_angle; // separate from angle
    uint8_t melee_hit_count;

    bool dead;
    float dead_time;

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

    ZombieModelIndex model_index;
    int model_texture;
} ZombieSpawn;

extern uint32_t zombie_info_id;
extern bool zombies_pursue;
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
bool zombie_draw(Zombie* z, bool batch);
void zombies_update(float delta_t);
void zombies_draw();
Zombie* zombie_get_by_id(uint32_t id);
void zombie_kill_all();

const char* zombie_anim_state_str(ZombieAnimState anim_state);
void zombie_melee_check_collision(Zombie* z);
