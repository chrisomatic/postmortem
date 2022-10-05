#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "window.h"
#include "gfx.h"
#include "world.h"

int ground_sheet;
uint32_t ground_tiles[32][32] = {0};

void world_init()
{
    ground_sheet = gfx_load_image("img/ground_set.png");

    ground_tiles[10][10] = 1;
    ground_tiles[10][11] = 2;
}

void world_update()
{

}

void world_draw()
{
    for(int i = 0; i < 32; ++i)
    {
        for(int j = 0; j < 32; ++j)
        {
            gfx_draw_sub_image(ground_sheet,ground_tiles[i][j],6,32,32,i*32,j*32,COLOR_TINT_NONE,1.0,0.0,1.0);
        }
    }
}
