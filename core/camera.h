#pragma once

#include "math2d.h"

typedef struct
{
    Vector3f pos;
} Camera;

extern float* camera_z;

void camera_init();
void camera_update();
void camera_move(float x, float y, float z, bool immediate, Rect* limit);
Matrix* get_camera_transform();
void get_camera_rect(Rect* rect);
bool is_in_camera_view(Rect* r);
