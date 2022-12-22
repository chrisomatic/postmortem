#pragma once

#include "particles.h"

extern bool editor_enabled;

void editor_init();
void editor_draw();
ParticleSpawner* editor_get_particle_spawner();
