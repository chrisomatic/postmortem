#pragma once



#define MAX_ENTITIES 10000

typedef enum
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_ZOMBIE,
    ENTITY_TYPE_BLOCK,
    ENTITY_TYPE_PARTICLE    //spawner
    // ENTITY_TYPE_PROJECTILE,
} EntityType;

typedef struct
{
    EntityType type;
    int y;
    void* data;
} SortEntity;


extern SortEntity entities[MAX_ENTITIES];
extern int num_entities;


void entities_update();
void entities_draw(bool batched);
