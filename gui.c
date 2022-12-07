#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "main.h"
#include "window.h"
#include "gfx.h"
#include "timer.h"
#include "player.h"
#include "world.h"
#include "imgui.h"
#include "camera.h"
#include "lighting.h"
#include "zombie.h"
#include "particles.h"
#include "io.h"
#include "effects.h"
#include "editor.h"
#include "gui.h"

static void draw_debug_box();
static int gun_profile_image;

void gui_init()
{
    gun_profile_image = gfx_load_image("img/gun_profile_set.png", false, true, 64, 64, NULL);
    editor_init();
}

void gui_draw()
{
    // test print
    /*
    char* test_str = "Hejg'\",.; _-QW!M-\"(0)\"";
    Vector2f s1 = gfx_draw_string(0,0,0x0000CCFF,1.0,0.0,1.0, false,true,test_str); // @TEST
    Vector2f s2 = gfx_string_get_size(1.0,test_str);
    */

    if(debug_enabled)
    {
        //gfx_draw_rect(&gbox, 0x001F1F1F, 0.0, 1.0, 0.6, true, false);
        draw_debug_box();

        // GAME ROLE
        gfx_draw_string(0,view_height-(64*0.4)-2,0x0000CCFF,0.4,0.0, 0.7, false,true,"%s", game_role_to_str(role));
    }

    if(console_enabled)
    {
        float scale = 0.24;
        scale *= 1.0;

        Vector2f size = gfx_string_get_size(scale, "x");

        float yspace = size.y*1.5;

        Rect tbox = {0};
        tbox.w = view_width*0.6;
        tbox.x = view_width*0.5;

        tbox.h = yspace * (CONSOLE_MSG_MAX+1);
        tbox.y = tbox.h/2.0;

        gfx_draw_rect(&tbox, 0x001F1F1F, 0.0, 1.0, 0.6, true, false);

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

    // controls
    imgui_begin("Controls",view_width - 150,view_height - 130);
        imgui_set_text_size(16);
        imgui_text("Controls");
        imgui_set_text_size(10);
        imgui_indent_begin(10);
            imgui_text("W,A,S,D: Move");
            imgui_text("Tab: Ready/Unready gun");
            imgui_text("R: Reload");
            imgui_text("1,2: Cycle through guns");
            imgui_text("Shift: Toggle Run");
            imgui_text("F2: Toggle Debug");
            imgui_text("F3: Toggle Editor");
            imgui_text("F10: Open Console");
        imgui_indent_end();
    imgui_end();

    // player HUD
    {


        //float factor = (window_height / (float)view_height);
        int num_boxes = 7;
        float hotbar_padding = 3.0;
        float hotbar_bottom_padding = 10.0;
        float hotbar_box_size = 50.0;
        float half_hotbar_box_size = hotbar_box_size/2.0;
        float hotbar_size = (num_boxes*hotbar_box_size)+((num_boxes-1)*hotbar_padding);

        float curr_x = (view_width - hotbar_size)/2.0;
        float curr_y = view_height - hotbar_bottom_padding;

        curr_x += half_hotbar_box_size;
        curr_y -= half_hotbar_box_size;

        int selected_index = -1;
        if(player->item_equipped)
        {
            selected_index = player->item_index-1;
        }

        PlayerItem* item = &player->item;
        if(item->item_type == ITEM_TYPE_GUN)
        {
            // show bullet count
            Gun* g = (Gun*)item->props;
            gfx_draw_image_ignore_light(particles_image, 80, curr_x-half_hotbar_box_size+10,curr_y-half_hotbar_box_size-12, COLOR_TINT_NONE,0.7,45.0,1.0,true,false);
            gfx_draw_string(curr_x-half_hotbar_box_size+20,curr_y-half_hotbar_box_size-22,COLOR_WHITE,0.24,0.0,1.0,false,true,"x%d",g->bullets);
        }

        // draw health
        float health_bar_width  = 200.0;
        float red_width = health_bar_width*(player->hp/player->hp_max);
        float health_bar_height = 12.0;
        float health_bar_padding = 4.0;

        float health_bar_x = curr_x + health_bar_width/2.0;
        float health_bar_y = curr_y - (hotbar_box_size + health_bar_height)/2.0 - health_bar_padding;

        gfx_draw_rect_xywh(health_bar_x,health_bar_y,health_bar_width,health_bar_height,COLOR_BLACK,0.0,1.0,0.7,true,false);
        gfx_draw_rect_xywh(curr_x + red_width/2.0,health_bar_y,red_width,health_bar_height,0x00CC0000,0.0,1.0,0.4,true,false);
        
        // draw hotbar
        for(int i = 0; i < num_boxes; ++i)
        {
            uint32_t color = 0x00333333;
            if(i == selected_index)
            {
                color = 0x00448800;
            }

            // @TEMP
            int sprite_index;
            switch(i)
            {
                case 0: sprite_index = 1; break;
                case 1: sprite_index = 2; break;
                case 2: sprite_index = 0; break;
                default: sprite_index = -1; break; // no sprite
            }

            gfx_draw_rect_xywh(curr_x,curr_y,hotbar_box_size,hotbar_box_size,COLOR_WHITE,0.0,1.0,0.8,false,false);
            gfx_draw_rect_xywh(curr_x,curr_y,hotbar_box_size-1,hotbar_box_size-1,color,0.0,1.0,0.5,true,false);
            gfx_draw_image_ignore_light(gun_profile_image, sprite_index, curr_x,curr_y, COLOR_TINT_NONE,0.7,0.0,1.0,true,false);
            gfx_draw_string(curr_x-half_hotbar_box_size+1,curr_y-half_hotbar_box_size,COLOR_WHITE,0.2,0.0,1.0,false,true,"%d",i+1);

            curr_x += (hotbar_box_size + hotbar_padding);
        }
    }

    if(editor_enabled)
    {
        editor_draw();

        // for testing new gui features
        //imgui_draw_demo(10,size.y+20);
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
            Zombie* z = zombie_get_by_id(idx);
            if(z != NULL)
            {
                // printf("goto zombie %d\n", idx);
                player->phys.pos.x = z->phys.pos.x;
                player->phys.pos.y = z->phys.pos.y;
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
        // spawn <object> <num> <row> <col>
        char* s_object = string_split_index_copy(text, " ", 1, true);
        if(s_object == NULL)
        {
            FREE(s_object);
            return;
        }

        int row, col;
        coords_to_map_grid(player->phys.pos.x, player->phys.pos.y, &row, &col);

        int num = 1;
        char* s_num = string_split_index_copy(text, " ", 2, true);
        if(s_num != NULL)
        {
            num = atoi(s_num);
        }

        char* s_row = string_split_index_copy(text, " ", 3, true);
        char* s_col = string_split_index_copy(text, " ", 4, true);
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
            for(int i = 0; i < num; ++i)
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


ParticleSpawner* editor_get_particle_spawner()
{
    get_particle_spawner();
}


static void draw_debug_box()
{
    // window
    float fps = timer_get_prior_frame_fps(&game_timer);

    // player
    float pvx = player->phys.vel.x;
    float pvy = player->phys.vel.y;
    float pv = sqrt(SQ(pvx) + SQ(pvy));

    char keys[16+1] = {0};
    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        keys[i] = player->actions[i].state ? '1' : '0';
    }

    // mouse
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

    {
        float factor = (window_height / (float)view_height);
        float big = 23.0/factor;
        float small = 15.0/factor;

        imgui_begin_panel("Debug",950,10);
            imgui_set_text_size(small);

            imgui_text_sized(big,"Window");
            imgui_indent_begin(small);
                imgui_text("FPS: %.2f", fps);
                imgui_text("View: %d, %d", view_width, view_height);
                imgui_text("Window: %d, %d", window_width, window_height);
            imgui_indent_end();

            imgui_text_sized(big,"Player");
            imgui_indent_begin(small);
                imgui_text("Pos: %d, %d", (int)player->phys.pos.x, (int)player->phys.pos.y);
                imgui_text("Vel: %.2f, %.2f (%.2f)", pvx, pvy, pv);
                imgui_text("Angle: %.2f, %.2f deg", player->angle, DEG(player->angle));
                imgui_text("State: %s (%d)", player_state_str(player->state), player->state);
                imgui_text("Anim State: %s (%d)", player_anim_state_str(player->anim_state), player->anim_state);
                imgui_text("Detection Radius: %.2f", player->detect_radius);
                imgui_text("Controls: %s", keys);
            imgui_indent_end();

            imgui_text_sized(big,"Player Item");
            imgui_indent_begin(small);
                imgui_text("Equipped: %s", player->item_equipped ? "true" :  "false");
                imgui_text("Type: %s", player_item_type_str(player->item.item_type));
                imgui_text("Index: %d", player->item_index);

                if(player->item.props != NULL)
                {
                    if(player->item.item_type == ITEM_TYPE_MELEE)
                    {
                        Melee* melee = (Melee*)player->item.props;
                        imgui_text("  Melee: %s (%s)", melee->name, melee_type_str(melee->type));
                        imgui_text("  Range: %.0f", melee->range);
                        imgui_text("  Period: %.0f", melee->period);
                    }
                    else if(player->item.item_type == ITEM_TYPE_GUN)
                    {
                        Gun* gun = (Gun*)player->item.props;
                        imgui_text("  Gun: %s (%s)", gun->name, gun_type_str(gun->type));
                        imgui_text("  Range: %.0f", gun->fire_range);
                        imgui_text("  Period: %.0f", gun->fire_period);
                        imgui_text("  Bullets: %d (max: %d)", gun->bullets, gun->bullets_max);
                        imgui_text("  Reload Timer: %.0f", player->reload_timer);
                    }
                    else if(player->item.item_type == ITEM_TYPE_BLOCK)
                    {
                        BlockProp* bp = (BlockProp*)player->item.props;
                        imgui_text("  Type: %d", bp->type);
                    }
                }
            imgui_indent_end();

            imgui_text_sized(big,"Mouse");
            imgui_indent_begin(small);
                imgui_text("World:  %d, %d", wmx, wmy);
                imgui_text("View:   %d, %d", vmx, vmy);
                imgui_text("Window: %d, %d", mx, my);
                imgui_text("Map Grid:   %d, %d", mr, mc);
                imgui_text("World Grid: %d, %d", wr, wc);
            imgui_indent_end();
            
            imgui_text_sized(big,"Camera");
            imgui_indent_begin(small);
                imgui_text("Pos: %.2f, %.2f", camera_rect.x, camera_rect.y);
                imgui_text("w,h: %.2f, %.2f", camera_rect.w, camera_rect.h);
                imgui_text("Offset: %.2f, %.2f", aim_camera_offset.x, aim_camera_offset.y);
            imgui_indent_end();

            imgui_text_sized(big,"Zombies");
            imgui_indent_begin(small);
                imgui_text("Count: %d", zlist->count);
                Zombie* z = zombie_get_by_id(zombie_info_id);
                if(z != NULL)
                {
                    imgui_text("ID: %d", z->id);
                    imgui_text("HP: %.2f", z->hp);
                    imgui_text("Scale: %.2f", z->scale);
                    imgui_text("Anim State: %s (%d)", zombie_anim_state_str(z->anim_state), z->anim_state);
                }
            imgui_indent_end();

            if(role == ROLE_CLIENT)
            {
                imgui_text_sized(big,"Network");
                imgui_indent_begin(small);
                    imgui_text("Server IP: %s",server_ip_str);
                    imgui_text("Player count: %u",player_count);
                    imgui_text("Ping: %.0f ms",net_client_get_rtt());
                imgui_indent_end();
            }
        imgui_end();
    }

    return;

}

