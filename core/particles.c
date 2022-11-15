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
#include "particles.h"

#define MAX_PARTICLE_SPAWNERS 100
#define MAX_PARTICLES_PER_SPAWNER 100

typedef struct
{
    Vector2f pos;
    Vector2f vel;
    Vector3f color;
    float rotation;
    float scale;
    float opacity;
    float life;
    float life_max;
} Particle;

typedef struct
{
    Vector2f pos;
    ParticleEffect effect;
    float life;
    float life_max;
    float spawn_time;
    float spawn_time_max;
    Particle particles[MAX_PARTICLES_PER_SPAWNER];
    int num_particles;
    bool mortal;
    bool in_world;
} ParticleSpawner;

static ParticleSpawner spawners[MAX_PARTICLE_SPAWNERS] = {0};
static int num_spawners;

static void emit_particle(ParticleSpawner* s)
{

}

static void delete_particle(int index)
{

}

static void delete_spawner(int index)
{

}

void particles_spawn_effect(float x, float y, ParticleEffect* effect, bool in_world)
{
    if(num_spawners >= MAX_PARTICLE_SPAWNERS)
    {
        LOGW("Too many spawners!");
        return;
    }

    ParticleSpawner* spawner = &spawners[num_spawners++];

    spawner->pos.x = x;
    spawner->pos.y = y;
    spawner->in_world = in_world;

    // effect
    spawner->effect.life.init_min = 1.0;
    spawner->effect.life.init_max = 2.0;
    spawner->effect.life.rate     = 1.0;

    spawner->effect.scale.init_min = 1.0;
    spawner->effect.scale.init_max = 2.0;
    spawner->effect.scale.rate     = 0.4;

    spawner->effect.velocity.init_min = 1.0;
    spawner->effect.velocity.init_max = 2.0;
    spawner->effect.velocity.rate     = 0.4;

    spawner->effect.opacity.init_min = 1.0;
    spawner->effect.opacity.init_max = 2.0;
    spawner->effect.opacity.rate     = 0.4;

    spawner->effect.angular_vel.init_min = 0.0;
    spawner->effect.angular_vel.init_max = 2.0;
    spawner->effect.angular_vel.rate     = 0.4;
}

void particles_update(double delta_t)
{
    for(int i = num_spawners-1; i >= 0; --i)
    {
        ParticleSpawner* spawner = &spawners[i];

        for(int j = spawner->num_particles-1; j >= 0; --j)
        {
            // update each particle
            Particle* p = &spawner->particles[j];

            // update life
            p->life += delta_t*(spawner->effect.life.rate);

            // check for death
            if(p->life >= p->life_max)
            {
                delete_particle(j);
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
        }

        if(spawner->mortal)
        {
            if(spawner->life >= spawner->life_max)
            {
                delete_spawner(i);
            }
        }

        spawner->spawn_time += delta_t;
        while(spawner->spawn_time >= spawner->spawn_time_max)
        {
            spawner->spawn_time -= delta_t;
            emit_particle(spawner);
        }
    }
}

void particles_draw()
{

    for(int i = 0; i < num_spawners; ++i)
    {
        ParticleSpawner* spawner = &spawners[i];

        for(int j = 0; j < spawner->num_particles; ++j)
        {
            Particle* p = &spawner->particles[j];

            //gfx_draw_rect_xywh(p->pos.x, p->pos.y, p->float w, float h, uint32_t color, float scale, float opacity, bool filled, bool in_world);
        }
    }
}
