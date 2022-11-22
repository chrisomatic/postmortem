
#include "main.h"
#include "window.h"
#include "shader.h"
#include "timer.h"
#include "gfx.h"
#include "math2d.h"
#include "world.h"
#include "camera.h"
#include "player.h"
// #include "gun.h"
#include "projectile.h"
#include "zombie.h"
#include "effects.h"
#include "gui.h"
#include "net.h"
#include "log.h"
#include "lighting.h"
#include "particles.h"
#include "bitpack.h"

// Settings
#define VIEW_WIDTH   1024
#define VIEW_HEIGHT  768
// #define VIEW_WIDTH   1812
// #define VIEW_HEIGHT  1359

// =========================
// Global Vars
// =========================

Timer game_timer = {0};
GameRole role;
Vector2f aim_camera_offset = {0};
Vector2f recoil_camera_offset = {0};

// gui might be a good spot for some of these variables

ConsoleMessage console_msg[CONSOLE_MSG_MAX] = {0};
int console_msg_count = 0;

char console_text[CONSOLE_TEXT_MAX+1] = {0};

char console_text_hist[CONSOLE_HIST_MAX][CONSOLE_TEXT_MAX+1] = {{0}};
int console_text_hist_index = 0;
int console_text_hist_selection = 0;

bool console_enabled = false;
bool debug_enabled = true;
bool editor_enabled = true;

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
    init_timer();
    log_init(0);
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

    init();
    gfx_print_times();  //DEBUG

    timer_set_fps(&game_timer,TARGET_FPS);
    timer_begin(&game_timer);

    double t0=timer_get_time();
    double t1=0.0;

    bitpack_test(); //@TEMP

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
    // gun_init();
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

    LOGI(" - Player.");
    players_init();

    LOGI(" - Zombies.");
    zombie_init();

    LOGI(" - Projectiles.");
    projectile_init();

    LOGI(" - Particles.");
    particles_init();

    LOGI(" - Effects.");
    effects_load_all();

    LOGI(" - GUI.");
    gui_init();

    camera_move(player->phys.pos.x, player->phys.pos.y, 0.0, true, &map.rect);
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

    if(player->item.mouse_aim)
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
    camera_move(cam_pos_x, cam_pos_y, 0.00, false, &map.rect);
}

void simulate(double delta_t)
{
    gfx_clear_lines();

    camera_set();
    camera_update();

    world_update();
    zombies_update(delta_t);

    // window_get_mouse_world_coords(&player->mouse_x, &player->mouse_y);   //MOVED to player_update
    player_update(player,delta_t);
    projectile_update(delta_t);

    particles_update(delta_t);
}

void simulate_client(double delta_t)
{
    gfx_clear_lines();

    camera_set();
    camera_update();

    world_update();
    //zombies_update(delta_t);
    // window_get_mouse_world_coords(&player->mouse_x, &player->mouse_y);
    player_update(player,delta_t);
    player_handle_net_inputs(player, delta_t);

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(players[i].active && &players[i] != player)
        {
            player_update_other(&players[i], delta_t);
        }
    }

    projectile_update(delta_t);
}

void draw()
{
    gfx_clear_buffer(50,50,50);

    world_draw();
    gfx_draw_lines();

    zombies_draw();
    projectile_draw();

    static bool activate_player = false;
    if(!activate_player)
    {
        players[2].active = true;
        players[2].phys.pos.x = 1000;
        players[2].phys.pos.y = 1000;
        players[2].phys.pos.w = 25;
        players[2].phys.pos.h = 60;
        players[2].sprite_index = 1;
        player_count++;
        activate_player = true;
    }

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        Player* p = &players[i];
        if(p->active)
        {
            player_draw(p);
            if(p != player)
            {
                // Rect* pos = &p->pos; //TODO
                Rect* pos = &p->phys.pos;
                bool in_view = is_in_camera_view(pos);
                if(!in_view)
                {
                    Rect camera_rect = {0};
                    get_camera_rect(&camera_rect);
                    // float angle = calc_angle_rad(player->phys.pos.x, player->phys.pos.y, p->phys.pos.x, p->phys.pos.y);
                    Rect prect = {0};
                    memcpy(&prect, pos, sizeof(Rect));
                    prect.w = 5.0;
                    prect.h = 5.0;
                    physics_limit_pos(&camera_rect, &prect);
                    gfx_draw_rect(&prect, player_colors[p->index], 0.0, 1.0, 0.5, true,true);
                }
            }

        }
    }

    particles_draw();
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
                window_controls_set_text_buf(console_text, CONSOLE_TEXT_MAX);
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



// FULL image drawn at draw_x, draw_y
// get the translated and scaled visible_rect of the image
void get_actual_pos(float draw_x, float draw_y, float scale, int img_w, int img_h, Rect* visible_rect, Rect* ret)
{
    float img_center_x = img_w/2.0;
    float img_center_y = img_h/2.0;
    float offset_x = (visible_rect->x - img_center_x)*scale;
    float offset_y = (visible_rect->y - img_center_y)*scale;

    // actual position
    ret->x = draw_x + offset_x;
    ret->y = draw_y + offset_y;
    ret->w = visible_rect->w*scale;
    ret->h = visible_rect->h*scale;
}

void limit_pos(Rect* limit, Rect* pos, Rect* phys_pos)
{
    Rect pos0 = *pos;
    physics_limit_pos(limit, &pos0);

    if(!FEQ(pos0.x, pos->x) || !FEQ(pos0.y, pos->y))
    {
        phys_pos->x += (pos0.x - pos->x);
        phys_pos->y += (pos0.y - pos->y);
        pos->x = pos0.x;
        pos->y = pos0.y;
    }
}


// location:
// 0 = top
// 1 = middle
// 2 = bottom
Rect calc_box(Rect* pos, float wscale, float hscale, int location)
{
    Rect r = {0};
    r.w = pos->w * wscale;
    r.h = pos->h * hscale;
    r.x = pos->x;

    float ytop = pos->y - pos->h/2.0; 
    float ybottom = pos->y + pos->h/2.0; 

    if(location == 0) //top
    {
        r.y = ytop + r.h/2.0;
    }
    else if(location == 2) //bottom
    {
        r.y = ybottom - r.h/2.0;
    }
    else //middle
    {
        r.y = pos->y;
    }

    return r;
}