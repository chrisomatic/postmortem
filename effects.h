#pragma once

#include "particles.h"

#define MAX_PARTICLE_EFFECTS 16

extern ParticleEffect particle_effects[MAX_PARTICLE_EFFECTS];

void effects_load_all();
bool effects_save(char* file_path, ParticleEffect* effect);
bool effects_load(char* file_path, ParticleEffect* effect);
