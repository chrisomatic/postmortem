#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "math2d.h"
#include "log.h"
#include "lighting.h"

PointLight point_lights[MAX_POINT_LIGHTS] = {0};
int point_light_count = 0;

uint32_t ambient_light = 0x00646464;

int lighting_point_light_add(float x, float y, float r, float g, float b, float radius)
{
    if(point_light_count >= MAX_POINT_LIGHTS)
    {
        LOGW("Too many point lights");
        return -1;
    }

    PointLight* pl = &point_lights[point_light_count++];

    pl->pos.x = x;
    pl->pos.y = y;
    pl->color.x = r;
    pl->color.y = g;
    pl->color.z = b;

    pl->attenuation.x = 1.5;
    pl->attenuation.y = 0.01 * radius;
    pl->attenuation.z = 0.0001 * radius;

    return point_light_count-1;
}

void lighting_point_light_move(int index, float x, float y)
{
    point_lights[index].pos.x = x;
    point_lights[index].pos.y = y;
}

void lighting_point_light_update_color(int index, float r, float g, float b)
{
    point_lights[index].color.x = r;
    point_lights[index].color.y = g;
    point_lights[index].color.z = b;
}

void lighting_point_light_update_radius(int index, float radius)
{
    point_lights[index].attenuation.x = 1.0;
    point_lights[index].attenuation.y = 0.001 / radius;
    point_lights[index].attenuation.z = 0.0001 / radius;
}
