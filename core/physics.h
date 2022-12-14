#pragma once

#include "math2d.h"

#define MAX_COLLIDING_ENTITIES 16 

typedef struct
{
    Rect pos;
    Vector2f pos_offset;
    Rect actual_pos;
    Rect collision;
    Rect prior_collision;
    Rect hit;

    Vector2f vel;
    Vector2f accel;
    float mass;
    float max_linear_vel;

    int num_colliding_entities;
    void* colliding_entities[MAX_COLLIDING_ENTITIES];
    bool  collision_overlap[MAX_COLLIDING_ENTITIES];
} Physics;

typedef struct
{
    // int xd, yd;
    int xo, yo;
    Rect check;
    bool collide;
} rect_collision_data_t;


void physics_begin(Physics* phys);
void physics_add_force(Physics* phys, float x, float y);
void physics_add_friction(Physics* phys, float mu);
void physics_print(Physics* phys, bool force);

void physics_apply_pos_offset(Physics* phys, float offset_x, float offset_y);
void physics_set_pos_offset(Physics* phys, float offset_x, float offset_y);
void physics_simulate(Physics* phys, Rect* limit, float delta_t);

void physics_limit_pos(Rect* limit, Rect* pos);

bool physics_check_collisions(Physics* phys1, Physics* phys2, double delta_t);
void physics_resolve_collisions(Physics* phys1, double delta_t);

bool physics_rect_collision(Rect* prior_box, Rect* curr_box, Rect* check, float delta_x_pos, float delta_y_pos, rect_collision_data_t* data);
