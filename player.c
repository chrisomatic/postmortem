#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "window.h"
#include "rat_math.h"
#include "gfx.h"
#include "player.h"

Player player;

void player_init()
{
    player.pos.x = 0.0;
    player.pos.y = 0.0;

    player.vel.x = 0.0;
    player.vel.y = 0.0;

    player.speed = 32.0;
    player.max_base_speed = 128.0;
    player.max_speed = player.max_base_speed;

    player.image = gfx_load_image("img/soldier_f1.png");

    window_controls_clear_keys();

    // map keys
    window_controls_add_key(&player.keys, GLFW_KEY_W, PLAYER_ACTION_UP);
    window_controls_add_key(&player.keys, GLFW_KEY_S, PLAYER_ACTION_DOWN);
    window_controls_add_key(&player.keys, GLFW_KEY_A, PLAYER_ACTION_LEFT);
    window_controls_add_key(&player.keys, GLFW_KEY_D, PLAYER_ACTION_RIGHT);
    window_controls_add_key(&player.keys, GLFW_KEY_LEFT_SHIFT, PLAYER_ACTION_RUN);
    window_controls_add_key(&player.keys, GLFW_KEY_SPACE, PLAYER_ACTION_JUMP);
    window_controls_add_key(&player.keys, GLFW_KEY_E, PLAYER_ACTION_INTERACT);

    window_controls_add_mouse_button(&player.keys, GLFW_MOUSE_BUTTON_LEFT, PLAYER_ACTION_PRIMARY_ACTION);
    window_controls_add_mouse_button(&player.keys, GLFW_MOUSE_BUTTON_RIGHT, PLAYER_ACTION_SECONDARY_ACTION);
}

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

    //printf("%d %d %d %d %d %d %d %d %d\n", up, down, left, right, run, jump, interact, primary_action, secondary_action);

    Vector2f accel = {0};

    if(up)    accel.y -= player.speed;
    if(down)  accel.y += player.speed;
    if(left)  accel.x -= player.speed;
    if(right) accel.x += player.speed;

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

    //printf("player pos: %f %f\n", player.pos.x, player.pos.y);
}

void player_draw()
{
    gfx_draw_image(player.image,(int)player.pos.x,(int)player.pos.y, COLOR_TINT_NONE,1.0,0.0,1.0);
}
