#pragma once

#include "math2d.h"

typedef struct
{
    Vector2f pos;
} Camera;

void camera_init();
void camera_update();
// void camera_move(float x, float y);
void camera_move(float x, float y, bool immediate);
Matrix* get_camera_transform();
void get_camera_rect(Rect* rect);
bool is_in_camera_view(Rect* r);
