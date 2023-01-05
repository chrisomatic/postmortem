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
#include "camera.h"
#include "particles.h"

#define PARTICLES_EFFECT_VERSION 1

ParticleSpawner spawners[MAX_PARTICLE_SPAWNERS] = {0};
glist* spawner_list;

static int global_id_count = 0;
int particles_image;

static void emit_particle(ParticleSpawner* s)
{
    if(list_is_full(s->particle_list))
    {
        LOGW("Too many particles!");
        return;
    }
    Particle* p = &s->particles[s->particle_list->count++];

    float angle = RAD((float)RAND_RANGE(0,360));
    float mag = RAND_FLOAT(0.0,s->effect.spawn_radius_max) + s->effect.spawn_radius_min;

    float x_offset = mag*cos(angle);
    float y_offset = mag*sin(angle);

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

void print_particle(Particle* p)
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

void print_particle_effect(ParticleEffect* e)
{
    printf("===================\n");
    printf("Particle Effect:\n");
    printf("  version: %u\n", e->version);
    printf("  life: %f %f, %f\n", e->life.init_min, e->life.init_max, e->life.rate);
    printf("  scale: %f %f, %f\n", e->scale.init_min, e->scale.init_max, e->scale.rate);
    printf("  velocity_x: %f %f, %f\n", e->velocity_x.init_min, e->velocity_x.init_max, e->velocity_x.rate);
    printf("  velocity_y: %f %f, %f\n", e->velocity_y.init_min, e->velocity_y.init_max, e->velocity_y.rate);
    printf("  opacity: %f %f, %f\n", e->opacity.init_min, e->opacity.init_max, e->opacity.rate);
    printf("  angular_vel: %f %f, %f\n", e->angular_vel.init_min, e->angular_vel.init_max, e->angular_vel.rate);
    printf("  color1: %08X\n", e->color1);
    printf("  color2: %08X\n", e->color2);
    printf("  color3: %08X\n", e->color3);
    printf("  spawn_radius: %f %f\n", e->spawn_radius_min, e->spawn_radius_max);
    printf("  rotation: %f %f\n", e->rotation_init_min, e->rotation_init_max);
    printf("  spawn_time: %f %f\n", e->spawn_time_min, e->spawn_time_max);
    printf("  burst_count: %d %d\n", e->burst_count_min, e->burst_count_max);
    printf("  sprite_index: %d\n", e->sprite_index);
    printf("  use_sprite: %s\n", e->use_sprite ? "true" : "false");
    printf("===================\n");
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
    particles_image = gfx_load_image("src/img/particles.png", false, true, 32, 32);
}

ParticleSpawner* particles_spawn_effect(float x, float y, ParticleEffect* effect, float lifetime, bool in_world, bool hidden)
{
    if(list_is_full(spawner_list))
    {
        LOGW("Too many spawners!");
        return NULL;
    }

    ParticleSpawner* spawner = &spawners[spawner_list->count++];

    memset(spawner,0,sizeof(ParticleSpawner));

    spawner->particle_list = list_create(spawner->particles,MAX_PARTICLES_PER_SPAWNER,sizeof(Particle));

    if(effect)
    {
        memcpy(&spawner->effect, effect, sizeof(ParticleEffect));
    }

    spawner->effect.version = PARTICLES_EFFECT_VERSION;
    spawner->id = get_id();
    spawner->pos.x = x;
    spawner->pos.y = y;
    spawner->in_world = in_world;
    spawner->spawn_time_max = RAND_FLOAT(spawner->effect.spawn_time_min, spawner->effect.spawn_time_max);
    spawner->spawn_time = spawner->spawn_time_max;
    spawner->hidden = hidden;
    spawner->mortal = (lifetime > 0.0);
    spawner->life = 0.0;
    spawner->life_max = lifetime;
    spawner->dead = false;

    return (spawner);
}

void particles_update(double delta_t)
{
    for(int i = spawner_list->count-1; i >= 0; --i)
    {
        ParticleSpawner* spawner = &spawners[i];

        if(spawner->hidden)
            continue;

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
            spawner->life += delta_t;

            if(!spawner->dead && spawner->life >= spawner->life_max)
            {
                spawner->dead = true;
            }

            if(spawner->dead && list_is_empty(spawner->particle_list))
            {
                delete_spawner(i);
            }
        }

        if(!spawner->dead)
        {
            spawner->spawn_time += delta_t;
            if(spawner->spawn_time >= spawner->spawn_time_max)
            {
                spawner->spawn_time = 0.0;
                spawner->spawn_time_max = RAND_FLOAT(spawner->effect.spawn_time_min, spawner->effect.spawn_time_max);

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
}

void particles_draw_spawner(ParticleSpawner* spawner, bool ignore_light, bool add_to_existing_batch)
{
    if(spawner == NULL) return;

    if(spawner->effect.use_sprite)
    {
        if(!add_to_existing_batch) gfx_sprite_batch_begin(spawner->in_world);
            
        for(int j = 0; j < spawner->particle_list->count; ++j)
        {
            Particle* p = &spawner->particles[j];
            gfx_sprite_batch_add(spawner->effect.img_index, spawner->effect.sprite_index, p->pos.x, p->pos.y, p->color, p->scale, p->rotation, p->opacity, false,ignore_light,spawner->effect.blend_additive);
        }
        if(!add_to_existing_batch) gfx_sprite_batch_draw();
    }
    else
    {
        for(int j = 0; j < spawner->particle_list->count; ++j)
        {
            Particle* p = &spawner->particles[j];
            gfx_draw_rect_xywh(p->pos.x, p->pos.y, 32.0, 32.0, p->color, p->rotation, p->scale, p->opacity, true,spawner->in_world);
        }
    }
}

bool particles_is_spawner_in_camera_view(ParticleSpawner* s)
{
    for(int j = 0; j < s->particle_list->count; ++j)
    {
        Particle* p = &s->particles[j];
        Rect r = {p->pos.x, p->pos.y, 32.0*p->scale,32.0*p->scale};
        if(is_in_camera_view(&r))
        {
            return true;
        }
    }
    return false;
}

void particles_draw()
{
    for(int i = 0; i < spawner_list->count; ++i)
    {
        ParticleSpawner* spawner = &spawners[i];

        if(spawner->hidden)
            continue;

        particles_draw_spawner(spawner, false, false);

    }
}
