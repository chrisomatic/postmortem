#pragma once

#define MAX_PARTICLE_SPAWNERS 100
#define MAX_PARTICLES_PER_SPAWNER 100

typedef struct
{
    float init_min;
    float init_max;
    float rate;
} ParticleParam;

typedef struct
{
    ParticleParam life;
    ParticleParam scale;
    ParticleParam velocity;
    ParticleParam opacity;
    ParticleParam angular_vel;

    Vector2f spawn_pos_offset_min;
    Vector2f spawn_pos_offset_max;
    float spawn_time_min;
    float spawn_time_max;
    int burst_count_min;
    int burst_count_max;
    int sprite_index;
} ParticleEffect;

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
    int id;
    Vector2f pos;
    ParticleEffect effect;
    float life;
    float life_max;
    float spawn_time;
    float spawn_time_max;
    Particle particles[MAX_PARTICLES_PER_SPAWNER];
    glist* particle_list;
    bool mortal;
    bool in_world;
    bool dead;
    bool hidden;
} ParticleSpawner;

void particles_init();
int particles_spawn_effect(float x, float y, ParticleEffect* effect, bool in_world, bool hidden);
void particles_update(double delta_t);
void particles_show_spawner(int id, bool show);
void particles_draw();
ParticleSpawner* particles_get_spawner(int id);
