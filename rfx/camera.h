#pragma once

#include "rat_math.h"

typedef struct
{
    Vector2f pos;
} Camera;

void camera_init();
void camera_move(float x, float y);
Matrix* get_camera_transform();
void get_camera_rect(Rect* rect);
