#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "window.h"
#include "math2d.h"
#include "gfx.h"
#include "camera.h"
#include "projectile.h"
#include "player.h"
#include "world.h"

Player player;

bool debug_enabled = true; // weird place for this variable

static int crosshair_image;
static float mouse_x, mouse_y;

void player_init()
{
    player.phys.pos.x = 400.0;
    player.phys.pos.y = 1000.0;

    player.phys.vel.x = 0.0;
    player.phys.vel.y = 0.0;
    player.w = 32;
    player.h = 48;
    player.sprite_index = 0;
    player.gun_ready = true;

    player.speed = 32.0;
    player.max_base_speed = 128.0;
    player.phys.max_linear_vel = player.max_base_speed;
    player.scale = 1.0;

    player.gun = gun_get(GUN_TYPE_HANDGUN);

    player.image = gfx_load_image_set("img/human_set_small.png",32,48);
    crosshair_image = gfx_load_image("img/crosshair.png", false, false);

    window_controls_clear_keys();

    // map keys
    window_controls_add_key(&player.keys, GLFW_KEY_W, PLAYER_ACTION_UP);
    window_controls_add_key(&player.keys, GLFW_KEY_S, PLAYER_ACTION_DOWN);
    window_controls_add_key(&player.keys, GLFW_KEY_A, PLAYER_ACTION_LEFT);
    window_controls_add_key(&player.keys, GLFW_KEY_D, PLAYER_ACTION_RIGHT);
    window_controls_add_key(&player.keys, GLFW_KEY_LEFT_SHIFT, PLAYER_ACTION_RUN);
    window_controls_add_key(&player.keys, GLFW_KEY_SPACE, PLAYER_ACTION_JUMP);
    window_controls_add_key(&player.keys, GLFW_KEY_E, PLAYER_ACTION_INTERACT);
    window_controls_add_key(&player.keys, GLFW_KEY_TAB, PLAYER_ACTION_TOGGLE_FIRE);
    window_controls_add_key(&player.keys, GLFW_KEY_F2, PLAYER_ACTION_TOGGLE_DEBUG);

    window_controls_add_mouse_button(&player.keys, GLFW_MOUSE_BUTTON_LEFT, PLAYER_ACTION_PRIMARY_ACTION);
    window_controls_add_mouse_button(&player.keys, GLFW_MOUSE_BUTTON_RIGHT, PLAYER_ACTION_SECONDARY_ACTION);
}

bool prior_toggle_fire;
bool prior_toggle_debug;

void player_update(double delta_t)
{
    bool up               = IS_BIT_SET(player.keys,PLAYER_ACTION_UP);
    bool down             = IS_BIT_SET(player.keys,PLAYER_ACTION_DOWN);
    bool left             = IS_BIT_SET(player.keys,PLAYER_ACTION_LEFT);
    bool right            = IS_BIT_SET(player.keys,PLAYER_ACTION_RIGHT);
    bool run              = IS_BIT_SET(player.keys,PLAYER_ACTION_RUN);
    bool jump             = IS_BIT_SET(player.keys,PLAYER_ACTION_JUMP);
    bool interact         = IS_BIT_SET(player.keys,PLAYER_ACTION_INTERACT);
    bool primary_action   = IS_BIT_SET(player.keys,PLAYER_ACTION_PRIMARY_ACTION);
    bool secondary_action = IS_BIT_SET(player.keys,PLAYER_ACTION_SECONDARY_ACTION);
    bool toggle_fire      = IS_BIT_SET(player.keys,PLAYER_ACTION_TOGGLE_FIRE);
    bool toggle_debug     = IS_BIT_SET(player.keys,PLAYER_ACTION_TOGGLE_DEBUG);


    if(toggle_fire && !prior_toggle_fire)
    {
        player.gun_ready = !player.gun_ready;
        //if(player.gun_ready)
        //    window_disable_cursor();
        //else
        //    window_enable_cursor();
    }

    prior_toggle_fire = toggle_fire;

    if(toggle_debug && !prior_toggle_debug)
        debug_enabled = !debug_enabled;

    prior_toggle_debug = toggle_debug;

    Vector2f accel = {0};

    if(up)    { accel.y -= player.speed; }
    if(down)  { accel.y += player.speed; }
    if(left)  { accel.x -= player.speed; }
    if(right) { accel.x += player.speed; }

    window_get_mouse_world_coords(&mouse_x, &mouse_y);

    Vector3f player_pos = {player.phys.pos.x, player.phys.pos.y, 0.0};
    Vector3f mouse_pos = {mouse_x, mouse_y, 0.0};
    Vector3f dist = {mouse_pos.x - player_pos.x, mouse_pos.y - player_pos.y, 0.0};

    player.angle = calc_angle_rad(player_pos.x, player_pos.y, mouse_pos.x, mouse_pos.y);
    float angle_deg = DEG(player.angle);

    if(player.gun_ready)
    {

        int sector = angle_sector(angle_deg, 16);

        if(sector == 15 || sector == 0)  // right
        {
            player.sprite_index = 6;
        }
        else if(sector == 1 || sector == 2)  // up-right
        {
            player.sprite_index = 5;
        }
        else if(sector == 3 || sector == 4)  // up
        {
            player.sprite_index = 4;
        }
        else if(sector == 5 || sector == 6)  // up-left
        {
            player.sprite_index = 3;
        }
        else if(sector == 7 || sector == 8)  // left
        {
            player.sprite_index = 2;
        }
        else if(sector == 9 || sector == 10)  // down-left
        {
            player.sprite_index = 1;
        }
        else if(sector == 11 || sector == 12)  // down
        {
            player.sprite_index = 0;
        }
        else if(sector == 13 || sector == 14)  // down-right
        {
            player.sprite_index = 7;
        }

    }
    else
    {
        if(up && left)
            player.sprite_index = 3;
        else if(up && right)
            player.sprite_index = 5;
        else if(down && left)
            player.sprite_index = 1;
        else if(down && right)
            player.sprite_index = 7;
        else if(up)
            player.sprite_index = 4;
        else if(down)
            player.sprite_index = 0;
        else if(left)
            player.sprite_index = 2;
        else if(right)
            player.sprite_index = 6;
    }

    GFXSubImageData* sid = gfx_images[player.image].sub_img_data;
    Rect* vr = &sid->visible_rects[player.sprite_index];
    memcpy(&player.visible_rect, vr, sizeof(Rect));

    if(player.gun_ready)
    {
        // update gun

        // gun orientation X:
        // +----------------------+
        // |                      |
        // X                      B
        // |                      |
        // +----------------------+
        // bullet should spawn at B

        player.gun.angle = player.angle;
        float gx0 = player.phys.pos.x;
        float gy0 = player.phys.pos.y-vr->h*0.1;

        Rect r = {0};
        RectXY rxy_rot = {0};
        Rect r_rot = {0};

        r.x = gx0;
        r.y = gy0;
        r.w = player.gun.visible_rect.w;
        r.h = player.gun.visible_rect.h;
        rotate_rect(&r, DEG(player.gun.angle), r.x, r.y, &rxy_rot);

        // 'push' the rotated rectangle out a bit from the player
        for(int i = 0; i < 4; ++i)
        {
            rxy_rot.x[i] += vr->w/2.0*cosf(player.gun.angle);
            // rxy_rot.y[i] -= vr->h/2.0*sinf(player.gun.angle);

            // rxy_rot.x[i] += 16*cosf(player.gun.angle);
            // rxy_rot.y[i] -= 16*sinf(player.gun.angle);
            // // rxy_rot.y[i] += 16*sinf(PI*2-player.gun.angle); //also works
        }

        // find center of rotated rectangle
        rectxy_to_rect(&rxy_rot, &r_rot);
        player.gun.pos.x = r_rot.x;
        player.gun.pos.y = r_rot.y;
        memcpy(&player.gun.rectxy, &rxy_rot, sizeof(RectXY));

        if(primary_action)
        {
            gun_fire(&player.gun);
        }
    }

    gun_update(&player.gun,delta_t);

    player.phys.max_linear_vel = player.max_base_speed;

    if(run)
    {
        accel.x *= 2.0;
        accel.y *= 2.0;
        player.phys.max_linear_vel *= 2.0;
    }

    physics_begin(&player.phys);
    physics_add_friction(&player.phys, 16.0);
    physics_add_force(&player.phys, accel.x, accel.y);
    physics_simulate(&player.phys, delta_t);
}

void player_draw()
{
    bool gun_drawn = false;
    if(player.gun_ready)
    {
        if(player.sprite_index >= 3 && player.sprite_index <= 5)
        {
            // facing up, drawing gun first
            gun_draw(&player.gun);
            gun_drawn = true;
        }
    }

    float px = player.phys.pos.x;
    float py = player.phys.pos.y;
    gfx_draw_sub_image(player.image, player.sprite_index, px, py, COLOR_TINT_NONE,player.scale,0.0,1.0);

    if(debug_enabled)
    {
        Rect r = {0};
        r.x = player.phys.pos.x;
        r.y = player.phys.pos.y;
        r.w = player.visible_rect.w;
        r.h = player.visible_rect.h;
        gfx_draw_rect(&r, 0x00FF0000, player.scale,1.0);
    }


    if(player.gun_ready)
    {
        if(!gun_drawn)
            gun_draw(&player.gun);

        GFXImage* img = &gfx_images[crosshair_image];
        gfx_draw_image(crosshair_image,mouse_x,mouse_y, 0x00FF00FF,1.0,0.0,0.80);
    }
}
