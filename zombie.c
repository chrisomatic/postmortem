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

#include "zombie.h"

#define IMG_ELEMENT_W 128
#define IMG_ELEMENT_H 128

#define NUM_ZOMBIES_INIT    10

uint32_t zombie_info_id = 0xFFFFFFFF;
// int zombie_info_index = -1;
bool zombies_idle = false;
ZombieModel zombie_models[ZOMBIE_MODELS_MAX];
int zombie_image_sets_none[ZOMBIE_MODELS_MAX][ZOMBIE_TEXTURES_MAX][ZANIM_MAX];
Zombie zombies[MAX_ZOMBIES] = {0};
glist* zlist = NULL;

// max width and heights for zombie models
static Vector2f maxwh[ZOMBIE_MODELS_MAX];
static Rect standard_size[ZOMBIE_MODELS_MAX];

static uint32_t zid = 0;

static void zombie_remove(int index);
static void wander(Zombie* zom, float delta_t);
static void zombie_die(int index);

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
    zombie.id = zid++;
    zombie.phys.pos.x = spawn->pos.x;
    zombie.phys.pos.y = spawn->pos.y;
    zombie.phys.accel.x = 0.0;
    zombie.phys.accel.y = 0.0;
    zombie.phys.max_linear_vel = spawn->max_linear_vel;
    zombie.push_vel.x = 0.0;
    zombie.push_vel.y = 0.0;
    zombie.action_timer = 0;
    zombie.sprite_index = 0;

    coords_to_map_grid(zombie.phys.pos.x, zombie.phys.pos.y, &zombie.map_grid_pos.x, &zombie.map_grid_pos.y);
    coords_to_world_grid(zombie.phys.pos.x, zombie.phys.pos.y, &zombie.world_grid_pos.x, &zombie.world_grid_pos.y);

    zombie.hp_max = spawn->hp_max;
    zombie.action = spawn->action;
    zombie.action_timer_max = spawn->action_timer_max;
    zombie.speed = spawn->speed;

    zombie.model_index = spawn->model_index;
    zombie.model_texture = spawn->model_texture;
    zombie.anim_state = ZANIM_WALK;
    zombie.sprite_index = 0;
    zombie.sprite_index_direction = 0;
    zombie.image = zombie_image_sets_none[zombie.model_index][zombie.model_texture][zombie.anim_state];

    // zombie.scale = (float)ZOMBIE_HEIGHT*spawn->scale/standard_size[zombie.model_index].h;
    zombie.scale = (float)ZOMBIE_HEIGHT/standard_size[zombie.model_index].h;

    // animation
    zombie.anim.curr_frame = 0;
    zombie.anim.max_frames = 16;
    zombie.anim.curr_frame_time = 0.0f;
    zombie.anim.max_frame_time = 0.04f;
    zombie.anim.finite = false;
    zombie.anim.curr_loop = 0;
    zombie.anim.max_loops = 0;
    zombie.anim.frame_sequence[0] = 0;
    zombie.anim.frame_sequence[1] = 1;
    zombie.anim.frame_sequence[2] = 2;
    zombie.anim.frame_sequence[3] = 3;
    zombie.anim.frame_sequence[4] = 4;
    zombie.anim.frame_sequence[5] = 5;
    zombie.anim.frame_sequence[6] = 6;
    zombie.anim.frame_sequence[7] = 7;
    zombie.anim.frame_sequence[8] = 8;
    zombie.anim.frame_sequence[9] = 9;
    zombie.anim.frame_sequence[10] = 10;
    zombie.anim.frame_sequence[11] = 11;
    zombie.anim.frame_sequence[12] = 12;
    zombie.anim.frame_sequence[13] = 13;
    zombie.anim.frame_sequence[14] = 14;
    zombie.anim.frame_sequence[15] = 15;

    zombie.hurt = false;
    zombie.attacking = false;
    zombie.attack_range = 40.0;
    zombie.attack_angle = 0.0;
    zombie.melee_hit_count = 0;

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

    zom->hurt = true;
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
            z->anim.max_frame_time = 0.055f;
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

    if(z->attacking)
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
    if(z->attacking)
    {
        float angle_deg = DEG(z->attack_angle);
        int sector = angle_sector(angle_deg, 16);

        if(sector == 15 || sector == 0)
            z->sprite_index_direction = 2;
        else if(sector == 1 || sector == 2)
            z->sprite_index_direction = 3;
        else if(sector == 3 || sector == 4)
            z->sprite_index_direction = 4;
        else if(sector == 5 || sector == 6)
            z->sprite_index_direction = 5;
        else if(sector == 7 || sector == 8)
            z->sprite_index_direction = 6;
        else if(sector == 9 || sector == 10)
            z->sprite_index_direction = 7;
        else if(sector == 11 || sector == 12)
            z->sprite_index_direction = 0;
        else if(sector == 13 || sector == 14) 
            z->sprite_index_direction = 1;
    }
    else
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

    }

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

    z->phys.pos.w = vr->w * z->scale;
    z->phys.pos.h = vr->h * z->scale;

    z->hit_box = calc_box(&z->phys.pos, 1.0, 0.5, 0);
    z->collision_box = calc_box(&z->phys.pos, 1.0, 0.4, 2);

    coords_to_map_grid(z->phys.pos.x, z->phys.pos.y, &z->map_grid_pos.x, &z->map_grid_pos.y);
    coords_to_world_grid(z->phys.pos.x, z->phys.pos.y, &z->world_grid_pos.x, &z->world_grid_pos.y);

    // for(int r = -2; r < 3; ++r)
    // {
    //     for(int c = -2; c < 3; ++c)
    //     {
    //         int row = z->map_grid_pos.x;
    //         int col = z->map_grid_pos.y;
    //         if(rectangles_colliding)
    //     }
    // }

}

void zombie_update_pursue(Zombie* z, float delta_t)
{

    if(z->anim_state == ZANIM_ATTACK1)
        return;

    if(z->anim_state == ZANIM_HURT)
        return;

    if(zombies_idle)
        return;

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
                printf("(%d) zombie is NOT pursuing player '%s'\n", z->id, z->pursue_player->name);
                z->pursue_player = NULL;
                has_target = false;
            }
        }

        if(!has_target)
        {
            float d = dist(z->phys.pos.x, z->phys.pos.y, z->pursue_target.x, z->pursue_target.y);
            if(d <= 5.0)
            {
                printf("(%d) zombie has reached last known player location\n", z->id);
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
                z->pursuing = true;
                printf("(%d) zombie is now pursuing player '%s'\n", z->id, p->name);
            }

        }

    }
}

Vector2f zombie_update_movement(Zombie* z, float delta_t)
{
    Vector2f accel = {0.0,0.0};

    if(zombies_idle)
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
        float angle = calc_angle_rad(z->phys.pos.x, z->phys.pos.y, tx, ty);

        // set accel
        accel.x += amt*cosf(angle);
        accel.y += amt*sinf(2*PI-angle);
    }
    else
    {
        wander(z, delta_t);
        switch(z->action)
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
    }

    return accel;
}

void zombie_update(Zombie* z, float delta_t)
{

    Vector2f accel = zombie_update_movement(z, delta_t);

    Rect prior_pos = z->phys.pos;
    Rect prior_collision_box = z->collision_box;

    physics_begin(&z->phys);
    physics_add_friction(&z->phys, 16.0);
    physics_add_force(&z->phys, accel.x, accel.y);
    physics_simulate(&z->phys, delta_t);
    physics_limit_pos(&map.rect, &z->phys.pos);


    z->moving = !(FEQ(accel.x,0.0) && FEQ(accel.y,0.0));

    if(!z->attacking)
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

    if(z->anim_state == ZANIM_HURT && z->anim.curr_loop > 0)
    {
        z->anim_state = ZANIM_IDLE;
        z->hurt = false;
        zombie_update_anim_state(z);
        zombie_update_anim_timing(z);
        zombie_update_image(z);
    }

    if(z->anim_state == ZANIM_ATTACK1 && z->anim.curr_loop > 0)
    {
        z->anim_state = ZANIM_IDLE;
        z->attacking = false;
        zombie_update_anim_state(z);
        zombie_update_anim_timing(z);
        zombie_update_image(z);
    }

    zombie_update_sprite_index(z);
    zombie_update_boxes(z);

    // disabled for now
    // zombie_check_block_collision(z, prior_pos, prior_collision_box);

    zombie_melee_check_collision(z);
}

bool zombie_check_block_collision(Zombie* z, Rect prior_pos, Rect prior_collision_box)
{
    rect_collision_data_t data = {0};
    data.collide = false;

    bool collide = false;
    for(int i = 0; i < blist->count; ++i)
    {
        block_t* b = &blocks[i];
        Rect cb = z->collision_box;
        float delta_x = z->phys.pos.x - prior_pos.x;
        float delta_y = z->phys.pos.y - prior_pos.y;
        collide = physics_rect_collision(&prior_collision_box, &cb, &b->collision_box, delta_x, delta_y, &data);
        if(collide)
        {
            // printf("block collision index: %d\n", i);
            prior_pos.x = z->phys.pos.x;
            prior_pos.y = z->phys.pos.y;
            // prior_collision_box = p->collision_box;

            z->phys.pos.x += (cb.x - z->collision_box.x);
            z->phys.pos.y += (cb.y - z->collision_box.y);
            zombie_update_boxes(z);

        }
    }
}


void zombie_draw(Zombie* z, bool add_to_existing_batch)
{
    if(z == NULL) return;

    if(is_in_camera_view(&z->phys.pos))
    {
        if(add_to_existing_batch)
        {
            gfx_sprite_batch_add(z->image, z->sprite_index,(int)z->phys.pos.x,(int)z->phys.pos.y, COLOR_TINT_NONE,z->scale,0.0,1.0,false,false,false);
        }
        else
        {
            gfx_draw_image(z->image, z->sprite_index,(int)z->phys.pos.x,(int)z->phys.pos.y, COLOR_TINT_NONE,z->scale,0.0,1.0,false,true);
        }

        // bool draw_debug_stuff = debug_enabled;
        // if(!draw_debug_stuff)
        // {
        //     // draw_debug_stuff = (z == &zombies[zombie_info_index]);
        //     draw_debug_stuff = (z == zombie_get_by_id(zombie_info_id));
        // }

        bool draw_debug_stuff = debug_enabled && (z == zombie_get_by_id(zombie_info_id));

        if(draw_debug_stuff)
        {
            float maxw = maxwh[z->model_index].x * z->scale;
            float maxh = maxwh[z->model_index].y * z->scale;
            Rect max_size = {0};
            max_size.x = z->phys.pos.x;
            max_size.y = z->phys.pos.y;
            max_size.w = maxw;
            max_size.h = maxh;

            gfx_draw_rect(&z->phys.pos, COLOR_POS, 0.0, 1.0,1.0, false, true);
            gfx_draw_rect(&z->collision_box, COLOR_COLLISON, 0.0, 1.0,1.0, false, true);
            gfx_draw_rect(&z->hit_box, COLOR_HIT, 0.0, 1.0,1.0, false, true);
            gfx_draw_rect(&max_size, COLOR_MAXSIZE, 0.0, 1.0,1.0, false, true);

            // health bars
            float h = 4.0;
            float y = z->phys.pos.y + maxh * 0.5 + h/2.0 + 2.0;
            float w = maxw;
            float x = z->phys.pos.x;

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
    }
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
                    // if(!is_in_world_grid(&z->phys.pos, wrow, wcol))
                    //     continue;
                    if(rectangles_colliding(&rm, &z->phys.pos))
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

const char* zombie_anim_state_str(ZombieAnimState anim_state)
{
    switch(anim_state)
    {
        case ZANIM_IDLE: return "idle";
        case ZANIM_WALK: return "walk";
        case ZANIM_HURT: return "hurt";
        case ZANIM_ATTACK1: return "attack1";
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

        float zx = z->phys.pos.x;
        float zy = z->phys.pos.y;

        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            Player* p = &players[i];
            if(!p->active) continue;

            float px = p->phys.pos.x;
            float py = p->phys.pos.y;
            bool collision = rectangles_colliding(&z->phys.pos, &p->hit_box);

            if(!collision)
            {
                float angle = calc_angle_rad(zx, zy, px, py);

                bool within_angle_range = ABS(angle - z->attack_angle) <= RAD(30); //hardcoded

                if(within_angle_range)
                {
                    float pr = MAX(p->hit_box.w, p->hit_box.h)/2.0;
                    float d = dist(px, py, zx, zy);
                    if(d <= (pr + z->attack_range))
                        collision = true;
                }
            }
            if(collision)
            {
                z->melee_hit_count++;
                printf("zombie hit collision with '%s'\n", p->name);
            }

        }
    }
}

// static functions
// ------------------------------------------------------------------------------------------

static void zombie_remove(int index)
{
    list_remove(zlist, index);
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


static void zombie_die(int index)
{
    zombie_remove(index);
}

