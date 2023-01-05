#pragma once

#include "math2d.h"

#define MAP_GRID_PXL_SIZE   32

// units are map grids, should divide evenly with map.rows (height) and map.cols (width)
// in other words, each world grid space is WORLD_GRID_WIDTH tiles wide and WORLD_GRID_HEIGHT tiles high
// num world grid rows = map.rows / WORLD_GRID_HEIGHT
// num world grid cols = map.cols / WORLD_GRID_WIDTH
// #define WORLD_GRID_WIDTH    25
// #define WORLD_GRID_HEIGHT   25
#define WORLD_GRID_WIDTH    4
#define WORLD_GRID_HEIGHT   4


#define MAP_GRID_ROWS_MAX   1000
#define MAP_GRID_COLS_MAX   1000

#define WORLD_GRID_ROWS_MAX (MAP_GRID_ROWS_MAX / WORLD_GRID_HEIGHT)
#define WORLD_GRID_COLS_MAX (MAP_GRID_COLS_MAX / WORLD_GRID_WIDTH)

#define WORLD_GRID_ROWS (map.rows / WORLD_GRID_HEIGHT)
#define WORLD_GRID_COLS (map.cols / WORLD_GRID_WIDTH)

typedef struct
{
    uint8_t id;
    uint32_t data_len;
    uint8_t version;
    uint8_t name_len;
    char* name;
    uint16_t rows;
    uint16_t cols;
    uint8_t* data;
    Rect rect;  //pixels
} WorldMap;


extern WorldMap map;
extern int ground_sheet;

void world_init();
void world_update();
void world_draw();

uint8_t map_get_tile_index(int row, int col);


void coords_to_map_grid(float x, float y, int* row, int* col);
void map_grid_to_coords(int row, int col, float* x, float* y);
void map_grid_to_coords_tl(int row, int col, float* x, float* y);
void map_grid_to_rect(int row, int col, Rect* r);
int map_grid_to_index(int row, int col);
void index_to_map_grid(int index, int* row, int* col);
void map_get_grid_dimensions(int* num_rows, int* num_cols);
bool is_grid_within_radius(int r1, int c1, int r2, int c2, int radius);

void coords_to_world_grid(float x, float y, int* row, int* col);
void world_grid_to_coords(int row, int col, float* x, float* y);
void world_grid_to_coords_tl(int row, int col, float* x, float* y);
void world_grid_to_rect(int row, int col, Rect* r);
int world_grid_to_index(int row, int col);
void index_to_world_grid(int index, int* row, int* col);
void world_get_grid_dimensions(int* num_rows, int* num_cols);
void world_grid_to_map_grid(int row, int col, int* mrow, int* mcol);


bool is_in_world_grid(Rect* pos, int row, int col);
