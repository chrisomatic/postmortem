#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "window.h"
#include "camera.h"

static Matrix view_matrix;
static Camera camera;

void camera_init()
{
    memcpy(&view_matrix,&IDENTITY_MATRIX, sizeof(Matrix));

    camera.pos.x = 0.0;
    camera.pos.y = 0.0;
}


void camera_move(float x, float y)
{
    camera.pos.x = x;
    camera.pos.y = y;

    float vw = view_width / 2.0;
    float vh = view_height / 2.0;

    Vector3f cam_pos = {
        -(camera.pos.x - vw),
        -(camera.pos.y - vh),
        0.0
    };

    cam_pos.x = MIN(cam_pos.x, 0.0);
    cam_pos.y = MIN(cam_pos.y, 0.0);

    // Rect r;
    // get_camera_rect(&r);
    // printf("%.2f, %.2f, %.2f, %.2f\n", r.x,r.y,r.w,r.h);



    // printf("camera: %.2f, %.2f    %d,%d   \n", camera.pos.x, camera.pos.y, view_width, view_height);

    get_translate_transform(&view_matrix,&cam_pos);
}

Matrix* get_camera_transform()
{
    return &view_matrix;
}

void get_camera_rect(Rect* rect)
{
    float vw = view_width / 2.0;
    float vh = view_height / 2.0;

    float x = MAX(0,camera.pos.x-vw);
    float y = MAX(0,camera.pos.y-vh);

    rect->w = vw*2.0;
    rect->h = vh*2.0;
    rect->x = x+rect->w/2.0;
    rect->y = y+rect->h/2.0;
}

bool is_in_camera_view(Rect* r)
{
    Rect r1 = {0};
    get_camera_rect(&r1);
    return rectangles_colliding(&r1, r);
}
