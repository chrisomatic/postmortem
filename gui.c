#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
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
    /*
    char* test_str = "Hejg'\",.; _-QW!M-\"(0)\"";
    Vector2f s1 = gfx_draw_string(0,0,0x0000CCFF,1.0,0.0,1.0, false,true,test_str); // @TEST
    Vector2f s2 = gfx_string_get_size(1.0,test_str);
    */

    if(console_enabled)
    {
        float scale = 0.12;

        Vector2f size = gfx_string_get_size(scale, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890|[](){}");
        // Vector2f size = gfx_string_get_size(scale, "j");


        float yspace = size.y*1.5;

        Rect tbox = {0};
        tbox.x = view_width*0.5;
        tbox.w = view_width*0.6;
        tbox.h = yspace * (CONSOLE_MSG_MAX+1);
        tbox.y = tbox.h/2.0;

        gfx_draw_rect(&tbox, 0x001F1F1F, 1.0, 0.6, true, false);


        float x0 = tbox.x-tbox.w/2.0;
        float y0 = tbox.y-tbox.h/2.0;

        for(int i = 0; i < console_msg_count; ++i)
        {
            float y = y0+i*yspace;
            float x = x0+1.0;
            gfx_draw_string(x, y, console_msg[i].color, scale, 0.0, 1.0, false, false, "%s", console_msg[i].msg);// y += size.y+ypad;
        }



        // float y0 = tbox.y-tbox.h/1.5/2.0;

        // printf("console_text[0] = %d\n", (int)console_text[0]);
        // const char* prompt = "cmd > ";
        Vector2f psize = gfx_string_get_size(scale, (char*)CONSOLE_PROMPT);

        int index = 0;
        int tlen = strlen(console_text);
        if(tlen > 0)
        {
            if(console_text[tlen-1] == '\n')
            {
                console_text[tlen-1] = '\0';
                // printf("submit command: '%s'\n", console_text);
                run_console_command(console_text);
                memset(console_text, 0, CONSOLE_TEXT_MAX*sizeof(console_text[0]));
            }
            else
            {
                float x1 = x0+1.0+tbox.w;
                for(int i = 0; i < tlen; ++i)
                {
                    Vector2f tsize = gfx_string_get_size(scale, console_text+i);
                    
                    if(x0 + psize.x + tsize.x <= x1)
                    {
                        index = i;
                        break;
                    }

                }
            }
        }

        size = gfx_draw_string(x0+1.0, y0+yspace*(CONSOLE_MSG_MAX), COLOR_WHITE, scale, 0.0, 1.0, false, false, "%s%s", CONSOLE_PROMPT, console_text+index);// y += size.y+ypad;

    }

    gfx_draw_string(0,view_height-(64*0.4)-2,0x0000CCFF,0.4,0.0, 0.7, false,true,"%s", game_role_to_str(role));

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
            gui_bg.h = 220;
        else
            gui_bg.h = 170;
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
        size = gfx_draw_string(start_x+2, y,0x00FFFFFF,scale_big,0.0, 1.0, false, drop_shadow, "Mouse"); y += size.y+ypad;
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
