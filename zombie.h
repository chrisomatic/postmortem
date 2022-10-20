#pragma once

#include "main.h"

#define MAX_ZOMBIES 2000

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
    Physics phys;
    Vector2f push_vel;
    float speed;
    float scale;
    float hp;
    float hp_max;
    ZombieAction action;
    float action_timer;
    float action_timer_max;
    // Rect visible_rect;
    Rect hit_box;
    Rect collision_box;

    // based on collision_box
    int map_row;
    int map_col;
    int world_row;
    int world_col;
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
} ZombieSpawn;

extern Zombie zombies[MAX_ZOMBIES];
extern glist* zlist;

void zombie_init();
bool zombie_add(ZombieSpawn* spawn);
bool zombie_add_to_world_grid(ZombieSpawn* spawn, int world_row, int world_col);
void zombie_update(float delta_t);
void zombie_draw();
void zombie_hurt(int index, float val);
void zombie_push(int index, Vector2f* force);
