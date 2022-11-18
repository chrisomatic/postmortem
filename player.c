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
// int player_image_sets[PLAYER_MODELS_MAX][PLAYER_TEXTURES_MAX][ANIM_MAX][WEAPON_TYPE_MAX];

int player_image_sets_none[PLAYER_MODELS_MAX][PLAYER_TEXTURES_MAX][ANIM_MAX];
int player_image_sets_guns[PLAYER_MODELS_MAX][PLAYER_TEXTURES_MAX][ANIM_MAX][GUN_TYPE_MAX];
int player_image_sets_melees[PLAYER_MODELS_MAX][PLAYER_TEXTURES_MAX][ANIM_MAX][MELEE_TYPE_MAX];

PlayerModel player_models[PLAYER_MODELS_MAX];

Gun guns[GUN_MAX] = {0};
Melee melees[MELEE_MAX] = {0};


// ------------------------------------------------------------

#define IMG_ELEMENT_W 128
#define IMG_ELEMENT_H 128


static int gun_image_sets[PLAYER_MODELS_MAX][ANIM_MAX][GUN_MAX];
static int melee_image_sets[PLAYER_MODELS_MAX][ANIM_MAX][MELEE_MAX];
static int crosshair_image;


//TEMP: blocks
block_t blocks[MAX_BLOCKS] = {0};
glist* blist = NULL;
BlockProp block_props[BLOCK_MAX] = {0};

// ------------------------------------------------------------


void mouse_gun_cb(void* player, MouseTrigger trigger);
void mouse_melee_cb(void* player, MouseTrigger trigger);
void mouse_block_cb(void* player, MouseTrigger trigger);
void mouse_block_remove_cb(void* player, MouseTrigger trigger);


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
                sprintf(fname, "img/characters/%s_%d-%s.png", player_models[pm].name, t, player_state_str(ps));
                if(access(fname, F_OK) == 0)
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
                    sprintf(fname, "img/characters/%s_%d-%s_%s.png", player_models[pm].name, t, player_state_str(ps), gun_type_str(wt));
                    if(access(fname, F_OK) == 0)
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
                    sprintf(fname, "img/characters/%s_%d-%s_%s.png", player_models[pm].name, t, player_state_str(ps), melee_type_str(wt));
                    if(access(fname, F_OK) == 0)
                    {
                        player_image_sets_melees[pm][t][ps][wt] = gfx_load_image(fname, false, true, IMG_ELEMENT_W, IMG_ELEMENT_H, NULL);
                        // printf("%s -> %d\n", fname, player_image_sets[pm][t][ps][wt]);
                    }
                }
            }
        }
    }


    crosshair_image = gfx_load_image("img/crosshair2.png", false, false, 0, 0, NULL);
}

void player_init_controls(Player* p)
{
    window_controls_clear_keys();

    // map keys
    window_controls_add_key(&p->keys, GLFW_KEY_W, PLAYER_ACTION_UP);
    window_controls_add_key(&p->keys, GLFW_KEY_S, PLAYER_ACTION_DOWN);
    window_controls_add_key(&p->keys, GLFW_KEY_A, PLAYER_ACTION_LEFT);
    window_controls_add_key(&p->keys, GLFW_KEY_D, PLAYER_ACTION_RIGHT);
    window_controls_add_key(&p->keys, GLFW_KEY_R, PLAYER_ACTION_RELOAD);
    window_controls_add_key(&p->keys, GLFW_KEY_LEFT_SHIFT, PLAYER_ACTION_RUN);
    window_controls_add_key(&p->keys, GLFW_KEY_SPACE, PLAYER_ACTION_JUMP);
    window_controls_add_key(&p->keys, GLFW_KEY_E, PLAYER_ACTION_INTERACT);
    window_controls_add_key(&p->keys, GLFW_KEY_TAB, PLAYER_ACTION_TOGGLE_EQUIP_WEAPON);
    window_controls_add_key(&p->keys, GLFW_KEY_F2, PLAYER_ACTION_TOGGLE_DEBUG);
    window_controls_add_key(&p->keys, GLFW_KEY_F3, PLAYER_ACTION_TOGGLE_EDITOR);
    window_controls_add_key(&p->keys, GLFW_KEY_G, PLAYER_ACTION_TOGGLE_GUN);
    window_controls_add_key(&p->keys, GLFW_KEY_B, PLAYER_ACTION_TOGGLE_BLOCK);

    window_controls_add_mouse_button(&p->keys, GLFW_MOUSE_BUTTON_LEFT, PLAYER_ACTION_PRIMARY_ACTION);
    window_controls_add_mouse_button(&p->keys, GLFW_MOUSE_BUTTON_RIGHT, PLAYER_ACTION_SECONDARY_ACTION);
}

static void player_init(int index)
{
    Player* p = &players[index];

    p->index = index;
    if(STR_EMPTY(p->name))
    {
        sprintf(p->name, "Player %d", p->index);
    }

    p->model_index = HUMAN1;
    p->model_texture = 0;
    p->anim_state = ANIM_IDLE;

    p->moving = false;
    p->running = false;

    p->state = PSTATE_NONE;
    // p->block_ready = false;
    // p->weapon_ready = false;
    // p->attacking = false;
    // p->reloading = false;

    player_set_equipped_item(p, 0);


    p->sprite_index = 0;
    p->sprite_index_direction = 0;
    p->angle = 0.0;

    p->phys.pos.x = 400.0;
    p->phys.pos.y = 1000.0;
    p->phys.vel.x = 0.0;
    p->phys.vel.y = 0.0;
    p->speed = 32.0;
    p->max_base_speed = 128.0;
    p->phys.max_linear_vel = p->max_base_speed;


    // int standard_img = player_get_image_index(p->model_index, p->model_texture, ANIM_IDLE, WEAPON_TYPE_NONE);
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


    // float maxw=0.0, maxh=0.0;
    // for(int ps = 0; ps < ANIM_MAX; ++ps)
    // {
    //     for(int wt = 0; wt < WEAPON_TYPE_MAX; ++wt)
    //     {
    //         int img = player_get_image_index(p->model_index, p->model_texture, ps, wt);
    //         if(img == -1) continue;
    //         for(int i = 0; i < gfx_images[img].element_count; ++i)
    //         {
    //             Rect* vr = &gfx_images[img].visible_rects[i];
    //             if(IS_RECT_EMPTY(vr)) continue;
    //             maxw = MAX(maxw, vr->w);
    //             maxh = MAX(maxh, vr->h);
    //         }
    //     }
    // }
    // p->max_size.w = maxw*p->scale;
    // p->max_size.h = maxh*p->scale;

    //TODO
    p->max_size.w = p->standard_size.w*1.4;
    p->max_size.h = p->standard_size.h*1.4;

    // animation
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

    p->predicted_state_index = 0;

    // light for player
    p->point_light = -1;
    if(p == player)
        p->point_light = lighting_point_light_add(p->phys.pos.x,p->phys.pos.y,1.0,1.0,1.0,1.0);

    player_update_anim_state(p);
    player_update_image(p);

    player_update_sprite_index(p);
}

void players_init()
{

    //TEMP: blocks
    // blocks_init()
    int idx = BLOCK_0;
    block_props[idx].type = idx;
    block_props[idx].hp = 100.0;
    block_props[idx].color = COLOR_RED;
    idx = BLOCK_1;
    block_props[idx].type = idx;
    block_props[idx].hp = 100.0;
    block_props[idx].color = COLOR_BLUE;



    player_init_models();
    player_init_images();

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

    //TEMP: blocks
    blist = list_create((void*)blocks, MAX_BLOCKS, sizeof(blocks[0]));
    if(blist == NULL)
    {
        LOGE("block list failed to create");
    }

}

const char* player_state_str(PlayerAnimState anim_state)
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

// int player_get_image_index(PlayerModelIndex model_index, int texture, PlayerAnimState anim_state, WeaponType wtype)
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


    return -1;
    // // player_image_sets[model][anim_state][wtype];
    // return player_image_sets[model_index][texture][anim_state][wtype];
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

// void player_set_weapon(Player* p, WeaponIndex weapon_index)
// {
//     if(p->weapon != NULL)
//     {
//         if(p->weapon->index == weapon_index)
//             return;
//     }

//     Weapon* w = &weapons[weapon_index];
//     p->weapon = w;

//     player_set_mouse(&p->lmouse, true, true, false, w->gun.fire_period, mouse_gun_cb);

//     // p->lmouse.cooldown = 0.0;
//     // p->lmouse.trigger_on_held = true;
//     // p->lmouse.trigger_on_press = true;
//     // p->lmouse.trigger_on_release = false;
//     // p->rmouse.cooldown = 0.0;
//     // p->rmouse.trigger_on_held = true;
//     // p->rmouse.trigger_on_press = true;
//     // p->rmouse.trigger_on_release = false;


//     // // left mouse
//     // if(w->primary_attack == ATTACK_SHOOT)
//     // {
//     //     p->lmouse.period = w->gun.fire_period;
//     // }
//     // else if(w->primary_attack == ATTACK_MELEE)
//     // {
//     //     p->lmouse.period = w->melee.period;
//     // }
//     // else if(w->primary_attack == ATTACK_POWER_MELEE)
//     // {
//     //     p->lmouse.period = w->melee.period;
//     // }

//     // // right mouse
//     // if(w->secondary_attack == ATTACK_SHOOT)
//     // {
//     //     p->rmouse.period = w->gun.fire_period;

//     // }
//     // else if(w->secondary_attack == ATTACK_MELEE)
//     // {
//     //     p->rmouse.period = w->melee.period;
//     // }
//     // else if(w->secondary_attack == ATTACK_POWER_MELEE)
//     // {
//     //     p->rmouse.period = w->melee.period;
//     // }


// }

void player_equip_gun(Player* p, GunIndex index)
{
    printf("equipping gun %d\n", index);

    Gun* gun = &guns[index];
    player_equip_item(p, ITEM_TYPE_GUN, (void*)gun, true, true);

    player_set_mouse(&p->lmouse, true, true, false, gun->fire_period, mouse_gun_cb);
}

void player_equip_melee(Player* p, MeleeIndex index)
{
    printf("equipping melee %d\n", index);

    Melee* melee = &melees[index];
    player_equip_item(p, ITEM_TYPE_MELEE, (void*)melee, true, true);

    player_set_mouse(&p->lmouse, true, true, false, melee->period, mouse_melee_cb);
}

void player_equip_block(Player* p, BlockType index)
{
    printf("equipping block %d\n", index);

    BlockProp* block = &block_props[index];
    player_equip_item(p, ITEM_TYPE_BLOCK, (void*)block, true, true);

    player_set_mouse(&p->lmouse, true, true, false, 20.0, mouse_block_cb);
    player_set_mouse(&p->rmouse, true, true, false, 20.0, mouse_block_remove_cb);
}

void player_set_equipped_item(Player* p, int idx)
{

    p->item_index = idx;

    if(idx == 1)
    {
        player_equip_gun(p, GUN_PISTOL1);
    }
    else if(idx == 2)
    {
        player_equip_gun(p, GUN_MACHINEGUN1);
    }
    else if(idx == 3)
    {
        player_equip_gun(p, GUN_SHOTGUN1);
    }
    else if(idx == 4)
    {
        player_equip_melee(p, MELEE_KNIFE1);
    }
    else if(idx == 5)
    {
        player_equip_block(p, BLOCK_0);
        // player_equip_item(p, ITEM_TYPE_OBJECT, NULL, false, true);
    }
    else if(idx == 6)
    {
        player_equip_block(p, BLOCK_1);
        // player_equip_item(p, ITEM_TYPE_OBJECT, NULL, false, true);
    }
    else
    {
        player_equip_item(p, ITEM_TYPE_NONE, NULL, false, false);
        p->item_index = 0;
    }


}


void player_equip_item(Player* p, PlayerItemType itype, void* props, bool drawable, bool mouse_aim)
{
    p->item.item_type = itype;
    p->item.props = props;
    p->item.drawable = drawable;
    p->item.mouse_aim = mouse_aim;
}


// void* to avoid circular reference in player struct
void mouse_gun_cb(void* player, MouseTrigger trigger)
{
    Player* p = (Player*)player;

    if(p->busy)
        return;

    if(p->item.item_type != ITEM_TYPE_GUN)
        return;

    if(p->item.props == NULL)
        return;

    Gun* gun = (Gun*)p->item.props;
    gun_fire(p->mouse_x, p->mouse_y, gun, trigger == MOUSE_TRIGGER_HOLD);
}

void mouse_melee_cb(void* player, MouseTrigger trigger)
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
}

void mouse_block_cb(void* player, MouseTrigger trigger)
{
    Player* p = (Player*)player;

    if(p->busy)
        return;

    if(p->item.props == NULL)
        return;

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
        // Vector2f block = {0};
        // block.x = p->mouse_r;
        // block.y = p->mouse_c;

        BlockProp* bp = (BlockProp*)p->item.props;

        block_t b = {0};
        b.row = p->mouse_r;
        b.col = p->mouse_c;
        b.type = bp->type;
        b.hp = bp->hp;
        list_add(blist, (void*)&b);
    }
}

void mouse_block_remove_cb(void* player, MouseTrigger trigger)
{
    Player* p = (Player*)player;

    if(p->busy)
        return;

    // if(p->item.props == NULL)
    //     return;

    for(int i = 0; i < blist->count; ++i)
    {
        if(p->mouse_r == blocks[i].row && p->mouse_c == blocks[i].col)
        {
            list_remove(blist, i);
            break;
        }
    }
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

    bool ready = mouse->cooldown <= 0.0;

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

    // if(p->weapon_ready && p->weapon->index != WEAPON_NONE)
    // {
    //     p->image = player_get_image_index(p->model_index, p->model_texture, p->anim_state, p->weapon->type);
    // }
    // else
    // {
    //     p->image = player_get_image_index(p->model_index, p->model_texture, p->anim_state, WEAPON_TYPE_NONE);
    // }
    return;
}

void player_update_boxes(Player* p)
{
    GFXImage* img = &gfx_images[p->image];
    Rect* vr = &img->visible_rects[p->sprite_index];

    // if(p->item.item_type == ITEM_TYPE_OBJECT)
    // {
    //     printf("   update boxes 1\n");
    //     printf("p->sprite_index: %d, image: %d\n", p->sprite_index, p->image);
    //     print_rect(&p->phys.pos);
    // }
    get_actual_pos(p->phys.pos.x, p->phys.pos.y, p->scale, img->element_width, img->element_height, vr, &p->pos);
    // if(p->item.item_type == ITEM_TYPE_OBJECT)
    // {
    //     printf("   update boxes 2\n");
    //     print_rect(&p->phys.pos);
    // }
    limit_pos(&map.rect, &p->pos, &p->phys.pos);
    // if(p->item.item_type == ITEM_TYPE_OBJECT)
    // {
    //     printf("   update boxes 3\n");
    //     print_rect(&p->phys.pos);
    // }

    float px = p->pos.x;
    float py = p->pos.y;

    p->standard_size.x = px;
    p->standard_size.y = py;

    p->max_size.x = px;
    p->max_size.y = py;

}


void player_update_sprite_index(Player* p)
{
    p->angle = calc_angle_rad(p->phys.pos.x, p->phys.pos.y, p->mouse_x, p->mouse_y);

    float angle_deg = DEG(p->angle);

    // if(p->weapon_ready || p->block_ready)
    if(p->item.mouse_aim)
    {
        int sector = angle_sector(angle_deg, 16);

        if(sector == 15 || sector == 0)  // p->actions.right
            p->sprite_index_direction = 2;
        else if(sector == 1 || sector == 2)  // p->actions.up-p->actions.right
            p->sprite_index_direction = 3;
        else if(sector == 3 || sector == 4)  // p->actions.up
            p->sprite_index_direction = 4;
        else if(sector == 5 || sector == 6)  // p->actions.up-p->actions.left
            p->sprite_index_direction = 5;
        else if(sector == 7 || sector == 8)  // p->actions.left
            p->sprite_index_direction = 6;
        else if(sector == 9 || sector == 10)  // p->actions.down-p->actions.left
            p->sprite_index_direction = 7;
        else if(sector == 11 || sector == 12)  // p->actions.down
            p->sprite_index_direction = 0;
        else if(sector == 13 || sector == 14)  // p->actions.down-p->actions.right
            p->sprite_index_direction = 1;
    }
    else
    {
        if(p->actions.up && p->actions.left)
            p->sprite_index_direction = 5;
        else if(p->actions.up && p->actions.right)
            p->sprite_index_direction = 3;
        else if(p->actions.down && p->actions.left)
            p->sprite_index_direction = 7;
        else if(p->actions.down && p->actions.right)
            p->sprite_index_direction = 1;
        else if(p->actions.up)
            p->sprite_index_direction = 4;
        else if(p->actions.down)
            p->sprite_index_direction = 0;
        else if(p->actions.left)
            p->sprite_index_direction = 6;
        else if(p->actions.right)
            p->sprite_index_direction = 2;
    }

    p->sprite_index = p->sprite_index_direction * 16;

    int anim_frame_offset = p->anim.frame_sequence[p->anim.curr_frame];
    assert(anim_frame_offset >= 0);

    p->sprite_index += anim_frame_offset;
    p->sprite_index = MIN(p->sprite_index, gfx_images[p->image].element_count);
}


void player_update(Player* p, double delta_t)
{
    window_get_mouse_world_coords(&player->mouse_x, &player->mouse_y);
    coords_to_map_grid(p->mouse_x, p->mouse_y, &p->mouse_r, &p->mouse_c);

    p->actions.up               = IS_BIT_SET(p->keys,PLAYER_ACTION_UP);
    p->actions.down             = IS_BIT_SET(p->keys,PLAYER_ACTION_DOWN);
    p->actions.left             = IS_BIT_SET(p->keys,PLAYER_ACTION_LEFT);
    p->actions.right            = IS_BIT_SET(p->keys,PLAYER_ACTION_RIGHT);
    p->actions.run              = IS_BIT_SET(p->keys,PLAYER_ACTION_RUN);
    p->actions.jump             = IS_BIT_SET(p->keys,PLAYER_ACTION_JUMP);
    p->actions.interact         = IS_BIT_SET(p->keys,PLAYER_ACTION_INTERACT);
    p->actions.primary_action   = IS_BIT_SET(p->keys,PLAYER_ACTION_PRIMARY_ACTION);
    p->actions.secondary_action = IS_BIT_SET(p->keys,PLAYER_ACTION_SECONDARY_ACTION);
    p->actions.toggle_equip_weapon= IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_EQUIP_WEAPON);
    p->actions.toggle_debug     = IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_DEBUG);
    p->actions.toggle_editor    = IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_EDITOR);
    p->actions.toggle_gun       = IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_GUN);
    p->actions.toggle_block     = IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_BLOCK);
    p->actions.reload           = IS_BIT_SET(p->keys,PLAYER_ACTION_RELOAD);

    bool run_toggled = p->actions.run && !p->actions_prior.run;
    bool primary_action_toggled = p->actions.primary_action && !p->actions_prior.primary_action;
    bool secondary_action_toggled = p->actions.secondary_action && !p->actions_prior.secondary_action;
    bool equip_weapon_toggled = p->actions.toggle_equip_weapon && !p->actions_prior.toggle_equip_weapon;
    bool debug_toggled = p->actions.toggle_debug && !p->actions_prior.toggle_debug;
    bool editor_toggled = p->actions.toggle_editor && !p->actions_prior.toggle_editor;
    bool gun_toggled = p->actions.toggle_gun && !p->actions_prior.toggle_gun;
    bool block_toggled = p->actions.toggle_block && !p->actions_prior.toggle_block;

    memcpy(&p->actions_prior, &p->actions, sizeof(PlayerActions));

    // static int prior_index = 0;
    // if(equip_weapon_toggled)
    // {
    //     if(p->actions.toggle_equip_weapon)
    //     {
    //         player_set_equipped_item(p, prior_index);
    //     }
    //     else
    //     {
    //         prior_index = p->item_index;
    //         player_set_equipped_item(p, 0);
    //     }
    // }

    if(!p->busy)
    {
        if(equip_weapon_toggled)
        {
            player_set_equipped_item(p, p->item_index+1);
        }

        if(p->actions.reload)
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


    //TODO
    // bool gun_equipped = p->weapon_ready && p->weapon->type < WEAPON_TYPE_MELEE;
    // if(p->actions.reload && gun_equipped && p->weapon->gun.bullets < p->weapon->gun.bullets_max)
    // {
    //     if(!p->reloading && !p->attacking)
    //     {
    //         p->reload_timer = p->weapon->gun.reload_time+delta_t;
    //         p->reloading = true;
    //     }
    // }
    // if(p->reloading)
    // {
    //     p->reload_timer -= delta_t*1000.0;
    //     if(p->reload_timer <= 0.0)
    //     {
    //         p->reloading = false;
    //         p->weapon->gun.bullets = p->weapon->gun.bullets_max;
    //     }
    // }

    // p->weapon_ready = true;

    player_update_mouse_click(p, p->actions.primary_action, primary_action_toggled, &p->lmouse, delta_t);
    player_update_mouse_click(p, p->actions.secondary_action, secondary_action_toggled, &p->rmouse, delta_t);


    // if(p->weapon != NULL)
    // {
    //     // if(p->weapon_ready || p->weapon->index == WEAPON_NONE)
    //     if(p->weapon_ready)
    //     {
    //         if(p->lmouse.triggered)
    //         {
    //             WeaponAttackType pa = p->weapon->primary_attack;
    //             if(pa == ATTACK_SHOOT)
    //             {
    //                 // spawn projectile
    //                 weapon_fire(p->mouse_x, p->mouse_y, p->weapon, p->lmouse.held);
    //             }
    //             else if(!p->attacking && (pa == ATTACK_MELEE || pa == ATTACK_POWER_MELEE))
    //             {
    //                 // printf("attacking = true\n");
    //                 p->melee_hit_count = 0;
    //                 p->attacking = true;
    //                 p->attacking_state = p->weapon->primary_state;
    //                 p->attacking_type = pa;
    //             }
    //         }

    //         if(p->rmouse.triggered)
    //         {
    //             WeaponAttackType sa = p->weapon->secondary_attack;
    //             if(sa == ATTACK_SHOOT)
    //             {
    //                 // spawn projectile
    //                 weapon_fire(p->mouse_x, p->mouse_y, p->weapon, p->rmouse.held);
    //             }
    //             else if(!p->attacking && (sa == ATTACK_MELEE || sa == ATTACK_POWER_MELEE))
    //             {
    //                 // printf("attacking = true\n");
    //                 p->melee_hit_count = 0;
    //                 p->attacking = true;
    //                 p->attacking_state = p->weapon->secondary_state;
    //                 p->attacking_type = sa;
    //             }
    //         }
    //     }
    // }

    // if(p->block_ready)
    // {
    //     if(p->lmouse.triggered)
    //     {
    //         bool add_block = true;
    //         for(int i = 0; i < blist->count; ++i)
    //         {
    //             if(FEQ(p->mouse_r, blocks[i].x) && FEQ(p->mouse_c, blocks[i].y))
    //             {
    //                 add_block = false;
    //                 break;
    //             }
    //         }
    //         if(add_block)
    //         {
    //             Vector2f block = {0};
    //             block.x = p->mouse_r;
    //             block.y = p->mouse_c;
    //             list_add(blist, (void*)&block);
    //         }
    //     }

    //     if(p->rmouse.triggered)
    //     {
    //         for(int i = 0; i < blist->count; ++i)
    //         {
    //             if(FEQ(p->mouse_r, blocks[i].x) && FEQ(p->mouse_c, blocks[i].y))
    //             {
    //                 list_remove(blist, i);
    //                 break;
    //             }
    //         }
    //     }
    // }


    // if(!p->reloading)
    // {

    //     if(block_toggled)
    //     {
    //         p->block_ready = !p->block_ready;
    //         p->weapon_ready = false;
    //         if(p->block_ready)
    //         {
    //             p->lmouse.cooldown = 0.0;
    //             p->lmouse.period = 20.0;
    //             p->lmouse.trigger_on_held = true;
    //             p->lmouse.trigger_on_press = true;
    //             p->lmouse.trigger_on_release = false;
    //             p->rmouse.cooldown = 0.0;
    //             p->rmouse.period = 20.0;
    //             p->rmouse.trigger_on_held = true;
    //             p->rmouse.trigger_on_press = true;
    //             p->rmouse.trigger_on_release = false;
    //         }
    //     }

    //     if(p->weapon_ready && gun_toggled)
    //     {
    //         int next = p->weapon->index+1;
    //         if(next >= WEAPON_MAX) next = 0;
    //         player_set_weapon(p, next);
    //     }

    //     if(equip_weapon_toggled)
    //     {
    //         p->weapon_ready = !p->weapon_ready;
    //         if(p->weapon_ready)
    //         {
    //             player_set_weapon(p, p->weapon->index);
    //         }
    //         p->block_ready = false;
    //     }
    // }


    if(debug_toggled)
    {
        debug_enabled = !debug_enabled;
    }

    if(editor_toggled)
    {
        editor_enabled = !editor_enabled;
        if(editor_enabled)
            window_enable_cursor();
        else
            window_disable_cursor();
    }

    if(role != ROLE_SERVER)
    {
        if(p->actions.primary_action)
        {
            if(window_is_cursor_enabled() && !editor_enabled)
            {
                window_disable_cursor();
            }
        }
    }

    Vector2f accel = {0};
    bool moving_player = MOVING_PLAYER(p);

    if(p->actions.up)    { accel.y -= p->speed; }
    if(p->actions.down)  { accel.y += p->speed; }
    if(p->actions.left)  { accel.x -= p->speed; }
    if(p->actions.right) { accel.x += p->speed; }

    if((p->actions.up || p->actions.down) && (p->actions.left || p->actions.right))
    {
        // moving diagonally
        accel.x *= SQRT2OVER2;
        accel.y *= SQRT2OVER2;
    }


    if(run_toggled)
    {
        p->running = !p->running;
    }

    p->phys.max_linear_vel = p->max_base_speed;

    if(p->running && moving_player && !p->busy)
    {
        accel.x *= 10.0;
        accel.y *= 10.0;
        p->phys.max_linear_vel *= 10.0;
    }

#if 0
    if(role == ROLE_SERVER)
    {
        printf("player pos: %f %f, accel: %f %f, delta_t: %f,map rect: %f %f %f %f",
                p->phys.pos.x,
                p->phys.pos.y,
                accel.x,
                accel.y,
                delta_t,
                map.rect.x,
                map.rect.y,
                map.rect.w,
                map.rect.h);
    }
#endif

    // if(p->item.item_type == ITEM_TYPE_OBJECT)
    // {
    //     printf("before\n");
    //     print_rect(&p->phys.pos);
    // }

    physics_begin(&p->phys);
    physics_add_friction(&p->phys, 16.0);
    physics_add_force(&p->phys, accel.x, accel.y);
    physics_simulate(&p->phys, delta_t);

    // if(p->item.item_type == ITEM_TYPE_OBJECT)
    // {
    //     printf("after\n");
    //     print_rect(&p->phys.pos);
    // }

    // // TODO: won't work for idle state
    // if(FEQ(accel.x,0.0) && FEQ(accel.y,0.0))
    // {
    //     p->moving = false;
    //     p->anim.curr_frame = 0;
    //     p->anim.curr_frame_time = 0.0;
    // }
    // else
    // {
    //     p->moving = true;
    //     gfx_anim_update(&p->anim,delta_t);
    // }


    // is this bueno?
    p->moving = !(FEQ(accel.x,0.0) && FEQ(accel.y,0.0));


    player_update_anim_state(p);
    player_update_anim_timing(p);
    player_update_image(p);

    gfx_anim_update(&p->anim,delta_t);

    // finish attack
    if(p->state == PSTATE_ATTACKING && p->anim.curr_loop > 0)
    {
        p->state = PSTATE_NONE;
        p->busy = false;
        player_update_anim_state(p);
        player_update_image(p);
    }

    player_update_sprite_index(p);
    player_update_boxes(p);

    // limit_pos(&map.rect, &p->phys.pos);

    player_weapon_melee_check_collision(p);


    lighting_point_light_move(p->point_light, p->pos.x, p->pos.y);


    if(debug_enabled)
    {
        float px = p->phys.pos.x;
        float py = p->phys.pos.y;
        // gfx_add_line(px,py,p->mouse_x,p->mouse_y,0x00FF0000);

        // if(p->attacking && (p->attacking_type == ATTACK_MELEE || p->attacking_type == ATTACK_POWER_MELEE))
        if(p->state == PSTATE_ATTACKING)
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
    }
}

void player_handle_net_inputs(Player* p, double delta_t)
{
    // handle input
    memcpy(&p->input_prior, &p->input, sizeof(NetPlayerInput));

    p->input.delta_t = delta_t;
    p->input.keys = p->keys;
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
    p->lerp_t += delta_t;

    float tick_time = 1.0/TICK_RATE;
    float t = (p->lerp_t / tick_time);

    Vector2f lp = lerp2f(&p->server_state_prior.pos,&p->server_state_target.pos,t);
    p->phys.pos.x = lp.x;
    p->phys.pos.y = lp.y;

    p->angle = lerp(p->server_state_prior.angle,p->server_state_target.angle,t);

}

void player_draw(Player* p)
{
    if(!is_in_camera_view(&p->phys.pos))
    {
        return;
    }

    GFXImage* img = &gfx_images[p->image];
    Rect* vr = &img->visible_rects[p->sprite_index];


    // if(p->item.item_type == ITEM_TYPE_OBJECT)
    // {
    //     printf("vr:\n");
    //   anim_state  print_rect(vr);
    // }

    //TEMP: blocks
    for(int i = 0; i < blist->count; ++i)
    {
        Rect r = {0};
        map_grid_to_rect(blocks[i].row, blocks[i].col, &r);
        gfx_draw_rect(&r, block_props[blocks[i].type].color, 0.0, 1.0, 0.50, true, true);
    }

    // if(p->block_ready)
    // {
    //     Rect r = {0};
    //     map_grid_to_rect(p->mouse_r, p->mouse_c, &r);
    //     gfx_draw_rect(&r, COLOR_BLUE, 0.0, 1.0, 0.15, true, true);
    // }

    // player
    gfx_draw_image(p->image, p->sprite_index, p->phys.pos.x, p->phys.pos.y, ambient_light,p->scale,0.0,1.0,true,true);

    if(p->item.drawable && p->item.props != NULL)
    {
        if(p->item.item_type == ITEM_TYPE_GUN || p->item.item_type == ITEM_TYPE_MELEE)
        {
            int wimage = -1;
            Rect* wpos = NULL;
            if(p->item.item_type == ITEM_TYPE_GUN)
            {
                Gun* gun = (Gun*)p->item.props;
                wimage = gun_get_image_index(p->model_index, p->anim_state, gun->type);
                wpos = &gun->pos;
            }
            else if(p->item.item_type == ITEM_TYPE_MELEE)
            {
                Melee* melee = (Melee*)p->item.props;
                wimage = melee_get_image_index(p->model_index, p->anim_state, melee->type);
                wpos = &melee->pos;
            }


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
                gfx_draw_image(wimage, p->sprite_index, p->phys.pos.x, p->phys.pos.y, ambient_light, p->scale,0,1.0,true,true);
            }

        }

        if(p->item.item_type == ITEM_TYPE_BLOCK)
        {
            BlockProp* bp = (BlockProp*)p->item.props;
            Rect r = {0};
            map_grid_to_rect(p->mouse_r, p->mouse_c, &r);
            gfx_draw_rect(&r, bp->color, 0.0, 1.0, 0.15, true, true);
        }

    }

    // bool draw_weapon = p->weapon_ready && p->weapon->index != WEAPON_NONE;
    // if(draw_weapon)
    // {

    //     GFXImage* wimg = &gfx_images[wimage];
    //     Rect* wvr = &wimg->visible_rects[p->sprite_index];

    //     if(!IS_RECT_EMPTY(wvr))
    //     {
    //         float wimg_center_x = IMG_ELEMENT_W/2.0;
    //         float wimg_center_y = IMG_ELEMENT_H/2.0;

    //         float gx = p->phys.pos.x + (wvr->x-wimg_center_x)*p->scale;
    //         float gy = p->phys.pos.y + (wvr->y-wimg_center_y)*p->scale;

    //         p->weapon->pos.x = gx;
    //         p->weapon->pos.y = gy;

    //         // weapon
    //         gfx_draw_image(wimage, p->sprite_index, p->phys.pos.x, p->phys.pos.y, ambient_light, p->scale,0,1.0,true);
    //     }

    // }


    if(debug_enabled)
    {
        Rect r = {0};

        // position box
        gfx_draw_rect(&p->pos, COLOR_RED, 0.0, 1.0,1.0, false, true);

        // phys.pos
        r.x = p->phys.pos.x;
        r.y = p->phys.pos.y;
        r.w = 2;
        r.h = 2;
        gfx_draw_rect(&r, COLOR_PURPLE, 0.0, 1.0,1.0, true, true);

        // pos
        r.x = p->pos.x;
        r.y = p->pos.y;
        gfx_draw_rect(&r, COLOR_ORANGE, 0.0, 1.0,1.0, true, true);


        // max_size
        r.x = p->pos.x;
        r.y = p->pos.y;
        r.w = p->max_size.w*p->scale;
        r.h = p->max_size.h*p->scale;
        // gfx_draw_rect(&r, COLOR_BLUE, 0.0, 1.0,1.0, false, true);
        gfx_draw_rect(&p->max_size, COLOR_BLUE, 0.0, 1.0,1.0, false, true);

        // // melee
        // if(!IS_RECT_EMPTY(&p->melee_box))
        // {
        //     gfx_draw_rect(&p->melee_box, COLOR_CYAN, 0.0, 1.0,1.0, false, true);
        // }

    }

    // crosshair
    gfx_draw_image(crosshair_image, 0, p->mouse_x, p->mouse_y, COLOR_PURPLE, 1.0,0.0,0.80, false,true);


    // name
    const float name_size = 0.11;
    Vector2f size = gfx_string_get_size(name_size, p->name);
    float x = p->phys.pos.x - size.x/2.0;
    float y = p->phys.pos.y + p->max_size.h*0.55;
    gfx_draw_string(x, y, player_colors[p->index], name_size, 0.0, 0.8, true, true, p->name);
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
    guns[idx].bullets = 32;
    guns[idx].bullets_max = 32;
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
    guns[idx].fire_period = 400.0; // milliseconds
    guns[idx].fire_spread = 30.0;
    guns[idx].fire_count = 5;
    guns[idx].bullets = 12;
    guns[idx].bullets_max = 12;
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


    // for(int w = 0; w < WEAPON_MAX; ++w)
    // {
    //     float maxw=0.0, maxh=0.0;
    //     for(int ps = 0; ps < ANIM_MAX; ++ps)
    //     {
    //         int img = weapons_get_image_index(HUMAN1, ps, w);
    //         if(img == -1) continue;
    //         for(int i = 0; i < gfx_images[img].element_count; ++i)
    //         {
    //             Rect* vr = &gfx_images[img].visible_rects[i];
    //             if(IS_RECT_EMPTY(vr)) continue;
    //             maxw = MAX(maxw, vr->w);
    //             maxh = MAX(maxh, vr->h);
    //         }
    //     }
    //     weapons[w].max_size.w = maxw;
    //     weapons[w].max_size.h = maxh;
    // }

}

// must call this after players_init()
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
                sprintf(fname, "img/characters/%s-%s_%s_%s.png", player_models[pm].name, player_state_str(ps), gun_type_str(guns[w].type), guns[w].name);
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
                sprintf(fname, "img/characters/%s-%s_%s_%s.png", player_models[pm].name, player_state_str(ps), melee_type_str(melees[w].type), melees[w].name);
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

// const char* weapon_type_str(WeaponType wtype)
// {
//     switch(wtype)
//     {
//         case WEAPON_TYPE_HANDGUN: return "handgun";
//         case WEAPON_TYPE_RIFLE: return "rifle";
//         case WEAPON_TYPE_BOW: return "bow";

//         case WEAPON_TYPE_MELEE0: return "bow";
//         default: return "";
//     }
// }

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



void gun_fire(int mx, int my, Gun* gun, bool held)
{

    if(gun->bullets <= 0) return; 

    if(gun->fire_count > 1)
    {
        for(int i = 0; i < gun->fire_count; ++i)
        {
            int direction = rand()%2 == 0 ? -1 : 1;
            float angle_offset = rand_float_between(0.0, gun->fire_spread/2.0) * direction;
            projectile_add(gun->projectile_type, gun, mx, my, angle_offset);
        }
    }
    else
    {
        float angle_offset = 0.0;
        if(held && !FEQ(gun->recoil_spread,0.0))
        {
            int direction = rand()%2 == 0 ? -1 : 1;
            angle_offset = rand_float_between(0.0, gun->recoil_spread/2.0) * direction;
        }
        projectile_add(gun->projectile_type, gun, mx, my, angle_offset);
    }
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


    float px = p->phys.pos.x;
    float py = p->phys.pos.y;

    Melee* melee = (Melee*)p->item.props;

    //TODO
    // if(p->attacking && (p->attacking_type == ATTACK_MELEE || p->attacking_type == ATTACK_POWER_MELEE))
    if(p->state == PSTATE_ATTACKING)
    {

        float f = 1.0;
        // if(p->attacking_type == ATTACK_POWER_MELEE)
        //     f = 1.5;

        for(int j = zlist->count - 1; j >= 0; --j)
        {

            bool collision = rectangles_colliding(&p->pos, &zombies[j].hit_box);
            // collision = false;

            if(!collision)
            {
                float zx = zombies[j].phys.pos.x;
                float zy = zombies[j].phys.pos.y;
                float angle = calc_angle_rad(px, py, zx, zy);

                bool within_angle_range = ABS(angle - p->angle) <= RAD(30);

                if(within_angle_range)
                {
                    float zr = MAX(zombies[j].hit_box.w, zombies[j].hit_box.h)/2.0;
                    float d = dist(px, py, zx, zy);
                    if(d <= (zr + melee->range))
                        collision = true;
                }
            }

            if(collision)
            {
                float damage = melee->power*f;
                // printf("zombie hurt\n");
                zombie_hurt(j,damage);
                p->melee_hit_count++;
                return;
            }

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
