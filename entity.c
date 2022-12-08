#include "particles.h"
#include "projectile.h"
#include "player.h"
#include "zombie.h"
#include "effects.h"
#include "gui.h"

#include "world.h"
#include "camera.h"

#include <string.h>

#include "entity.h"

// glist* entity_list = NULL;
// Entity entities[MAX_ENTITIES];


glist* entity_draw_list = NULL;
Entity draw_entities[MAX_DRAW_ENTITIES];



GridBox grid_boxes[WORLD_GRID_ROWS_MAX][WORLD_GRID_COLS_MAX];
// int num_entities = 0;

void entities_init()
{
    // entity_list = list_create((void*)entities, MAX_ENTITIES, sizeof(Entity));
    // if(entity_list == NULL)
    // {
    //     LOGE("entity list failed to create");
    // }

    entity_draw_list = list_create((void*)draw_entities, MAX_DRAW_ENTITIES, sizeof(Entity));
    if(entity_draw_list == NULL)
    {
        LOGE("entity draw list failed to create");
    
    }

    printf("WORLD_GRID_ROWS_MAX: %d\n", WORLD_GRID_ROWS_MAX);
    printf("WORLD_GRID_COLS_MAX: %d\n", WORLD_GRID_COLS_MAX);

    for(int r = 0; r < WORLD_GRID_ROWS_MAX; ++r)
    {
        for(int c = 0; c < WORLD_GRID_COLS_MAX; ++c)
        {
            grid_boxes[r][c].num = 0;
        }
    }
}

// bool entity_add(Entity* entity)
// {
//     return list_add(entity_list, (void*)entity);
// }


// bool entity_remove(int index)
// {
//     return list_remove(entity_list, index);
// }

// int entity_get_index(uint32_t id)
// {
//     for(int i = 0; i < entity_list->count; ++i)
//     {
//         if(entities[i] == id)
//             return i;
//     }
// }





static void sort_entity_list(glist* list)
{
    Entity* lst = (Entity*)list->buf;
    int count = list->count;

    // insertion sort
    int i, j;
    Entity key;
    for (i = 1; i < count; i++) 
    {
        memcpy(&key, &lst[i], sizeof(Entity));
        j = i - 1;

        while (j >= 0 && lst[j].sort_val > key.sort_val)
        {
            memcpy(&lst[j+1], &lst[j], sizeof(Entity));
            j = j - 1;
        }
        memcpy(&lst[j+1], &key, sizeof(Entity));
    }

}




void entities_update_draw_list()
{
    list_clear(entity_draw_list);

    // players
    for(int i = 0; i < MAX_CLIENTS;++i)
    {
        Player* p = &players[i];
        if(p->active)
        {
            if(is_in_camera_view(&p->phys.actual_pos))
            {
                Entity entity = {0};
                entity.type = ENTITY_TYPE_PLAYER;
                // entity.sort_val = p->pos.y + p->pos.h/2.0;
                entity.sort_val = p->phys.pos.y + p->standard_size.h/2.0;
                // printf("player sort val: %d\n", entity.sort_val);
                entity.data = (void*)p;
                list_add(entity_draw_list, (void*)&entity);
            }
        }
    }

    // not drawing projectiles
    // // projectiles
    // for(int i = plist->count - 1; i >= 0 ; --i)
    // {
    //     Projectile* p = &projectiles[i];
    //     if(is_in_camera_view(&p->hurt_box))
    //     {
    //         Entity entity = {0};
    //         entity.type = ENTITY_TYPE_PROJECTILE;
    //         entity.sort_val = p->pos.y + p->hurt_box.h/2.0;
    //         entity.data = (void*)p;
    //         list_add(entity_draw_list, (void*)&entity);
    //     }
    // }

    // zombies
    for(int i = zlist->count - 1; i >= 0 ; --i)
    {
        Zombie* z = &zombies[i];
        if(is_in_camera_view(&z->phys.pos))
        {
            Entity entity = {0};
            entity.type = ENTITY_TYPE_ZOMBIE;
            entity.sort_val = z->phys.pos.y + z->phys.pos.h/2.0;
            entity.data = (void*)z;
            list_add(entity_draw_list, (void*)&entity);
        }
    }

    // blocks
    for(int i = 0; i < blist->count; ++i)
    {
        block_t* b = &blocks[i];
        Rect r = {0};
        map_grid_to_rect(b->row, b->col, &r);
        if(is_in_camera_view(&r))
        {
            Entity entity = {0};
            entity.type = ENTITY_TYPE_BLOCK;
            entity.sort_val = r.y + r.h/2.0;
            entity.data = (void*)b;
            list_add(entity_draw_list, (void*)&entity);
        }
    }

    // particle spawners
    for(int i = 0; i < spawner_list->count; ++i)
    {
        ParticleSpawner* s = &spawners[i];
        if(!s->hidden && s != editor_get_particle_spawner()) //particle_spawner is in editor.h
        {
            if(particles_is_spawner_in_camera_view(s))
            {
                Entity entity = {0};
                entity.type = ENTITY_TYPE_PARTICLE;
                entity.sort_val = s->pos.y;
                entity.data = (void*)s;
                
                list_add(entity_draw_list, (void*)&entity);
            }
        }
    }

    // sort draw_entities
    sort_entity_list(entity_draw_list);
}

void entities_draw(bool batched)
{
    if(batched)
        gfx_sprite_batch_begin(true);

    for(int i = 0; i < entity_draw_list->count; ++i)
    {
        Entity* e = &draw_entities[i];

        switch(e->type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                player_draw((Player*)e->data, batched);
            } break;

            case ENTITY_TYPE_PROJECTILE:
            {
                // not drawing projectiles
                // (Projectile*)e->data
            } break;

            case ENTITY_TYPE_ZOMBIE:
            {
                zombie_draw((Zombie*)e->data, batched);
            } break;

            case ENTITY_TYPE_BLOCK:
            {
                block_draw((block_t*)e->data, batched);
            } break;

            case ENTITY_TYPE_PARTICLE:
            {
                particles_draw_spawner((ParticleSpawner*)e->data, false, batched);
            } break;
        }
    }

    if(batched)
        gfx_sprite_batch_draw();
}



bool dbg = false;

// @NOTE: assumes that r is smaller than a grid space
static int get_grid_boxes(Rect* rect, int rows[4], int cols[4])
{
    int row = 0, col = 0;
    coords_to_world_grid(rect->x, rect->y, &row, &col);

    int count = 0;
    rows[count] = row;
    cols[count] = col;
    count++;

    // Rect g = {0};
    // world_grid_to_rect(row, col, &g);

    if(dbg) printf("=================\n");

    for(int r = -1; r < 2; ++r)
    {
        for(int c = -1; c < 2; ++c)
        {
            if(r == 0 && c == 0) continue;
            int _row = r+row;
            int _col = c+col;
            if(_row < 0 || _col < 0) continue;
            if(_row >= WORLD_GRID_ROWS_MAX || _col >= WORLD_GRID_COLS_MAX) continue;

            if(dbg) printf("%d, %d\n", _row, _col);


            Rect check = {0};
            world_grid_to_rect(_row, _col, &check);
            if(rectangles_colliding(rect, &check))
            {
                rows[count] = _row;
                cols[count] = _col;
                count++;
            }
        }
    }
    if(dbg) printf("count: %d\n", count);

    return count;
}

int entity_get_grid_boxes(EntityType type, void* data, int rows[4], int cols[4])
{
    if(type == ENTITY_TYPE_PARTICLE)
        return 0;

    Rect rect = {0};
    switch(type)
    {
        case ENTITY_TYPE_PLAYER:
        {
            Player* p = (Player*)data;
            rect = p->phys.actual_pos;
        } break;

        case ENTITY_TYPE_PROJECTILE:
        {
            Projectile* p = (Projectile*)data;
            rect = p->hurt_box;
        } break;

        case ENTITY_TYPE_ZOMBIE:
        {
            Zombie* z = (Zombie*)data;
            rect = z->phys.pos;
        } break;

        case ENTITY_TYPE_BLOCK:
        {
            block_t* b = (block_t*)data;
            map_grid_to_rect(b->row, b->col, &rect);
        } break;
    }

    int count = get_grid_boxes(&rect, rows, cols);
    return count;
}


void add_to_grid_boxes(EntityType type, void* data)
{

    int rows[4] = {0};
    int cols[4] = {0};

    int count = entity_get_grid_boxes(type, data, rows, cols);

    for(int i = 0; i < count; ++i)
    {
        int r = rows[i];
        int c = cols[i];
        if(r > WORLD_GRID_ROWS_MAX || c > WORLD_GRID_COLS_MAX)
            continue;
        GridBox* g = &grid_boxes[r][c];

        if(g->num >= MAX_GRIDBOX_ENTITIES)
            continue;

        g->entities[g->num].type = type;
        g->entities[g->num].data = data;
        g->num++;
    }

}


void entity_remove_from_grid_boxes(EntityType type, void* data)
{

    int rows[4] = {0};
    int cols[4] = {0};
    // int count = get_grid_boxes(&rect, rows, cols);

    int count = entity_get_grid_boxes(type, data, rows, cols);

    for(int i = 0; i < count; ++i)
    {
        int r = rows[i];
        int c = cols[i];
        if(r > WORLD_GRID_ROWS_MAX || c > WORLD_GRID_COLS_MAX)
            continue;
        GridBox* g = &grid_boxes[r][c];

        int idx = 0;
        for(;;)
        {
            if(idx >= g->num)
                break;

            if(g->entities[idx].type = type)
            {
                if(g->entities[idx].data = data)
                {
                    memcpy(&g->entities[idx], &g->entities[g->num], sizeof(Entity));
                    g->num--;
                    continue;
                }
            }
            idx++;
        }

    }


    // // could make this more efficient probably
    // for(int r = 0; r < WORLD_GRID_ROWS_MAX; ++r)
    // {
    //     for(int c = 0; c < WORLD_GRID_COLS_MAX; ++c)
    //     {
    //         GridBox* g = &grid_boxes[r][c];
    //         int num = g->num;
    //         for(int i = 0; i < num; ++i)
    //         {
    //             if(g->entities[i].type = type)
    //             {
    //                 if(g->entities[i].data = data)
    //                 {
    //                     memcpy(&g->entities[i], &g->entities[g->num], sizeof(Entity));
    //                     g->num--;
    //                 }
    //             }
    //         }
    //     }
    // }
}


void entities_update_grid_boxes()
{

    for(int r = 0; r < WORLD_GRID_ROWS_MAX; ++r)
    {
        for(int c = 0; c < WORLD_GRID_COLS_MAX; ++c)
        {
            grid_boxes[r][c].num = 0;
        }
    }

    // players
    for(int i = 0; i < MAX_CLIENTS;++i)
    {
        Player* p = &players[i];
        if(p->active)
        {
            // if(p->index == 0) dbg = true;
            add_to_grid_boxes(ENTITY_TYPE_PLAYER, (void*)p);
            dbg = false;
        }
    }

    // projectiles
    for(int i = plist->count - 1; i >= 0 ; --i)
    {
        Projectile* p = &projectiles[i];
        add_to_grid_boxes(ENTITY_TYPE_PROJECTILE, (void*)p);

    }

    // zombies
    for(int i = zlist->count - 1; i >= 0 ; --i)
    {
        Zombie* z = &zombies[i];
        add_to_grid_boxes(ENTITY_TYPE_ZOMBIE, (void*)z);
    }

    // blocks
    for(int i = 0; i < blist->count; ++i)
    {
        block_t* b = &blocks[i];
        add_to_grid_boxes(ENTITY_TYPE_BLOCK, (void*)b);
    }

    // // particle spawners
    // for(int i = 0; i < spawner_list->count; ++i)
    // {
    //     ParticleSpawner* s = &spawners[i];
    //     if(!s->hidden && s != editor_get_particle_spawner()) //particle_spawner is in editor.h
    //     {
    //     }
    // }

}
