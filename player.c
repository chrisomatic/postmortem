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

uint32_t player_colors[MAX_CLIENTS] = {
    COLOR_BLUE,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_ORANGE,
    COLOR_PURPLE,
    COLOR_CYAN,
    COLOR_PINK,
    COLOR_YELLOW,
};

Player players[MAX_CLIENTS];
Player* player = &players[1];
int player_count = 0;

static int player_image_set;
static int crosshair_image;
static float mouse_x, mouse_y;

static void player_update_sprite_index(Player* p);
static void player_gun_set_position(Player* p);

void player_init_images()
{
    player_image_set = gfx_load_image_set("img/human_set_small.png",32,48);
    crosshair_image = gfx_load_image("img/crosshair.png", false, false);
}

void player_init_controls(Player* p)
{
    window_controls_clear_keys();

    // map keys
    window_controls_add_key(&p->keys, GLFW_KEY_W, PLAYER_ACTION_UP);
    window_controls_add_key(&p->keys, GLFW_KEY_S, PLAYER_ACTION_DOWN);
    window_controls_add_key(&p->keys, GLFW_KEY_A, PLAYER_ACTION_LEFT);
    window_controls_add_key(&p->keys, GLFW_KEY_D, PLAYER_ACTION_RIGHT);
    window_controls_add_key(&p->keys, GLFW_KEY_LEFT_SHIFT, PLAYER_ACTION_RUN);
    window_controls_add_key(&p->keys, GLFW_KEY_SPACE, PLAYER_ACTION_JUMP);
    window_controls_add_key(&p->keys, GLFW_KEY_E, PLAYER_ACTION_INTERACT);
    window_controls_add_key(&p->keys, GLFW_KEY_TAB, PLAYER_ACTION_TOGGLE_FIRE);
    window_controls_add_key(&p->keys, GLFW_KEY_F2, PLAYER_ACTION_TOGGLE_DEBUG);
    window_controls_add_key(&p->keys, GLFW_KEY_G, PLAYER_ACTION_TOGGLE_GUN);
    // window_controls_add_key(&p->keys, GLFW_KEY_C, PLAYER_ACTION_CONSOLE);

    window_controls_add_mouse_button(&p->keys, GLFW_MOUSE_BUTTON_LEFT, PLAYER_ACTION_PRIMARY_ACTION);
    window_controls_add_mouse_button(&p->keys, GLFW_MOUSE_BUTTON_RIGHT, PLAYER_ACTION_SECONDARY_ACTION);
}

static void player_init(int index)
{
    Player* p = &players[index];
    p->index = index;

    p->phys.pos.x = 400.0;
    p->phys.pos.y = 1000.0;

    p->phys.vel.x = 0.0;
    p->phys.vel.y = 0.0;
    p->sprite_index = 0;
    p->gun_ready = true;

    p->speed = 32.0;
    p->max_base_speed = 128.0;
    p->phys.max_linear_vel = p->max_base_speed;
    p->scale = 1.0;
    p->predicted_state_index = 0;

    p->gun = gun_get(GUN_TYPE_MACHINEGUN);
    p->image = player_image_set;

}

void players_init()
{
    player_init_images();
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        Player* p = &players[i];
        player_init(i);
        if(role == ROLE_LOCAL || role == ROLE_CLIENT)
        {
            if(player == p)
            {
                player_init_controls(p);
                p->active = true;
            }
        }

        if(p->active)
            player_count++;
    }
}


void player_update_other(Player* p, double delta_t)
{
    p->lerp_t += delta_t;

    float tick_time = 1.0/TICK_RATE;
    float t = (p->lerp_t / tick_time);

    Vector2f lp = lerp2f(&p->state_prior.pos,&p->state_target.pos,t);
    p->phys.pos.x = lp.x;
    p->phys.pos.y = lp.y;

    p->angle = lerp(p->state_prior.angle,p->state_target.angle,t);

    player_update_sprite_index(p);
    player_gun_set_position(p);
}

void player_update_sprite_index(Player* p)
{
    if(role != ROLE_SERVER && p == player)
        p->angle = calc_angle_rad(p->phys.pos.x, p->phys.pos.y, mouse_x, mouse_y);

    float angle_deg = DEG(p->angle);

    if(p->gun_ready)
    {
        int sector = angle_sector(angle_deg, 16);

        if(sector == 15 || sector == 0)  // p->actions.right
        {
            p->sprite_index = 6;
        }
        else if(sector == 1 || sector == 2)  // p->actions.up-p->actions.right
        {
            p->sprite_index = 5;
        }
        else if(sector == 3 || sector == 4)  // p->actions.up
        {
            p->sprite_index = 4;
        }
        else if(sector == 5 || sector == 6)  // p->actions.up-p->actions.left
        {
            p->sprite_index = 3;
        }
        else if(sector == 7 || sector == 8)  // p->actions.left
        {
            p->sprite_index = 2;
        }
        else if(sector == 9 || sector == 10)  // p->actions.down-p->actions.left
        {
            p->sprite_index = 1;
        }
        else if(sector == 11 || sector == 12)  // p->actions.down
        {
            p->sprite_index = 0;
        }
        else if(sector == 13 || sector == 14)  // p->actions.down-p->actions.right
        {
            p->sprite_index = 7;
        }

    }
    else
    {
        if(p->actions.up && p->actions.left)
            p->sprite_index = 3;
        else if(p->actions.up && p->actions.right)
            p->sprite_index = 5;
        else if(p->actions.down && p->actions.left)
            p->sprite_index = 1;
        else if(p->actions.down && p->actions.right)
            p->sprite_index = 7;
        else if(p->actions.up)
            p->sprite_index = 4;
        else if(p->actions.down)
            p->sprite_index = 0;
        else if(p->actions.left)
            p->sprite_index = 2;
        else if(p->actions.right)
            p->sprite_index = 6;
    }
    
    GFXSubImageData* sid = gfx_images[p->image].sub_img_data;
    Rect* vr = &sid->visible_rects[p->sprite_index];
    p->phys.pos.w = vr->w*p->scale;
    p->phys.pos.h = vr->h*p->scale;
}

void player_gun_set_position(Player* p)
{
    // update gun
    // gun orientation X:
    // +----------------------+
    // |                      |
    // X                      B
    // |                      |
    // +----------------------+
    // bullet should spawn at B

    Rect* vr = &gfx_images[p->image].sub_img_data->visible_rects[p->sprite_index];

    // p->gun.angle = p->angle;
    float gx0 = p->phys.pos.x;
    float gy0 = p->phys.pos.y-vr->h*0.1;

    p->gun.angle = p->angle;

    if(role != ROLE_SERVER && p == player)
        p->gun.angle = calc_angle_rad(gx0, gy0, mouse_x, mouse_y);

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
}

void player_update(Player* p, double delta_t)
{
    p->actions.up               = IS_BIT_SET(p->keys,PLAYER_ACTION_UP);
    p->actions.down             = IS_BIT_SET(p->keys,PLAYER_ACTION_DOWN);
    p->actions.left             = IS_BIT_SET(p->keys,PLAYER_ACTION_LEFT);
    p->actions.right            = IS_BIT_SET(p->keys,PLAYER_ACTION_RIGHT);
    p->actions.run              = IS_BIT_SET(p->keys,PLAYER_ACTION_RUN);
    p->actions.jump             = IS_BIT_SET(p->keys,PLAYER_ACTION_JUMP);
    p->actions.interact         = IS_BIT_SET(p->keys,PLAYER_ACTION_INTERACT);
    p->actions.primary_action   = IS_BIT_SET(p->keys,PLAYER_ACTION_PRIMARY_ACTION);
    p->actions.secondary_action = IS_BIT_SET(p->keys,PLAYER_ACTION_SECONDARY_ACTION);
    p->actions.toggle_fire      = IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_FIRE);
    p->actions.toggle_debug     = IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_DEBUG);
    p->actions.toggle_gun       = IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_GUN);
    // p->actions.toggle_console   = IS_BIT_SET(p->keys,PLAYER_ACTION_CONSOLE);

    bool run_toggled = p->actions.run && !p->actions_prior.run;
    bool primary_action_toggled = p->actions.primary_action && !p->actions_prior.primary_action;
    bool fire_toggled = p->actions.toggle_fire && !p->actions_prior.toggle_fire;
    bool debug_toggled = p->actions.toggle_debug && !p->actions_prior.toggle_debug;
    bool gun_toggled = p->actions.toggle_gun && !p->actions_prior.toggle_gun;
    // bool console_toggled = p->actions.toggle_console && !p->actions_prior.toggle_console;

    memcpy(&p->actions_prior, &p->actions, sizeof(PlayerActions));

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


    if(p->actions.primary_action)
    {
        if(window_is_cursor_enabled())
        {
            window_disable_cursor();
        }
    }

    // if(console_toggled)
    // {
    //     console_enabled = !console_enabled;
    //     if(console_enabled)
    //     {
    //         memset(console_text, 0, CONSOLE_TEXT_MAX);
    //         window_set_key_mode(KEY_MODE_TEXT);
    //     }
    //     else
    //     {
    //         window_set_key_mode(KEY_MODE_NORMAL);
    //     }
    // }

    Vector2f accel = {0};
    bool player_moving = PLAYER_MOVING(p);

    if(p->actions.up)    { accel.y -= p->speed; }
    if(p->actions.down)  { accel.y += p->speed; }
    if(p->actions.left)  { accel.x -= p->speed; }
    if(p->actions.right) { accel.x += p->speed; }

    window_get_mouse_world_coords(&mouse_x, &mouse_y);

    player_update_sprite_index(p);


    if(p->gun_ready)
    {
        player_gun_set_position(p);

        if(p->actions.primary_action)
        {
            gun_fire(&p->gun, !primary_action_toggled);
        }
    }

    gun_update(&p->gun,delta_t);

    p->phys.max_linear_vel = p->max_base_speed;

    if(run_toggled)
    {
        p->running = !p->running;
    }

    if(p->running && player_moving)
    {
        accel.x *= 10.0;
        accel.y *= 10.0;
        p->phys.max_linear_vel *= 10.0;
    }

#if 0
    if(role == ROLE_SERVER)
    {
        printf("player pos: %f %f, accel: %f %f, delta_t: %f,map rect: %f %f %f %f",
                p->phys.pos.x,
                p->phys.pos.y,
                accel.x,
                accel.y,
                delta_t,
                map.rect.x,
                map.rect.y,
                map.rect.w,
                map.rect.h);
    }
#endif

    physics_begin(&p->phys);
    physics_add_friction(&p->phys, 16.0);
    physics_add_force(&p->phys, accel.x, accel.y);
    physics_simulate(&p->phys, delta_t);
    limit_pos(&map.rect, &p->phys.pos);

    if(role == ROLE_CLIENT)
    {
        // handle input
        memcpy(&p->input_prior, &p->input, sizeof(NetPlayerInput));

        p->input.delta_t = delta_t;
        p->input.keys = p->keys;
        p->input.angle = p->angle;
        
        if(p->input.keys != p->input_prior.keys || p->input.angle != p->input_prior.angle)
        {
            net_client_add_player_input(&p->input);

            if(net_client_get_input_count() >= 3) // @HARDCODED 3
            {
                // add position, angle to predicted player state
                NetPlayerState* state = &p->predicted_states[p->predicted_state_index];

                // circular buffer
                if(p->predicted_state_index == MAX_CLIENT_PREDICTED_STATES -1)
                {
                    // shift
                    for(int i = 1; i <= MAX_CLIENT_PREDICTED_STATES -1; ++i)
                    {
                        memcpy(&p->predicted_states[i-1],&p->predicted_states[i],sizeof(NetPlayerState));
                    }
                }
                else if(p->predicted_state_index < MAX_CLIENT_PREDICTED_STATES -1)
                {
                    p->predicted_state_index++;
                }

                state->associated_packet_id = net_client_get_latest_local_packet_id();
                state->pos.x = p->phys.pos.x;
                state->pos.y = p->phys.pos.y;
                state->angle = p->angle;
            }
        }
    }
}

void player_draw(Player* p)
{

    if(!is_in_camera_view(&p->phys.pos))
    {
        return;
    }

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
        gfx_draw_rect(&p->phys.pos, COLOR_RED, 1.0,1.0, false, true);
    }


    if(p->gun_ready)
    {
        if(!gun_drawn)
            gun_draw(&p->gun);

        GFXImage* img = &gfx_images[crosshair_image];
        gfx_draw_image(crosshair_image,mouse_x,mouse_y, COLOR_PURPLE,1.0,0.0,0.80);
    }


    char name[20] = {0};
    snprintf(name, 20, "Player: %d", p->index);
    Vector2f size = gfx_string_get_size(0.1, name);
    gfx_draw_string(p->phys.pos.x - size.x/2.0, p->phys.pos.y + p->phys.pos.h/2.0,player_colors[p->index],0.1,0.0, 0.8, true, true, name);

}
