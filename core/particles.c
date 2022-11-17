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
#include "lighting.h"
#include "particles.h"

static ParticleSpawner spawners[MAX_PARTICLE_SPAWNERS] = {0};
static glist* spawner_list;
static int global_id_count = 0;
static int particles_image;

static void print_particle(Particle* p)
{
    printf("===================\n");
    printf("Particle:\n");
    printf("  position: %f %f\n", p->pos.x, p->pos.y);
    printf("  velocity: %f %f\n", p->vel.x, p->vel.y);
    printf("  color: %08X\n", p->color);
    printf("  scale: %f\n", p->scale);
    printf("  opacity: %f\n", p->opacity);
    printf("  life: %f\n", p->life);
    printf("  life_max: %f\n", p->life_max);
    printf("===================\n");
}

static void emit_particle(ParticleSpawner* s)
{
    if(list_is_full(s->particle_list))
    {
        LOGW("Too many particles!");
        return;
    }
    Particle* p = &s->particles[s->particle_list->count++];

    float x_offset = RAND_FLOAT(-s->effect.spawn_radius,s->effect.spawn_radius);
    float y_offset = RAND_FLOAT(-s->effect.spawn_radius,s->effect.spawn_radius);

    p->pos.x = s->pos.x + x_offset;
    p->pos.y = s->pos.y + y_offset;
    p->vel.x = RAND_FLOAT(s->effect.velocity_x.init_min, s->effect.velocity_x.init_max);
    p->vel.y = RAND_FLOAT(s->effect.velocity_y.init_min, s->effect.velocity_y.init_max);
    p->color = s->effect.color1;
    p->rotation = RAND_FLOAT(s->effect.rotation_init_min, s->effect.rotation_init_max);
    p->angular_vel = RAND_FLOAT(s->effect.angular_vel.init_min, s->effect.angular_vel.init_max);
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
    particles_image = gfx_load_image("img/particles.png", false, true, 32, 32, NULL);

}

ParticleSpawner* particles_spawn_effect(float x, float y, ParticleEffect* effect, float lifetime, bool in_world, bool hidden)
{
    if(list_is_full(spawner_list))
    {
        LOGW("Too many spawners!");
        return NULL;
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
    spawner->mortal = (lifetime > 0.0);
    spawner->life = 0.0;
    spawner->life_max = lifetime;

    return (spawner);
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
            p->angular_vel += delta_t*(spawner->effect.angular_vel.rate);
            p->opacity  += delta_t*(spawner->effect.opacity.rate);
            p->vel.x    += delta_t*(spawner->effect.velocity_x.rate);
            p->vel.y    += delta_t*(spawner->effect.velocity_y.rate);

            float r1,g1,b1;
            float r2,g2,b2;
            float r3,g3,b3;

            gfx_color2floats(spawner->effect.color1, &r1, &g1, &b1);
            gfx_color2floats(spawner->effect.color2, &r2, &g2, &b2);
            gfx_color2floats(spawner->effect.color3, &r3, &g3, &b3);

            float life_factor = (p->life / p->life_max);

            float r = life_factor >= 0.5 ?  lerp(r2,r3,(life_factor-0.5)*2.0) : lerp(r1,r2,life_factor*2.0);
            float g = life_factor >= 0.5 ?  lerp(g2,g3,(life_factor-0.5)*2.0) : lerp(g1,g2,life_factor*2.0);
            float b = life_factor >= 0.5 ?  lerp(b2,b3,(life_factor-0.5)*2.0) : lerp(b1,b2,life_factor*2.0);

            p->color = COLOR2(r,g,b);
            p->rotation += p->angular_vel*delta_t;

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
            int burst_count = spawner->effect.burst_count_min;
            if(spawner->effect.burst_count_min < spawner->effect.burst_count_max)
                burst_count = RAND_RANGE(spawner->effect.burst_count_min,spawner->effect.burst_count_max);

            burst_count = MAX(0, burst_count);
            for(int i = 0; i < burst_count; ++i)
            {
                emit_particle(spawner);
            }
        }
    }
}

void particles_draw_spawner(ParticleSpawner* spawner)
{
    for(int j = 0; j < spawner->particle_list->count; ++j)
    {
        Particle* p = &spawner->particles[j];
        if(spawner->effect.use_sprite)
        {
            gfx_draw_image(particles_image, spawner->effect.sprite_index, p->pos.x, p->pos.y, ambient_light,p->scale,p->rotation,p->opacity,false);
        }
        else
        {
            gfx_draw_rect_xywh(p->pos.x, p->pos.y, 32.0, 32.0, p->color, p->rotation, p->scale, p->opacity, true,spawner->in_world);
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

        particles_draw_spawner(spawner);

    }
}
