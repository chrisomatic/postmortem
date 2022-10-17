#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include "main.h"
#include "window.h"
#include "gfx.h"
#include "timer.h"
#include "player.h"
#include "gui.h"

void gui_draw()
{
    gfx_draw_string("Kameron",player.phys.pos.x,player.phys.pos.y + player.h,0x00FFFFFF,0.1,0.0, 1.0, true);

    if(debug_enabled)
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


        float start_x = 20.0, start_y = 20.0;

        float scale = 0.1;

        float h;
        gfx_string_get_size("Hello",scale,NULL, &h);
        h+=4;

        // player stats
        float y = start_y;
        gfx_draw_string("Player",start_x+2.0,y,0x00FFFFFF,0.16,0.0, 1.0, false); y += 12;
        gfx_draw_stringf(start_x+10,y   ,0x00FFFFFF,scale,0.0, 1.0, false,"Pos: %d, %d", (int)player.phys.pos.x, (int)player.phys.pos.y);
        gfx_draw_stringf(start_x+10,y+=h,0x00FFFFFF,scale,0.0, 1.0, false,"Controls: %d%d%d%d%d%d%d%d%d", up, down, left, right, run, jump, interact, primary_action, secondary_action);
        gfx_draw_stringf(start_x+10,y+=h,0x00FFFFFF,scale,0.0, 1.0, false,"Angle: %.2f, %.2f deg", player.angle, DEG(player.angle));
        
        float mx, my;
        window_get_mouse_world_coords(&mx, &my);
        gfx_draw_stringf(start_x+10,y+=h,0x00FFFFFF,scale,0.0, 1.0, false,"Mouse: %.2f, %.2f", mx, my);

        // fps
        float w;
        float fps = timer_get_prior_frame_fps(&game_timer);
        gfx_string_get_size("fps: 60.00",scale, &w, NULL);
        w+=2;

        gfx_draw_stringf(view_width-w,0,0x00FFFF00,scale,0.0, 1.0, false,"fps: %.2f", fps);
    }


}
