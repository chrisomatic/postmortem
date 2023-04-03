#include "headers.h"
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

#define ENABLE_FPS_HIST 1

#if ENABLE_FPS_HIST
#define FPS_HIST_MAX  60
static float fps_hist[FPS_HIST_MAX] = {0};
static int fps_hist_index = 0;
static int fps_hist_count = 0;
static float fps_min = 0.0;
static float fps_max = 0.0;
#endif

static void update_fps_hist();
static void draw_debug_box();
static int item_profile_image;

#define MINIMAP_MAX_RANGE   1000
static uint8_t minimap_data[MINIMAP_MAX_RANGE*MINIMAP_MAX_RANGE*4] = {0};
static int minimap_image = -1;


static Vector2f skills_panel_size;

void gui_init()
{
    item_profile_image = gfx_load_image("src/img/item_profile_set.png", false, true, 64, 64);
    editor_init();

    minimap_image = gfx_raw_image_create(NULL, MINIMAP_MAX_RANGE, MINIMAP_MAX_RANGE, true);
}

void minimap_set_color(int index, uint32_t color, float opacity)
{
    minimap_data[index+0] = (color >> 16) & 0xFF;
    minimap_data[index+1] = (color >> 8)  & 0xFF;
    minimap_data[index+2] = (color >> 0)  & 0xFF;
    minimap_data[index+3] = 0xFF * opacity;
}

//@NOTE: row and col are relative to the minimap
void minimap_set_color2(int row, int col, int radius, uint32_t color, float opacity)
{
    int end = radius == 0 ? 1 : radius;

    for(int _row = row-radius; _row < row+end; ++_row)
    {
        for(int _col = col-radius; _col < col+end; ++_col)
        {
            if( _row < 0 || _col < 0 || _row >= minimap_range || _col >= minimap_range) continue;
            int index = (_row * minimap_range + _col)*4;
            minimap_set_color(index, color, opacity);
        }
    }

}


void minimap_draw()
{
    Rect r;
    get_camera_rect(&r);

    int mr, mc;
    coords_to_map_grid(r.x, r.y, &mr, &mc);

    int mrows, mcols;
    map_get_grid_dimensions(&mrows, &mcols);

    int mr1,mc1,mr2,mc2;

    int half_range = minimap_range/2;
    int rem_range = minimap_range - half_range;


    int start_row = mr - half_range;
    int start_col = mc - half_range;

    if(start_row < 0)
    {
        start_row = 0;
    }
    else if(mr+rem_range >= mrows)
    {
        start_row = mrows - minimap_range;
    }

    if(start_col < 0)
    {
        start_col = 0;
    }
    else if(mc+rem_range >= mcols)
    {
        start_col = mcols - minimap_range;
    }

    // printf("row, col:   %d, %d  -->  %d, %d\n", start_row, start_col, start_row+minimap_range-1, start_col+minimap_range-1);

    int idx = 0;
    int prow,pcol;

    for(int r = 0; r < minimap_range; ++r)
    {
        for(int c = 0; c < minimap_range; ++c)
        {

            int row = start_row + r;
            int col = start_col + c;
            uint32_t color = 0;
            uint8_t index = map_get_tile_index(row,col);
            if(index != 0xFF)
            {
                color = gfx_images[ground_sheet].avg_color[index];
            }
            minimap_set_color(idx, color, minimap_opacity);
            idx += 4;
        }
    }




    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(players[i].active)
        {
            uint32_t color = player_colors[players[i].index];
            coords_to_map_grid(players[i].phys.actual_pos.x, players[i].phys.actual_pos.y, &prow, &pcol);
            minimap_set_color2(prow-start_row, pcol-start_col, minimap_range/100, color, minimap_opacity);
        }
    }


    gfx_raw_image_update(minimap_image, (unsigned char*)minimap_data, minimap_range, minimap_range);

    float scale = (float)minimap_draw_size / (float)minimap_range;
    float draw_x = 10 + (minimap_range*scale)/2.0;
    float draw_y = view_height - 10 - (minimap_range*scale)/2.0;
    gfx_draw_image_ignore_light(minimap_image, 0, draw_x, draw_y, COLOR_TINT_NONE, scale, 0.0, 1.0, true, false);
}

void gui_draw()
{
    minimap_draw();

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


    }
    else
    {
        imgui_begin("",view_width - 55, 5);
        float fps = timer_get_prior_frame_fps(&game_timer);
        imgui_set_text_size(10);
        imgui_text("FPS: %.2f", fps);
        imgui_end();
    }

    // GAME ROLE
    imgui_begin("",view_width - 30, view_height - 15);
        imgui_set_text_size(10);
        imgui_text("%s", game_role_to_str(role));
    imgui_end();
    // gfx_draw_string(0,view_height-(64*0.4)-2,0x0000CCFF,0.4,0.0, 0.7, false,true,"%s", game_role_to_str(role));


    // controls
    /*
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
    */

    // player HUD
    {
        //float factor = (window_height / (float)view_height);
        int num_boxes = 7;
        float hotbar_padding = 3.0;
        float hotbar_bottom_padding = 10.0;
        float hotbar_box_size = 48.0;
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
            float x = (view_width + hotbar_size)/2.0;
            float y = curr_y-half_hotbar_box_size;
            gfx_draw_image_ignore_light(particles_image, 80, x+12,y+22, COLOR_TINT_NONE,0.7,45.0,1.0,true,false);
            gfx_draw_string(x+20,y+12,COLOR_WHITE,0.24,0.0,1.0,false,true,"x%d",g->bullets);
        }

        // draw health
        float health_bar_width  = hotbar_size;
        float red_width = health_bar_width*(player->hp/player->hp_max);
        float health_bar_height = 15.0;
        float health_bar_padding = 4.0;

        float health_bar_x = (view_width - hotbar_size)/2.0;
        float health_bar_y = curr_y - (hotbar_box_size + health_bar_height)/2.0 - health_bar_padding;

        gfx_draw_rect_xywh(health_bar_x + health_bar_width/2.0,health_bar_y,health_bar_width,health_bar_height,COLOR_BLACK,0.0,1.0,0.7,true,false);
        gfx_draw_rect_xywh(health_bar_x + red_width/2.0,health_bar_y,red_width,health_bar_height,0x00CC0000,0.0,1.0,0.4,true,false);
        gfx_draw_rect_xywh(health_bar_x + health_bar_width/2.0,health_bar_y,health_bar_width,health_bar_height,COLOR_BLACK,0.0,1.0,0.7,false,false); // border

        // draw level
        gfx_draw_string((view_width-hotbar_size)/2.0-14,view_height - hotbar_box_size/2.0,0x0000CCCC,0.3,0.0,1.0,false,true,"%d",player->level);
        float xp_width = health_bar_width*(player->xp/player->max_xp);
        gfx_draw_rect_xywh(health_bar_x + health_bar_width/2.0,view_height - 8,health_bar_width,3,COLOR_BLACK,0.0,1.0,0.9,true,false);
        gfx_draw_rect_xywh(health_bar_x + xp_width/2.0,view_height - 8,xp_width,3,0x0000FFFF,0.0,1.0,0.7,true,false);
        
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

    bool skills_menu_enabled = true;
    if(skills_menu_enabled)
    {
        imgui_begin_panel("Skills##Skills",view_width - skills_panel_size.x-10,view_height - skills_panel_size.y-10,true);
            //imgui_text_sized(18,"Skills");
            //imgui_horizontal_line();
            imgui_newline();
            imgui_indent_begin(10);

            for(int i = 0; i < SKILL_MAX; ++i)
            {
                char* skill_str = player_skill_get_name(i);
                char* skill_desc = player_skill_get_desc(i);
                
                if((i % 3) == 0)
                    imgui_horizontal_begin();

                imgui_tooltip("%s\n  %s",skill_str,skill_desc);
                if(imgui_image_button(particles_image,i,1.3,"%d##%s", player->skills[i],skill_str))
                {
                    if(player->avail_skill_points > 0)
                    {
                        player->skills[i]++;
                        player->avail_skill_points--;
                    }
                }
                if(((i+1) % 3) == 0)
                    imgui_horizontal_end();
            }
        imgui_horizontal_end();
        imgui_indent_end();
        imgui_text_colored(0x00888888,"Remaining Points: %d",player->avail_skill_points);
        skills_panel_size = imgui_end();
    }

    if(editor_enabled)
    {
        editor_draw();
        // for testing new gui features
        //imgui_draw_demo(10,size.y+20);
    }
    else
    {
        player->click_ready = true;
    }
}


static void update_fps_hist()
{
#if ENABLE_FPS_HIST
    float fps = timer_get_prior_frame_fps(&game_timer);

    fps_hist[fps_hist_index++] = fps;

    if(fps_hist_index >= FPS_HIST_MAX)
        fps_hist_index = 0;

    fps_hist_count = MIN(fps_hist_count+1, FPS_HIST_MAX);

    fps_min = fps;
    fps_max = fps;
    for(int i = 0; i < fps_hist_count; ++i)
    {
        float _fps = fps_hist[i];
        if(_fps < fps_min) fps_min = _fps;
        if(_fps > fps_max) fps_max = _fps;
    }
#endif
}


static void draw_debug_box()
{
    // window
    float fps = timer_get_prior_frame_fps(&game_timer);
    update_fps_hist();

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
        float _small = 15.0/factor;

        imgui_begin_panel("Debug",950,10,true);
            imgui_set_text_size(_small);

            imgui_text_sized(big,"Window");
            imgui_indent_begin(_small);
                imgui_text("FPS: %.2f", fps);
#if ENABLE_FPS_HIST
                imgui_text("Min FPS: %.2f", fps_min);
                imgui_text("Max FPS: %.2f", fps_max);
#endif
                imgui_text("View: %d, %d", view_width, view_height);
                imgui_text("Window: %d, %d", window_width, window_height);
            imgui_indent_end();

            imgui_text_sized(big,"Player");
            imgui_indent_begin(_small);
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
            imgui_indent_begin(_small);
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
            imgui_indent_begin(_small);
                imgui_text("World:  %d, %d", wmx, wmy);
                imgui_text("View:   %d, %d", vmx, vmy);
                imgui_text("Window: %d, %d", mx, my);
                imgui_text("Map Grid:   %d, %d", mr, mc);
                imgui_text("World Grid: %d, %d", wr, wc);
                imgui_text("Entity Count: %d", w_num_entities);
            imgui_indent_end();
            
            imgui_text_sized(big,"Camera");
            imgui_indent_begin(_small);
                // imgui_text("VH: %d", cam_view_height);
                imgui_text("Pos: %.2f, %.2f", camera_rect.x, camera_rect.y);
                imgui_text("w,h: %.2f, %.2f", camera_rect.w, camera_rect.h);
                imgui_text("Offset: %.2f, %.2f", aim_camera_offset.x, aim_camera_offset.y);
            imgui_indent_end();

            imgui_text_sized(big,"Zombies");
            imgui_indent_begin(_small);
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
                imgui_indent_begin(_small);
                    imgui_text("Server IP: %s",server_ip_str);
                    imgui_text("Player count: %u",player_count);
                    imgui_text("Ping: %.0f ms",net_client_get_rtt());
                imgui_indent_end();
            }
        imgui_end();
    }

    return;

}

