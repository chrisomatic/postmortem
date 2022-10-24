#pragma once

#include "math2d.h"

typedef struct
{
    Rect pos;
    Vector2f vel;
    Vector2f accel;
    float max_linear_vel;

    // for client-side interpolation
    double lerp_t;
    Vector2f pos_prior;
    Vector2f pos_target;
} Physics;

void physics_begin(Physics* phys);
void physics_add_force(Physics* phys, float x, float y);
void physics_add_friction(Physics* phys, float mu);
void physics_print(Physics* phys, bool force);
void physics_simulate(Physics* phys, float delta_t);
