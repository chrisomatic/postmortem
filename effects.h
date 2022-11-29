#pragma once

#include "particles.h"

#define MAX_PARTICLE_EFFECTS 16

typedef enum
{
    EFFECT_GUN_SMOKE1,
    EFFECT_SPARKS1,
    EFFECT_BLOOD1,
    EFFECT_MELEE1,
    EFFECT_BULLET_CASING,
    EFFECT_FIRE,
    EFFECT_MAX,
} Effect;

typedef struct
{
    int effect_index;
    char* file_name;
} EffectEntry;

extern ParticleEffect particle_effects[MAX_PARTICLE_EFFECTS];

void effects_load_all();
bool effects_save(char* file_path, ParticleEffect* effect);
bool effects_load(char* file_path, ParticleEffect* effect);
