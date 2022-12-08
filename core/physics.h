#pragma once

#include "math2d.h"



typedef struct
{
    Rect pos;
    Vector2f pos_offset;
    Rect actual_pos;
    Rect collision;
    Rect hit;

    Vector2f vel;
    Vector2f accel;
    float mass;
    float max_linear_vel;
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

void physic_apply_pos_offset(Physics* phys, float offset_x, float offset_y);
void physic_set_pos_offset(Physics* phys, float offset_x, float offset_y);
void physics_simulate(Physics* phys, Rect* limit, float delta_t);

void physics_limit_pos(Rect* limit, Rect* pos);

void physics_handle_collision(Physics* phys1, Physics* phys2, double delta_t);

bool physics_rect_collision(Rect* prior_box, Rect* curr_box, Rect* check, float delta_x_pos, float delta_y_pos, rect_collision_data_t* data);
