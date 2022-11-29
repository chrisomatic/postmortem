#pragma once

#include "math2d.h"



typedef struct
{
    Rect pos;
    Vector2f vel;
    Vector2f accel;
    float max_linear_vel;
} Physics;

void physics_begin(Physics* phys);
void physics_add_force(Physics* phys, float x, float y);
void physics_add_friction(Physics* phys, float mu);
void physics_print(Physics* phys, bool force);
void physics_simulate(Physics* phys, float delta_t);
void physics_limit_pos(Rect* limit, Rect* pos);


bool physics_rect_collision(Rect* prior_box, Rect* curr_box, Rect* check, float delta_x_pos, float delta_y_pos);
