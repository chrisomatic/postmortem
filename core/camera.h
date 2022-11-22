#pragma once

#include "math2d.h"

typedef struct
{
    Vector3f pos;
} Camera;

void camera_init();
void camera_update(int default_view_width, int default_view_height);
void camera_move(float x, float y, bool immediate, Rect* limit);
void camera_zoom(float z, bool immediate);
void camera_get_pos(Vector3f* p);
Matrix* get_camera_transform();
void get_camera_rect(Rect* rect);
bool is_in_camera_view(Rect* r);
