#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include "main.h"
#include "window.h"
#include "gfx.h"
#include "timer.h"
#include "player.h"
#include "gui.h"
#include "world.h"
#include "camera.h"

void gui_draw()
{
    Vector2f size = {0};

    // test print
    gfx_draw_string(0,view_height-22,0x0000CCFF,0.4,0.0, 0.7, false,true,"%s", game_role_to_str(role));

    if(debug_enabled)
    {
        // -----
        // Player
        // ------

        bool up               = IS_BIT_SET(player->keys,PLAYER_ACTION_UP);
        bool down             = IS_BIT_SET(player->keys,PLAYER_ACTION_DOWN);
        bool left             = IS_BIT_SET(player->keys,PLAYER_ACTION_LEFT);
        bool right            = IS_BIT_SET(player->keys,PLAYER_ACTION_RIGHT);
        bool run              = IS_BIT_SET(player->keys,PLAYER_ACTION_RUN);
        bool jump             = IS_BIT_SET(player->keys,PLAYER_ACTION_JUMP);
        bool interact         = IS_BIT_SET(player->keys,PLAYER_ACTION_INTERACT);
        bool primary_action   = IS_BIT_SET(player->keys,PLAYER_ACTION_PRIMARY_ACTION);
        bool secondary_action = IS_BIT_SET(player->keys,PLAYER_ACTION_SECONDARY_ACTION);
        bool toggle_fire      = IS_BIT_SET(player->keys,PLAYER_ACTION_TOGGLE_FIRE);
        bool toggle_debug     = IS_BIT_SET(player->keys,PLAYER_ACTION_TOGGLE_DEBUG);


        float start_x = 20.0, start_y = 20.0;
        float y = start_y;
        float ypad = 3.0;
        float scale = 0.1;
        float scale_big = 0.16;
        bool drop_shadow = true;

        Rect gui_bg = {0};
        gui_bg.w = 120;
        if(role == ROLE_CLIENT)
            gui_bg.h = 200;
        else
            gui_bg.h = 150;
        gui_bg.x = start_x-5 + gui_bg.w/2.0;
        gui_bg.y = start_y-5 + gui_bg.h/2.0;
        gfx_draw_rect(&gui_bg, 0x001F1F1F, 1.0, 0.6, true, false);

        // player
        size = gfx_draw_string(start_x+2, y,0x00FFFFFF,scale_big,0.0, 1.0, false, drop_shadow, "Player"); y += size.y+ypad;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "Pos: %d, %d", (int)player->phys.pos.x, (int)player->phys.pos.y); y += size.y+ypad;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "Controls: %d%d%d%d%d%d%d%d%d", up, down, left, right, run, jump, interact, primary_action, secondary_action); y += size.y+ypad;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "Angle: %.2f, %.2f deg", player->angle, DEG(player->angle)); y += size.y+ypad;

        // mouse
        float wmx, wmy;
        int vmx, vmy, mx, my, mr, mc, wr, wc;
        window_get_mouse_world_coords(&wmx, &wmy);
        window_get_mouse_view_coords(&vmx, &vmy);
        window_get_mouse_coords(&mx, &my);
        coords_to_map_grid(wmx, wmy, &mr, &mc);
        coords_to_world_grid(wmx, wmy, &wr, &wc);
        y += ypad;
        size = gfx_draw_string(start_x+2, y,0x00FFFFFF,scale_big,0.0, 1.0, false, drop_shadow, "Mouse"); y += size.y+5;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "World:  %.2f, %.2f", wmx, wmy); y += size.y+ypad;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "View:   %d, %d", vmx, vmy); y += size.y+ypad;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "Window: %d, %d", mx, my); y += size.y+ypad;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "Map Grid:   %d, %d", mr, mc); y += size.y+ypad;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "World Grid: %d, %d", wr, wc); y += size.y+ypad;

        // camera
        Rect camera_rect = {0};
        get_camera_rect(&camera_rect);
        y += ypad;
        size = gfx_draw_string(start_x+2, y,0x00FFFFFF,scale_big,0.0, 1.0, false, drop_shadow, "Camera"); y += size.y+ypad;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "Pos: %.2f, %.2f", camera_rect.x, camera_rect.y); y += size.y+ypad;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "w,h: %.2f, %.2f", camera_rect.w, camera_rect.h); y += size.y+ypad;
        size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "Offset: %.2f, %.2f", aim_camera_offset.x, aim_camera_offset.y); y += size.y+ypad;

        if(role == ROLE_CLIENT)
        {

            char server_ip_str[32] = {0};
            net_client_get_server_ip_str(server_ip_str);

            int player_count = net_client_get_player_count();

            // network
            y += ypad;
            size = gfx_draw_string(start_x+2, y,0x00FFFFFF,scale_big,0.0, 1.0, false, drop_shadow, "Network"); y += size.y+ypad;
            size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "Server IP: %s",server_ip_str); y += size.y+ypad;
            size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "Player count: %u",player_count); y += size.y+ypad;
            size = gfx_draw_string(start_x+10,y,0x00FFFFFF,scale,    0.0, 1.0, false, drop_shadow, "Ping: %.0f ms",net_client_get_rtt()); y += size.y+ypad;

        }

        float fps = timer_get_prior_frame_fps(&game_timer);
        size = gfx_string_get_size(scale, "fps: %.2f", fps);
        gfx_draw_string(view_width-size.x,0,0x00FFFF00,scale,0.0, 1.0, false,drop_shadow,"fps: %.2f", fps);

    }


}
