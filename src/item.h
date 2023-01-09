#pragma once

#include "main.h"
#include "effects.h"

#include "glist.h"
#include "physics.h"
#include "math2d.h"
#include "particles.h"
#include "gfx.h"


typedef enum
{
    ITEM_TYPE_NONE,
    ITEM_TYPE_MELEE,
    ITEM_TYPE_GUN,
    ITEM_TYPE_BLOCK,
    ITEM_TYPE_OBJECT,
} PlayerItemType;

typedef struct
{
    PlayerItemType item_type;
    void* props;
    bool drawable;
    bool mouse_aim;
    Rect pos;
} PlayerItem;


// blocks
// -----------------------------------------------------------
#define MAX_BLOCKS  1000

typedef enum
{
    BLOCK_0,
    BLOCK_1,

    BLOCK_MAX
} BlockType;

typedef struct
{
    BlockType type;
    float hp;
    int image;
    int sprite_index;
    uint32_t color;
} BlockProp;

typedef struct
{
    Physics phys;
    float hp;
    BlockType type;
} block_t;

// guns
// -----------------------------------------------------------
typedef enum
{
    GUN_TYPE_HANDGUN,
    GUN_TYPE_RIFLE,
    GUN_TYPE_BOW,

    GUN_TYPE_MAX
} GunType;

typedef enum
{
    GUN_PISTOL1,
    GUN_MACHINEGUN1,
    GUN_SHOTGUN1,

    GUN_MAX
} GunIndex;

typedef struct
{
    GunIndex index;
    GunType type;
    const char* name;

    int anim_state; // PlayerAnimState
    Rect pos;

    // TODO
    int projectile_type; // sprite index

    float power;

    float recoil_spread;
    float fire_range;
    float fire_speed;
    float fire_period;
    float fire_spread;
    int fire_count;

    int bullets;
    int bullets_max;
    float reload_time;
} Gun;

// melee
// -----------------------------------------------------------

typedef enum
{
    MELEE_TYPE0,
    MELEE_TYPE1,

    MELEE_TYPE_MAX
} MeleeType;

typedef enum
{
    MELEE_KNIFE1,

    MELEE_MAX
} MeleeIndex;

typedef struct
{
    MeleeIndex index;
    MeleeType type;
    const char* name;

    int anim_state; // PlayerAnimState
    Rect pos;

    float period;
    float power;
    float range;
} Melee;


// Collectible
typedef struct
{
    Vector3f pos;
    Vector3f vel;
    GFXAnimation anim;
    char* name;
} Collectible;

// global vars
// -----------------------------------------------------------

extern int blocks_image;
extern BlockProp block_props[BLOCK_MAX];
extern block_t blocks[MAX_BLOCKS];
extern glist* blist;

extern Gun guns[GUN_MAX];
extern Melee melees[MELEE_MAX];


// functions
// -----------------------------------------------------------

void player_items_init();

const char* player_item_type_str(PlayerItemType item_type);
const char* gun_type_str(GunType gtype);
const char* melee_type_str(MeleeType mtype);

// model_index: PlayerModelIndex
// anim_state: PlayerAnimState
int gun_get_image_index(int model_index, int anim_state, GunType gtype);
int melee_get_image_index(int model_index, int anim_state, MeleeType mtype);

bool block_add(BlockProp* bp, int row, int col);
bool block_remove(int row, int col);
void block_hurt(block_t* b, float damage);
void block_draw(block_t* b, bool add_to_existing_batch);
void block_draw_debug(block_t* b);
void block_destroy(block_t* b);

void gun_fire(void* _player, Gun* gun, bool held);

// collectibles
void collectibles_init();
void collectibles_spawn(char* name,float x, float y);
void collectibles_update(Collectible* c, double delta_t);
void collectibles_update_all(double delta_t);
void collectibles_draw(Collectible* c);
void collectibles_draw_all();
