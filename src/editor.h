#pragma once

#include "particles.h"

extern float minimap_opacity;
extern int minimap_range;
extern int minimap_draw_size;
extern bool editor_enabled;

void editor_init();
void editor_draw();
ParticleSpawner* editor_get_particle_spawner();
