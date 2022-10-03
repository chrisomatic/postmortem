#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include "window.h"
#include "shader.h"
#include "timer.h"
#include "gfx.h"
#include "rat_math.h"

// Settings
#define TARGET_FPS 60.0f
#define NUM_RATS   1000

// =========================
// Global Vars
// =========================

Timer game_timer = {0};

GFXImage* rat_img_data = NULL;

int rat_img = 0;
int rat2_img = 0;
int mouse_x = 0;
int mouse_y = 0;

double g_delta_t = 0.0f;
double t0=0.0,t1=0.0;

typedef struct
{
    float x,y;
    float angle;
    float rotate;
} Rat;

Rat rats[NUM_RATS] = {0};

// =========================
// Function Prototypes
// =========================

void start_game();
void init();
void deinit();
void update();
void draw();

// =========================
// Main Loop
// =========================

int main(int argc, char* argv[])
{
    start_game();
    return 0;
}

// =========================
// Functions
// =========================

void start_game()
{
    init();

    timer_set_fps(&game_timer,TARGET_FPS);
    timer_begin(&game_timer);

    // main game loop
    for(;;)
    {
        g_delta_t = t1-t0;

        window_poll_events();
        if(window_should_close())
            break;

        t0 = timer_get_time();

        update();
        draw();

        timer_wait_for_frame(&game_timer);
        printf("fps: %f\n",timer_get_prior_frame_fps(&game_timer));
        window_swap_buffers();

        t1 = timer_get_time();
    }

    deinit();
}

void handle_rat_border_collision(float* x, float* y, int w, int h, float* angle)
{

    if(*x+w >= VIEW_WIDTH)
    {
        if(angle != NULL) *angle = PI-*angle;
        *x = VIEW_WIDTH - w - 1;
    }
    else if(*x <= 0)
    {
        if(angle != NULL) *angle = PI-*angle;
        *x = 1;
    }
    else if(*y+h >= VIEW_HEIGHT)
    {
        if(angle != NULL) *angle = PI*2 - *angle;
        *y = VIEW_HEIGHT - h - 1;
    }
    else if(*y <= 0)
    {
        if(angle != NULL) *angle = PI*2 - *angle;
        *y = 1;
    }

}

void init()
{
    bool success;

    success = window_init(VIEW_WIDTH, VIEW_HEIGHT);

    if(!success)
    {
        fprintf(stderr,"Failed to initialize window!\n");
        exit(1);
    }

    time_t t;
    // srand((unsigned) time(&t));
    srand((unsigned int)1);

    printf("Initializing...\n");

    printf(" - Shaders.\n");
    shader_load_all();

    printf(" - Graphics.\n");
    gfx_init(VIEW_WIDTH, VIEW_HEIGHT);

    rat_img = gfx_load_image("img/rat_small.png");
    rat_img_data = gfx_get_image_data(rat_img);

    for(int i = 0; i < NUM_RATS; ++i)
    {
        rats[i].x = rand() % VIEW_WIDTH;
        rats[i].y = rand() % VIEW_HEIGHT;
        rats[i].angle = RAD(rand() % 360);
        rats[i].rotate = rand() % 360;
    }
}

void deinit()
{
    shader_deinit();
    window_deinit();
}

void update()
{
    int w = rat_img_data->w;
    int h = rat_img_data->h;
    for(int i = 0; i < NUM_RATS; ++i)
    {
        Rat* r = &rats[i];

        float xadd = cos(r->angle)*5;
        float yadd = sin(r->angle)*5;
        r->x += xadd;
        r->y += yadd;

        handle_rat_border_collision(&r->x, &r->y, w, h, &r->angle);
    }

    int x,y;
    window_get_mouse_view_coords(&x, &y);
    // window_get_mouse_view_coords(&x, &y);
    if(x != mouse_x || y != mouse_y)
    {
        mouse_x = x;
        mouse_y = y;
        // printf("X: %d, Y: %d\n",x,y);
    }
}

void draw()
{
    gfx_clear_buffer(50,50,50);

    for(int i = 0; i < NUM_RATS; ++i)
    {
        gfx_draw_image(rat_img,(int)rats[i].x,(int)rats[i].y, COLOR_TINT_NONE,1.0,0.0,1.0); //rats[i].rotate,1.0);
        //gfx_draw_image(rat_img,(int)rats[i].x,(int)rats[i].y, gfx_rgb_to_color(25,25,25),1.0,0.0,1.0);
    }

    int x = mouse_x;
    int y = mouse_y;
    gfx_draw_image(rat_img,x-rat_img_data->w/2.0,y-rat_img_data->h/2.0,0xFFFF00FF,1.0,0.0,1.0);
}

