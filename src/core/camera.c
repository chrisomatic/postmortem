#include "headers.h"

#include "window.h"
#include "physics.h"
#include "camera.h"

static void move_camera(float x, float y, float z);

static Matrix view_matrix;

static Camera camera;                //current position
static Camera camera_delta_target;   //target delta position

static int cam_view_width;
static int cam_view_height;

static void calc_cam_view(int default_view_width, int default_view_height)
{
    cam_view_width = default_view_width - (camera.pos.z*default_view_width);
    cam_view_height = default_view_height - (camera.pos.z*default_view_height);
}

void camera_init()
{
    memcpy(&view_matrix,&IDENTITY_MATRIX, sizeof(Matrix));

    camera.pos.x = 0.0;
    camera.pos.y = 0.0;
    camera.pos.z = 0.0;

    camera_delta_target.pos.x = 0.0;
    camera_delta_target.pos.y = 0.0;
    camera_delta_target.pos.z = 0.0;
}

void camera_update(int default_view_width, int default_view_height)
{
    if(!FEQ(camera_delta_target.pos.x,0.0) || !FEQ(camera_delta_target.pos.y,0.0) || !FEQ(camera_delta_target.pos.z,0.0))
    {
        const int num_frames = 15;
        float dx = camera_delta_target.pos.x/num_frames;
        float dy = camera_delta_target.pos.y/num_frames;
        float dz = camera_delta_target.pos.z/num_frames;
        move_camera(camera.pos.x+dx, camera.pos.y+dy, camera.pos.z+dz);
    }

    calc_cam_view(default_view_width, default_view_height);

}

void camera_zoom(float z, bool immediate)
{
    if(immediate)
    {
        camera.pos.z = z;
        camera_delta_target.pos.z = 0.0;
        move_camera(camera.pos.x, camera.pos.y, camera.pos.z);
    }
    else
    {
        camera_delta_target.pos.z = z - camera.pos.z;
    }
}

float camera_get_zoom()
{
    return camera.pos.z;
}

void camera_get_pos(Vector3f* p)
{
    p->x = camera.pos.x;
    p->y = camera.pos.y;
    p->z = camera.pos.z;
}

void camera_move(float x, float y, bool immediate, Rect* limit)
{
    if(limit != NULL)
    {
        Rect cam_rect = {0};
        cam_rect.x = x;
        cam_rect.y = y;
        cam_rect.w = cam_view_width;
        cam_rect.h = cam_view_height;
        physics_limit_pos(limit, &cam_rect);
        x = cam_rect.x;
        y = cam_rect.y;
    }

    if(immediate)
    {
        camera.pos.x = x;
        camera.pos.y = y;
        camera_delta_target.pos.x = 0.0;
        camera_delta_target.pos.y = 0.0;
        move_camera(camera.pos.x, camera.pos.y, camera.pos.z);
    }
    else
    {
        camera_delta_target.pos.x = x - camera.pos.x;
        camera_delta_target.pos.y = y - camera.pos.y;
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
    float vw = cam_view_width / 2.0;
    float vh = cam_view_height / 2.0;

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
