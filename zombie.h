#pragma once

#define MAX_ZOMBIES 1024

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

    ZombieAction action;
    float w,h;
    float vw,vh;
    float vx,vy;
    float speed;
    float action_timer;
    float action_timer_max;

    float hp;
    float hp_max;

    Rect collision_box;
    Rect hit_box;

} Zombie;

extern Zombie zombies[MAX_ZOMBIES];
extern int num_zombies;

void zombie_init();
void zombie_update(float delta_t);
void zombie_draw();
void zombie_hurt(int index, float val);
void zombie_push(int index, Vector2f* force);
