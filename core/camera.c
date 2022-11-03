#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "window.h"
#include "camera.h"

static void move_camera(float x, float y, float z);

static Matrix view_matrix;

static Camera camera;                //current position
static Camera camera_delta_target;   //target delta position

void camera_init()
{
    memcpy(&view_matrix,&IDENTITY_MATRIX, sizeof(Matrix));

    camera.pos.x = 0.0;
    camera.pos.y = 0.0;
}

void camera_update()
{
    if(!FEQ(camera_delta_target.pos.x,0.0) || !FEQ(camera_delta_target.pos.y,0.0) || !FEQ(camera_delta_target.pos.z,0.0))
    {

        float dx = camera_delta_target.pos.x/10.0;
        float dy = camera_delta_target.pos.y/10.0;
        float dz = camera_delta_target.pos.z/10.0;

        move_camera(camera.pos.x+dx, camera.pos.y+dy, camera.pos.z+dz);
    }
}

void camera_move(float x, float y, float z, bool immediate)
{

    if(immediate)
    {
        camera.pos.x = x;
        camera.pos.y = y;
        camera.pos.z = z;
        camera_delta_target.pos.x = 0.0;
        camera_delta_target.pos.y = 0.0;
        camera_delta_target.pos.z = 0.0;
    }
    else
    {
        camera_delta_target.pos.x = x - camera.pos.x;
        camera_delta_target.pos.y = y - camera.pos.y;
        camera_delta_target.pos.z = z - camera.pos.z;
    }

}

static void move_camera(float x, float y, float z)
{
    camera.pos.x = x;
    camera.pos.y = y;
    camera.pos.z = z;

    float vw = view_width / 2.0;
    float vh = view_height / 2.0;

    Vector3f cam_pos = {
        -(camera.pos.x - vw),
        -(camera.pos.y - vh),
        -camera.pos.z
    };

    // cam_pos.x = MIN(cam_pos.x, 0.0);
    // cam_pos.y = MIN(cam_pos.y, 0.0);

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

    float x = camera.pos.x-vw;
    float y = camera.pos.y-vh;

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
