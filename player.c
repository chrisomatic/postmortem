#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "window.h"
#include "rat_math.h"
#include "gfx.h"
#include "camera.h"
#include "projectile.h"
#include "player.h"

Player player;

bool debug_enabled; // weird place for this variable

void player_init()
{
    player.pos.x = 400.0;
    player.pos.y = 300.0;

    player.vel.x = 0.0;
    player.vel.y = 0.0;
    player.w = 32;
    player.h = 48;
    player.sprite_index = 0;
    player.gun_ready = true;

    player.speed = 32.0;
    player.max_base_speed = 128.0;
    player.max_speed = player.max_base_speed;
    player.scale = 1.0;

    player.gun = gun_get(GUN_TYPE_HANDGUN);

    player.image = gfx_load_image_set("img/human_set_small.png",32,48);
    //player.image = gfx_load_image("img/soldier_f1.png");

    window_controls_clear_keys();

    // map keys
    window_controls_add_key(&player.keys, GLFW_KEY_W, PLAYER_ACTION_UP);
    window_controls_add_key(&player.keys, GLFW_KEY_S, PLAYER_ACTION_DOWN);
    window_controls_add_key(&player.keys, GLFW_KEY_A, PLAYER_ACTION_LEFT);
    window_controls_add_key(&player.keys, GLFW_KEY_D, PLAYER_ACTION_RIGHT);
    window_controls_add_key(&player.keys, GLFW_KEY_LEFT_SHIFT, PLAYER_ACTION_RUN);
    window_controls_add_key(&player.keys, GLFW_KEY_SPACE, PLAYER_ACTION_JUMP);
    window_controls_add_key(&player.keys, GLFW_KEY_E, PLAYER_ACTION_INTERACT);
    window_controls_add_key(&player.keys, GLFW_KEY_R, PLAYER_ACTION_TOGGLE_FIRE);
    window_controls_add_key(&player.keys, GLFW_KEY_TAB, PLAYER_ACTION_TOGGLE_DEBUG);

    window_controls_add_mouse_button(&player.keys, GLFW_MOUSE_BUTTON_LEFT, PLAYER_ACTION_PRIMARY_ACTION);
    window_controls_add_mouse_button(&player.keys, GLFW_MOUSE_BUTTON_RIGHT, PLAYER_ACTION_SECONDARY_ACTION);
}

bool prior_toggle_fire;
bool prior_toggle_debug;

void player_update(double delta_t)
{
    //printf("%u\n",player.keys);

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

    //printf("%d %d %d %d %d %d %d %d %d\n", up, down, left, right, run, jump, interact, primary_action, secondary_action);

    if(toggle_fire && !prior_toggle_fire)
        player.gun_ready = !player.gun_ready;

    prior_toggle_fire = toggle_fire;

    if(toggle_debug && !prior_toggle_debug)
        debug_enabled = !debug_enabled;

    prior_toggle_debug = toggle_debug;

    Vector2f accel = {0};

    if(up)    { accel.y -= player.speed; }
    if(down)  { accel.y += player.speed; }
    if(left)  { accel.x -= player.speed; }
    if(right) { accel.x += player.speed; }

    float mouse_x, mouse_y;
    window_get_mouse_world_coords(&mouse_x, &mouse_y);

    // // DEBUG
    // if(primary_action)
    // {
    //     printf("mouse_pos: %f,%f\n", mouse_x, mouse_y);
    // }

    Vector3f player_pos = {player.pos.x + (player.w*player.scale)/2.0, (player.pos.y + (player.h*player.scale)/2.0), 0.0};
    Vector3f mouse_pos = {mouse_x, mouse_y, 0.0};
    Vector3f dist = {mouse_pos.x - player_pos.x, mouse_pos.y - player_pos.y, 0.0};

    //printf("mouse_pos: %f %f\n",mouse_pos.x, mouse_pos.y);
    //printf("player pos: %f %f\n",player_pos.x, player_pos.y);
    //printf("dist: %f, %f\n",dist.x, dist.y);

    //float angle = get_angle_between_vectors_rad(&dist, &x_axis);
    player.angle = calc_angle_rad(mouse_pos.x, mouse_pos.y, player_pos.x, player_pos.y);
    float angle_deg = DEG(player.angle);

    //printf("angle_deg: %f\n",angle_deg);

    if(player.gun_ready)
    {
        if(angle_deg >= 337.5 ||  angle_deg < 22.5)
            player.sprite_index = 2; // left
        else if(angle_deg >= 22.5 && angle_deg < 67.5)
            player.sprite_index = 3; // up-left
        else if(angle_deg >= 67.5 && angle_deg < 112.5)
            player.sprite_index = 4; // up
        else if(angle_deg >= 112.5 && angle_deg < 157.5)
            player.sprite_index = 5; // up-right
        else if(angle_deg >= 157.5 && angle_deg < 202.5)
            player.sprite_index = 6; // right
        else if(angle_deg >= 202.5 && angle_deg < 247.5)
            player.sprite_index = 7; // down-right
        else if(angle_deg >= 247.5 && angle_deg < 292.5)
            player.sprite_index = 0; // down
        else if(angle_deg >= 292.5 && angle_deg < 337.5)
            player.sprite_index = 1; // down-left

        // update gun
        player.gun.pos.x = player.pos.x;
        player.gun.pos.y = player.pos.y;
        player.gun.angle = player.angle;

        if(primary_action)
        {
            gun_fire(&player.gun);
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
    
    gun_update(&player.gun,delta_t);

    player.max_speed = player.max_base_speed;

    if(run)
    {
        accel.x *= 2.0;
        accel.y *= 2.0;
        player.max_speed *= 2.0;
    }

    float friction = 16.0f;

    if(accel.x == 0.0)
    {
        // apply friction in x direction
        float abs_vel_x = ABS(player.vel.x);

        if(abs_vel_x > 0.0)
        {
            float val_x = MIN(friction, abs_vel_x);
            player.vel.x += (player.vel.x > 0 ? -val_x : val_x); // friction
        }
    }

    if(accel.y == 0.0)
    {
        // apply friction in y direction
        float abs_vel_y = ABS(player.vel.y);
        if(abs_vel_y > 0.0)
        {
            float val_y = MIN(friction, abs_vel_y);
            player.vel.y += (player.vel.y > 0 ? -val_y : val_y); // friction
        }
    }

    player.vel.x += accel.x*delta_t*TARGET_FPS;
    player.vel.y += accel.y*delta_t*TARGET_FPS;

    player.vel.x = RANGE(player.vel.x, -player.max_speed,player.max_speed);
    player.vel.y = RANGE(player.vel.y, -player.max_speed,player.max_speed);

    player.pos.x += delta_t*player.vel.x;
    player.pos.y += delta_t*player.vel.y;

    // limit range
    player.pos.x = MAX(player.pos.x, 0.0);
    player.pos.y = MAX(player.pos.y, 0.0);

    //printf("player pos: %f %f\n", player.pos.x, player.pos.y);
}

void player_draw()
{
    //gfx_draw_image(player.image,(int)player.pos.x,(int)player.pos.y, COLOR_TINT_NONE,0.16,0.0,1.0);
    gfx_draw_sub_image(player.image,player.sprite_index,player.pos.x,player.pos.y, COLOR_TINT_NONE,player.scale,0.0,1.0);
    //gfx_draw_line2(100.0,50.0,200.0,200.0);
}
