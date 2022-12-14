#pragma once
#include "glist.h"
#include "world.h"


#define MAX_ENTITIES 10000
#define MAX_DRAW_ENTITIES 1000
#define MAX_GRIDBOX_ENTITIES 128


typedef enum
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_ZOMBIE,
    ENTITY_TYPE_BLOCK,
    ENTITY_TYPE_PARTICLE,    //spawner
    ENTITY_TYPE_PROJECTILE,
} EntityType;

typedef struct
{
    EntityType type;
    void* data;
    int sort_val;
} Entity;

typedef struct
{
    Entity entities[MAX_GRIDBOX_ENTITIES];
    int num;
} GridBox;


extern glist* entity_draw_list;
extern Entity draw_entities[MAX_DRAW_ENTITIES];
extern GridBox grid_boxes[WORLD_GRID_ROWS_MAX][WORLD_GRID_COLS_MAX];

void entities_init();
void entities_update_draw_list();
void entities_draw(bool batched);


void entity_remove_from_grid_boxes(EntityType type, void* data);
void entities_update_grid_boxes();
void entities_handle_collisions(double delta_t);


#if DEBUG_PROJ_GRIDS
extern Rect pg[20];
extern int pg_count;
extern Rect cb;
extern Rect pcb;
#endif