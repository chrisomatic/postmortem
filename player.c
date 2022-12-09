#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "window.h"
#include "math2d.h"
#include "gfx.h"
#include "camera.h"
#include "projectile.h"
#include "player.h"
#include "world.h"
#include "lighting.h"
#include "net.h"
#include "zombie.h"
#include "io.h"
#include "effects.h"
#include "main.h"

// global vars
// ------------------------------------------------------------
Player players[MAX_CLIENTS];
Player* player = &players[0];
int player_count = 0;
uint32_t player_colors[MAX_CLIENTS] = {
    COLOR_BLUE,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_ORANGE,
    COLOR_PURPLE,
    COLOR_CYAN,
    COLOR_PINK,
    COLOR_YELLOW,
};

int player_image_sets_none[PLAYER_MODELS_MAX][PLAYER_TEXTURES_MAX][ANIM_MAX];
int player_image_sets_guns[PLAYER_MODELS_MAX][PLAYER_TEXTURES_MAX][ANIM_MAX][GUN_TYPE_MAX];
int player_image_sets_melees[PLAYER_MODELS_MAX][PLAYER_TEXTURES_MAX][ANIM_MAX][MELEE_TYPE_MAX];

PlayerModel player_models[PLAYER_MODELS_MAX];

Gun guns[GUN_MAX] = {0};
Melee melees[MELEE_MAX] = {0};

bool moving_zombie = false;

// ------------------------------------------------------------

#define IMG_ELEMENT_W 128
#define IMG_ELEMENT_H 128


static int gun_image_sets[PLAYER_MODELS_MAX][ANIM_MAX][GUN_MAX];
static int melee_image_sets[PLAYER_MODELS_MAX][ANIM_MAX][MELEE_MAX];
static int blocks_image;
static int crosshair_image;



//TEMP: blocks
block_t blocks[MAX_BLOCKS] = {0};
glist* blist = NULL;
BlockProp block_props[BLOCK_MAX] = {0};

// ------------------------------------------------------------


static void mouse_gun_cb(void* player, MouseTrigger trigger);
static void mouse_melee_cb(void* player, MouseTrigger trigger);
static void mouse_block_add_cb(void* player, MouseTrigger trigger);
static void mouse_block_remove_cb(void* player, MouseTrigger trigger);
static void mouse_zombie_move_cb(void* player, MouseTrigger trigger);

// ------------------------------------------------------------



void player_init_models()
{
    int idx = HUMAN1;
    player_models[idx].index = idx;
    player_models[idx].name = "human1";
    player_models[idx].textures = 1;
    // player_model_texture_count += player_models[idx].textures;

    // idx = HUMAN2;
    // player_models[idx].index = idx;
    // player_models[idx].name = "human2";
    // player_models[idx].textures = 1;
    // player_model_texture_count += player_models[idx].textures;
}

void player_init_images()
{
    //none
    for(int pm = 0; pm < PLAYER_MODELS_MAX; ++pm)
    {
        for(int t = 0; t < PLAYER_TEXTURES_MAX; ++t)
        {
            for(int ps = 0; ps < ANIM_MAX; ++ps)
            {
                player_image_sets_none[pm][t][ps] = -1;

                char fname[100] = {0};
                sprintf(fname, "img/characters/%s_%d-%s.png", player_models[pm].name, t, player_anim_state_str(ps));
                // if(access(fname, F_OK) == 0)
                if(io_file_exists(fname))
                {
                    player_image_sets_none[pm][t][ps] = gfx_load_image(fname, false, true, IMG_ELEMENT_W, IMG_ELEMENT_H, NULL);
                    // printf("%s -> %d\n", fname, player_image_sets[pm][t][ps][wt]);
                }
            }
        }
    }


    // guns
    for(int pm = 0; pm < PLAYER_MODELS_MAX; ++pm)
    {
        for(int t = 0; t < PLAYER_TEXTURES_MAX; ++t)
        {
            for(int ps = 0; ps < ANIM_MAX; ++ps)
            {
                for(int wt = 0; wt < GUN_TYPE_MAX; ++wt)
                {
                    player_image_sets_guns[pm][t][ps][wt] = -1;

                    char fname[100] = {0};
                    sprintf(fname, "img/characters/%s_%d-%s_%s.png", player_models[pm].name, t, player_anim_state_str(ps), gun_type_str(wt));
                    // if(access(fname, F_OK) == 0)
                    if(io_file_exists(fname))
                    {
                        player_image_sets_guns[pm][t][ps][wt] = gfx_load_image(fname, false, true, IMG_ELEMENT_W, IMG_ELEMENT_H, NULL);
                        // printf("%s -> %d\n", fname, player_image_sets[pm][t][ps][wt]);
                    }
                }
            }
        }
    }

    // melees
    for(int pm = 0; pm < PLAYER_MODELS_MAX; ++pm)
    {
        for(int t = 0; t < PLAYER_TEXTURES_MAX; ++t)
        {
            for(int ps = 0; ps < ANIM_MAX; ++ps)
            {
                for(int wt = 0; wt < MELEE_TYPE_MAX; ++wt)
                {
                    player_image_sets_melees[pm][t][ps][wt] = -1;

                    char fname[100] = {0};
                    sprintf(fname, "img/characters/%s_%d-%s_%s.png", player_models[pm].name, t, player_anim_state_str(ps), melee_type_str(wt));
                    // if(access(fname, F_OK) == 0)
                    if(io_file_exists(fname))
                    {
                        player_image_sets_melees[pm][t][ps][wt] = gfx_load_image(fname, false, true, IMG_ELEMENT_W, IMG_ELEMENT_H, NULL);
                        // printf("%s -> %d\n", fname, player_image_sets[pm][t][ps][wt]);
                    }
                }
            }
        }
    }

    blocks_image = gfx_load_image("img/block_set.png", false, true, 32, 50, NULL);
    crosshair_image = gfx_load_image("img/crosshair2.png", false, false, 0, 0, NULL);
}


void player_init_controls(Player* p)
{
    window_controls_clear_keys();

    // map keys
    window_controls_add_key(&p->actions[PLAYER_ACTION_UP].state, GLFW_KEY_W);
    window_controls_add_key(&p->actions[PLAYER_ACTION_DOWN].state, GLFW_KEY_S);
    window_controls_add_key(&p->actions[PLAYER_ACTION_LEFT].state, GLFW_KEY_A);
    window_controls_add_key(&p->actions[PLAYER_ACTION_RIGHT].state, GLFW_KEY_D);

    window_controls_add_key(&p->actions[PLAYER_ACTION_RUN].state, GLFW_KEY_LEFT_SHIFT);
    window_controls_add_key(&p->actions[PLAYER_ACTION_JUMP].state, GLFW_KEY_SPACE);
    window_controls_add_key(&p->actions[PLAYER_ACTION_INTERACT].state, GLFW_KEY_E);

    window_controls_add_mouse_button(&p->actions[PLAYER_ACTION_PRIMARY_ACTION].state, GLFW_MOUSE_BUTTON_LEFT);
    window_controls_add_mouse_button(&p->actions[PLAYER_ACTION_SECONDARY_ACTION].state, GLFW_MOUSE_BUTTON_RIGHT);
    window_controls_add_key(&p->actions[PLAYER_ACTION_RELOAD].state, GLFW_KEY_R);

    window_controls_add_key(&p->actions[PLAYER_ACTION_EQUIP].state, GLFW_KEY_TAB);
    window_controls_add_key(&p->actions[PLAYER_ACTION_SPAWN_ZOMBIE].state, GLFW_KEY_Z);
    window_controls_add_key(&p->actions[PLAYER_ACTION_CYCLE_EQUIP_DOWN].state, GLFW_KEY_1);
    window_controls_add_key(&p->actions[PLAYER_ACTION_CYCLE_EQUIP_UP].state, GLFW_KEY_2);

    window_controls_add_key(&p->actions[PLAYER_ACTION_DEBUG].state, GLFW_KEY_F2);
    window_controls_add_key(&p->actions[PLAYER_ACTION_EDITOR].state, GLFW_KEY_F3);
    window_controls_add_key(&p->actions[PLAYER_ACTION_MENU].state, GLFW_KEY_ESCAPE);
}

static void player_init(int index)
{
    Player* p = &players[index];

    p->index = index;
    if(STR_EMPTY(p->name))
    {
        sprintf(p->name, "Player %d", p->index);
    }


    // animation
    // --------------------------------------------------------
    p->anim.curr_frame = 0;
    p->anim.max_frames = 16;
    p->anim.curr_frame_time = 0.0f;
    p->anim.max_frame_time = 0.04f;
    p->anim.finite = false;
    p->anim.curr_loop = 0;
    p->anim.max_loops = 0;
    p->anim.frame_sequence[0] = 12;
    p->anim.frame_sequence[1] = 13;
    p->anim.frame_sequence[2] = 14;
    p->anim.frame_sequence[3] = 15;
    p->anim.frame_sequence[4] = 0;
    p->anim.frame_sequence[5] = 1;
    p->anim.frame_sequence[6] = 2;
    p->anim.frame_sequence[7] = 3;
    p->anim.frame_sequence[8] = 4;
    p->anim.frame_sequence[9] = 5;
    p->anim.frame_sequence[10] = 6;
    p->anim.frame_sequence[11] = 7;
    p->anim.frame_sequence[12] = 8;
    p->anim.frame_sequence[13] = 9;
    p->anim.frame_sequence[14] = 10;
    p->anim.frame_sequence[15] = 11;

    // state vars
    // --------------------------------------------------------
    p->item_equipped = false;
    p->item_index = 1;
    player_set_equipped_item(p, 0);

    p->state = PSTATE_NONE;
    p->busy = false;
    p->moving = false;
    p->running = false;

    // model and texture
    // --------------------------------------------------------
    p->model_index = HUMAN1;
    p->model_texture = 0;
    p->anim_state = ANIM_IDLE;
    player_update_image(p);

    // boxes and phys
    // --------------------------------------------------------
    p->sprite_index_direction = 0;
    p->sprite_index = 0;
    p->angle = 0.0;
    player_update_sprite_index(p);

    int standard_img = player_image_sets_none[p->model_index][p->model_texture][ANIM_IDLE];
    if(standard_img != -1)
    {
        Rect* r = &gfx_images[standard_img].visible_rects[0];
        p->scale = (float)PLAYER_HEIGHT/r->h;
        p->standard_size = *r;
    }
    else
    {
        LOGE("Player standard_img is -1");
    }
    p->standard_size.w *= p->scale;
    p->standard_size.h *= p->scale;

    float maxw=0.0, maxh=0.0;
    player_get_maxwh(p, &maxw, &maxh);
    p->max_size.w = maxw;
    p->max_size.h = maxh;

    p->phys.pos.x = 1000.0;
    p->phys.pos.y = 1000.0;
    p->phys.actual_pos.x = p->phys.pos.x;
    p->phys.actual_pos.y = p->phys.pos.y;

    p->phys.mass = 1.0;

    Rect r = p->standard_size;
    r.x = p->phys.pos.x;
    r.y = p->phys.pos.y;
    p->phys.hit = calc_sub_box(&r, 1.0, 0.5, 0);
    p->phys.collision = calc_sub_box(&r, 1.0, 0.4, 2);

    player_update_static_boxes(p);
    player_update_boxes(p);
    player_update_pos_offset(p);

    p->speed = 128.0;
    p->max_base_speed = 128.0;
    p->phys.max_linear_vel = p->max_base_speed;
    p->phys.vel.x = 0.0;
    p->phys.vel.y = 0.0;



    // other
    // --------------------------------------------------------
    p->hp_max = 100.0;
    p->hp = p->hp_max;
    coords_to_map_grid(p->phys.actual_pos.x, p->phys.actual_pos.y, &p->grid_pos.x, &p->grid_pos.y);


    p->predicted_state_index = 0;

    // light for player
    p->point_light = -1;

    p->detect_radius = 10.0;

    /*
    if(p == player)
    {
        //p->point_light = lighting_point_light_add(p->phys.pos.x,p->phys.pos.y,1.0,1.0,1.0,0.2);
    }
    */


}

void players_init()
{
    player_init_models();
    player_init_images();
    
    //TEMP: blocks
    // blocks_init()
    int idx = BLOCK_0;
    block_props[idx].type = idx;
    block_props[idx].hp = 100.0;
    block_props[idx].color = COLOR_TINT_NONE;
    block_props[idx].image = blocks_image;
    block_props[idx].sprite_index = idx;

    idx = BLOCK_1;
    block_props[idx].type = idx;
    block_props[idx].hp = 100.0;
    block_props[idx].color = COLOR_TINT_NONE;
    block_props[idx].image = blocks_image;
    block_props[idx].sprite_index = idx;

    blist = list_create((void*)blocks, MAX_BLOCKS, sizeof(blocks[0]));
    if(blist == NULL)
    {
        LOGE("block list failed to create");
    }

    weapons_init();

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        Player* p = &players[i];
        player_init(i);
        if(role == ROLE_LOCAL || role == ROLE_CLIENT)
        {
            if(player == p)
            {
                LOGI("My player: %d", i);
                player_init_controls(p);
                p->active = true;
            }
        }

        if(p->active)
            player_count++;
    }

}

const char* player_state_str(PlayerState state)
{
    switch(state)
    {
        case PSTATE_NONE: return "none";
        case PSTATE_ATTACKING: return "attacking";
        case PSTATE_RELOADING: return "reloading";
        default: return "unknown";
    }
}

const char* player_anim_state_str(PlayerAnimState anim_state)
{
    switch(anim_state)
    {
        case ANIM_IDLE: return "idle";
        case ANIM_WALK: return "walk";
        case ANIM_ATTACK1: return "attack1";
        case ANIM_NONE: return "";
        default: return "";
    }
}

int player_get_image_index(Player* p)
{

    if(p->item.item_type == ITEM_TYPE_GUN)
    {
        Gun* gun = (Gun*)p->item.props;
        return player_image_sets_guns[p->model_index][p->model_texture][p->anim_state][gun->type];
    }
    else if(p->item.item_type == ITEM_TYPE_MELEE)
    {
        Melee* melee = (Melee*)p->item.props;
        return player_image_sets_melees[p->model_index][p->model_texture][p->anim_state][melee->type];
    }

    return player_image_sets_none[p->model_index][p->model_texture][p->anim_state];
}

int players_get_count()
{
    player_count = 0;
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(players[i].active)
            player_count++;
    }

    return player_count;
}

void player_get_maxwh(Player* p, float* w, float* h)
{
    float maxw = 0.0;
    float maxh = 0.0;

    // none
    for(int ps = 0; ps < ANIM_MAX; ++ps)
    {
        int img = player_image_sets_none[p->model_index][p->model_texture][ps];
        if(img == -1) continue;
        for(int i = 0; i < gfx_images[img].element_count; ++i)
        {
            Rect* vr = &gfx_images[img].visible_rects[i];
            if(IS_RECT_EMPTY(vr)) continue;
            maxw = MAX(maxw, vr->w);
            maxh = MAX(maxh, vr->h);
        }
    }

    // guns
    for(int ps = 0; ps < ANIM_MAX; ++ps)
    {
        for(int wt = 0; wt < GUN_TYPE_MAX; ++wt)
        {
            int img = player_image_sets_guns[p->model_index][p->model_texture][ps][wt];
            if(img == -1) continue;
            for(int i = 0; i < gfx_images[img].element_count; ++i)
            {
                Rect* vr = &gfx_images[img].visible_rects[i];
                if(IS_RECT_EMPTY(vr)) continue;
                maxw = MAX(maxw, vr->w);
                maxh = MAX(maxh, vr->h);
            }
        }
    }

    // melees
    for(int ps = 0; ps < ANIM_MAX; ++ps)
    {
        for(int wt = 0; wt < MELEE_TYPE_MAX; ++wt)
        {
            int img = player_image_sets_melees[p->model_index][p->model_texture][ps][wt];
            if(img == -1) continue;
            for(int i = 0; i < gfx_images[img].element_count; ++i)
            {
                Rect* vr = &gfx_images[img].visible_rects[i];
                if(IS_RECT_EMPTY(vr)) continue;
                maxw = MAX(maxw, vr->w);
                maxh = MAX(maxh, vr->h);
            }
        }
    }
    *w = maxw;
    *h = maxh;
}



void player_equip_gun(Player* p, GunIndex index)
{
    Gun* gun = &guns[index];
    player_equip_item(p, ITEM_TYPE_GUN, (void*)gun, true, true);

    player_set_mouse(&p->lmouse, true, true, false, gun->fire_period, mouse_gun_cb);
    player_set_mouse_nothing(&p->rmouse);

    // player_set_mouse(&p->rmouse, false, true, true, 0.0, mouse_zombie_move_cb);
}

void player_equip_melee(Player* p, MeleeIndex index)
{
    Melee* melee = &melees[index];
    player_equip_item(p, ITEM_TYPE_MELEE, (void*)melee, true, true);

    player_set_mouse(&p->lmouse, true, true, false, melee->period, mouse_melee_cb);
    player_set_mouse_nothing(&p->rmouse);

    // player_set_mouse(&p->rmouse, false, true, true, 0.0, mouse_zombie_move_cb);
}

void player_equip_block(Player* p, BlockType index)
{
    BlockProp* block = &block_props[index];
    player_equip_item(p, ITEM_TYPE_BLOCK, (void*)block, true, false);

    player_set_mouse(&p->lmouse, true, true, false, 20.0, mouse_block_add_cb);
    player_set_mouse(&p->rmouse, true, true, false, 20.0, mouse_block_remove_cb);
}

void player_set_equipped_item(Player* p, int idx) //TEMP
{
    if(!p->item_equipped)
    {
        player_equip_item(p, ITEM_TYPE_NONE, NULL, false, false);
        player_set_mouse_nothing(&p->lmouse);
        player_set_mouse(&p->rmouse, false, true, true, 0.0, mouse_zombie_move_cb);
        return;
    }

    p->item_index = idx;
    if(idx > 6)
    {
        p->item_index = 1;
    }
    else if(idx <= 0)
    {
        p->item_index = 6;
    }
    idx = p->item_index;


    if(idx == 1)
    {
        player_equip_gun(p, GUN_SHOTGUN1);
    }
    else if(idx == 2)
    {
        player_equip_gun(p, GUN_MACHINEGUN1);
    }
    else if(idx == 3)
    {
        player_equip_gun(p, GUN_PISTOL1);
    }
    else if(idx == 4)
    {
        player_equip_melee(p, MELEE_KNIFE1);
    }
    else if(idx == 5)
    {
        player_equip_block(p, BLOCK_0);
    }
    else if(idx == 6)
    {
        player_equip_block(p, BLOCK_1);
    }
    // else
    // {
    //     player_equip_item(p, ITEM_TYPE_NONE, NULL, false, false);
    //     p->item_equipped = false;
    // }

}


void player_equip_item(Player* p, PlayerItemType itype, void* props, bool drawable, bool mouse_aim)
{
    p->item.item_type = itype;
    p->item.props = props;
    p->item.drawable = drawable;
    p->item.mouse_aim = mouse_aim;
}

Rect* player_get_equipped_item_pos(Player* p)
{
    if(p->item.drawable && p->item.props != NULL)
    {
        if(p->item.item_type == ITEM_TYPE_GUN)
        {
            Gun* gun = (Gun*)p->item.props;
            return &gun->pos;
        }
        else if(p->item.item_type == ITEM_TYPE_MELEE)
        {
            Melee* melee = (Melee*)p->item.props;
            return &melee->pos;
        }
    }

    return &p->phys.actual_pos;
}

int player_get_equipped_item_img(Player* p)
{
    if(p->item.drawable && p->item.props != NULL)
    {
        if(p->item.item_type == ITEM_TYPE_GUN)
        {
            Gun* gun = (Gun*)p->item.props;
            return gun_get_image_index(p->model_index, p->anim_state, gun->type);
        }
        else if(p->item.item_type == ITEM_TYPE_MELEE)
        {
            Melee* melee = (Melee*)p->item.props;
            return melee_get_image_index(p->model_index, p->anim_state, melee->type);
        }
    }

    return -1;
}


// void* to avoid circular reference in player struct
static void mouse_gun_cb(void* player, MouseTrigger trigger)
{
    Player* p = (Player*)player;

    if(p->busy)
        return;

    if(p->item.item_type != ITEM_TYPE_GUN)
        return;

    if(p->item.props == NULL)
        return;

    Gun* gun = (Gun*)p->item.props;
    gun_fire(p, gun, trigger == MOUSE_TRIGGER_HOLD);
}

static void mouse_melee_cb(void* player, MouseTrigger trigger)
{
    Player* p = (Player*)player;

    if(p->busy)
        return;

    if(p->item.props == NULL)
        return;

    Melee* melee = (Melee*)p->item.props;
    p->melee_hit_count = 0;
    p->attacking_state = melee->anim_state;
    p->state = PSTATE_ATTACKING;
    p->busy = true;
    player_add_detect_radius(p, 2.0);
}

static void mouse_block_add_cb(void* player, MouseTrigger trigger)
{
    Player* p = (Player*)player;

    if(p->busy)
        return;

    if(p->item.props == NULL)
        return;

    bool in_range = is_grid_within_radius(p->mouse_r, p->mouse_c, p->grid_pos.x, p->grid_pos.y, PLAYER_BLOCK_PLACEMENT_RADIUS);
    if(!in_range) return;

    bool add_block = true;
    for(int i = 0; i < blist->count; ++i)
    {
        if(p->mouse_r == blocks[i].row && p->mouse_c == blocks[i].col)
        {
            add_block = false;
            break;
        }
    }
    if(add_block)
    {
        BlockProp* bp = (BlockProp*)p->item.props;

        block_t b = {0};
        b.row = p->mouse_r;
        b.col = p->mouse_c;

        map_grid_to_rect(b.row, b.col, &b.phys.pos);
        memcpy(&b.phys.collision,&b.phys.pos,sizeof(Rect));
        //b.phys.collision = calc_sub_box(&b.phys.pos, 1.0, 0.6, 2);
        b.phys.mass = 10000.0;

        b.type = bp->type;
        b.hp = bp->hp;
        list_add(blist, (void*)&b);
    }
}

static void mouse_block_remove_cb(void* player, MouseTrigger trigger)
{
    Player* p = (Player*)player;

    if(p->busy)
        return;

    for(int i = 0; i < blist->count; ++i)
    {
        if(p->mouse_r == blocks[i].row && p->mouse_c == blocks[i].col)
        {
            ParticleEffect pe ={0};
            memcpy(&pe, &particle_effects[EFFECT_BLOCK_DESTROY],sizeof(ParticleEffect));
            pe.sprite_index = blocks[i].type;
            particles_spawn_effect(blocks[i].phys.collision.x, blocks[i].phys.collision.y, &pe, 1.0, true, false);
            particles_spawn_effect(blocks[i].phys.collision.x, blocks[i].phys.collision.y-16, &particle_effects[EFFECT_SMOKE2], 1.0, true, false);

            list_remove(blist, i);

            break;
        }
    }
}

static void mouse_zombie_move_cb(void* player, MouseTrigger trigger)
{
    if(trigger == MOUSE_TRIGGER_PRESS)
    {
        Zombie* z = zombie_get_by_id(zombie_info_id);
        if(z != NULL)
        {
            moving_zombie = true;
        }
    }

    if(moving_zombie && trigger == MOUSE_TRIGGER_RELEASE)
    {
        moving_zombie = false;
    }

}


void player_set_mouse_nothing(MouseData* mouse_data)
{
    mouse_data->cooldown = 0.0;
    mouse_data->period = 9999999.0;
    mouse_data->cb = NULL;
    mouse_data->trigger_on_held = false;
    mouse_data->trigger_on_press = false;
    mouse_data->trigger_on_release = false;
}

void player_set_mouse(MouseData* mouse_data, bool held, bool press, bool release, float period, mouse_trigger_cb_t cb)
{
    mouse_data->cooldown = 0.0;
    mouse_data->period = period;
    mouse_data->cb = cb;
    mouse_data->trigger_on_held = held;
    mouse_data->trigger_on_press = press;
    mouse_data->trigger_on_release = release;
}


void player_update_mouse_click(Player* p, bool active, bool toggled, MouseData* mouse, float delta_t)
{
    if(mouse->cooldown > 0.0)
    {
        mouse->cooldown -= (delta_t*1000);
    }

    bool ready = mouse->cooldown <= 0.0 || mouse->period <= 0.0;

    MouseTrigger trigger = MOUSE_TRIGGER_NONE;

    // clear the trigger
    mouse->triggered = false;

    // freshly clicked
    if(active && toggled)
    {
        // printf("lclick (%.2f)\n", mouse->cooldown);
        if(ready && mouse->trigger_on_press)
        {
            trigger = MOUSE_TRIGGER_PRESS;
            mouse->triggered = true;
        }
        mouse->held = false;

    }
    // freshly released
    else if(!active && toggled)
    {
        if(ready && mouse->trigger_on_release)
        {
            trigger = MOUSE_TRIGGER_RELEASE;
            // ready = true;
            mouse->triggered = true;
        }
        mouse->held = false;

    }
    // held
    else if(active)
    {
        if(ready && mouse->trigger_on_held)
        {
            trigger = MOUSE_TRIGGER_HOLD;
            mouse->triggered = true;
            mouse->held = true;
        }
    }

    if(ready && mouse->triggered)
    {
        mouse->cooldown = mouse->period;
    }

    if(mouse->triggered)
    {
        if(mouse->cb != NULL)
        {
            mouse->cb((void*)p, trigger);
        }
    }
}

void player_add_detect_radius(Player* p, float add)
{
    p->detect_radius = RANGE(p->detect_radius + add, 6.0, 50.0);
}

void player_update_anim_timing(Player* p)
{
    switch(p->anim_state)
    {
        case ANIM_IDLE:
            p->anim.max_frame_time = 0.15f;
            break;
        case ANIM_WALK:
        {
            p->anim.max_frame_time = 0.055f;
            float pvx = p->phys.vel.x;
            float pvy = p->phys.vel.y;
            float pv = sqrt(SQ(pvx) + SQ(pvy));
            float scale = 128.0/pv;
            p->anim.max_frame_time *= scale;
        } break;
        case ANIM_ATTACK1:
            p->anim.max_frame_time = 0.025f;
            break;
        default:
            p->anim.max_frame_time = 0.04f;
            break;
    }

}

void player_update_anim_state(Player* p)
{
    PlayerAnimState prior = p->anim_state;

    if(p->state == PSTATE_ATTACKING)
    {
        p->anim_state = p->attacking_state;
        //TEMP
        p->anim.frame_sequence[0] = 0;
        p->anim.frame_sequence[1] = 1;
        p->anim.frame_sequence[2] = 2;
        p->anim.frame_sequence[3] = 3;
        p->anim.frame_sequence[4] = 4;
        p->anim.frame_sequence[5] = 5;
        p->anim.frame_sequence[6] = 6;
        p->anim.frame_sequence[7] = 7;
        p->anim.frame_sequence[8] = 8;
        p->anim.frame_sequence[9] = 9;
        p->anim.frame_sequence[10] = 10;
        p->anim.frame_sequence[11] = 11;
        p->anim.frame_sequence[12] = 12;
        p->anim.frame_sequence[13] = 13;
        p->anim.frame_sequence[14] = 14;
        p->anim.frame_sequence[15] = 15;
    }
    else if(p->moving)
    {
        p->anim_state = ANIM_WALK;
    }
    else
    {
        p->anim_state = ANIM_IDLE;
    }

    // reset the animation
    if(p->anim_state != prior)
    {
        // printf("Player state change: %d -> %d\n", prior, p->anim_state);
        p->anim.curr_frame = 0;
        p->anim.curr_frame_time = 0.0;
        p->anim.curr_loop = 0;
    }

    return;
}

void player_update_image(Player* p)
{
    p->image = player_get_image_index(p);
}

void player_update_pos_offset(Player* p)
{
    GFXImage* img = &gfx_images[p->image];
    Rect* vr = &img->visible_rects[p->sprite_index];
    float img_center_x = img->element_width/2.0;
    float img_center_y = img->element_height/2.0;
    float offset_x = (vr->x - img_center_x)*p->scale;
    float offset_y = (vr->y - img_center_y)*p->scale;
    physics_set_pos_offset(&p->phys, offset_x, offset_y);    // change offset based off new sprite
}

void player_update_boxes(Player* p)
{
    GFXImage* img = &gfx_images[p->image];
    Rect* vr = &img->visible_rects[p->sprite_index];

    p->phys.actual_pos.w = vr->w * p->scale;
    p->phys.actual_pos.h = vr->h * p->scale;

    // maybe not necessary
    p->phys.pos.w = p->phys.actual_pos.w;
    p->phys.pos.h = p->phys.actual_pos.h;
}

void player_update_static_boxes(Player* p)
{
    float px = p->phys.actual_pos.x;
    float py = p->phys.actual_pos.y;

    p->standard_size.x = px;
    p->standard_size.y = py;

    p->max_size.x = px;
    p->max_size.y = py;
}



void player_update_sprite_index(Player* p)
{
    float angle_deg = DEG(p->angle);

    bool up = p->actions[PLAYER_ACTION_UP].state;
    bool down = p->actions[PLAYER_ACTION_DOWN].state;
    bool left = p->actions[PLAYER_ACTION_LEFT].state;
    bool right = p->actions[PLAYER_ACTION_RIGHT].state;

    if(p->item.mouse_aim)
    {
        int sector = player_angle_sector(angle_deg);
        if(sector == 0) p->sprite_index_direction = 2;
        else if(sector == 1) p->sprite_index_direction = 3;
        else if(sector == 2) p->sprite_index_direction = 4;
        else if(sector == 3) p->sprite_index_direction = 5;
        else if(sector == 4) p->sprite_index_direction = 6;
        else if(sector == 5) p->sprite_index_direction = 7;
        else if(sector == 6) p->sprite_index_direction = 0;
        else if(sector == 7) p->sprite_index_direction = 1;
    }
    else
    {
        if(up && left)         p->sprite_index_direction = 5;
        else if(up && right)   p->sprite_index_direction = 3;
        else if(down && left)  p->sprite_index_direction = 7;
        else if(down && right) p->sprite_index_direction = 1;
        else if(up)            p->sprite_index_direction = 4;
        else if(down)          p->sprite_index_direction = 0;
        else if(left)          p->sprite_index_direction = 6;
        else if(right)         p->sprite_index_direction = 2;
    }

    p->sprite_index = p->sprite_index_direction * 16;

    int anim_frame_offset = p->anim.frame_sequence[p->anim.curr_frame];
    assert(anim_frame_offset >= 0);

    p->sprite_index += anim_frame_offset;
    p->sprite_index = MIN(p->sprite_index, gfx_images[p->image].element_count);
}


void player_update(Player* p, double delta_t)
{

    player_add_detect_radius(p, delta_t * -0.8);

    window_get_mouse_world_coords(&player->mouse_x, &player->mouse_y);
    coords_to_map_grid(p->mouse_x, p->mouse_y, &p->mouse_r, &p->mouse_c);

    if(moving_zombie && player == p)
    {
        Zombie* z = zombie_get_by_id(zombie_info_id);
        if(z != NULL)
        {
            float prior_x = z->phys.pos.x;
            float prior_y = z->phys.pos.y;

            z->phys.pos.x = p->mouse_x;
            z->phys.pos.y = p->mouse_y;

            float dx = z->phys.pos.x - prior_x;
            float dy = z->phys.pos.y - prior_y;

            physics_apply_pos_offset(&z->phys, dx, dy);
            zombie_update_boxes(z);
        }
    }

    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        PlayerAction* pa = &p->actions[i];
        if(pa->state && !pa->prior_state)
        {
            pa->toggled_on = true;
        }
        else
        {
            pa->toggled_on = false;
        }

        if(!pa->state && pa->prior_state)
        {
            pa->toggled_off = true;
        }
        else
        {
            pa->toggled_off = false;
        }

        pa->prior_state = pa->state;
    }

    if(p->actions[PLAYER_ACTION_SPAWN_ZOMBIE].toggled_on)
    {
        ZombieSpawn spawn = {0};
        spawn.pos.x = p->mouse_x;
        spawn.pos.y = p->mouse_y;
        zombie_add(&spawn);
    }

    if(!p->busy)
    {
        if(p->actions[PLAYER_ACTION_EQUIP].toggled_on)
        {
            p->item_equipped = !p->item_equipped;
            if(p->item_equipped)
            {
                player_set_equipped_item(p, p->item_index);
            }
            else
            {
                player_set_equipped_item(p, 0);
            }
        }

        if(p->item_equipped)
        {
            if(p->actions[PLAYER_ACTION_CYCLE_EQUIP_UP].toggled_on)
            {
                player_set_equipped_item(p, p->item_index+1);
            }
            if(p->actions[PLAYER_ACTION_CYCLE_EQUIP_DOWN].toggled_on)
            {
                player_set_equipped_item(p, p->item_index-1);
            }
        }

        if(p->actions[PLAYER_ACTION_RELOAD].toggled_on)
        {
            if(p->item.item_type == ITEM_TYPE_GUN)
            {
                Gun* gun = (Gun*)p->item.props;
                if(gun->bullets < gun->bullets_max)
                {
                    p->state = PSTATE_RELOADING;
                    p->busy = true;
                    p->reload_timer = gun->reload_time+delta_t;
                }
            }
        }
    }

    if(p->state == PSTATE_RELOADING)
    {
        p->reload_timer -= delta_t*1000.0;
        if(p->reload_timer <= 0.0)
        {
            Gun* gun = (Gun*)p->item.props;
            p->state = PSTATE_NONE;
            p->busy = false;
            gun->bullets = gun->bullets_max;
        }
    }

    PlayerAction* pa = &p->actions[PLAYER_ACTION_PRIMARY_ACTION];
    player_update_mouse_click(p, p->actions[PLAYER_ACTION_PRIMARY_ACTION].state, pa->toggled_on||pa->toggled_off, &p->lmouse, delta_t);

    pa = &p->actions[PLAYER_ACTION_SECONDARY_ACTION];
    player_update_mouse_click(p, p->actions[PLAYER_ACTION_SECONDARY_ACTION].state, pa->toggled_on||pa->toggled_off, &p->rmouse, delta_t);


    if(p->actions[PLAYER_ACTION_DEBUG].toggled_on)
    {
        debug_enabled = !debug_enabled;
    }

    if(p->actions[PLAYER_ACTION_EDITOR].toggled_on)
    {
        editor_enabled = !editor_enabled;
        if(editor_enabled)
            window_enable_cursor();
        else
            window_disable_cursor();
    }

    if(p->actions[PLAYER_ACTION_MENU].toggled_on)
    {
        show_menu = !show_menu;
    }

    if(role != ROLE_SERVER)
    {
        if(p->actions[PLAYER_ACTION_PRIMARY_ACTION].toggled_on)
        {
            if(window_is_cursor_enabled() && !editor_enabled)
            {
                window_disable_cursor();
            }
        }
    }

    Vector2f accel = {0};
    bool moving_player = MOVING_PLAYER(p);

    bool up = p->actions[PLAYER_ACTION_UP].state;
    bool down = p->actions[PLAYER_ACTION_DOWN].state;
    bool left = p->actions[PLAYER_ACTION_LEFT].state;
    bool right = p->actions[PLAYER_ACTION_RIGHT].state;

    if(up)    { accel.y -= p->speed; }
    if(down)  { accel.y += p->speed; }
    if(left)  { accel.x -= p->speed; }
    if(right) { accel.x += p->speed; }

    if(p->actions[PLAYER_ACTION_RUN].toggled_on)
    {
        p->running = !p->running;
    }

    p->phys.max_linear_vel = p->max_base_speed;

    // Rect* wpos = player_get_equipped_item_pos(p);
    // p->angle = calc_angle_rad(wpos->x, wpos->y, p->mouse_x, p->mouse_y);
    p->angle = calc_angle_rad(p->phys.pos.x, p->phys.pos.y, p->mouse_x, p->mouse_y);

    float accel_factor = 1.0;

    if(p->running && moving_player && !p->busy)
    {
        accel_factor *= 3.0;
    }

    if((up || down) && (left || right))
    {
        // moving diagonally
        accel_factor *= SQRT2OVER2;
    }

    accel.x *= accel_factor;
    accel.y *= accel_factor;
    p->phys.max_linear_vel *= accel_factor;

    p->moving = !(FEQ(accel.x,0.0) && FEQ(accel.y,0.0));

    // finish attack
    if(p->state == PSTATE_ATTACKING && p->anim.curr_loop > 0)
    { 
        p->state = PSTATE_NONE;
        p->busy = false;
    }

    player_update_anim_state(p);
    player_update_anim_timing(p);
    player_update_image(p);
    gfx_anim_update(&p->anim, delta_t);
    player_update_sprite_index(p);
    // player_update_static_boxes(p);
    player_update_boxes(p);  // update width and heights of phys boxes

    player_update_pos_offset(p);

    physics_begin(&p->phys);
    physics_add_friction(&p->phys, 16.0);
    physics_add_force(&p->phys, accel.x, accel.y);
    physics_simulate(&p->phys, &map.rect, delta_t);

    player_update_static_boxes(p);
    // player_update_boxes(p);

    coords_to_map_grid(p->phys.actual_pos.x, p->phys.actual_pos.y, &p->grid_pos.x, &p->grid_pos.y);



    // player_check_block_collision(p, prior_pos, prior_collision_box);
    // player_weapon_melee_check_collision(p);

    //lighting_point_light_move(p->point_light, p->phys.actual_pos.x, p->phys.actual_pos.y);

    if(debug_enabled)
    {
        float px = p->phys.actual_pos.x;
        float py = p->phys.actual_pos.y;
        // gfx_add_line(px,py,p->mouse_x,p->mouse_y,0x00FF0000);

        if(p->state == PSTATE_ATTACKING && p->item.props != NULL)
        {
            Melee* melee = (Melee*)p->item.props;
            float d = melee->range;
            float a0 = p->angle - RAD(15);
            float a1 = p->angle + RAD(15);
            float x0 = px + d*cosf(a0);
            float y0 = py - d*sinf(a0);
            float x1 = px + d*cosf(a1);
            float y1 = py - d*sinf(a1);
            gfx_add_line(px,py,x0,y0,0x00FF0000);
            gfx_add_line(px,py,x1,y1,0x00FF0000);
        }
        // if(p->item.props != NULL)
        // {
        //     float x0 = p->mouse_x;
        //     float y0 = p->mouse_y;
        //     gfx_add_line(px,py,x0,y0,0x00FF0000);
        //     Rect* wpos = player_get_equipped_item_pos(p);
        //     gfx_add_line(wpos->x,wpos->y,x0,y0,0x000000FF);
        // }

    }
}

void player_handle_net_inputs(Player* p, double delta_t)
{
    // handle input
    memcpy(&p->input_prior, &p->input, sizeof(NetPlayerInput));

    p->input.delta_t = delta_t;

    p->input.keys = 0;
    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        if(p->actions[i].state)
        {
            p->input.keys |= (1<<i);
        }
    }
    p->input.mouse_x = p->mouse_x;
    p->input.mouse_y = p->mouse_y;
    
    if(p->input.keys != p->input_prior.keys || p->input.mouse_x != p->input_prior.mouse_x || p->input.mouse_y != p->input_prior.mouse_y)
    {
        net_client_add_player_input(&p->input);

        if(net_client_get_input_count() >= 3) // @HARDCODED 3
        {
            // add position, angle to predicted player state
            PlayerNetState* state = &p->predicted_states[p->predicted_state_index];

            // circular buffer
            if(p->predicted_state_index == MAX_CLIENT_PREDICTED_STATES -1)
            {
                // shift
                for(int i = 1; i <= MAX_CLIENT_PREDICTED_STATES -1; ++i)
                {
                    memcpy(&p->predicted_states[i-1],&p->predicted_states[i],sizeof(PlayerNetState));
                }
            }
            else if(p->predicted_state_index < MAX_CLIENT_PREDICTED_STATES -1)
            {
                p->predicted_state_index++;
            }

            state->associated_packet_id = net_client_get_latest_local_packet_id();
            state->pos.x = p->phys.pos.x;
            state->pos.y = p->phys.pos.y;
            state->angle = p->angle;
        }
    }
}

//TODO: fix this function
void player_update_other(Player* p, double delta_t)
{

    if(!p->active) return;
    if(p == player) return;

    player_add_detect_radius(p, delta_t * -0.8);



    p->lerp_t += delta_t;

    float tick_time = 1.0/TICK_RATE;
    float t = (p->lerp_t / tick_time);

    Vector2f lp = lerp2f(&p->server_state_prior.pos,&p->server_state_target.pos,t);
    p->phys.pos.x = lp.x;
    p->phys.pos.y = lp.y;

    p->angle = lerp(p->server_state_prior.angle,p->server_state_target.angle,t);

    // player_update_anim_state(p);
    // player_update_anim_timing(p);
    // player_update_image(p);
    // gfx_anim_update(&p->anim, delta_t);
    // player_update_sprite_index(p);
    // player_update_boxes(p);


}

void player_draw(Player* p, bool add_to_existing_batch)
{
    if(p == NULL) return;

    if(!is_in_camera_view(&p->phys.actual_pos))
    {
        return;
    }

    GFXImage* img = &gfx_images[p->image];
    Rect* vr = &img->visible_rects[p->sprite_index];

    // player
    if(add_to_existing_batch)
    {
        gfx_sprite_batch_add(p->image, p->sprite_index, p->phys.pos.x, p->phys.pos.y, COLOR_TINT_NONE,p->scale,0.0,1.0,true,false,false);
    }
    else
    {
        gfx_draw_image(p->image, p->sprite_index, p->phys.pos.x, p->phys.pos.y, COLOR_TINT_NONE,p->scale,0.0,1.0,true,true);
    }

    if(p->item.drawable && p->item.props != NULL)
    {
        if(p->item.item_type == ITEM_TYPE_GUN || p->item.item_type == ITEM_TYPE_MELEE)
        {

            int wimage = player_get_equipped_item_img(p);
            Rect* wpos = player_get_equipped_item_pos(p);

            GFXImage* wimg = &gfx_images[wimage];
            Rect* wvr = &wimg->visible_rects[p->sprite_index];

            if(!IS_RECT_EMPTY(wvr))
            {
                float wimg_center_x = IMG_ELEMENT_W/2.0;
                float wimg_center_y = IMG_ELEMENT_H/2.0;

                float gx = p->phys.pos.x + (wvr->x-wimg_center_x)*p->scale;
                float gy = p->phys.pos.y + (wvr->y-wimg_center_y)*p->scale;

                wpos->x = gx;
                wpos->y = gy;

                // weapon
                if(add_to_existing_batch)
                {
                    gfx_sprite_batch_add(wimage, p->sprite_index, p->phys.pos.x, p->phys.pos.y, COLOR_TINT_NONE, p->scale,0,1.0,true,false,false);
                }
                else
                {
                    gfx_draw_image(wimage, p->sprite_index, p->phys.pos.x, p->phys.pos.y, COLOR_TINT_NONE, p->scale,0,1.0,true,true);
                }
            }

        }

        if(p->item.item_type == ITEM_TYPE_BLOCK)
        {
            BlockProp* bp = (BlockProp*)p->item.props;
            Rect r = {0};
            map_grid_to_rect(p->mouse_r, p->mouse_c, &r);

            uint32_t color = bp->color;
            bool in_range = is_grid_within_radius(p->mouse_r, p->mouse_c, p->grid_pos.x, p->grid_pos.y, PLAYER_BLOCK_PLACEMENT_RADIUS);
            if(!in_range)
                color = COLOR_BLACK;

            gfx_draw_rect(&r, color, 0.0, 1.0, 0.15, true, true);
        }
    }

    if(debug_enabled)
    {
        gfx_draw_rect(&p->phys.actual_pos, COLOR_POS, 0.0, 1.0,1.0, false, true);
        gfx_draw_rect(&p->phys.collision, COLOR_COLLISON, 0.0, 1.0,1.0, false, true);
        gfx_draw_rect(&p->phys.hit, COLOR_HIT, 0.0, 1.0,1.0, false, true);
        gfx_draw_rect(&p->max_size, COLOR_MAXSIZE, 0.0, p->scale,1.0, false, true);

        //dots
        Rect r = {0};

        // phys.pos
        r.x = p->phys.pos.x;
        r.y = p->phys.pos.y;
        r.w = 2;
        r.h = 2;
        gfx_draw_rect(&r, COLOR_PURPLE, 0.0, 1.0,1.0, true, true);

        // pos
        r.x = p->phys.actual_pos.x;
        r.y = p->phys.actual_pos.y;
        gfx_draw_rect(&r, COLOR_ORANGE, 0.0, 1.0,1.0, true, true);

        // detect
        gfx_draw_circle(p->phys.actual_pos.x,p->phys.actual_pos.y,p->detect_radius*MAP_GRID_PXL_SIZE,COLOR_PINK,1.0,false,true);
    }

    // name
    const float name_size = 0.11;
    Vector2f size = gfx_string_get_size(name_size, p->name);
    float x = p->phys.pos.x - size.x/2.0;
    float y = p->phys.pos.y + p->scale*p->max_size.h*0.5 + 2.0;
    gfx_draw_string(x, y, player_colors[p->index], name_size, 0.0, 0.8, true, true, p->name);
}

void player_draw_offscreen()
{
    // //@TEMP
    // static bool activate_player = false;
    // if(!activate_player)
    // {
    //     Player* p = &players[2];
    //     p->phys.pos.x = 1000;
    //     p->phys.pos.y = 3000;
    //     player_update_anim_state(p);
    //     player_update_image(p);
    //     player_update_sprite_index(p);
    //     // player_update_boxes(p);
    //     p->active = true;
    //     player_count++;
    //     activate_player = true;
    // }

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        Player* p = &players[i];
        if(p->active)
        {
            // Rect* pos = &p->phys.actual_pos;
            Rect* pos = &p->phys.pos;
            if(!is_in_camera_view(pos))
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

void player_draw_all()
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        Player* p = &players[i];
        if(p->active)
        {
            player_draw(p, false);
        }
    }
}


void player_draw_crosshair(Player* p)
{
    // crosshair
    gfx_draw_image_ignore_light(crosshair_image, 0, p->mouse_x,p->mouse_y, 0x00CCCCCC, 1.0,0.0,0.80, false,true);
}

const char* player_item_type_str(PlayerItemType item_type)
{
    switch(item_type)
    {
        case ITEM_TYPE_NONE: return "none";
        case ITEM_TYPE_MELEE: return "melee";
        case ITEM_TYPE_GUN: return "gun";
        case ITEM_TYPE_BLOCK: return "block";
        case ITEM_TYPE_OBJECT: return "object";
        default: return "UNKNOWN";
    }
}


void block_draw(block_t* b, bool add_to_existing_batch)
{
    if(b == NULL) return;

    if(add_to_existing_batch)
    {
        gfx_sprite_batch_add(block_props[b->type].image, block_props[b->type].sprite_index, b->phys.pos.x, b->phys.pos.y-9, block_props[b->type].color,1.0,0.0,1.0,true,false,false);
    }
    else
    {
        gfx_draw_image(block_props[b->type].image, block_props[b->type].sprite_index, b->phys.pos.x, b->phys.pos.y-9, block_props[b->type].color,1.0,0.0,1.0,true,true);
    }

    if(debug_enabled)
    {
        gfx_draw_rect(&b->phys.collision, COLOR_COLLISON, 0.0, 1.0, 1.0, false, true);
    }
}


void weapons_init()
{
    int idx = 0;

    idx = GUN_PISTOL1;
    guns[idx].index = idx;
    guns[idx].name = "pistol1";
    guns[idx].type = GUN_TYPE_HANDGUN;
    guns[idx].anim_state = ANIM_NONE;    // no change in state
    guns[idx].power = 1.0;
    guns[idx].recoil_spread = 2.0;
    guns[idx].fire_range = 500.0;
    guns[idx].fire_speed = 4000.0;
    guns[idx].fire_period = 500.0; // milliseconds
    guns[idx].fire_spread = 0.0;
    guns[idx].fire_count = 1;
    guns[idx].bullets = 7;
    guns[idx].bullets_max = 7;
    guns[idx].reload_time = 1000.0;
    guns[idx].projectile_type = PROJECTILE_TYPE_BULLET;

    idx = GUN_MACHINEGUN1;
    guns[idx].index = idx;
    guns[idx].name = "pistol1"; //TODO
    guns[idx].type = GUN_TYPE_HANDGUN; //TODO
    guns[idx].anim_state = ANIM_NONE;    // no change in state
    guns[idx].power = 1.0;
    guns[idx].recoil_spread = 4.0;
    guns[idx].fire_range = 500.0;
    guns[idx].fire_speed = 4000.0;
    guns[idx].fire_period = 100.0; // milliseconds
    guns[idx].fire_spread = 0.0;
    guns[idx].fire_count = 1;
    guns[idx].bullets = 9999;
    guns[idx].bullets_max = 9999;
    guns[idx].reload_time = 1000.0;
    guns[idx].projectile_type = PROJECTILE_TYPE_BULLET;

    idx = GUN_SHOTGUN1;
    guns[idx].index = idx;
    guns[idx].name = "pistol1"; //TODO
    guns[idx].type = GUN_TYPE_HANDGUN; //TODO
    guns[idx].anim_state = ANIM_NONE;    // no change in state
    guns[idx].power = 1.0;
    guns[idx].recoil_spread = 0.0;
    guns[idx].fire_range = 300.0;
    guns[idx].fire_speed = 4000.0;
    guns[idx].fire_period = 100.0; // milliseconds
    guns[idx].fire_spread = 30.0;
    guns[idx].fire_count = 5;
    guns[idx].bullets = 9999;
    guns[idx].bullets_max = 9999;
    guns[idx].reload_time = 1000.0;
    guns[idx].projectile_type = PROJECTILE_TYPE_BULLET;



    idx = MELEE_KNIFE1;
    melees[idx].index = idx;
    melees[idx].name = "pistol1"; //TEMP
    melees[idx].type = MELEE_TYPE0;
    melees[idx].anim_state = ANIM_ATTACK1;
    melees[idx].range = 40.0;
    melees[idx].power = 0.5;
    melees[idx].period = 100.0;

    weapons_init_images();

}

// must call this after players_init()?
void weapons_init_images()
{

    // guns
    for(int pm = 0; pm < PLAYER_MODELS_MAX; ++pm)
    {
        for(int ps = 0; ps < ANIM_MAX; ++ps)
        {
            for(int w = 0; w < GUN_MAX; ++w)
            {
                gun_image_sets[pm][ps][w] = -1;

                char fname[100] = {0};
                sprintf(fname, "img/characters/%s-%s_%s_%s.png", player_models[pm].name, player_anim_state_str(ps), gun_type_str(guns[w].type), guns[w].name);
                gun_image_sets[pm][ps][w] = gfx_load_image(fname, false, false, IMG_ELEMENT_W, IMG_ELEMENT_H, NULL);
            }
        }
    }

    // melee
    for(int pm = 0; pm < PLAYER_MODELS_MAX; ++pm)
    {
        for(int ps = 0; ps < ANIM_MAX; ++ps)
        {
            for(int w = 0; w < MELEE_MAX; ++w)
            {
                melee_image_sets[pm][ps][w] = -1;
                char fname[100] = {0};
                sprintf(fname, "img/characters/%s-%s_%s_%s.png", player_models[pm].name, player_anim_state_str(ps), melee_type_str(melees[w].type), melees[w].name);
                melee_image_sets[pm][ps][w] = gfx_load_image(fname, false, false, IMG_ELEMENT_W, IMG_ELEMENT_H, NULL);
            }
        }
    }
}


int gun_get_image_index(PlayerModelIndex model_index, PlayerAnimState anim_state, GunType gtype)
{
    return gun_image_sets[model_index][anim_state][gtype];
}

int melee_get_image_index(PlayerModelIndex model_index, PlayerAnimState anim_state, MeleeType mtype)
{
    return melee_image_sets[model_index][anim_state][mtype];
}


const char* gun_type_str(GunType gtype)
{
    switch(gtype)
    {
        case GUN_TYPE_HANDGUN: return "handgun";
        case GUN_TYPE_RIFLE: return "rifle";
        case GUN_TYPE_BOW: return "bow";
        default: return "";
    }
}

const char* melee_type_str(MeleeType mtype)
{
    switch(mtype)
    {
        case MELEE_TYPE0: return "handgun";//TEMP
        case MELEE_TYPE1: return "type1";
        default: return "";
    }
}


void gun_fire(Player* p, Gun* gun, bool held)
{

    if(gun->bullets <= 0) return;

    if(gun->fire_count > 1)
    {
        for(int i = 0; i < gun->fire_count; ++i)
        {
            float angle_offset = RAND_FLOAT(-gun->fire_spread/2.0, gun->fire_spread/2.0);
            projectile_add(p, gun, angle_offset);
        }
    }
    else
    {
        float angle_offset = 0.0;
        if(held && !FEQ(gun->recoil_spread,0.0))
        {
            angle_offset = RAND_FLOAT(-gun->recoil_spread/2.0, gun->recoil_spread/2.0);
            // recoil_camera_offset.x = 5.0*cosf(RAD(angle_offset));
            // recoil_camera_offset.y = 5.0*sinf(RAD(angle_offset));
            // float cam_pos_x = player->phys.pos.x + aim_camera_offset.x + recoil_camera_offset.x;
            // float cam_pos_y = player->phys.pos.y + aim_camera_offset.y + recoil_camera_offset.y;
            // camera_move(cam_pos_x, cam_pos_y, 0.00, true, &map.rect);
        }
        projectile_add(p, gun, angle_offset);
    }

    player_add_detect_radius(p, 10.0);

    particles_spawn_effect(gun->pos.x, gun->pos.y-5, &particle_effects[EFFECT_GUN_SMOKE1], 0.5, true, false); // smoke
    particles_spawn_effect(gun->pos.x, gun->pos.y-5, &particle_effects[EFFECT_SPARKS1], 0.5, true, false); // sparks
    particles_spawn_effect(gun->pos.x, gun->pos.y, &particle_effects[EFFECT_BULLET_CASING], 1.4, true, false); // bullet casing

    gun->bullets--;
}

void player_weapon_melee_check_collision(Player* p)
{
    if(p->item.item_type != ITEM_TYPE_MELEE)
        return;

    if(p->melee_hit_count > 0)
        return;

    if(zlist->count == 0)
        return;

    if(p->item.props == NULL)
        return;


    if(p->state == PSTATE_ATTACKING)
    {
        float px = p->phys.pos.x;
        float py = p->phys.pos.y;

        Melee* melee = (Melee*)p->item.props;

        for(int j = zlist->count - 1; j >= 0; --j)
        {
            if(zombies[j].dead)
                continue;

            bool collision = rectangles_colliding(&p->phys.actual_pos, &zombies[j].phys.hit);
            // collision = false;

            if(!collision)
            {
                float zx = zombies[j].phys.actual_pos.x;
                float zy = zombies[j].phys.actual_pos.y;
                float angle = calc_angle_rad(px, py, zx, zy);

                bool within_angle_range = ABS(angle - p->angle) <= RAD(30); //hardcoded

                if(within_angle_range)
                {
                    float zr = MAX(zombies[j].phys.hit.w, zombies[j].phys.hit.h)/2.0;
                    float d = dist(px, py, zx, zy);
                    if(d <= (zr + melee->range))
                        collision = true;
                }
            }

            if(collision)
            {
                float damage = melee->power;
                // printf("zombie hurt\n");
                zombie_hurt(j,damage);
                p->melee_hit_count++;
                return;
            }

        }
    }
}

void player_hurt(Player* p, float damage)
{
    if(!p)
        return;

    p->hp -= damage;
    particles_spawn_effect(p->phys.pos.x, p->phys.pos.y, &particle_effects[EFFECT_BLOOD1], 0.6, true, false);

    if(p->hp <= 0.0)
    {
        p->hp = 0.0;
        //player_die(p);
    }
}
