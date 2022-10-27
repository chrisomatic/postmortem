#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include "main.h"
#include "window.h"
#include "shader.h"
#include "timer.h"
#include "gfx.h"
#include "math2d.h"
#include "world.h"
#include "camera.h"
#include "player.h"
#include "gun.h"
#include "projectile.h"
#include "zombie.h"
#include "gui.h"
#include "net.h"
#include "log.h"
#include "bitpack.h"

// Settings
#define VIEW_WIDTH   800
#define VIEW_HEIGHT  600

// =========================
// Global Vars
// =========================

Timer game_timer = {0};
GameRole role;
Vector2f aim_camera_offset = {0};

// gui might be a good spot for some of these variables
char console_text[CONSOLE_TEXT_MAX+1] = {0};

char console_text_hist[CONSOLE_HIST_MAX][CONSOLE_TEXT_MAX+1] = {{0}};
int console_text_hist_index = 0;
int console_text_hist_selection = 0;

bool console_enabled = false;
bool debug_enabled = true;

bool backspace_held = false;
double t0_backspace = 0.0;

// =========================
// Function Prototypes
// =========================

void parse_args(int argc, char* argv[]);
void start_local();
void start_client();
void start_server();
void init();
void deinit();
void simulate(double);
void simulate_client(double);
void draw();
void key_cb(GLFWwindow* window, int key, int scan_code, int action, int mods);

// =========================
// Main Loop
// =========================


int main(int argc, char* argv[])
{
    parse_args(argc, argv);

    switch(role)
    {
        case ROLE_LOCAL:
            start_local();
            break;
        case ROLE_CLIENT:
            start_client();
            break;
        case ROLE_SERVER:
            start_server();
            break;
    }

    return 0;
}

// =========================
// Functions
// =========================

void parse_args(int argc, char* argv[])
{
    role = ROLE_LOCAL;

    if(argc > 1)
    {
        for(int i = 1; i < argc; ++i)
        {
            if(argv[i][0] == '-' && argv[i][1] == '-')
            {
                // server
                if(strncmp(argv[i]+2,"server",6) == 0)
                    role = ROLE_SERVER;

                // client
                else if(strncmp(argv[i]+2,"client",6) == 0)
                    role = ROLE_CLIENT;
            }
            else
            {
                net_client_set_server_ip(argv[i]);
            }
        }
    }
}

const char* game_role_to_str(GameRole _role)
{
    switch(_role)
    {
        case ROLE_LOCAL:  return "Local";
        case ROLE_CLIENT: return "Client";
        case ROLE_SERVER: return "Server";
    }
    return "Unknown";
}

void start_local()
{
    LOGI("--------------");
    LOGI("Starting Local");
    LOGI("--------------");

    time_t t;
    srand((unsigned) time(&t));

    // player = &players[0];

    init();

    timer_set_fps(&game_timer,TARGET_FPS);
    timer_begin(&game_timer);

    double t0=timer_get_time();
    double t1=0.0;

    //bitpack_test(); //@TEMP

    // main game loop
    for(;;)
    {
        window_poll_events();

        handle_backspace_timer();


        if(window_should_close())
        {
            break;
        }


        t1 = timer_get_time();

        simulate(t1-t0);
        draw();

        timer_wait_for_frame(&game_timer);
        window_swap_buffers();
        t0 = t1;

    }

    deinit();
}

void start_client()
{
    LOGI("---------------");
    LOGI("Starting Client");
    LOGI("---------------");

    time_t t;
    srand((unsigned) time(&t));

    timer_set_fps(&game_timer,TARGET_FPS);
    timer_begin(&game_timer);

    net_client_init();

    int client_id = net_client_connect();
    if(client_id < 0)
        return;

    LOGN("Client ID: %d", client_id);
    player = &players[client_id];

    init();

    double t0=timer_get_time();
    double t1=0.0;

    // main game loop
    for(;;)
    {
        window_poll_events();

        handle_backspace_timer();

        if(window_should_close())
        {
            break;
        }

        if(!net_client_is_connected())
            break;

        t1 = timer_get_time();

        double delta_t = t1-t0;

        simulate_client(delta_t); // client-side prediction

        //printf("player pos %f %f, angle %f\n",player->phys.pos.x, player->phys.pos.y, player->angle);
        net_client_update();

        draw();

        timer_wait_for_frame(&game_timer);
        window_swap_buffers();
        t0 = t1;
    }

    net_client_disconnect();
    deinit();
}

void start_server()
{
    LOGI("---------------");
    LOGI("Starting Server");
    LOGI("---------------");

    time_t t;
    srand((unsigned) time(&t));

    view_width = VIEW_WIDTH;
    view_height = VIEW_HEIGHT;

    // server init
    gfx_image_init();
    world_init();
    gun_init();

    players_init();

    zombie_init();
    projectile_init();

    net_server_start();
}

void init()
{
    bool success;

    LOGI("resolution: %d %d",VIEW_WIDTH, VIEW_HEIGHT);
    success = window_init(VIEW_WIDTH, VIEW_HEIGHT);

    if(!success)
    {
        fprintf(stderr,"Failed to initialize window!\n");
        exit(1);
    }

    window_controls_set_cb(key_cb);
    window_controls_set_text_buf(console_text, CONSOLE_TEXT_MAX);
    window_controls_set_key_mode(KEY_MODE_NORMAL);

    LOGI("Initializing...");

    LOGI(" - Shaders.");
    shader_load_all();

    LOGI(" - Graphics.");
    gfx_init(VIEW_WIDTH, VIEW_HEIGHT);

    LOGI(" - Camera.");
    camera_init();

    LOGI(" - World.");
    world_init();

    LOGI(" - Guns.");
    gun_init();

    LOGI(" - Player.");
    players_init();

    LOGI(" - Zombies.");
    zombie_init();

    LOGI(" - Projectiles.");
    projectile_init();

    camera_move(player->phys.pos.x, player->phys.pos.y, true);
}

void deinit()
{
    shader_deinit();
    window_deinit();
}

// also checks if the mouse is off the screen
void camera_set()
{
    int mx, my;
    window_get_mouse_view_coords(&mx, &my);

    if(player->gun_ready)
    {
        float r = 0.2;  //should be <= 0.5 to make sense otherwise player will end up off of the screen
        float ox = (mx - view_width/2.0);
        float oy = (my - view_height/2.0);
        float xr = view_width*r;
        float yr = view_height*r;
        ox = 2.0*xr*(ox/view_width);
        oy = 2.0*yr*(oy/view_height);
        ox = RANGE(ox, -1.0*xr, xr);
        oy = RANGE(oy, -1.0*yr, yr);
        aim_camera_offset.x = ox;
        aim_camera_offset.y = oy;
    }
    else
    {
        aim_camera_offset.x = 0.0;
        aim_camera_offset.y = 0.0;
    }

    if(!window_is_cursor_enabled())
    {
        if(mx >= view_width || mx <= 0 || my >= view_height || my <= 0)
        {
            int new_mx = RANGE(mx, 0, view_width);
            int new_my = RANGE(my, 0, view_height);
            window_set_mouse_view_coords(new_mx, new_my);
        }
    }

    float cam_pos_x = player->phys.pos.x + aim_camera_offset.x;
    float cam_pos_y = player->phys.pos.y + aim_camera_offset.y;
    Rect cam_rect = {0};
    cam_rect.x = cam_pos_x;
    cam_rect.y = cam_pos_y;
    cam_rect.w = view_width;
    cam_rect.h = view_height;
    limit_pos(&map.rect, &cam_rect);
    camera_move(cam_rect.x, cam_rect.y, false);
}

void simulate(double delta_t)
{
    gfx_clear_lines();

    camera_set();
    camera_update();

    world_update();
    zombie_update(delta_t);
    player_update(player,delta_t);
    projectile_update(delta_t);
}

void simulate_client(double delta_t)
{
    gfx_clear_lines();

    camera_set();
    camera_update();

    world_update();
    //zombie_update(delta_t);
    player_update(player,delta_t);

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(players[i].active && &players[i] != player)
        {
            player_update_other(&players[i], delta_t);
        }
    }

    projectile_update(delta_t);

    // if(!window_is_cursor_enabled())
    //     window_set_mouse_world_coords(400.0,1000.0);

}

void draw()
{
    gfx_clear_buffer(50,50,50);

    world_draw();
    gfx_draw_lines();

    zombie_draw();
    projectile_draw();

    // static bool activate_player = false;
    // if(!activate_player)
    // {
    //     players[2].active = true;
    //     players[2].phys.pos.x = 1000;
    //     players[2].phys.pos.y = 1000;
    //     players[2].phys.pos.w = 25;
    //     players[2].phys.pos.h = 60;
    //     players[2].sprite_index = 1;
    //     player_count++;
    //     activate_player = true;
    // }

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        Player* p = &players[i];
        if(p->active)
        {
            player_draw(p);
            if(p != player)
            {
                bool in_view = is_in_camera_view(&p->phys.pos);
                if(!in_view)
                {
                    Rect camera_rect = {0};
                    get_camera_rect(&camera_rect);
                    // float angle = calc_angle_rad(player->phys.pos.x, player->phys.pos.y, p->phys.pos.x, p->phys.pos.y);
                    Rect prect = {0};
                    memcpy(&prect, &p->phys.pos, sizeof(Rect));
                    prect.w = 4.0;
                    prect.h = 4.0;
                    limit_pos(&camera_rect, &prect);
                    gfx_draw_rect(&prect, player_colors[p->index], 1.0,1.0,true,true);
                }
            }

        }
    }

    gui_draw();
}


void key_cb(GLFWwindow* window, int key, int scan_code, int action, int mods)
{
    // printf("key: %d, action: %d\n", key, action);
    bool ctrl = (mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL;

    KeyMode kmode = window_controls_get_key_mode();
    if(kmode == KEY_MODE_NORMAL)
    {
        if(action == GLFW_PRESS)
        {
            if(key == GLFW_KEY_Q)
            {
                window_set_close(1);
            }
            else if(key == GLFW_KEY_F10)
            {
                window_controls_set_key_mode(KEY_MODE_TEXT);
                console_enabled = true;
            }
            else if(key == GLFW_KEY_ESCAPE)
            {
                window_enable_cursor();
            }

            if(ctrl && key == GLFW_KEY_C)
            {
                window_set_close(1);
            }

        }

    }
    else if(kmode == KEY_MODE_TEXT)
    {
        if(action == GLFW_PRESS)
        {
            if(key == GLFW_KEY_ESCAPE || key == GLFW_KEY_F10)
            {
                window_controls_set_key_mode(KEY_MODE_NORMAL);
                console_enabled = false;
                backspace_held = false;
                // printf("console not enabled\n");
            }


            if(key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)
            {
                // printf("console_text_hist_selection: %d  -> ", console_text_hist_selection);
                console_text_hist_selection = console_text_hist_get(key == GLFW_KEY_UP ? -1: 1);
                // printf("%d\n", console_text_hist_selection);

                if(console_text_hist_selection != -1)
                {
                    // memset(console_text, 0, CONSOLE_TEXT_MAX*sizeof(console_text[0]));
                    memcpy(console_text, console_text_hist[console_text_hist_selection], CONSOLE_TEXT_MAX);

                }
            }
            else
            {
                // reset selection on any other key press
                console_text_hist_selection = -1;
            }


            if(ctrl && key == GLFW_KEY_C)
            {
                memset(console_text, 0, CONSOLE_TEXT_MAX*sizeof(console_text[0]));
            }
        }

        if(key == GLFW_KEY_BACKSPACE)
        {
            if(action == GLFW_PRESS)
            {
                backspace_held = true;
                t0_backspace = timer_get_time();
            }
            else if(action == GLFW_RELEASE)
            {
                backspace_held = false;
            }
        }
    }

}


char* string_split_index(char* str, const char* delim, int index, int* ret_len, bool split_past_index)
{
    char* s = str;
    char* s_end = str+strlen(str);

    for(int i = 0; i < (index+1); ++i)
    {
        char* end = strstr(s, delim);

        if(end == NULL)
            end = s_end;

        int len = end - s;

        if(len == 0)
        {
            *ret_len = 0;
            return NULL;
        }

        // printf("%d]  '%.*s' \n", i, len, s);

        if(i == index)
        {
            if(split_past_index)
                *ret_len = len;
            else
                *ret_len = s_end-s;
            return s;
        }

        if(end == s_end)
        {
            *ret_len = 0;
            return NULL;
        }

        s += len+strlen(delim);
    }

    *ret_len = 0;
    return NULL;
}

char* string_split_index_copy(char* str, const char* delim, int index, bool split_past_index)
{
    int len = 0;
    char* s = string_split_index(str, delim, index, &len, split_past_index);

    if(s == NULL || len == 0)
        return NULL;

    char* ret = calloc(len+1,sizeof(char));
    memcpy(ret, s, len);
    return ret;
}

void console_text_hist_add(char* text)
{
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

void handle_backspace_timer()
{
    if(window_controls_get_key_mode() == KEY_MODE_TEXT && backspace_held)
    {
        // printf("delta: %.2f\n", timer_get_time() - t0_backspace);
        if((timer_get_time() - t0_backspace) >= 0.1)
        {
            window_text_mode_buf_backspace();
            t0_backspace = timer_get_time();
        }
    }
}


// lists
// -------------------------------------------------------------------------------

glist* list_create(void* buf, int max_count, int item_size)
{
    if(item_size <= 0 || max_count <= 1)
    {
        LOGE("Invalid item_size (%d) or max_count (%d) for list", item_size, max_count);
        return NULL;
    }
    if(buf == NULL)
    {
        LOGE("List buffer is NULL");
        return NULL;
    }

    glist* list = calloc(1, sizeof(glist));
    list->count = 0;
    list->max_count = max_count;
    list->item_size = item_size;
    list->buf = buf;
    // if(list->buf == NULL)
    // {
    //     LOGI("Allocating %d bytes for list %p", max_count*item_size, list);
    //     list->buf = calloc(max_count, item_size);
    // }
    return list;
}

void list_delete(glist* list)
{
    if(list != NULL) free(list);
    list = NULL;
}

bool list_add(glist* list, void* item)
{
    if(list == NULL)
        return false;

    if(list->count >= list->max_count)
        return false;

    memcpy(list->buf + list->count*list->item_size, item, list->item_size);
    list->count++;
    return true;
}

bool list_remove(glist* list, int index)
{
    if(list == NULL)
        return false;

    if(index >= list->count)
        return false;

    memcpy(list->buf + index*list->item_size, list->buf+(list->count-1)*list->item_size, list->item_size);
    list->count--;
}

void* list_get(glist* list, int index)
{
    if(list == NULL)
        return NULL;

    return list->buf + index*list->item_size;
}

void limit_pos(Rect* limit, Rect* pos)
{
    // printf("-------------------------------------\n");
    // printf("map: "); print_rect(limit);
    // printf("before: "); print_rect(pos);
    float lx0 = limit->x - limit->w/2.0;
    float lx1 = lx0 + limit->w;
    float ly0 = limit->y - limit->h/2.0;
    float ly1 = ly0 + limit->h;

    float px0 = pos->x - pos->w/2.0;
    float px1 = px0 + pos->w;
    float py0 = pos->y - pos->h/2.0;
    float py1 = py0 + pos->h;

    if(px0 < lx0)
        pos->x = lx0+pos->w/2.0;
    if(px1 > lx1)
        pos->x = lx1-pos->w/2.0;
    if(py0 < ly0)
        pos->y = ly0+pos->h/2.0;
    if(py1 > ly1)
        pos->y = ly1-pos->h/2.0;
    // printf("after: "); print_rect(pos);
}
