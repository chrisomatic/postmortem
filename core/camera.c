#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "window.h"
#include "physics.h"
#include "camera.h"

static void move_camera(float x, float y, float z);

static Matrix view_matrix;

static Camera camera;                //current position
static Camera camera_delta_target;   //target delta position

float* camera_z;

void camera_init()
{
    memcpy(&view_matrix,&IDENTITY_MATRIX, sizeof(Matrix));

    camera.pos.x = 0.0;
    camera.pos.y = 0.0;
    camera.pos.z = 0.0;

    camera_z = &camera.pos.z;

    camera_delta_target.pos.x = 0.0;
    camera_delta_target.pos.y = 0.0;
    camera_delta_target.pos.z = 0.0;
}

void camera_update()
{
    if(!FEQ(camera_delta_target.pos.x,0.0) || !FEQ(camera_delta_target.pos.y,0.0) || !FEQ(camera_delta_target.pos.z,0.0))
    {
        const int num_frames = 15;
        float dx = camera_delta_target.pos.x/num_frames;
        float dy = camera_delta_target.pos.y/num_frames;
        float dz = camera_delta_target.pos.z/num_frames;
        move_camera(camera.pos.x+dx, camera.pos.y+dy, camera.pos.z+dz);
    }
}

void camera_move(float x, float y, float z, bool immediate, Rect* limit)
{

    if(limit != NULL)
    {
        Rect cam_rect = {0};
        cam_rect.x = x;
        cam_rect.y = y;
        cam_rect.w = view_width;
        cam_rect.h = view_height;
        physics_limit_pos(limit, &cam_rect);
        x = cam_rect.x;
        y = cam_rect.y;
    }

    if(immediate)
    {
        camera.pos.x = x;
        camera.pos.y = y;
        camera.pos.z = z;
        camera_delta_target.pos.x = 0.0;
        camera_delta_target.pos.y = 0.0;
        camera_delta_target.pos.z = 0.0;
        move_camera(camera.pos.x, camera.pos.y, camera.pos.z);
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
