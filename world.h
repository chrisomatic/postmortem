#pragma once

#define WORLD_GRID_SIZE   32  //pixels
#define WORLD_WIDTH  1000
#define WORLD_HEIGHT 1000

void world_init();
void world_update();
void world_draw();

void world_xy_to_grid(float x, float y, int* row, int* col);
void world_grid_to_xy(int row, int col, float* x, float* y);
uint8_t map_get_tile_index(int row, int col);