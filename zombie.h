#pragma once

typedef enum
{
    ZOMBIE_ACTION_NONE = 0,
    ZOMBIE_ACTION_MOVE_UP,
    ZOMBIE_ACTION_MOVE_DOWN,
    ZOMBIE_ACTION_MOVE_LEFT,
    ZOMBIE_ACTION_MOVE_RIGHT,
    ZOMBIE_ACTION_MAX,
} ZombieAction;

typedef struct
{
    Vector2f pos;
    ZombieAction action;
    float speed;
    float action_timer;
    float action_timer_max;

    float hp;
    float hp_max;

    Rect collision_box;
} Zombie;

void zombie_init();
void zombie_update(float delta_t);
void zombie_draw();
