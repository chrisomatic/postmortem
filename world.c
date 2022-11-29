#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "window.h"
#include "gfx.h"
#include "log.h"
#include "camera.h"
#include "player.h"
#include "lighting.h"
#include "effects.h"

#include "world.h"

int ground_sheet;

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

    int byte_count = 0;

    uint8_t bytes[1024*1024] = {0};
    for(;;)
    {
        int c = fgetc(fp);
        if(c == EOF) break;
        bytes[byte_count++] = (uint8_t)c;
    }

    fclose(fp);

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

    float width = map.cols*MAP_GRID_PXL_SIZE;
    float height = map.rows*MAP_GRID_PXL_SIZE;
    map.rect.w = width;
    map.rect.h = height;
    map.rect.x = map.rect.w/2.0;
    map.rect.y = map.rect.h/2.0;
}

void world_init()
{
    load_map_file("map/test.map");
    ground_sheet = gfx_load_image("img/ground_set.png", false, false, 32, 32, NULL);

    // world lights
    lighting_point_light_add(100.0,100.0,1.0,0.0,0.0,0.5);
    lighting_point_light_add(200.0,200.0,0.0,1.0,0.0,0.5);
    lighting_point_light_add(300.0,300.0,0.0,0.0,1.0,0.5);

    // test fire pit
    lighting_point_light_add(300.0,500.0,1.0,0.7,0.6,2.0);
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
    coords_to_map_grid(r.x-r.w/2.0, r.y-r.h/2.0, &r1, &c1);
    coords_to_map_grid(r.x+r.w/2.0, r.y+r.h/2.0, &r2, &c2);


#if 0
    // draw tile grid
    if(debug_enabled)
    {
        // float xadj = -1.0*MAP_GRID_PXL_SIZE/2.0;
        // float yadj = -1.0*MAP_GRID_PXL_SIZE/2.0;
        for(int r = (r1-1); r < (r2+1); ++r)
        {
            float x0,y0,x1,y1;
            map_grid_to_coords_tl(r, c1-1, &x0, &y0);
            map_grid_to_coords_tl(r, c2+1, &x1, &y1);
            // x0 += xadj; x1 += xadj;
            // y0 += yadj; y1 += yadj;
            gfx_add_line(x0,y0,x1,y1,0x00FF0000);
        }
        for(int c = (c1-1); c < (c2+1); ++c)
        {
            float x0,y0,x1,y1;
            map_grid_to_coords_tl(r1-1, c, &x0, &y0);
            map_grid_to_coords_tl(r2+1, c, &x1, &y1);
            // x0 += xadj; x1 += xadj;
            // y0 += yadj; y1 += yadj;
            gfx_add_line(x0,y0,x1,y1,0x00FF0000);
        }
    }
#endif

#if 1
    // draw world grid
    if(debug_enabled)
    {
        uint32_t line_color = 0x000000FF;
        int wr1,wc1,wr2,wc2;
        coords_to_world_grid(r.x-r.w/2.0, r.y-r.h/2.0, &wr1, &wc1);
        coords_to_world_grid(r.x+r.w/2.0, r.y+r.h/2.0, &wr2, &wc2);
        for(int r = (wr1-1); r < (wr2+1); ++r)
        {
            float x0,y0,x1,y1;
            world_grid_to_coords_tl(r, wc1-1, &x0, &y0);
            world_grid_to_coords_tl(r, wc2+1, &x1, &y1);
            gfx_add_line(x0,y0,x1,y1,line_color);
        }
        for(int c = (wc1-1); c < (wc2+1); ++c)
        {
            float x0,y0,x1,y1;
            world_grid_to_coords_tl(wr1-1, c, &x0, &y0);
            world_grid_to_coords_tl(wr2+1, c, &x1, &y1);
            gfx_add_line(x0,y0,x1,y1,line_color);
        }
    }
#endif

    gfx_sprite_batch_begin(ground_sheet,true,false,false);

    for(int r = (r1-1); r < (r2+1); ++r)
    {
        for(int c = (c1-1); c < (c2+1); ++c)
        {
            uint8_t index = map_get_tile_index(r,c);
            if(index == 0xFF) continue;
            float x,y;
            map_grid_to_coords(r, c, &x, &y);
            //gfx_draw_image(ground_sheet,index,x,y,COLOR_TINT_NONE,1.0,0.0,1.0, true,true);
            gfx_sprite_batch_add(index,x,y,COLOR_TINT_NONE,1.0,0.0,1.0, true);

#if 0
            if(debug_enabled)
            {
                map_grid_to_coords_tl(r, c, &x, &y);
                gfx_draw_string(x,y,0x00404040,0.1, 0.0, 1.0, true,false,"%d,%d", r, c);
            }
#endif
        }
    }

    gfx_sprite_batch_draw();
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
    *x = (float)(col+0.5)*MAP_GRID_PXL_SIZE;
    *y = (float)(row+0.5)*MAP_GRID_PXL_SIZE;
}

void map_grid_to_coords_tl(int row, int col, float* x, float* y)
{
    *x = (float)(col)*MAP_GRID_PXL_SIZE;
    *y = (float)(row)*MAP_GRID_PXL_SIZE;
}

void map_grid_to_rect(int row, int col, Rect* r)
{
    map_grid_to_coords(row, col, &r->x, &r->y);
    r->w = MAP_GRID_PXL_SIZE;
    r->h = MAP_GRID_PXL_SIZE;
}

int map_grid_to_index(int row, int col)
{
    return row*map.cols+col;
}

bool is_grid_within_radius(int r1, int c1, int r2, int c2, int radius)
{
    return ((ABS(r1 - r2) <= radius) && (ABS(c1 - c2) <= radius));

}

void index_to_map_grid(int index, int* row, int* col)
{
    *row = (index / map.cols);
    *col = (index % map.cols);
}

void map_get_grid_dimensions(int* num_rows, int* num_cols)
{
    *num_rows = (int)map.rows;
    *num_cols = (int)map.cols;
}


void coords_to_world_grid(float x, float y, int* row, int* col)
{
    *row = (int)y/(MAP_GRID_PXL_SIZE*WORLD_GRID_HEIGHT);
    *col = (int)x/(MAP_GRID_PXL_SIZE*WORLD_GRID_WIDTH);
}

void world_grid_to_coords(int row, int col, float* x, float* y)
{
    *x = (float)(col+0.5)*(MAP_GRID_PXL_SIZE*WORLD_GRID_WIDTH);
    *y = (float)(row+0.5)*(MAP_GRID_PXL_SIZE*WORLD_GRID_HEIGHT);
}

void world_grid_to_coords_tl(int row, int col, float* x, float* y)
{
    *x = (float)(col)*(MAP_GRID_PXL_SIZE*WORLD_GRID_WIDTH);
    *y = (float)(row)*(MAP_GRID_PXL_SIZE*WORLD_GRID_HEIGHT);
}

void world_grid_to_rect(int row, int col, Rect* r)
{
    world_grid_to_coords(row, col, &r->x, &r->y);
    r->w = MAP_GRID_PXL_SIZE*WORLD_GRID_WIDTH;
    r->h = MAP_GRID_PXL_SIZE*WORLD_GRID_HEIGHT;
}

int world_grid_to_index(int row, int col)
{
    return row*WORLD_GRID_WIDTH+col;
}

void index_to_world_grid(int index, int* row, int* col)
{
    *row = (index / WORLD_GRID_WIDTH);
    *col = (index % WORLD_GRID_WIDTH);
}

void world_get_grid_dimensions(int* num_rows, int* num_cols)
{
    *num_rows = map.rows/WORLD_GRID_HEIGHT;
    *num_cols = map.cols/WORLD_GRID_WIDTH;
}

bool is_in_world_grid(Rect* pos, int row, int col)
{
    Rect r = {0};
    world_grid_to_rect(row, col, &r);
    return rectangles_colliding(pos, &r);
}
