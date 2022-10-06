#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "rat_math.h"
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

    get_translate_transform(&view_matrix,&cam_pos);
}

Matrix* get_camera_transform()
{
    return &view_matrix;
}
