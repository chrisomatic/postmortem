#include "projectile.h"
#include "player.h"
#include "zombie.h"
#include "effects.h"
#include "gui.h"
#include "item.h"

#include "world.h"
#include "camera.h"

#include <string.h>

#include "entity.h"

glist* entity_draw_list = NULL;
Entity draw_entities[MAX_DRAW_ENTITIES];
GridBox grid_boxes[WORLD_GRID_ROWS_MAX][WORLD_GRID_COLS_MAX];


static bool dbg = false;

static int rect_get_grid_boxes(Rect* rect, int radius, int* rows, int* cols);
static int entity_get_grid_boxes(EntityType type, void* data, int rows[20], int cols[20]);
static void add_to_grid_boxes(EntityType type, void* data);
static void sort_entity_list(glist* list);
static Physics* entity_get_physics(EntityType type, void* data);
static void handle_collisions(EntityType type, void* data, double delta_t);
static void handle_proj_collisions(void* data, double delta_t);
static int scum(Physics* phys, int rows[20], int cols[20]);

void entities_init()
{
    entity_draw_list = list_create((void*)draw_entities, MAX_DRAW_ENTITIES, sizeof(Entity));
    if(entity_draw_list == NULL)
    {
        LOGE("entity draw list failed to create");
    }

    LOGI("WORLD_GRID_ROWS_MAX: %d", WORLD_GRID_ROWS_MAX);
    LOGI("WORLD_GRID_COLS_MAX: %d", WORLD_GRID_COLS_MAX);

    for(int r = 0; r < WORLD_GRID_ROWS_MAX; ++r)
    {
        for(int c = 0; c < WORLD_GRID_COLS_MAX; ++c)
        {
            grid_boxes[r][c].num = 0;
        }
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
    for(int i = plist->count - 1; i >= 0 ; --i)
    {
         Projectile* p = &projectiles[i];
         if(is_in_camera_view(&p->phys.collision))
         {
             Entity entity = {0};
             entity.type = ENTITY_TYPE_PROJECTILE;
             entity.sort_val = p->phys.pos.y + p->phys.collision.h/2.0;
             entity.data = (void*)p;
             list_add(entity_draw_list, (void*)&entity);
         }
     }

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
        if(is_in_camera_view(&b->phys.pos))
        {
            Entity entity = {0};
            entity.type = ENTITY_TYPE_BLOCK;
            entity.sort_val = b->phys.pos.y + b->phys.pos.h/2.0;
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
                projectile_draw((Projectile*)e->data,batched);
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
        if(!z->dead)
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

void entity_remove_from_grid_boxes(EntityType type, void* data)
{
    int rows[20] = {0};
    int cols[20] = {0};
    int count = entity_get_grid_boxes(type, data, rows, cols);

    for(int i = 0; i < count; ++i)
    {
        int r = rows[i];
        int c = cols[i];

        GridBox* g = &grid_boxes[r][c];

        int idx = 0;
        for(;;)
        {
            if(idx >= g->num)
                break;

            if(g->entities[idx].type == type)
            {
                if(g->entities[idx].data == data)
                {
                    memcpy(&g->entities[idx], &g->entities[g->num], sizeof(Entity));
                    g->num--;
                    continue;
                }
            }
            idx++;
        }
    }
}

static void reset_total_adjs()
{
    for(int i = 0; i < MAX_CLIENTS;++i)
    {
        Player* p = &players[i];
        if(p->active)
        {
            p->phys.total_adj.x = 0.0;
            p->phys.total_adj.y = 0.0;
        }
    }

    // zombies
    for(int i = zlist->count - 1; i >= 0 ; --i)
    {
        Zombie* z = &zombies[i];
        if(z->dead)
            continue;

        z->phys.total_adj.x = 0.0;
        z->phys.total_adj.y = 0.0;
    }
    
    // blocks
    for(int i = 0; i < blist->count; ++i)
    {
        block_t* b = &blocks[i];
        b->phys.total_adj.x = 0.0;
        b->phys.total_adj.y = 0.0;
    }

    // projectiles
    for(int i = plist->count - 1; i >= 0 ; --i)
    {
        Projectile* p = &projectiles[i];
        if(p->dead)
            continue;
        p->phys.total_adj.x = 0.0;
        p->phys.total_adj.y = 0.0;
    }
}

void entities_handle_collisions(double delta_t)
{
    reset_total_adjs();

    // players
    for(int i = 0; i < MAX_CLIENTS;++i)
    {
        Player* p = &players[i];
        if(p->active)
        {
            handle_collisions(ENTITY_TYPE_PLAYER, p, delta_t);
        }
    }

    // zombies
    for(int i = zlist->count - 1; i >= 0 ; --i)
    {
        Zombie* z = &zombies[i];
        if(z->dead)
            continue;

        handle_collisions(ENTITY_TYPE_ZOMBIE, z, delta_t);
    }
    
    // blocks
    for(int i = 0; i < blist->count; ++i)
    {
        block_t* b = &blocks[i];
        handle_collisions(ENTITY_TYPE_BLOCK, b, delta_t);
    }

    // projectiles
    for(int i = plist->count - 1; i >= 0 ; --i)
    {
        Projectile* p = &projectiles[i];
        if(p->dead)
            continue;
        handle_proj_collisions(p,delta_t);
    }
}


// static functions
// --------------------------------------------------------------------------------------


static int rect_get_grid_boxes(Rect* rect, int radius, int* rows, int* cols)
{
    int row = 0, col = 0;
    coords_to_world_grid(rect->x, rect->y, &row, &col);

    // bool in_world = !(row < 0 || col < 0 || row >= WORLD_GRID_ROWS_MAX || col >= WORLD_GRID_COLS_MAX);
    
    int count = 0;
    row = RANGE(row, 0, WORLD_GRID_ROWS_MAX);
    col = RANGE(col, 0, WORLD_GRID_COLS_MAX);
    rows[count] = row;
    cols[count] = col;
    count++;

    // Rect g = {0};
    // world_grid_to_rect(row, col, &g);

    if(dbg) printf("=================\n");

    for(int r = -radius; r <= radius; ++r)
    {
        for(int c = -radius; c <= radius; ++c)
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
                if(count >= 8)
                    return count;
            }
        }
    }
    if(dbg) printf("count: %d\n", count);
    dbg = false;

    return count;
}

static int entity_get_grid_boxes(EntityType type, void* data, int rows[20], int cols[20])
{
    if(type == ENTITY_TYPE_PARTICLE)
        return 0;

    Physics* phys = entity_get_physics(type,data);
    int count = scum(phys, rows, cols);

    return count;
}

static void add_to_grid_boxes(EntityType type, void* data)
{
    int rows[20] = {0};
    int cols[20] = {0};

    int count = entity_get_grid_boxes(type, data, rows, cols);

    for(int i = 0; i < count; ++i)
    {
        int r = rows[i];
        int c = cols[i];

        GridBox* g = &grid_boxes[r][c];

        if(g->num >= MAX_GRIDBOX_ENTITIES)
        {
            LOGW("(%d, %d) grid box is full", r, c);
            continue;
        }

        g->entities[g->num].type = type;
        g->entities[g->num].data = data;
        g->num++;
    }
}

static void sort_entity_list(glist* list)
{
    if(!list)
        return;

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

static Physics* entity_get_physics(EntityType type, void* data)
{
    switch(type)
    {
        case ENTITY_TYPE_PLAYER:
            return &((Player*)data)->phys;
        case ENTITY_TYPE_ZOMBIE:
        {
            Zombie* z = (Zombie*)data;
            if(z->dead)
                return NULL;
            return &z->phys;
        }
        case ENTITY_TYPE_BLOCK:
        {
            return &((block_t*)data)->phys;
        }
        case ENTITY_TYPE_PROJECTILE:
        {
            Projectile* p = (Projectile*)data;
            if(p->dead)
                return NULL;
            return &p->phys;
        }
        default:
            return NULL;
    }
}

static void handle_collisions(EntityType type, void* data, double delta_t)
{
    int rows[20] = {0};
    int cols[20] = {0};

    int count = entity_get_grid_boxes(type, data, rows, cols);

    Physics* phys1 = entity_get_physics(type,data);
    if(!phys1)
        return;

    phys1->num_colliding_entities = 0;

    for(int i = 0; i < count; ++i)
    {
        int r = rows[i];
        int c = cols[i];

        if(r > WORLD_GRID_ROWS_MAX || c > WORLD_GRID_COLS_MAX)
            continue;

        GridBox* g = &grid_boxes[r][c];

        for(int i = 0; i < g->num; ++i)
        {
            Entity* e = &g->entities[i];

            if(e->data == data) // don't check self
                continue;

            Physics* phys2 = entity_get_physics(e->type,e->data);
            if(!phys2)
                continue;

            bool collision = physics_check_collisions(phys1, phys2, delta_t);

            if(collision)
            {
                //@NOTE: update grid boxes?
            }
        }
    }

    physics_resolve_collisions(phys1, delta_t);
}

#if DEBUG_PROJ_GRIDS
Rect pg[20] = {0};
int pg_count = 0;
Rect cb = {0};
Rect pcb = {0};
#endif

static int scum(Physics* phys, int rows[20], int cols[20])
{
    int gc = 0;

    Rect* rc = &phys->collision;
    Rect* prc = &phys->prior_collision;

#if DEBUG_PROJ_GRIDS
    pg_count = 0;
    cb = *rc;
    pcb = *prc;
#endif

    int edge_rows[8] = {0};
    int edge_cols[8] = {0};

    int pcount = rect_get_grid_boxes(prc, 1, edge_rows, edge_cols);
    int ccount = rect_get_grid_boxes(rc, 1, edge_rows+pcount, edge_cols+pcount);
    int count = pcount + ccount;

    if(count == 0)
    {
        LOGW("count is 0\n");
        return 0;
    }

    int min_row=edge_rows[0], max_row=edge_rows[0], min_col=edge_cols[0], max_col=edge_cols[0];
    for(int i = 1; i < count; ++i)
    {
        int r = edge_rows[i];
        int c = edge_cols[i];
        if(r < min_row) min_row = r;
        if(r > max_row) max_row = r;
        if(c < min_col) min_col = c;
        if(c > max_col) max_col = c;
    }

    //printf("-----------------------------------------------------------------------------\n");
    //printf("min: %d, %d\n", min_row, min_col);
    //printf("max: %d, %d\n", max_row, max_col);

    // 1 grid space
    if(min_row == max_row && min_col == max_col)
    {
        rows[gc] = min_row;
        cols[gc] = min_col;
        gc = 1;

#if DEBUG_PROJ_GRIDS
        pg_count = 0;
        world_grid_to_rect(min_row, min_col, &pg[pg_count++]);
#endif

        goto scum_exit;
    }


    int rrange = max_row - min_row;
    int crange = max_col - min_col;

    int xdir = 1;   // moving right
    int start_col = min_col;
    if(prc->x > rc->x)
    {
        xdir = -1;  // moving left
        start_col = max_col;
    }

    int ydir = 1;   // moving down
    int start_row = min_row;
    if(prc->y > rc->y)
    {
        ydir = -1;  // moving up
        start_row = max_row;
    }


    if(min_row == max_row)
    {

        for(int i = 0; i <= crange; ++i)
        {
            int c = start_col + i*xdir;
            rows[gc] = min_row;
            cols[gc] = c;
            gc++;
            if(gc >= 20) goto scum_exit;
        }

    }
    else if(min_col == max_col)
    {

        for(int i = 0; i <= rrange; ++i)
        {
            int r = start_row + i*ydir;
            rows[gc] = r;
            cols[gc] = min_col;
            gc++;
            if(gc >= 20) goto scum_exit;
        }

    }
    else
    {

        LineSeg segs[5] = {0};
        rects_to_ling_segs(prc, rc, segs);

        for(int i = 0; i <= crange; ++i)
        {
            for(int j = 0; j <= rrange; ++j)
            {
                int c = start_col + i*xdir;
                int r = start_row + j*ydir;
                Rect wr = {0};
                world_grid_to_rect(r, c, &wr);
                bool intersect = are_line_segs_intersecting_rect(segs, 5, &wr);
                if(intersect)
                {
                    rows[gc] = r;
                    cols[gc] = c;
                    gc++;
                    if(gc >= 20) goto scum_exit;
                }

            }
        }

    }

scum_exit:

#if DEBUG_PROJ_GRIDS
    pg_count = 0;
    for(int i = 0; i < gc; ++i)
    {
        world_grid_to_rect(rows[i], cols[i], &pg[pg_count++]);
    }
#endif

    return gc;
}

static void handle_proj_collisions(void* data, double delta_t)
{
    EntityType type = ENTITY_TYPE_PROJECTILE;
    Projectile* proj = (Projectile*)data;

    if(!proj)
        return;

    if(proj->dead)
        return;

    int rows[20],cols[20];
    int count = scum(&proj->phys, rows, cols);


    Physics* phys1 = &proj->phys;
    if(!phys1)
        return;

    #define HITS_MAX 100
    Entity* hits[HITS_MAX] = {0};
    int num_hits = 0;

    for(int i = 0; i < count; ++i)
    {
        int r = rows[i];
        int c = cols[i];

        if(r > WORLD_GRID_ROWS_MAX || c > WORLD_GRID_COLS_MAX)
            continue;

        GridBox* g = &grid_boxes[r][c];

        for(int j = 0; j < g->num; ++j)
        {
            Entity* e = &g->entities[j];

            if(e->data == data) // don't check self
                continue;

            if(e->data == player) // don't hit player
                continue;

            Physics* phys2 = entity_get_physics(e->type,e->data);
            if(!phys2)
                continue;

            if(num_hits >= HITS_MAX)
                break;

            if(are_rects_colliding(&phys1->prior_collision, &phys1->collision, &phys2->hit))
            {
                hits[num_hits++] = e;
            }
        }
    }

    if(num_hits > 0)
    {
        Entity* min_e;
        Physics* min_phys2;

        float min_d = INFINITY;
        for(int _j = 0; _j < num_hits; ++_j)
        {
            Entity* e = hits[_j];
            Physics* phys2 = entity_get_physics(e->type,e->data);

            float d = dist(phys1->prior_collision.x, phys1->prior_collision.y, phys2->hit.x, phys2->hit.y);
            if(d < min_d)
            {
                min_d = d;
                min_e = e;
                min_phys2 = phys2;
            }
        }

        ParticleEffect pe;

        switch(min_e->type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                Player* p = (Player*)min_e->data;
                player_hurt(p,proj->damage);
                memcpy(&pe,&particle_effects[EFFECT_BLOOD1],sizeof(ParticleEffect));
            } break;
            case ENTITY_TYPE_ZOMBIE:
            {
                Zombie* z = (Zombie*)min_e->data;
                zombie_hurt(z,proj->damage);
                memcpy(&pe,&particle_effects[EFFECT_BLOOD1],sizeof(ParticleEffect));
            } break;
            case ENTITY_TYPE_BLOCK:
            {
                block_t* b = (block_t*)min_e->data;
                block_hurt(b,proj->damage);
                memcpy(&pe,&particle_effects[EFFECT_DEBRIS1],sizeof(ParticleEffect));
                pe.sprite_index = b->type;
            } break;
        }

        pe.scale.init_min *= 0.5;
        pe.scale.init_max *= 0.5;
        pe.velocity_x.init_min = -(proj->vel.x*0.02);
        pe.velocity_x.init_max = -(proj->vel.x*0.02);
        pe.velocity_x.rate = -0.02;
        pe.velocity_y.init_min = (proj->vel.y*0.02);
        pe.velocity_y.init_max = 0.0;
        pe.velocity_y.rate = -0.02;

        particles_spawn_effect(min_phys2->pos.x, min_phys2->pos.y, &pe, 0.6, true, false);

        proj->dead = true;
    }
}

