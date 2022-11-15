#pragma once

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

void particles_spawn_effect(float x, float y, ParticleEffect* effect, bool in_world);
void particles_update(double delta_t);
void particles_draw();
