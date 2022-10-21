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
#include "net.h"
#include "main.h"

typedef struct
{
    bool up, down, left, right;
    bool run, jump, interact;
    bool primary_action, secondary_action;
    bool toggle_fire, toggle_debug, toggle_gun;
} PlayerActions;

Player players[MAX_CLIENTS];
Player* player = &players[0];
int player_count = 0;

bool debug_enabled = true; // weird place for this variable
static int crosshair_image;
static float mouse_x, mouse_y;
static PlayerActions player_actions = {0};
static PlayerActions player_actions_prior = {0};

void player_init()
{

    player->phys.pos.x = 400.0;
    player->phys.pos.y = 1000.0;

    player->phys.vel.x = 0.0;
    player->phys.vel.y = 0.0;
    player->sprite_index = 0;
    player->gun_ready = true;

    player->speed = 32.0;
    player->max_base_speed = 128.0;
    player->phys.max_linear_vel = player->max_base_speed;
    player->scale = 1.0;
    player->predicted_state_index = 0;

    player->gun = gun_get(GUN_TYPE_MACHINEGUN);

    player->image = gfx_load_image_set("img/human_set_small.png",32,48);
    crosshair_image = gfx_load_image("img/crosshair.png", false, false);

    window_controls_clear_keys();

    // map keys
    window_controls_add_key(&player->keys, GLFW_KEY_W, PLAYER_ACTION_UP);
    window_controls_add_key(&player->keys, GLFW_KEY_S, PLAYER_ACTION_DOWN);
    window_controls_add_key(&player->keys, GLFW_KEY_A, PLAYER_ACTION_LEFT);
    window_controls_add_key(&player->keys, GLFW_KEY_D, PLAYER_ACTION_RIGHT);
    window_controls_add_key(&player->keys, GLFW_KEY_LEFT_SHIFT, PLAYER_ACTION_RUN);
    window_controls_add_key(&player->keys, GLFW_KEY_SPACE, PLAYER_ACTION_JUMP);
    window_controls_add_key(&player->keys, GLFW_KEY_E, PLAYER_ACTION_INTERACT);
    window_controls_add_key(&player->keys, GLFW_KEY_TAB, PLAYER_ACTION_TOGGLE_FIRE);
    window_controls_add_key(&player->keys, GLFW_KEY_F2, PLAYER_ACTION_TOGGLE_DEBUG);
    window_controls_add_key(&player->keys, GLFW_KEY_G, PLAYER_ACTION_TOGGLE_GUN);

    window_controls_add_mouse_button(&player->keys, GLFW_MOUSE_BUTTON_LEFT, PLAYER_ACTION_PRIMARY_ACTION);
    window_controls_add_mouse_button(&player->keys, GLFW_MOUSE_BUTTON_RIGHT, PLAYER_ACTION_SECONDARY_ACTION);

    player_count = 1;

    if(role == ROLE_CLIENT || role == ROLE_SERVER)
    {
        // initialize all players
        for(int i = 1; i < MAX_CLIENTS; ++i)
        {
            memcpy(&players[i],player, sizeof(Player));
        }
    }
}


void player_update(Player* p, double delta_t)
{
    player_actions.up               = IS_BIT_SET(player->keys,PLAYER_ACTION_UP);
    player_actions.down             = IS_BIT_SET(player->keys,PLAYER_ACTION_DOWN);
    player_actions.left             = IS_BIT_SET(player->keys,PLAYER_ACTION_LEFT);
    player_actions.right            = IS_BIT_SET(player->keys,PLAYER_ACTION_RIGHT);
    player_actions.run              = IS_BIT_SET(player->keys,PLAYER_ACTION_RUN);
    player_actions.jump             = IS_BIT_SET(player->keys,PLAYER_ACTION_JUMP);
    player_actions.interact         = IS_BIT_SET(player->keys,PLAYER_ACTION_INTERACT);
    player_actions.primary_action   = IS_BIT_SET(player->keys,PLAYER_ACTION_PRIMARY_ACTION);
    player_actions.secondary_action = IS_BIT_SET(player->keys,PLAYER_ACTION_SECONDARY_ACTION);
    player_actions.toggle_fire      = IS_BIT_SET(player->keys,PLAYER_ACTION_TOGGLE_FIRE);
    player_actions.toggle_debug     = IS_BIT_SET(player->keys,PLAYER_ACTION_TOGGLE_DEBUG);
    player_actions.toggle_gun       = IS_BIT_SET(player->keys,PLAYER_ACTION_TOGGLE_GUN);

    bool primary_action_toggled = player_actions.primary_action && !player_actions_prior.primary_action;
    bool fire_toggled = player_actions.toggle_fire && !player_actions_prior.toggle_fire;
    bool debug_toggled = player_actions.toggle_debug && !player_actions_prior.toggle_debug;
    bool gun_toggled = player_actions.toggle_gun && !player_actions_prior.toggle_gun;

    memcpy(&player_actions_prior, &player_actions, sizeof(PlayerActions));

    if(gun_toggled)
    {
        int next = p->gun.type+1;
        if(next >= GUN_TYPE_MAX) next = 0;
        p->gun = gun_get(next);
    }

    if(fire_toggled)
    {
        p->gun_ready = !p->gun_ready;
        //if(p->gun_ready)
        //    window_disable_cursor();
        //else
        //    window_enable_cursor();
    }

    if(debug_toggled)
    {
        debug_enabled = !debug_enabled;
    }


    Vector2f accel = {0};

    if(player_actions.up)    { accel.y -= player->speed; }
    if(player_actions.down)  { accel.y += player->speed; }
    if(player_actions.left)  { accel.x -= player->speed; }
    if(player_actions.right) { accel.x += player->speed; }

    window_get_mouse_world_coords(&mouse_x, &mouse_y);

    Vector3f player_pos = {p->phys.pos.x, p->phys.pos.y, 0.0};
    Vector3f mouse_pos = {mouse_x, mouse_y, 0.0};
    Vector3f dist = {mouse_pos.x - player_pos.x, mouse_pos.y - player_pos.y, 0.0};

    if(role != ROLE_SERVER)
        player->angle = calc_angle_rad(player->phys.pos.x, player->phys.pos.y, mouse_pos.x, mouse_pos.y);
    float angle_deg = DEG(player->angle);

    if(p->gun_ready)
    {

        int sector = angle_sector(angle_deg, 16);

        if(sector == 15 || sector == 0)  // player_actions.right
        {
            p->sprite_index = 6;
        }
        else if(sector == 1 || sector == 2)  // player_actions.up-player_actions.right
        {
            p->sprite_index = 5;
        }
        else if(sector == 3 || sector == 4)  // player_actions.up
        {
            p->sprite_index = 4;
        }
        else if(sector == 5 || sector == 6)  // player_actions.up-player_actions.left
        {
            p->sprite_index = 3;
        }
        else if(sector == 7 || sector == 8)  // player_actions.left
        {
            p->sprite_index = 2;
        }
        else if(sector == 9 || sector == 10)  // player_actions.down-player_actions.left
        {
            p->sprite_index = 1;
        }
        else if(sector == 11 || sector == 12)  // player_actions.down
        {
            p->sprite_index = 0;
        }
        else if(sector == 13 || sector == 14)  // player_actions.down-player_actions.right
        {
            p->sprite_index = 7;
        }

    }
    else
    {
        if(player_actions.up && player_actions.left)
            player->sprite_index = 3;
        else if(player_actions.up && player_actions.right)
            player->sprite_index = 5;
        else if(player_actions.down && player_actions.left)
            player->sprite_index = 1;
        else if(player_actions.down && player_actions.right)
            player->sprite_index = 7;
        else if(player_actions.up)
            player->sprite_index = 4;
        else if(player_actions.down)
            player->sprite_index = 0;
        else if(player_actions.left)
            player->sprite_index = 2;
        else if(player_actions.right)
            player->sprite_index = 6;
    }
    

    GFXSubImageData* sid = gfx_images[player->image].sub_img_data;
    Rect* vr = &sid->visible_rects[player->sprite_index];
    player->phys.pos.w = vr->w*player->scale;
    player->phys.pos.h = vr->h*player->scale;

    if(p->gun_ready)
    {
        // update gun

        // gun orientation X:
        // +----------------------+
        // |                      |
        // X                      B
        // |                      |
        // +----------------------+
        // bullet should spawn at B

        // p->gun.angle = p->angle;
        float gx0 = p->phys.pos.x;
        float gy0 = p->phys.pos.y-vr->h*0.1;

        if(role != ROLE_SERVER)
            p->gun.angle = calc_angle_rad(gx0, gy0, mouse_pos.x, mouse_pos.y);

        Rect r = {0};
        RectXY rxy_rot = {0};
        Rect r_rot = {0};

        r.x = gx0;
        r.y = gy0;
        r.w = p->gun.visible_rect.w;
        r.h = p->gun.visible_rect.h;
        rotate_rect(&r, DEG(p->gun.angle), r.x, r.y, &rxy_rot);

        // 'push' the rotated rectangle out a bit from the player
        for(int i = 0; i < 4; ++i)
        {
            rxy_rot.x[i] += (vr->w*0.7)*cosf(p->gun.angle);
            // rxy_rot.y[i] -= vr->h/2.0*sinf(p->gun.angle);

            // rxy_rot.x[i] += 16*cosf(p->gun.angle);
            // rxy_rot.y[i] -= 16*sinf(p->gun.angle);
            // // rxy_rot.y[i] += 16*sinf(PI*2-p->gun.angle); //also works
        }

        // find center of rotated rectangle
        rectxy_to_rect(&rxy_rot, &r_rot);
        p->gun.pos.x = r_rot.x;
        p->gun.pos.y = r_rot.y;
        memcpy(&p->gun.rectxy, &rxy_rot, sizeof(RectXY));

        if(player_actions.primary_action)
        {
            gun_fire(&player->gun, !primary_action_toggled);
        }
    }

    gun_update(&p->gun,delta_t);

    p->phys.max_linear_vel = p->max_base_speed;

    if(player_actions.run)
    {
        accel.x *= 20.0;
        accel.y *= 20.0;
        player->phys.max_linear_vel *= 20.0;
    }

    physics_begin(&player->phys);
    physics_add_friction(&player->phys, 16.0);
    physics_add_force(&player->phys, accel.x, accel.y);
    physics_simulate(&player->phys, delta_t);
    limit_pos(&map.rect, &player->phys.pos);

    if(role == ROLE_CLIENT)
    {
        // add position, angle to predicted player state
        NetPlayerState* state = &p->predicted_states[p->predicted_state_index];

        // circular buffer
        if(p->predicted_state_index == 31)
        {
            // shift
            for(int i = 31; i >= 1; --i)
            {
                memcpy(&p->predicted_states[i-1],&p->predicted_states[i],sizeof(NetPlayerState));
            }
        }
        else if(p->predicted_state_index < 31)
        {
            p->predicted_state_index++;
        }

        state->pos.x = p->phys.pos.x;
        state->pos.y = p->phys.pos.y;
        state->angle = p->angle;
    }
}

void player_draw(Player* p)
{
    bool gun_drawn = false;
    if(p->gun_ready)
    {
        if(p->sprite_index >= 3 && p->sprite_index <= 5)
        {
            // facing up, drawing gun first
            gun_draw(&p->gun);
            gun_drawn = true;
        }
    }

    float px = p->phys.pos.x;
    float py = p->phys.pos.y;
    gfx_draw_sub_image(p->image, p->sprite_index, px, py, COLOR_TINT_NONE,p->scale,0.0,1.0);

    if(debug_enabled)
    {
        gfx_draw_rect(&player->phys.pos, 0x00FF0000, 1.0,1.0, false, true);
    }


    if(p->gun_ready)
    {
        if(!gun_drawn)
            gun_draw(&p->gun);

        GFXImage* img = &gfx_images[crosshair_image];
        gfx_draw_image(crosshair_image,mouse_x,mouse_y, 0x00FF00FF,1.0,0.0,0.80);
    }
}
