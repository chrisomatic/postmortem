#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "gfx.h"
#include "math2d.h"
#include "log.h"
#include "glist.h"
#include "particles.h"

static ParticleSpawner spawners[MAX_PARTICLE_SPAWNERS] = {0};
static glist* spawner_list;
static int global_id_count = 0;

static void print_particle(Particle* p)
{
    printf("===================\n");
    printf("Particle:\n");
    printf("  position: %f %f\n", p->pos.x, p->pos.y);
    printf("  velocity: %f %f\n", p->vel.x, p->vel.y);
    printf("  color: %f %f %f\n", p->color.x, p->color.y, p->color.z);
    printf("  scale: %f\n", p->scale);
    printf("  opacity: %f\n", p->opacity);
    printf("  life: %f\n", p->life);
    printf("  life_max: %f\n", p->life_max);
    printf("===================\n");
}

static void emit_particle(ParticleSpawner* s)
{
    Particle* p = &s->particles[s->particle_list->count++];

    p->pos.x = s->pos.x;
    p->pos.y = s->pos.y;
    p->vel.x = RAND_FLOAT(s->effect.velocity.init_min, s->effect.velocity.init_max);
    p->vel.y = RAND_FLOAT(s->effect.velocity.init_min, s->effect.velocity.init_max);
    p->color.x = 0.8;
    p->color.y = 0.0;
    p->color.z = 0.0;
    p->rotation = RAND_FLOAT(s->effect.angular_vel.init_min, s->effect.angular_vel.init_max);
    p->scale = RAND_FLOAT(s->effect.scale.init_min, s->effect.scale.init_max);
    p->opacity = RAND_FLOAT(s->effect.opacity.init_min, s->effect.opacity.init_max);
    p->life_max = RAND_FLOAT(s->effect.life.init_min, s->effect.life.init_max);
    p->life = 0.0;
}

static void delete_particle(ParticleSpawner* spawner, int index)
{
    list_remove(spawner->particle_list, index);
}

static int get_id()
{
    return global_id_count++;
}

static ParticleSpawner* get_spawner_by_id(int id)
{
    for(int i = 0; i < spawner_list->count; ++i)
    {
        if(spawners[i].id == id)
        {
            return &spawners[i];
        }
    }
    return NULL;
}

void delete_spawner(int index)
{
    list_delete(spawners[index].particle_list);
    list_remove(spawner_list, index);
}

void particles_show_spawner(int id, bool show)
{
    ParticleSpawner* spawner = get_spawner_by_id(id);

    if(spawner)
    {
        spawner->hidden = !show;
    }
}

ParticleSpawner* particles_get_spawner(int id)
{
    ParticleSpawner* spawner = get_spawner_by_id(id);

    if(spawner)
    {
        return spawner;
    }
}

void particles_init()
{
    spawner_list = list_create(spawners,MAX_PARTICLE_SPAWNERS,sizeof(ParticleSpawner));
}

int particles_spawn_effect(float x, float y, ParticleEffect* effect, bool in_world, bool hidden)
{
    if(list_is_full(spawner_list))
    {
        LOGW("Too many spawners!");
        return -1;
    }

    ParticleSpawner* spawner = &spawners[spawner_list->count++];

    spawner->particle_list = list_create(spawner->particles,MAX_PARTICLES_PER_SPAWNER,sizeof(Particle));

    if(effect)
    {
        memcpy(&spawner->effect, effect, sizeof(ParticleEffect));
    }

    
    spawner->id = get_id();
    spawner->pos.x = x;
    spawner->pos.y = y;
    spawner->in_world = in_world;
    spawner->spawn_time = 0.0;
    spawner->spawn_time_max = RAND_FLOAT(spawner->effect.spawn_time_min, spawner->effect.spawn_time_max);
    spawner->hidden = hidden;

    return (spawner->id);
}

void particles_update(double delta_t)
{
    for(int i = spawner_list->count-1; i >= 0; --i)
    {
        ParticleSpawner* spawner = &spawners[i];

        for(int j = spawner->particle_list->count-1; j >= 0; --j)
        {
            // update each particle
            Particle* p = &spawner->particles[j];

            // update life
            p->life += delta_t*(spawner->effect.life.rate);

            // check for death
            if(p->life >= p->life_max)
            {
                delete_particle(spawner,j);
                continue;
            }

            // update params
            p->scale    += delta_t*(spawner->effect.scale.rate);
            p->rotation += delta_t*(spawner->effect.angular_vel.rate);
            p->opacity  += delta_t*(spawner->effect.opacity.rate);
            p->vel.x    += delta_t*(spawner->effect.velocity.rate);
            p->vel.y    += delta_t*(spawner->effect.velocity.rate);
            //@TODO: Color

            // update position
            p->pos.x += p->vel.x*delta_t;
            p->pos.y += p->vel.y*delta_t;

            // limit
            p->scale    = MAX(p->scale,0.0);
            p->opacity  = RANGE(p->opacity,0.0,1.0);
        }

        if(spawner->mortal)
        {
            if(!spawner->dead && spawner->life >= spawner->life_max)
            {
                spawner->dead = true;
            }

            if(spawner->dead && list_is_empty(spawner->particle_list))
            {
                delete_spawner(i);
            }
        }

        spawner->spawn_time += delta_t;
        if(spawner->spawn_time >= spawner->spawn_time_max)
        {
            spawner->spawn_time = 0.0;
            emit_particle(spawner);
        }
    }
}

void particles_draw()
{
    for(int i = 0; i < spawner_list->count; ++i)
    {
        ParticleSpawner* spawner = &spawners[i];

        if(spawner->hidden)
            continue;

        for(int j = 0; j < spawner->particle_list->count; ++j)
        {
            Particle* p = &spawner->particles[j];
            gfx_draw_rect_xywh(p->pos.x, p->pos.y, 32.0, 32.0, COLOR2(p->color.x, p->color.y, p->color.z), p->rotation, p->scale, p->opacity, true,spawner->in_world);
        }
    }
}
