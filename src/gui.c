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
#include "entity.h"
#include "editor.h"
#include "gui.h"

static void draw_debug_box();
static int item_profile_image;

void gui_init()
{
    item_profile_image = gfx_load_image("src/img/item_profile_set.png", false, true, 64, 64, NULL);
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
    else
    {
        imgui_begin("",view_width - 55, 5);
        float fps = timer_get_prior_frame_fps(&game_timer);
        imgui_set_text_size(10);
        imgui_text("FPS: %.2f", fps);
    }


    // controls
    imgui_begin("Controls",view_width - 150,view_height - 130);
        imgui_set_text_size(16);
        imgui_text("Controls");
        imgui_set_text_size(10);
        imgui_indent_begin(10);
            imgui_text("W,A,S,D: Move");
            imgui_text("Tab: Ready/Unready item");
            imgui_text("R: Reload");
            imgui_text("1-6: Cycle through items");
            imgui_text("Shift: Toggle Run");
            imgui_text("F2: Toggle Debug");
            imgui_text("F3: Toggle Editor");
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
                case 3: sprite_index = 3; break;
                case 4: sprite_index = 4; break;
                case 5: sprite_index = 5; break;
                default: sprite_index = -1; break; // no sprite
            }

            gfx_draw_rect_xywh(curr_x,curr_y,hotbar_box_size,hotbar_box_size,COLOR_WHITE,0.0,1.0,0.8,false,false);
            gfx_draw_rect_xywh(curr_x,curr_y,hotbar_box_size-1,hotbar_box_size-1,color,0.0,1.0,0.5,true,false);
            gfx_draw_image_ignore_light(item_profile_image, sprite_index, curr_x,curr_y, COLOR_TINT_NONE,0.7,0.0,1.0,true,false);
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

static void draw_debug_box()
{
    // window
    float fps = timer_get_prior_frame_fps(&game_timer);

    // player
    float pvx = player->phys.vel.x;
    float pvy = player->phys.vel.y;
    float pv = sqrt(SQ(pvx) + SQ(pvy));

    char keys[32+1] = {0};
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
    int w_num_entities = grid_boxes[wr][wc].num;

    // camera
    Rect camera_rect = {0};
    get_camera_rect(&camera_rect);

    // network
    char server_ip_str[32] = {0};
    net_client_get_server_ip_str(server_ip_str);
    int player_count = net_client_get_player_count();

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
                imgui_text("Actual Pos: %d, %d", (int)player->phys.actual_pos.x, (int)player->phys.actual_pos.y);
                imgui_text("Vel: %.2f, %.2f (%.2f)", pvx, pvy, pv);
                imgui_text("Angle: %.2f, %.2f deg", player->angle, DEG(player->angle));
                imgui_text("State: %s (%d)", player_state_str(player->state), player->state);
                imgui_text("Anim State: %s (%d)", player_anim_state_str(player->anim_state), player->anim_state);
                imgui_text("Detection Radius: %.2f", player->detect_radius);
                imgui_text("Controls: %s", keys);
                imgui_newline();
                imgui_text("Blocked Up:    %s", player->phys.blocked[0] ? "True" : "False");
                imgui_text("Blocked Down:  %s", player->phys.blocked[1] ? "True" : "False");
                imgui_text("Blocked Left:  %s", player->phys.blocked[2] ? "True" : "False");
                imgui_text("Blocked Right: %s", player->phys.blocked[3] ? "True" : "False");
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
                imgui_text("Entity Count: %d", w_num_entities);
            imgui_indent_end();
            
            imgui_text_sized(big,"Camera");
            imgui_indent_begin(small);
                // imgui_text("VH: %d", cam_view_height);
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

