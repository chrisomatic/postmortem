#include "particles.h"
#include "player.h"
#include "zombie.h"
#include "effects.h"
#include "gui.h"

#include "world.h"
#include "camera.h"

#include <string.h>

#include "entity.h"

SortEntity entities[MAX_ONSCREEN_ENTITIES];
int num_entities = 0;

static void sort_entity_list()
{
    // insertion sort
    int i, j;
    SortEntity key;
    for (i = 1; i < num_entities; i++) 
    {
        memcpy(&key, &entities[i], sizeof(SortEntity));
        j = i - 1;

        while (j >= 0 && entities[j].y > key.y)
        {
            memcpy(&entities[j+1], &entities[j], sizeof(SortEntity));
            j = j - 1;
        }
        memcpy(&entities[j+1], &key, sizeof(SortEntity));
    }

    // printf("==============================\n");
    // for(int i = 0; i < num_entities; ++i)
    // {
    //     SortEntity* e = &entities[i];
    //     printf("%d) type: %d, y: %d\n", i, e->type, e->y);
    // }
}


void entities_update()
{
    num_entities = 0;

    // players
    for(int i = 0; i < MAX_CLIENTS;++i)
    {
        Player* p = &players[i];
        if(p->active)
        {
            if(is_in_camera_view(&p->pos))
            {
                entities[num_entities].type = ENTITY_TYPE_PLAYER;
                // entities[num_entities].y = p->pos.y + p->pos.h/2.0;
                entities[num_entities].y = p->phys.pos.y + p->standard_size.h/2.0;
                entities[num_entities].data = (void*)p;
                num_entities++;
            }
        }
    }

    // zombies
    for(int i = zlist->count - 1; i >= 0 ; --i)
    {
        Zombie* z = &zombies[i];
        if(is_in_camera_view(&z->phys.pos))
        {
            entities[num_entities].type = ENTITY_TYPE_ZOMBIE;
            entities[num_entities].y = z->phys.pos.y + z->phys.pos.h/2.0;
            entities[num_entities].data = (void*)z;
            num_entities++;
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
            entities[num_entities].type = ENTITY_TYPE_BLOCK;
            entities[num_entities].y = r.y + r.h/2.0;
            entities[num_entities].data = (void*)b;
            num_entities++;
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
                entities[num_entities].type = ENTITY_TYPE_PARTICLE;
                entities[num_entities].y = s->pos.y;
                entities[num_entities].data = (void*)s;
                num_entities++;
            }
        }
    }

    // sort entities
    sort_entity_list();
}

void entities_draw(bool batched)
{
    if(batched)
        gfx_sprite_batch_begin(true);

    for(int i = 0; i < num_entities; ++i)
    {
        SortEntity* e = &entities[i];

        switch(e->type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                player_draw((Player*)e->data, batched);
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
                particles_draw_spawner((ParticleSpawner*)e->data, batched);
            } break;
        }
    }

    if(batched)
        gfx_sprite_batch_draw();
}
