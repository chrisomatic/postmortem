#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "window.h"
#include "gfx.h"
#include "log.h"
#include "world.h"

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
        if(c == EOF || prop_index > 7)
            break;

        bytes[byte_count++] = (uint8_t)c;

        switch(prop_index)
        {
            case 0: // id
                map.id = bytes[0];
                byte_count = 0;
                prop_index++;
                break;
            case 1: // data len
                if(byte_count >= 4)
                {
                    map.data_len = (uint32_t)((bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | (bytes[0] << 0));
                    byte_count = 0;
                    prop_index++;
                }
                break;
            case 2: // version
                map.version = bytes[0];
                byte_count = 0;
                prop_index++;
                break;
            case 3: // name len
                map.name_len = bytes[0];
                byte_count = 0;
                prop_index++;
                break;
            case 4: // name
                if(byte_count >= map.name_len)
                {
                    map.name = malloc(map.name_len*sizeof(char));
                    memcpy(map.name, bytes, byte_count*sizeof(char));
                    map.name[byte_count] = '\0';
                    byte_count = 0;
                    prop_index++;
                }
                break;
            case 5: // rows
                if(byte_count >= 2)
                {
                    map.rows = bytes[1] << 8 | bytes[0] << 0;
                    byte_count = 0;
                    prop_index++;
                }
                break;
            case 6: // cols
                if(byte_count >= 2)
                {
                    map.cols = bytes[1] << 8 | bytes[0] << 0;
                    prop_index++;
                }
                break;
            case 7: // data
                if(byte_count >= (map.rows*map.cols))
                {
                    map.data = malloc(map.rows*map.cols*sizeof(uint8_t));
                    memcpy(map.data, bytes, byte_count*sizeof(uint8_t));
                    byte_count = 0;
                    prop_index++;
                }
                break;
            default:
                break;
        }
    }

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
    for(int i = 0; i < 100; ++i)
    {
        for(int j = 0; j < 100; ++j)
        {
            int index = i*1000+j;
            gfx_draw_sub_image(ground_sheet,map.data[index],j*32,i*32,COLOR_TINT_NONE,1.0,0.0,1.0);
        }
    }
}
