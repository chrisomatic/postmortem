#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "main.h"
#include "window.h"
#include "camera.h"
#include "math2d.h"
#include "player.h"
#include "gfx.h"
#include "world.h"
#include "lighting.h"
#include "log.h"
#include "main.h"

#include "zombie.h"

ZombieModel zombie_models[ZOMBIE_MODELS_MAX];

#define IMG_ELEMENT_W 128
#define IMG_ELEMENT_H 128

int zombie_image_sets_none[ZOMBIE_MODELS_MAX][ZOMBIE_TEXTURES_MAX][ZANIM_MAX];


#define ZOMBIES_IDLE 0

Zombie zombies[MAX_ZOMBIES] = {0};
glist* zlist = NULL;

static int zombie_image;

static void zombie_remove(int index);
static void sort_zombies(Zombie arr[], int n);
static void wander(Zombie* zom, float delta_t);
static void zombie_die(int index);

const char* zombie_anim_state_str(ZombieAnimState anim_state)
{
    switch(anim_state)
    {
        case ZANIM_IDLE: return "idle";
        case ZANIM_WALK: return "walk";
        case ZANIM_ATTACK1: return "attack1";
        case ZANIM_NONE: return "";
        default: return "";
    }
}

void zombie_init()
{

    int idx = ZOMBIE1;
    zombie_models[idx].index = idx;
    zombie_models[idx].name = "zombie1";
    zombie_models[idx].textures = 1;

    //none
    for(int pm = 0; pm < ZOMBIE_MODELS_MAX; ++pm)
    {
        for(int t = 0; t < ZOMBIE_TEXTURES_MAX; ++t)
        {
            for(int ps = 0; ps < ZANIM_MAX; ++ps)
            {
                zombie_image_sets_none[pm][t][ps] = -1;

                char fname[100] = {0};
                sprintf(fname, "img/characters/%s_%d-%s.png", zombie_models[pm].name, t, zombie_anim_state_str(ps));
                if(access(fname, F_OK) == 0)
                {
                    zombie_image_sets_none[pm][t][ps] = gfx_load_image(fname, false, true, IMG_ELEMENT_W, IMG_ELEMENT_H, NULL);
                    // printf("%s -> %d\n", fname, player_image_sets[pm][t][ps][wt]);
                }
            }
        }
    }

    zombie_image = gfx_load_image("img/zombie_f1.png", false, false, 0, 0, NULL);

    zlist = list_create((void*)zombies, MAX_ZOMBIES, sizeof(Zombie));
    if(zlist == NULL)
    {
        LOGE("zombie list failed to create");
    }

    //if(role == ROLE_LOCAL || role == ROLE_SERVER)
    //{
        /*
        int wrows, wcols;
        world_get_grid_dimensions(&wrows, &wcols);
        for(int r = 0; r < wrows; ++r)
        {
            for(int c = 0; c < wcols; ++c)
            {
                ZombieSpawn spawn = {0};
                spawn.scale = rand_float_between(0.2, 5.0);
                zombie_add_to_world_grid(&spawn, r, c);
            }
        }
        */

        for(int i = 0; i < 1; ++i)
        {
            ZombieSpawn spawn = {0};
            spawn.model_index = ZOMBIE1;
            spawn.model_texture = 0;
            spawn.pos.x = rand() % view_width;
            spawn.pos.y = rand() % view_height;
            for(int j = 0; j < 10; ++j)
            {
                // spawn.scale = rand_float_between(0.5, 1.2);
                spawn.scale = 1.0/(128.0/50.0);
                // spawn.scale = 2.0;
                // printf("%d) %.0f %.0f\n", i, spawn.pos.x, spawn.pos.y);
                zombie_add(&spawn);
            }
        }
    //}

    /*
    ZombieSpawn spawn = {0};
    spawn.pos.x = player->phys.pos.x;
    spawn.pos.y = player->phys.pos.y;
    spawn.scale = 1.0;
    zombie_add(&spawn);
    */

    LOGI("zombie count: %d", zlist->count);
}

bool zombie_add(ZombieSpawn* spawn)
{
    Zombie zombie = {0};
    // zombie.dead = false;
    zombie.phys.pos.x = spawn->pos.x;
    zombie.phys.pos.y = spawn->pos.y;
    zombie.phys.accel.x = 0.0;
    zombie.phys.accel.y = 0.0;
    zombie.phys.max_linear_vel = spawn->max_linear_vel;
    zombie.push_vel.x = 0.0;
    zombie.push_vel.y = 0.0;
    zombie.action_timer = 0;
    zombie.sprite_index = 0;

    zombie.hp_max = spawn->hp_max;
    zombie.action = spawn->action;
    zombie.action_timer_max = spawn->action_timer_max;
    zombie.speed = spawn->speed;
    zombie.scale = spawn->scale;

    zombie.model_index = spawn->model_index;
    zombie.model_texture = spawn->model_texture;
    zombie.anim_state = ZANIM_WALK;
    zombie.sprite_index = 0;
    zombie.sprite_index_direction = 0;
    zombie.image = zombie_image_sets_none[zombie.model_index][zombie.model_texture][zombie.anim_state];

    // animation
    zombie.anim.curr_frame = 0;
    zombie.anim.max_frames = 16;
    zombie.anim.curr_frame_time = 0.0f;
    zombie.anim.max_frame_time = 0.04f;
    zombie.anim.finite = false;
    zombie.anim.curr_loop = 0;
    zombie.anim.max_loops = 0;
    zombie.anim.frame_sequence[0] = 12;
    zombie.anim.frame_sequence[1] = 13;
    zombie.anim.frame_sequence[2] = 14;
    zombie.anim.frame_sequence[3] = 15;
    zombie.anim.frame_sequence[4] = 0;
    zombie.anim.frame_sequence[5] = 1;
    zombie.anim.frame_sequence[6] = 2;
    zombie.anim.frame_sequence[7] = 3;
    zombie.anim.frame_sequence[8] = 4;
    zombie.anim.frame_sequence[9] = 5;
    zombie.anim.frame_sequence[10] = 6;
    zombie.anim.frame_sequence[11] = 7;
    zombie.anim.frame_sequence[12] = 8;
    zombie.anim.frame_sequence[13] = 9;
    zombie.anim.frame_sequence[14] = 10;
    zombie.anim.frame_sequence[15] = 11;


    // set default values if not set by spawn
    if(FEQ(zombie.phys.max_linear_vel, 0.0))
        zombie.phys.max_linear_vel = 128.0;

    if(FEQ(zombie.hp_max, 0.0))
        zombie.hp_max = 3.0;

    if(FEQ(zombie.speed, 0.0))
        zombie.speed = 16.0;

    if(FEQ(zombie.scale, 0.0))
        zombie.scale = 1.0;

    zombie.hp = zombie.hp_max;

    zombie_update_boxes(&zombie);

    return list_add(zlist, (void*)&zombie);
}

bool zombie_add_to_world_grid(ZombieSpawn* spawn, int world_row, int world_col)
{
    Rect r = {0};
    world_grid_to_rect(world_row, world_col, &r);

    float x0 = r.x - r.w/2.0;
    float y0 = r.y - r.h/2.0;

    float x = rand_float_between(0.0, r.w) + x0;
    float y = rand_float_between(0.0, r.h) + y0;

    spawn->pos.x = x;
    spawn->pos.y = y;

    // world_grid_to_coords
    return zombie_add(spawn);
}

void zombie_update_image(Zombie* z)
{
    z->image = zombie_image_sets_none[z->model_index][z->model_texture][z->anim_state];
}


void zombie_update_anim_state(Zombie* z)
{
    ZombieAnimState prior = z->anim_state;

    if(z->moving)
    {
        z->anim_state = ZANIM_WALK;
    }
    else
    {
        z->anim_state = ZANIM_IDLE;
    }

    // reset the animation
    if(z->anim_state != prior)
    {
        // printf("Player state change: %d -> %d\n", prior, p->anim_state);
        z->anim.curr_frame = 0;
        z->anim.curr_frame_time = 0.0;
        z->anim.curr_loop = 0;
    }

    return;
}

void zombie_update_sprite_index(Zombie* z)
{
    bool up = z->phys.accel.y < 0;
    bool down = z->phys.accel.y > 0;
    bool left = z->phys.accel.x < 0;
    bool right = z->phys.accel.x > 0;


    if(up && left)
        z->sprite_index_direction = 5;
    else if(up && right)
        z->sprite_index_direction = 3;
    else if(down && left)
        z->sprite_index_direction = 7;
    else if(down && right)
        z->sprite_index_direction = 1;
    else if(up)
        z->sprite_index_direction = 4;
    else if(down)
        z->sprite_index_direction = 0;
    else if(left)
        z->sprite_index_direction = 6;
    else if(right)
        z->sprite_index_direction = 2;

    z->sprite_index = z->sprite_index_direction * 16;

    int anim_frame_offset = z->anim.frame_sequence[z->anim.curr_frame];
    assert(anim_frame_offset >= 0);

    z->sprite_index += anim_frame_offset;
    z->sprite_index = MIN(z->sprite_index, gfx_images[z->image].element_count);
}

void zombie_update_boxes(Zombie* zom)
{
    // const float shrink_factor = 0.80;

    // make sure scale gets recalculated if needed
    zom->phys.pos.w = gfx_images[zom->image].visible_rects[0].w*zom->scale;
    zom->phys.pos.h = gfx_images[zom->image].visible_rects[0].h*zom->scale;

    // Rect* vr = &zom->visible_rect;
    float w = zom->phys.pos.w;
    float h = zom->phys.pos.h;
    float x0 = zom->phys.pos.x;
    float y0 = zom->phys.pos.y;
    float ytop = y0-h/2.0;
    float ybot = y0+h/2.0;

    float collision_height = h*0.4;
    zom->collision_box.x = x0;
    zom->collision_box.y = ybot-collision_height/2.0;
    zom->collision_box.w = w;
    zom->collision_box.h = collision_height;

    float hit_box_height = h*0.5;
    zom->hit_box.x = x0;
    zom->hit_box.y = ytop+hit_box_height/2.0;
    zom->hit_box.w = w;
    zom->hit_box.h = hit_box_height;

    float x = zom->collision_box.x;
    float y = zom->collision_box.y;
    coords_to_map_grid(x, y, &zom->map_row, &zom->map_col);
    coords_to_map_grid(x, y, &zom->world_row, &zom->world_col);


    //TODO
    // GFXImage* img = &gfx_images[p->image];
    // Rect* vr = &img->visible_rects[p->sprite_index];

    // get_actual_pos(p->phys.pos.x, p->phys.pos.y, p->scale, img->element_width, img->element_height, vr, &p->pos);
    // limit_pos(&map.rect, &p->pos, &p->phys.pos);

    // float px = p->pos.x;
    // float py = p->pos.y;

    // p->standard_size.x = px;
    // p->standard_size.y = py;

    // p->max_size.x = px;
    // p->max_size.y = py;

}

void zombie_update(float delta_t)
{

    for(int i = zlist->count - 1; i >= 0 ; --i)
    {
        Zombie* zom = &zombies[i];
        Vector2f accel = {0.0,0.0};

#if !ZOMBIES_IDLE
        wander(zom, delta_t);

        float amt = zom->speed;

        switch(zom->action)
        {
            case ZOMBIE_ACTION_NONE:
                break;
            case ZOMBIE_ACTION_MOVE_UP:
                accel.y -= amt;
                break;
            case ZOMBIE_ACTION_MOVE_UP_RIGHT:
                accel.x += amt;
                accel.y -= amt;
                break;
            case ZOMBIE_ACTION_MOVE_RIGHT:
                accel.x += amt;
                break;
            case ZOMBIE_ACTION_MOVE_DOWN_RIGHT:
                accel.x += amt;
                accel.y += amt;
                break;
            case ZOMBIE_ACTION_MOVE_DOWN:
                accel.y += amt;
                break;
            case ZOMBIE_ACTION_MOVE_DOWN_LEFT:
                accel.x -= amt;
                accel.y += amt;
                break;
            case ZOMBIE_ACTION_MOVE_LEFT:
                accel.x -= amt;
                break;
            case ZOMBIE_ACTION_MOVE_UP_LEFT:
                accel.x -= amt;
                accel.y -= amt;
                break;
        }

        if(zom->push_vel.x > 0.0)
        {
            accel.x += zom->push_vel.x;
        }
        if(zom->push_vel.y > 0.0)
        {
            accel.y -= zom->push_vel.y;
        }
#endif

        physics_begin(&zom->phys);
        physics_add_friction(&zom->phys, 16.0);
        physics_add_force(&zom->phys, accel.x, accel.y);
        physics_simulate(&zom->phys, delta_t);


        zom->moving = !(FEQ(accel.x,0.0) && FEQ(accel.y,0.0));

        zombie_update_anim_state(zom);
        // player_update_anim_timing(p);
        zombie_update_image(zom);

        gfx_anim_update(&zom->anim, delta_t);

        zombie_update_sprite_index(zom);
        zombie_update_boxes(zom);

        //TODO
        physics_limit_pos(&map.rect, &zom->phys.pos);
    }

    sort_zombies(zombies,zlist->count);
}

void zombie_draw()
{
    for(int i = 0; i < zlist->count; ++i)
    {
        Zombie* zom = &zombies[i];

        if(is_in_camera_view(&zom->phys.pos))
        {
            // gfx_draw_image(zombie_image,(int)zom->phys.pos.x,(int)zom->phys.pos.y, ambient_light,zom->scale,0.0,1.0);
            gfx_draw_image(zom->image, zom->sprite_index,(int)zom->phys.pos.x,(int)zom->phys.pos.y, ambient_light,zom->scale,0.0,1.0,true,true);

            if(debug_enabled)
            {
                Rect* cbox  = &zom->collision_box;
                Rect* hbox  = &zom->hit_box;
                gfx_draw_rect(cbox, COLOR_GREEN, 0.0, 1.0,1.0, false, true);
                gfx_draw_rect(hbox, COLOR_YELLOW, 0.0, 1.0,1.0, false, true);


                // health bars
                Rect r = {0};
                float h = 4.0;
                float y = zom->phys.pos.y+zom->phys.pos.h*0.55 + h/2.0;
                float x = zom->phys.pos.x;
                float w = zom->phys.pos.w;

                r.x = x;
                r.y = y;
                r.w = w;
                r.h = h;
                gfx_draw_rect(&r, COLOR_WHITE, 0.0, 1.0,1.0, true, true);


                float p = zom->hp/zom->hp_max;
                r.h *= 0.8;
                r.x = r.x-r.w/2.0;
                r.w *= p;
                r.x = r.x+r.w/2.0;
                gfx_draw_rect(&r, COLOR_RED, 0.0, 1.0,1.0, true, true);

            }
        }


    }
}

void zombie_push(int index, Vector2f* force)
{
    if(index < 0 || index >= zlist->count)
    {
        LOGW("Zombie index %d is out of range",index);
        return;
    }

    Zombie* zom = &zombies[index];

    zom->push_vel.x = force->x;
    zom->push_vel.y = force->y;
}

void zombie_hurt(int index, float val)
{
    if(index < 0 || index >= zlist->count)
    {
        LOGW("Zombie index %d is out of range",index);
        return;
    }

    Zombie* zom = &zombies[index];

    zom->hp -= val;
    if(zom->hp <= 0.0)
    {
        zombie_die(index);
    }
}

static void zombie_remove(int index)
{
    list_remove(zlist, index);
}

static void sort_zombies(Zombie arr[], int n)
{
    // insertion sort
    int i, j;
    Zombie key;
    for (i = 1; i < n; i++) 
    {
        memcpy(&key,&arr[i],sizeof(Zombie));
        j = i - 1;

        while (j >= 0 && arr[j].phys.pos.y >= arr[i].phys.pos.y)
        {
            memcpy(&arr[j+1],&arr[j], sizeof(Zombie));
            j = j - 1;
        }
        memcpy(&arr[j+1],&key, sizeof(Zombie));
    }
}

static void wander(Zombie* zom, float delta_t)
{
    zom->action_timer+=delta_t;

    if(zom->action_timer >= zom->action_timer_max)
    {
        zom->action_timer = 0.0;//zom->action_timer_max;
        zom->action = rand() % ZOMBIE_ACTION_MAX;
        zom->action_timer_max = (rand() % 100)/50.0 + 0.5; // 0.5 to 2.0 seconds
    }
}

static void update_zombie_boxes(Zombie* zom)
{
}

static void zombie_die(int index)
{
    zombie_remove(index);
}
