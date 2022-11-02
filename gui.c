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
#include "zombie.h"

static Rect gui_box = {0};
static float gui_start_x = 20.0, gui_start_y = 20.0;

static Vector2f gui_draw_string(bool draw, float x, float y, uint32_t color, float scale, float rotation, float opacity, bool in_world, bool drop_shadow, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char str[256] = {0};
    vsprintf(str,fmt, args);
    va_end(args);

    if(draw)
    {
        return gfx_draw_string(x, y, color, scale, rotation, opacity, in_world, drop_shadow, str);
    }

    return gfx_string_get_size(scale, str);
}

Rect gui_draw_text(bool draw, float wscale, float hscale)
{
    // float start_x = 20.0, start_y = 20.0;

    bool drop_shadow = true;

    float ypad = 3.0;
    float xpad = 10.0;
    float xpad_big = 2.0;
    float text_scale = 0.25;
    float text_scale_big = 0.3;

    ypad *= hscale;
    xpad *= wscale;
    xpad_big *= wscale;
    text_scale *= wscale;
    text_scale_big *= wscale;

    // window
    float fps = timer_get_prior_frame_fps(&game_timer);

    // player
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

    // mouse
    // float wmx, wmy;
    int wmx, wmy, vmx, vmy, mx, my, mr, mc, wr, wc;
    window_get_mouse_world_coords(&wmx, &wmy);
    window_get_mouse_view_coords(&vmx, &vmy);
    window_get_mouse_coords(&mx, &my);
    coords_to_map_grid(wmx, wmy, &mr, &mc);
    coords_to_world_grid(wmx, wmy, &wr, &wc);

    // camera
    Rect camera_rect = {0};
    get_camera_rect(&camera_rect);

    // network
    char server_ip_str[32] = {0};
    net_client_get_server_ip_str(server_ip_str);
    int player_count = net_client_get_player_count();

    Vector2f size = {0};
    float maxw = 0.0;

    float y = gui_start_y;
    float x = gui_start_x;


    // window
    y += ypad;
    size = gui_draw_string(draw, x+xpad_big, y,0x00FFFFFF,text_scale_big,0.0, 1.0, false, drop_shadow, "Window"); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "FPS: %.2f", fps); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "View: %d, %d", view_width, view_height); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Window: %d, %d", window_width, window_height); y += size.y+ypad; maxw = MAX(maxw, size.x);

    // player
    y += ypad;
    size = gui_draw_string(draw, x+xpad_big, y,0x00FFFFFF,text_scale_big,0.0, 1.0, false, drop_shadow, "Player"); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Pos: %d, %d", (int)player->phys.pos.x, (int)player->phys.pos.y); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Controls: %d%d%d%d%d%d%d%d%d", up, down, left, right, run, jump, interact, primary_action, secondary_action); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Angle: %.2f, %.2f deg", player->angle, DEG(player->angle)); y += size.y+ypad; maxw = MAX(maxw, size.x);

    // mouse
    y += ypad;
    size = gui_draw_string(draw, x+xpad_big, y,0x00FFFFFF,text_scale_big,0.0, 1.0, false, drop_shadow, "Mouse"); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "World:  %d, %d", wmx, wmy); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "View:   %d, %d", vmx, vmy); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Window: %d, %d", mx, my); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Map Grid:   %d, %d", mr, mc); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "World Grid: %d, %d", wr, wc); y += size.y+ypad; maxw = MAX(maxw, size.x);

    // camera
    y += ypad;
    size = gui_draw_string(draw, x+xpad_big, y,0x00FFFFFF,text_scale_big,0.0, 1.0, false, drop_shadow, "Camera"); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Pos: %.2f, %.2f", camera_rect.x, camera_rect.y); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "w,h: %.2f, %.2f", camera_rect.w, camera_rect.h); y += size.y+ypad; maxw = MAX(maxw, size.x);
    size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Offset: %.2f, %.2f", aim_camera_offset.x, aim_camera_offset.y); y += size.y+ypad; maxw = MAX(maxw, size.x);


    // network
    if(role == ROLE_CLIENT)
    {
        y += ypad;
        size = gui_draw_string(draw, x+xpad_big, y,0x00FFFFFF,text_scale_big,0.0, 1.0, false, drop_shadow, "Network"); y += size.y+ypad; maxw = MAX(maxw, size.x);
        size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Server IP: %s",server_ip_str); y += size.y+ypad; maxw = MAX(maxw, size.x);
        size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Player count: %u",player_count); y += size.y+ypad; maxw = MAX(maxw, size.x);
        size = gui_draw_string(draw, x+xpad,y,0x00FFFFFF,text_scale,    0.0, 1.0, false, drop_shadow, "Ping: %.0f ms",net_client_get_rtt()); y += size.y+ypad; maxw = MAX(maxw, size.x);
    }

    Rect ret = {0};
    ret.w = maxw+xpad*2.0;
    ret.h = y-gui_start_y+ypad;
    ret.x = gui_start_x + ret.w/2.0;
    ret.y = gui_start_y + ret.h/2.0;
    return ret;

}


void gui_draw()
{

    // test print
    /*
    char* test_str = "Hejg'\",.; _-QW!M-\"(0)\"";
    Vector2f s1 = gfx_draw_string(0,0,0x0000CCFF,1.0,0.0,1.0, false,true,test_str); // @TEST
    Vector2f s2 = gfx_string_get_size(1.0,test_str);
    */

    bool have_rect = !(FEQ(gui_box.w,0.0) || FEQ(gui_box.h,0.0));
    if(!have_rect)
    {
        gui_box = gui_draw_text(false, 1.0, 1.0);
    }
    float wscale = (window_width/(float)view_width);
    float hscale = (window_height/(float)view_height);
    wscale = 1.0/wscale;
    hscale = 1.0/hscale;

    Rect gbox = {0};
    gbox.w = gui_box.w*(wscale);
    gbox.h = gui_box.h*(hscale);
    gbox.x = gui_start_x + gbox.w/2.0;
    gbox.y = gui_start_y + gbox.h/2.0;

    // GAME ROLE
    gfx_draw_string(0,view_height-(64*0.4)-2,0x0000CCFF,0.4,0.0, 0.7, false,true,"%s", game_role_to_str(role));

    if(debug_enabled)
    {
        gfx_draw_rect(&gbox, 0x001F1F1F, 1.0, 0.6, true, false);
        gui_draw_text(true, wscale, hscale);
    }

    if(console_enabled)
    {
        float scale = 0.24;
        scale *= wscale;

        Vector2f size = gfx_string_get_size(scale, "x");

        float yspace = size.y*1.5;

        Rect tbox = {0};
        tbox.w = view_width*0.6;
        if(debug_enabled)
            tbox.x = gbox.x+gbox.w/2.0 + 5.0 + tbox.w/2.0;
        else
            tbox.x = view_width*0.5;

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

}

void console_message_add(uint32_t color, char* fmt, ...)
{
    // 0 is oldest
    if(console_msg_count >= CONSOLE_MSG_MAX)
    {
        for(int i = 1; i < CONSOLE_MSG_MAX; ++i)
        {
            memcpy(&console_msg[i-1], &console_msg[i], sizeof(ConsoleMessage));
        }
    }

    int index = MIN(console_msg_count, CONSOLE_MSG_MAX-1);

    va_list args;
    va_start(args, fmt);
    vsnprintf(console_msg[index].msg, CONSOLE_TEXT_MAX+1, fmt, args);
    va_end(args);

    console_msg[index].color = color;
    console_msg_count = MIN(console_msg_count+1, CONSOLE_MSG_MAX);
    // for(int i = 0; i < console_msg_count; ++i)
    // {
    //     printf("%d) %s\n", i, console_msg[i].msg);
    // }
}

void console_text_hist_add(char* text)
{
    console_message_add(COLOR_WHITE, "%s%s", CONSOLE_PROMPT, text);

    if(!STR_EQUAL(text, console_text_hist[console_text_hist_index]))
    {
        int next_index = console_text_hist_index+1;
        if(next_index >= CONSOLE_HIST_MAX) next_index = 0;
        memset(console_text_hist[next_index], 0, CONSOLE_TEXT_MAX);
        memcpy(console_text_hist[next_index], text, strlen(text));
        console_text_hist_index = next_index;
        // printf("added to index: %d\n", next_index);
    }
}

// gets next non-empty index
int console_text_hist_get(int direction)
{
    int index = console_text_hist_selection;
    if(index == -1)
    {
        index = console_text_hist_index;
        if(direction == -1)
            index += 1;
    }

    // printf("start search at index: %d\n", index);
    for(int i = 0; i < CONSOLE_HIST_MAX; ++i)
    {
        index += direction;
        if(index < 0) index = CONSOLE_HIST_MAX-1;
        if(index >= CONSOLE_HIST_MAX) index = 0;
        if(!STR_EMPTY(console_text_hist[index]))
            return index;
    }
    return -1;
}

//TODO: some of these commands should only work locally
void run_console_command(char* text)
{
    if(STR_EMPTY(text))
        return;

    LOGI("parse command: '%s'", text);

    console_text_hist_add(text);
    console_text_hist_selection = -1;


    int cmd_len = 0;
    char* cmd = string_split_index(text, " ", 0, &cmd_len, true);

    // this is the case if there's a space before the command
    if(cmd_len == 0) return;

    // char* arg0 = string_split_index_copy(text, " ", 1, true);
    // char* args = string_split_index_copy(text, " ", 1, false);
    // printf("arg0: %s\n", arg0 == NULL ? "NULL" : arg0);
    // printf("args: %s\n", args == NULL ? "NULL" : args);
    // if(arg0 != NULL) free(arg0);
    // if(args != NULL) free(args);

    // printf("cmdlen: %d\n", cmd_len);
    LOGI("  cmd: '%.*s'", cmd_len, cmd);

    if(STRN_EQUAL(cmd,"exit",cmd_len))
    {
        // printf("closing\n");
        window_set_close(1);
    }
    else if(STRN_EQUAL(cmd,"setname",cmd_len))
    {
        // setname <name>
        // char* name = cmd+cmd_len+1; //+1 for space delimiter
        char* name = string_split_index_copy(text, " ", 1, false);
        if(name != NULL)
        {
            memset(player->name, 0, PLAYER_NAME_MAX);
            memcpy(player->name, name, MIN(strlen(name),PLAYER_NAME_MAX));
            FREE(name);
        }
    }
    else if(STRN_EQUAL(cmd,"teleport",cmd_len) || STRN_EQUAL(cmd,"tp",cmd_len))
    {
        // teleport <row> <col>
        char* s_row = string_split_index_copy(text, " ", 1, true);
        char* s_col = string_split_index_copy(text, " ", 2, true);

        if(s_row == NULL || s_col == NULL)
        {
            FREE(s_row);
            FREE(s_col);
            return;
        }

        int row = atoi(s_row);
        int col = atoi(s_col);

        row = RANGE(row, 0, map.rows-1);
        col = RANGE(col, 0, map.cols-1);
        // printf("row, col: %d, %d\n", row, col);
        float x,y;
        map_grid_to_coords(row, col, &x, &y);
        player->phys.pos.x = x;
        player->phys.pos.y = y;

        FREE(s_row);
        FREE(s_col);
    }
    else if(STRN_EQUAL(cmd,"goto",cmd_len))
    {
        // goto <object> <index>

        char* s_object = string_split_index_copy(text, " ", 1, true);
        char* s_index = string_split_index_copy(text, " ", 2, true);

        if(s_object == NULL || s_index == NULL)
        {
            FREE(s_object);
            FREE(s_index);
            return;
        }

        int idx = atoi(s_index);

        if(STR_EQUAL(s_object,"zombie"))
        {
            if(idx < zlist->count)
            {
                // printf("goto zombie %d\n", idx);
                player->phys.pos.x = zombies[idx].phys.pos.x;
                player->phys.pos.y = zombies[idx].phys.pos.y;
            }
        }
        else if(STR_EQUAL(s_object,"player"))
        {
            // printf("goto player %d (my index: %d)\n", idx, player->index);
            if(idx != player->index && idx < MAX_CLIENTS)
            {
                if(players[idx].active)
                {
                    player->phys.pos.x = players[idx].phys.pos.x;
                    player->phys.pos.y = players[idx].phys.pos.y;
                }
            }
        }

        FREE(s_object);
        FREE(s_index);
    }
    else if(STRN_EQUAL(cmd,"spawn",cmd_len))
    {
        // spawn <object> <row> <col>
        char* s_object = string_split_index_copy(text, " ", 1, true);
        if(s_object == NULL)
        {
            FREE(s_object);
            return;
        }

        int row, col;
        coords_to_map_grid(player->phys.pos.x, player->phys.pos.y, &row, &col);

        char* s_row = string_split_index_copy(text, " ", 2, true);
        char* s_col = string_split_index_copy(text, " ", 3, true);
        if(s_row != NULL && s_col != NULL)
        {
            row = atoi(s_row);
            col = atoi(s_col);
        }

        float x, y;
        map_grid_to_coords(row, col, &x, &y);

        if(STR_EQUAL(s_object,"zombie"))
        {
            ZombieSpawn spawn = {0};
            spawn.pos.x = x;
            spawn.pos.y = y;
            zombie_add(&spawn);
        }
        else if(STR_EQUAL(s_object,"player"))
        {
            for(int i = 0; i < MAX_CLIENTS; ++i)
            {
                if(!players[i].active)
                {
                    players[i].phys.pos.x = x;
                    players[i].phys.pos.y = y;
                    players[i].active = true;
                    break;
                }
            }
        }

        FREE(s_object);
        FREE(s_row);
        FREE(s_col);
    }
}
