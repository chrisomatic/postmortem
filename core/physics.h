#pragma once

#include "math2d.h"

// typedef struct
// {
//     Rect pos0;      // unrotated position
//     float angle;
//     Rect pos;       // axis-aligned rotated position
//     RectXY pos_rot; // rotated rectangle points
// } PositionRects;

// // set pos0.w and pos0.h    (once)
// // set pos0.x, pos0.y, and angle
// // calculate the rest

// void update_position_rects(float x, float y, float* angle, float mouse_x, float mouse_y, PositionRects* pos)
// {
//     pos->pos0.x = x;
//     pos->pos0.y = y;


//     float a = 0.0;
//     if(angle == NULL)
//     {
//         a = calc_angle_rad(x, y, mouse_x, mouse_y);
//     }
//     else
//     {
//         a = angle;
//     }

//     pos->angle = a;





//     rotate_rect(&pos->pos0, DEG(p->gun.angle), r.x, r.y, &rxy_rot);

// }




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
