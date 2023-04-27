#pragma once

#define MAX_NPCS 32

typedef struct
{
    Physics phys;
    GFXAnimation anim;

    int image;
    uint8_t sprite_index;
    uint8_t sprite_index_direction; // 0-7

} NPC;

extern NPC npcs[MAX_NPCS];

void npc_init();
