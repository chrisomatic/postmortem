#pragma once

#define MAP_GRID_PXL_SIZE   32

#define WORLD_GRID_WIDTH    25
#define WORLD_GRID_HEIGHT   25


void world_init();
void world_update();
void world_draw();

uint8_t map_get_tile_index(int row, int col);

void coords_to_map_grid(float x, float y, int* row, int* col);
void map_grid_to_coords(int row, int col, float* x, float* y);
int map_grid_to_index(int row, int col);
void index_to_map_grid(int index, int* row, int* col);

void coords_to_world_grid(float x, float y, int* row, int* col);
void world_grid_to_coords(int row, int col, float* x, float* y);
int world_grid_to_index(int row, int col);
void index_to_world_grid(int index, int* row, int* col);

bool is_in_camera_view(Rect* r);
