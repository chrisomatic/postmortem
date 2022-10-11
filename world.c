#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "window.h"
#include "gfx.h"
#include "log.h"
#include "world.h"
#include "camera.h"

int ground_sheet;

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
} WorldMap;

WorldMap map;

static void load_map_file(const char* file_path)
{
    FILE* fp = fopen(file_path,"rb");

    if(!fp)
    {
        LOGW("Map file doesn't exist: %s", file_path);
        return;
    }
    
    //  # id (1), data len (4), version (1), name len (1), name..., rows (2), cols (2), data...

    if(map.name) free(map.name);
    if(map.data) free(map.data);

    memset(&map, 0, sizeof(WorldMap));

    int prop_index = 0;
    int byte_count = 0;

    uint8_t bytes[1024*1024] = {0};
    for(;;)
    {
        int c = fgetc(fp);
        if(c == EOF) break;
        bytes[byte_count++] = (uint8_t)c;
    }

    int idx = 0;

    map.id = bytes[idx++];

    map.data_len = (uint32_t)((bytes[idx+3] << 24) | (bytes[idx+2] << 16) | (bytes[idx+1] << 8) | (bytes[idx] << 0));
    idx += 4;

    map.version = bytes[idx++];

    map.name_len = bytes[idx++];

    map.name = malloc((map.name_len+1)*sizeof(char));
    memcpy(map.name, &bytes[idx], map.name_len*sizeof(char));
    idx += map.name_len;
   
    map.rows = bytes[idx+1] << 8 | bytes[idx] << 0;
    idx += 2;

    map.cols = bytes[idx+1] << 8 | bytes[idx] << 0;
    idx += 2;

    map.data = malloc(map.rows*map.cols*sizeof(uint8_t));
    memcpy(map.data, &bytes[idx], byte_count*sizeof(uint8_t));

    // print map
    LOGI("Map Loaded (%s):", file_path);
    LOGI("  ID: %u",map.id);
    LOGI("  Data Len: %u", map.data_len);
    LOGI("  Version: %u",map.version);
    LOGI("  Name (len: %u): %s",map.name_len, map.name);
    LOGI("  Rows: %u",map.rows);
    LOGI("  Cols: %u",map.cols);
    LOGI("  Data:");
    print_hex(map.data, 100);
}

void world_init()
{
    load_map_file("map/test.map");
    ground_sheet = gfx_load_image_set("img/ground_set.png",32,32);

}

void world_update()
{

}

void world_draw()
{

    Rect r;
    get_camera_rect(&r);
    // printf("%.2f, %.2f, %.2f, %.2f\n", r.x,r.y,r.w,r.h);
    int r1,c1,r2,c2;
    coords_to_map_grid(r.x, r.y, &r1, &c1);
    coords_to_map_grid(r.x+r.w, r.y+r.h, &r2, &c2);

    for(int r = (r1-1); r < (r2+1); ++r)
    {
        for(int c = (c1-1); c < (c2+1); ++c)
        {
            uint8_t index = map_get_tile_index(r,c);
            if(index == 0xFF) continue;

            float x,y;
            map_grid_to_coords(r, c, &x, &y);

            gfx_draw_sub_image(ground_sheet,index,x,y,COLOR_TINT_NONE,1.0,0.0,1.0);
        }
    }

}

uint8_t map_get_tile_index(int row, int col)
{
    if(row >= map.rows || col >= map.cols || row < 0 || col < 0)
        return 0xFF;
    return map.data[row*map.cols+col];
}


void coords_to_map_grid(float x, float y, int* row, int* col)
{
    *row = (int)y/MAP_GRID_PXL_SIZE;
    *col = (int)x/MAP_GRID_PXL_SIZE;
}

void map_grid_to_coords(int row, int col, float* x, float* y)
{
    *x = (float)col*MAP_GRID_PXL_SIZE;
    *y = (float)row*MAP_GRID_PXL_SIZE;
}

void coords_to_world_grid(float x, float y, int* row, int* col)
{
    *row = (int)y/(MAP_GRID_PXL_SIZE*WORLD_GRID_HEIGHT);
    *col = (int)x/(MAP_GRID_PXL_SIZE*WORLD_GRID_WIDTH);
}

void world_grid_to_coords(int row, int col, float* x, float* y)
{
    *x = (float)col*(MAP_GRID_PXL_SIZE*WORLD_GRID_WIDTH);
    *y = (float)row*(MAP_GRID_PXL_SIZE*WORLD_GRID_HEIGHT);
}

bool is_in_camera_view(Rect* r)
{
    Rect r1 = {0};
    get_camera_rect(&r1);
    return rectangles_colliding(&r1, r);
}

bool is_in_camera_view_xywh(float x, float y, float w, float h)
{
    Rect r1 = {0};
    get_camera_rect(&r1);

    Rect r2 = {0};
    r2.x = x;
    r2.y = y;
    r2.w = w;
    r2.h = h;

    return rectangles_colliding(&r1, &r2);
}
