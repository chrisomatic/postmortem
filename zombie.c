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
#include "io.h"
#include "main.h"
#include "effects.h"
#include "entity.h"

#include "zombie.h"

// #define IMG_ELEMENT_W 128
// #define IMG_ELEMENT_H 128

#define NUM_ZOMBIES_INIT    0

uint32_t zombie_info_id = 0xFFFFFFFF;
bool zombie_debug = false;
bool zombies_pursue = true;
bool zombies_idle = false;
ZombieModel zombie_models[ZOMBIE_MODELS_MAX];
int zombie_image_sets_none[ZOMBIE_MODELS_MAX][ZOMBIE_TEXTURES_MAX][ZANIM_MAX];
Zombie zombies[MAX_ZOMBIES] = {0};
glist* zlist = NULL;

// max width and heights for zombie models
static Vector2f maxwh[ZOMBIE_MODELS_MAX];
static Rect standard_size[ZOMBIE_MODELS_MAX];

static uint32_t zid = 0;

static void zombie_remove(Zombie* zom);
static void wander(Zombie* z, float delta_t);
static void zombie_die2(int index);
static void zombie_die(Zombie* z);

void zombie_init()
{

    //models
    int idx = ZOMBIE1;
    zombie_models[idx].index = idx;
    zombie_models[idx].name = "zombie1";
    zombie_models[idx].textures = 1;

    //none
    for(int pm = 0; pm < ZOMBIE_MODELS_MAX; ++pm)
    {
        float maxw=0.0, maxh=0.0;
        for(int t = 0; t < ZOMBIE_TEXTURES_MAX; ++t)
        {
            for(int ps = 0; ps < ZANIM_MAX; ++ps)
            {
                int img = -1;
                char fname[100] = {0};
                sprintf(fname, "img/characters/%s_%d-%s.png", zombie_models[pm].name, t, zombie_anim_state_str(ps));
                // if(access(fname, F_OK) == 0)
                if(io_file_exists(fname))
                {
                    img = gfx_load_image(fname, false, true, IMG_ELEMENT_W, IMG_ELEMENT_H, NULL);
                    // printf("%s -> %d\n", fname, player_image_sets[pm][t][ps][wt]);
                }
                zombie_image_sets_none[pm][t][ps] = img;

                if(img == -1)
                    continue;

                for(int i = 0; i < gfx_images[img].element_count; ++i)
                {
                    Rect* vr = &gfx_images[img].visible_rects[i];
                    if(IS_RECT_EMPTY(vr)) continue;
                    maxw = MAX(maxw, vr->w);
                    maxh = MAX(maxh, vr->h);
                }
            }
        }

        maxwh[pm].x = maxw;
        maxwh[pm].y = maxh;
    }

    // standard size
    for(int pm = 0; pm < ZOMBIE_MODELS_MAX; ++pm)
    {
        int standard_img = zombie_image_sets_none[pm][0][ZANIM_IDLE];
        if(standard_img != -1)
        {
            Rect* r = &gfx_images[standard_img].visible_rects[0];
            standard_size[pm] = *r;
        }
        else
        {
            LOGE("Zombie standard_img is -1 (%d)", pm);
        }
    }



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

        for(int i = 0; i < NUM_ZOMBIES_INIT; ++i)
        {
            ZombieSpawn spawn = {0};
            spawn.model_index = ZOMBIE1;
            spawn.model_texture = 0;
            spawn.pos.x = rand() % view_width;
            spawn.pos.y = rand() % view_height;
            spawn.scale = 0.1;
            zombie_add_to_world_grid(&spawn, 0, 0);
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
    Zombie* z = &zombie;
    z->id = zid++;

    // set default values if not set by spawn
    // --------------------------------------------------------
    if(FEQ(spawn->max_linear_vel, 0.0))
        spawn->max_linear_vel = 128.0;

    if(FEQ(spawn->hp_max, 0.0))
        spawn->hp_max = 3.0;

    if(FEQ(spawn->speed, 0.0))
        spawn->speed = 16.0;

    if(FEQ(spawn->scale, 0.0))
        spawn->scale = 1.0;

    // animation
    // --------------------------------------------------------
    z->anim.curr_frame = 0;
    z->anim.max_frames = 16;
    z->anim.curr_frame_time = 0.0f;
    z->anim.max_frame_time = 0.04f;
    z->anim.finite = false;
    z->anim.curr_loop = 0;
    z->anim.max_loops = 0;
    z->anim.frame_sequence[0] = 0;
    z->anim.frame_sequence[1] = 1;
    z->anim.frame_sequence[2] = 2;
    z->anim.frame_sequence[3] = 3;
    z->anim.frame_sequence[4] = 4;
    z->anim.frame_sequence[5] = 5;
    z->anim.frame_sequence[6] = 6;
    z->anim.frame_sequence[7] = 7;
    z->anim.frame_sequence[8] = 8;
    z->anim.frame_sequence[9] = 9;
    z->anim.frame_sequence[10] = 10;
    z->anim.frame_sequence[11] = 11;
    z->anim.frame_sequence[12] = 12;
    z->anim.frame_sequence[13] = 13;
    z->anim.frame_sequence[14] = 14;
    z->anim.frame_sequence[15] = 15;



    // state vars
    // --------------------------------------------------------
    z->hurt = false;
    z->attacking = false;
    z->dead = false;

    z->action_timer_max = 2.0;
    z->action_timer = z->action_timer_max;


    // model and texture
    // --------------------------------------------------------
    z->model_index = spawn->model_index;
    z->model_texture = spawn->model_texture;
    z->anim_state = ZANIM_WALK;
    z->sprite_index = 0;
    z->sprite_index_direction = 0;
    zombie_update_image(&zombie);
    // z->image = zombie_image_sets_none[z->model_index][z->model_texture][z->anim_state];

    // z->scale = (float)ZOMBIE_HEIGHT*spawn->scale/standard_size[z->model_index].h;
    z->scale = (float)ZOMBIE_HEIGHT/standard_size[z->model_index].h;

    z->color = COLOR_TINT_NONE;
    z->opacity = 1.0;

    z->angle = 0.0f;
    zombie_update_sprite_index(&zombie);

    // boxes and phys stuff
    // --------------------------------------------------------
    z->phys.mass = 1.0;
    z->phys.accel.x = 0.0;
    z->phys.accel.y = 0.0;
    z->phys.pos.x = spawn->pos.x;
    z->phys.pos.y = spawn->pos.y;
    z->phys.actual_pos.x = z->phys.pos.x;
    z->phys.actual_pos.y = z->phys.pos.y;

    float sw = standard_size[z->model_index].w * z->scale;
    float sh = standard_size[z->model_index].h * z->scale;
    Rect standard = {0};
    standard.x = z->phys.pos.x;
    standard.y = z->phys.pos.y;
    standard.w = sw;
    standard.h = sh;
    z->phys.hit = calc_sub_box(&standard, 1.0, 0.85, 0);
    z->phys.collision = calc_sub_box(&standard, 1.0, 0.4, 2);

    zombie_update_boxes(z);

    z->phys.max_linear_vel = spawn->max_linear_vel;
    z->speed = spawn->speed;
    z->push_vel.x = 0.0;
    z->push_vel.y = 0.0;


    // other
    // --------------------------------------------------------
    coords_to_map_grid(z->phys.actual_pos.x, z->phys.actual_pos.y, &z->map_grid_pos.x, &z->map_grid_pos.y);
    coords_to_world_grid(z->phys.actual_pos.x, z->phys.actual_pos.y, &z->world_grid_pos.x, &z->world_grid_pos.y);
    z->attack_range = 40.0;
    z->attack_angle = 0.0;
    z->melee_hit_count = 0;
    z->damage_min = 1.0;
    z->damage_max = 2.0;
    z->hp_max = spawn->hp_max;
    z->hp = z->hp_max;


    return list_add(zlist, (void*)z);
}

bool zombie_add_to_world_grid(ZombieSpawn* spawn, int world_row, int world_col)
{
    Rect r = {0};
    world_grid_to_rect(world_row, world_col, &r);

    float x0 = r.x - r.w/2.0;
    float y0 = r.y - r.h/2.0;

    // float x = rand_float_between(0.0, r.w) + x0;
    // float y = rand_float_between(0.0, r.h) + y0;
    float x = RAND_FLOAT(0.0, r.w) + x0;
    float y = RAND_FLOAT(0.0, r.h) + y0;

    spawn->pos.x = x;
    spawn->pos.y = y;

    // world_grid_to_coords
    return zombie_add(spawn);
}

void zombie_push(int index, Vector2f* force)
{
    if(index < 0 || index >= zlist->count)
    {
        LOGW("Zombie index %d is out of range",index);
        return;
    }

    Zombie* z = &zombies[index];

    z->push_vel.x = force->x;
    z->push_vel.y = force->y;
}

void zombie_hurt2(int index, float val)
{
    if(index < 0 || index >= zlist->count)
    {
        LOGW("Zombie index %d is out of range",index);
        return;
    }

    Zombie* z = &zombies[index];
    z->hurt = true;

    z->hp -= val;
    particles_spawn_effect(z->phys.actual_pos.x, z->phys.actual_pos.y, &particle_effects[EFFECT_BLOOD1], 0.6, true, false);

    if(z->hp <= 0.0)
    {
        zombie_die2(index);
    }

}

void zombie_hurt(Zombie* z, float val)
{
    z->hurt = true;

    z->hp -= val;
    particles_spawn_effect(z->phys.actual_pos.x, z->phys.actual_pos.y, &particle_effects[EFFECT_BLOOD1], 0.6, true, false);

    if(z->hp <= 0.0)
    {
        zombie_die(z);
    }
}


void zombie_update_image(Zombie* z)
{
    z->image = zombie_image_sets_none[z->model_index][z->model_texture][z->anim_state];
}

void zombie_update_anim_timing(Zombie* z)
{
    switch(z->anim_state)
    {
        case ZANIM_IDLE:
            z->anim.max_frame_time = 0.15f;
            break;
        case ZANIM_WALK:
        {
            z->anim.max_frame_time = 0.03f;
            float pvx = z->phys.vel.x;
            float pvy = z->phys.vel.y;
            float pv = sqrt(SQ(pvx) + SQ(pvy));
            float scale = 128.0/pv;
            z->anim.max_frame_time *= scale;
        } break;
        case ZANIM_ATTACK1:
            z->anim.max_frame_time = 0.045f;
            break;
        default: 
            z->anim.max_frame_time = 0.04f;
            break;
    }
}

void zombie_update_anim_state(Zombie* z)
{
    ZombieAnimState prior = z->anim_state;

    if(z->dead)
    {
        z->anim_state = ZANIM_DEAD;
    }
    else if(z->attacking)
    {
        z->anim_state = ZANIM_ATTACK1;
    }
    else if(z->hurt)
    {
        z->anim_state = ZANIM_HURT;
    }
    else if(z->moving)
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
    float angle_deg = DEG(z->angle);
    if(z->attacking) angle_deg = DEG(z->attack_angle);
    // int sector = angle_sector(angle_deg, 16);

    int sector = player_angle_sector(angle_deg);
    if(sector == 0) z->sprite_index_direction = 2;
    else if(sector == 1) z->sprite_index_direction = 3;
    else if(sector == 2) z->sprite_index_direction = 4;
    else if(sector == 3) z->sprite_index_direction = 5;
    else if(sector == 4) z->sprite_index_direction = 6;
    else if(sector == 5) z->sprite_index_direction = 7;
    else if(sector == 6) z->sprite_index_direction = 0;
    else if(sector == 7) z->sprite_index_direction = 1;

    z->sprite_index = z->sprite_index_direction * 16;

    int anim_frame_offset = z->anim.frame_sequence[z->anim.curr_frame];
    assert(anim_frame_offset >= 0);

    z->sprite_index += anim_frame_offset;
    z->sprite_index = MIN(z->sprite_index, gfx_images[z->image].element_count);
}

void zombie_update_boxes(Zombie* z)
{
    GFXImage* img = &gfx_images[z->image];
    Rect* vr = &img->visible_rects[z->sprite_index];

    z->phys.actual_pos.w = vr->w * z->scale;
    z->phys.actual_pos.h = vr->h * z->scale;

    z->phys.pos.w = z->phys.actual_pos.w;
    z->phys.pos.h = z->phys.actual_pos.h;

}

void zombie_update_pursue(Zombie* z, float delta_t)
{

    if(z->anim_state == ZANIM_ATTACK1)
        return;

    if(z->anim_state == ZANIM_HURT)
        return;

    if(zombies_idle)
        return;

    if(!zombies_pursue)
    {
        z->pursuing = false;
        z->pursue_player = NULL;
        return;
    }

    if(z->pursuing)
    {
        bool has_target = z->pursue_player != NULL;

        if(has_target)
        {

            // current target position
            float px = z->pursue_player->phys.pos.x;
            float py = z->pursue_player->phys.pos.y;

            bool in_range = (dist(z->phys.pos.x, z->phys.pos.y, px, py) <= z->pursue_player->detect_radius*MAP_GRID_PXL_SIZE);

            // only update target position if in range
            if(in_range)
            {
                z->pursue_target.x = px;
                z->pursue_target.y = py;
            }
            else
            {
                // printf("(%d) zombie is NOT pursuing player '%s'\n", z->id, z->pursue_player->name);
                z->pursue_player = NULL;
                has_target = false;
            }
        }

        if(!has_target)
        {
            float d = dist(z->phys.pos.x, z->phys.pos.y, z->pursue_target.x, z->pursue_target.y);
            if(d <= MAX(z->phys.pos.w, z->phys.pos.h)) //TODO
            {
                // printf("(%d) zombie has reached last known player location\n", z->id);
                z->pursuing = false;
            }
        }

    }
    else
    {

        float min_range = INFINITY;
        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            Player* p = &players[i];
            if(!p->active) continue;

            float d = dist(z->phys.pos.x, z->phys.pos.y, p->phys.pos.x, p->phys.pos.y);
            bool in_range = (d <= (p->detect_radius*MAP_GRID_PXL_SIZE));
            if(in_range && d < min_range)
            {
                min_range = d;
                z->pursue_player = p;
                z->pursue_target.x = p->phys.pos.x;
                z->pursue_target.y = p->phys.pos.y;

                Rect r = {0};
                r.x = z->pursue_target.x;
                r.y = z->pursue_target.y;
                r.w = 1.0;
                r.h = 1.0;
                Rect camera_rect = {0};
                get_camera_rect(&camera_rect);
                physics_limit_pos(&camera_rect, &r);

                z->pursue_target.x = r.x;
                z->pursue_target.y = r.y;
                z->pursuing = true;
                // printf("(%d) zombie is now pursuing player '%s', target: %.1f, %.1f\n", z->id, p->name, z->pursue_target.x, z->pursue_target.y);
            }

        }

    }
}

Vector2f zombie_update_movement(Zombie* z, float delta_t)
{
    Vector2f accel = {0.0,0.0};

    if(zombies_idle)
        return accel;

    if(z->anim_state == ZANIM_DEAD)
        return accel;

    if(z->anim_state == ZANIM_ATTACK1)
        return accel;

    if(z->anim_state == ZANIM_HURT)
        return accel;

    zombie_update_pursue(z, delta_t);

    float amt = z->speed;

    if(z->pursuing)
    {
        amt *= 1.5;
        float tx = z->pursue_target.x;
        float ty = z->pursue_target.y;
        z->angle = calc_angle_rad(z->phys.pos.x, z->phys.pos.y, tx, ty);

        // set accel
        accel.x += amt*cosf(z->angle);
        accel.y += amt*sinf(2*PI-z->angle);
    }
    else
    {
        wander(z, delta_t);
        if(!z->action_none)
        {
            accel.y += amt*sinf(2*PI-z->angle);
            accel.x += amt*cosf(z->angle);
        }
    }

    return accel;
}

void zombie_update(Zombie* z, float delta_t)
{
    if(z->dead)
    {
        z->dead_time += delta_t;

        float fade_time = 3.0;
        float fade_start = ZOMBIE_DEAD_MAX_TIME - fade_time;
        float factor1 = (z->dead_time/1.0);
        float factor2 = z->dead_time < fade_start ? 0.0 : (z->dead_time - fade_start)/fade_time;

        z->color = gfx_blend_colors(COLOR_TINT_NONE, 0x00AAAAAA, factor1);
        z->opacity = lerp(1.0, 0.0, factor2); 

        if(z->dead_time >= ZOMBIE_DEAD_MAX_TIME)
        {
            zombie_remove(z);
            return;
        }
    }

    Vector2f accel = zombie_update_movement(z, delta_t);

    z->moving = !(FEQ(accel.x,0.0) && FEQ(accel.y,0.0));

    if(!z->attacking && !zombies_idle && !z->dead)
    {
        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            Player* p = &players[i];
            if(!p->active) continue;
            z->attacking = (dist(z->phys.pos.x, z->phys.pos.y, p->phys.pos.x, p->phys.pos.y) <= z->attack_range);
            if(z->attacking)
            {
                z->melee_hit_count = 0;
                // set angle to face player
                z->attack_angle = calc_angle_rad(z->phys.pos.x, z->phys.pos.y, p->phys.pos.x, p->phys.pos.y);
                break;
            }
        }
    }

    zombie_update_anim_state(z);
    zombie_update_anim_timing(z);
    zombie_update_image(z);
    gfx_anim_update(&z->anim, delta_t);
    zombie_update_sprite_index(z);
    zombie_update_boxes(z);

    physics_begin(&z->phys);
    physics_add_friction(&z->phys, 16.0);
    physics_add_force(&z->phys, accel.x, accel.y);
    physics_simulate(&z->phys, &map.rect, delta_t);

    coords_to_map_grid(z->phys.pos.x, z->phys.pos.y, &z->map_grid_pos.x, &z->map_grid_pos.y);
    coords_to_world_grid(z->phys.pos.x, z->phys.pos.y, &z->world_grid_pos.x, &z->world_grid_pos.y);

    if(z->anim_state == ZANIM_HURT && z->anim.curr_loop > 0)
    {
        z->anim_state = ZANIM_IDLE;
        z->hurt = false;
    }

    if(z->anim_state == ZANIM_ATTACK1 && z->anim.curr_loop > 0)
    {
        z->anim_state = ZANIM_IDLE;
        z->attacking = false;
    }

}

bool zombie_draw(Zombie* z, bool add_to_existing_batch)
{
    if(z == NULL) return false;

    if(is_in_camera_view(&z->phys.pos))
    {
        if(add_to_existing_batch)
        {
            gfx_sprite_batch_add(z->image, z->sprite_index,(int)z->phys.pos.x,(int)z->phys.pos.y, z->color,z->scale,0.0,z->opacity,false,false,false);
        }
        else
        {
            gfx_draw_image(z->image, z->sprite_index,(int)z->phys.pos.x,(int)z->phys.pos.y, z->color,z->scale,0.0,z->opacity,false,true);
        }

        return true;
    }

    return false;
}

Rect zombie_get_max_size(Zombie* z)
{
    Rect max_size = {0};

    float maxw = maxwh[z->model_index].x * z->scale;
    float maxh = maxwh[z->model_index].y * z->scale;

    max_size.x = z->phys.actual_pos.x;
    max_size.y = z->phys.actual_pos.y;
    max_size.w = maxw;
    max_size.h = maxh;

    return max_size;
}

void zombie_draw_debug(Zombie* z)
{
    if(z == NULL) return;

    Rect max_size = zombie_get_max_size(z);

    bool draw_debug_stuff = (z == zombie_get_by_id(zombie_info_id));
    if(zombie_debug) draw_debug_stuff = true;

    if(draw_debug_stuff)
    {
        gfx_draw_rect(&z->phys.actual_pos, COLOR_POS, 0.0, 1.0,1.0, false, true);
        gfx_draw_rect(&z->phys.collision, COLOR_COLLISON, 0.0, 1.0,1.0, false, true);
        gfx_draw_rect(&z->phys.hit, COLOR_HIT, 0.0, 1.0,1.0, false, true);
        gfx_draw_rect(&max_size, COLOR_MAXSIZE, 0.0, 1.0,1.0, false, true);

        // health bars
        float h = 4.0;
        float y = z->phys.actual_pos.y + max_size.h * 0.5 + h/2.0 + 2.0;
        float w = max_size.w;
        float x = z->phys.actual_pos.x;

        Rect r = {0};
        r.x = x;
        r.y = y;
        r.w = w;
        r.h = h;
        gfx_draw_rect(&r, COLOR_WHITE, 0.0, 1.0,1.0, true, true);

        float p = z->hp/z->hp_max;
        r.h *= 0.8;
        r.x = r.x-r.w/2.0;
        r.w *= p;
        r.x = r.x+r.w/2.0;
        gfx_draw_rect(&r, COLOR_RED, 0.0, 1.0,1.0, true, true);
    }

    // anim state
    // const float name_size = 0.11;
    // char* str = (char*)zombie_anim_state_str(z->anim_state);
    // Vector2f size = gfx_string_get_size(name_size, str);
    // float _x = z->phys.pos.x - size.x/2.0;
    // float _y = z->phys.pos.y + max_size.h*0.5 + 2.0;
    // gfx_draw_string(_x, _y, COLOR_WHITE, name_size, 0.0, 0.8, true, true, str);
}


void zombies_update(float delta_t)
{
    for(int i = zlist->count - 1; i >= 0 ; --i)
    {
        Zombie* z = &zombies[i];
        zombie_update(z, delta_t);
    }

    if((role == ROLE_LOCAL || role == ROLE_CLIENT))
    {
        // int wrow,wcol;
        // coords_to_world_grid(player->mouse_x, player->mouse_y, &wrow, &wcol);

        if(debug_enabled)
        {
            if(!moving_zombie)
            {
                zombie_info_id = 0xFFFFFFFF;

                Rect rm = {0};
                rm.x = player->mouse_x;
                rm.y = player->mouse_y;
                rm.w = 10;
                rm.h = rm.w;

                for(int j = zlist->count - 1; j >= 0; --j)
                {
                    Zombie* z = &zombies[j];
                    if(z->dead)
                        continue;
                    // if(!is_in_world_grid(&z->phys.pos, wrow, wcol))
                    //     continue;
                    if(rectangles_colliding(&rm, &z->phys.actual_pos))
                    {
                        zombie_info_id = z->id;
                        break;
                    }
                }
            }

        }
        else
        {
            moving_zombie = false;
            zombie_info_id = 0xFFFFFFFF;
        }

    }

}

void zombies_draw()
{
    for(int i = 0; i < zlist->count; ++i)
    {
        Zombie* z = &zombies[i];
        zombie_draw(z,false);
    }
}

Zombie* zombie_get_by_id(uint32_t id)
{
    for(int i = 0; i < zlist->count; ++i)
    {
        Zombie* z = &zombies[i];
        if(z->id == id) return z;
    }
    return NULL;
}

void zombie_kill_all()
{
    for(int i = zlist->count - 1; i >= 0 ; --i)
    {
        zombie_die2(i);
        // Zombie* z = &zombies[i];
        // zombie_remove(z);
    }
}

const char* zombie_anim_state_str(ZombieAnimState anim_state)
{
    switch(anim_state)
    {
        case ZANIM_IDLE: return "idle";
        case ZANIM_WALK: return "walk";
        case ZANIM_HURT: return "hurt";
        case ZANIM_ATTACK1: return "attack1";
        case ZANIM_DEAD: return "die";
        case ZANIM_NONE: return "";
        default: return "";
    }
}

void zombie_melee_check_collision(Zombie* z)
{
    if(z->melee_hit_count > 0)
        return;

    if(z->anim_state == ZANIM_ATTACK1)
    {

        float zx = z->phys.actual_pos.x;
        float zy = z->phys.actual_pos.y;

        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            Player* p = &players[i];
            if(!p->active) continue;

            float px = p->phys.actual_pos.x;
            float py = p->phys.actual_pos.y;
            bool collision = rectangles_colliding(&z->phys.actual_pos, &p->phys.hit);

            if(!collision)
            {
                float angle = calc_angle_rad(zx, zy, px, py);

                bool within_angle_range = ABS(angle - z->attack_angle) <= RAD(30); //hardcoded

                if(within_angle_range)
                {
                    float pr = MAX(p->phys.hit.w, p->phys.hit.h)/2.0;
                    float d = dist(px, py, zx, zy);
                    if(d <= (pr + z->attack_range))
                        collision = true;
                }
            }
            if(collision)
            {
                z->melee_hit_count++;
                printf("(%d) zombie hit '%s'\n", z->id, p->name);
                float damage = RAND_FLOAT(z->damage_min, z->damage_max);
                player_hurt(p,damage);
            }

        }
    }
}

// static functions
// ------------------------------------------------------------------------------------------

static void zombie_remove(Zombie* z)
{
    // don't need to do this actually
    // entity_remove_from_grid_boxes(ENTITY_TYPE_ZOMBIE,(void*)z);
    list_remove_by_item(zlist, z);
}

static void wander(Zombie* z, float delta_t)
{
    z->action_timer += delta_t;

    if(z->action_timer >= z->action_timer_max)
    {
        z->action_none = RAND_RANGE(1, 10) <= 2;
        z->action_timer = 0.0;
        z->angle = RAND_FLOAT(0.0,PI*2);
        z->action_timer_max = (rand() % 100)/50.0 + 0.5; // 0.5 to 2.0 seconds
    }
}

static void zombie_die2(int index)
{
    zombies[index].dead = true;
    zombies[index].anim.max_loops = 1;
    zombies[index].anim.finite = true;
}

static void zombie_die(Zombie* z)
{
    z->dead = true;
    z->anim.max_loops = 1;
    z->anim.finite = true;
}

