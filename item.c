#include "item.h"
#include "projectile.h"
#include "player.h"
#include "lighting.h"
#include "world.h"

// global vars
// -----------------------------------------------------------
int blocks_image = -1;
BlockProp block_props[BLOCK_MAX] = {0};
block_t blocks[MAX_BLOCKS] = {0};
glist* blist = NULL;

Gun guns[GUN_MAX] = {0};
Melee melees[MELEE_MAX] = {0};

// static vars
// -----------------------------------------------------------
static int gun_image_sets[PLAYER_MODELS_MAX][ANIM_MAX][GUN_MAX];
static int melee_image_sets[PLAYER_MODELS_MAX][ANIM_MAX][MELEE_MAX];

// static funtions prototypes
// -----------------------------------------------------------
static void blocks_init();
static void weapons_init();
static void weapons_init_images();

// global functions
// -----------------------------------------------------------

void player_items_init()
{
    weapons_init();
    blocks_init();
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

int gun_get_image_index(int model_index, int anim_state, GunType gtype)
{
    return gun_image_sets[model_index][anim_state][gtype];
}

int melee_get_image_index(int model_index, int anim_state, MeleeType mtype)
{
    return melee_image_sets[model_index][anim_state][mtype];
}

bool block_add(BlockProp* bp, int row, int col)
{
    Rect brect = {0};
    map_grid_to_rect(row, col, &brect);

    for(int i = blist->count-1; i >= 0; --i)
    {
        if(rectangles_colliding(&brect, &blocks[i].phys.actual_pos))
        {
            return false;
        }
    }

    block_t b = {0};
    memcpy(&b.phys.pos,&brect,sizeof(Rect));
    memcpy(&b.phys.actual_pos,&brect,sizeof(Rect));
    memcpy(&b.phys.collision,&brect,sizeof(Rect));
    memcpy(&b.phys.prior_collision,&brect,sizeof(Rect));
    memcpy(&b.phys.hit,&brect,sizeof(Rect));

    b.phys.mass = 10000.0;
    if(bp->type == 0)
        b.phys.mass = 5.0;

    b.type = bp->type;
    b.hp = bp->hp;
    list_add(blist, (void*)&b);

    return true;
}

void block_destroy(block_t* b)
{
    ParticleEffect pe ={0};
    memcpy(&pe, &particle_effects[EFFECT_BLOCK_DESTROY],sizeof(ParticleEffect));
    pe.sprite_index = b->type;
    particles_spawn_effect(b->phys.actual_pos.x, b->phys.actual_pos.y, &pe, 1.0, true, false);
    particles_spawn_effect(b->phys.actual_pos.x, b->phys.actual_pos.y-16, &particle_effects[EFFECT_SMOKE2], 1.0, true, false);

    list_remove_by_item(blist, b);
}


void block_hurt(block_t* b, float damage)
{
    b->hp -= damage;
    if(b->hp <= 0.0)
    {
        block_destroy(b);
    }
}


bool block_remove(int row, int col)
{
    Rect brect = {0};
    map_grid_to_rect(row, col, &brect);

    for(int i = blist->count-1; i >= 0; --i)
    {
        if(rectangles_colliding(&brect, &blocks[i].phys.actual_pos))
        {
            block_destroy(&blocks[i]);
            return true;
        }
    }
    return false;
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

}

void block_draw_debug(block_t* b)
{
    gfx_draw_rect(&b->phys.collision, COLOR_COLLISON, 0.0, 1.0, 1.0, false, true);
}

void gun_fire(void* _player, Gun* gun, bool held)
{
    Player* p = (Player*)_player;

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

    particles_spawn_effect(gun->pos.x + 10*cos(player->angle), gun->pos.y - 10*sin(player->angle), &particle_effects[EFFECT_GUN_BLAST], 0.1, true, false);

    lighting_point_light_add(gun->pos.x,gun->pos.y,1.0,1.0,1.0,0.5,0.2);

    gun->bullets--;
}

// static funtions
// -----------------------------------------------------------

static void blocks_init()
{
    blocks_image = gfx_load_image("img/block_set.png", false, true, 32, 50, NULL);

    blist = list_create((void*)blocks, MAX_BLOCKS, sizeof(blocks[0]));
    if(blist == NULL)
    {
        LOGE("block list failed to create");
    }

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
}

static void weapons_init()
{
    int idx = 0;

    idx = GUN_PISTOL1;
    guns[idx].index = idx;
    guns[idx].name = "pistol1";
    guns[idx].type = GUN_TYPE_HANDGUN;
    guns[idx].anim_state = ANIM_NONE;    // no change in state
    guns[idx].power = 1.0;
    guns[idx].recoil_spread = 2.0;
    guns[idx].fire_range = MAP_GRID_PXL_SIZE*16; //pixels
    // guns[idx].fire_range = MAP_GRID_PXL_SIZE*1000; //pixels
    guns[idx].fire_speed = 4000.0;
    guns[idx].fire_period = 500.0; // milliseconds
    guns[idx].fire_spread = 0.0;
    guns[idx].fire_count = 1;
    guns[idx].bullets = 9999;
    guns[idx].bullets_max = 9999;
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

static void weapons_init_images()
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
