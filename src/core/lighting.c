#include "headers.h"
#include "math2d.h"
#include "log.h"
#include "glist.h"
#include "lighting.h"

PointLight point_lights[MAX_POINT_LIGHTS] = {0};
glist* lighting_list = NULL;

uint32_t ambient_light = 0x00646464;

void lighting_init()
{
    lighting_list = list_create((void*)point_lights,MAX_POINT_LIGHTS,sizeof(PointLight));
    if(lighting_list == NULL)
    {
        LOGE("point light list failed to create");
    }
}
int lighting_point_light_add(float x, float y, float r, float g, float b, float radius, float lifetime)
{
    if(list_is_full(lighting_list))
    {
        LOGW("Too many point lights");
        return -1;
    }

    PointLight* pl = &point_lights[lighting_list->count];

    pl->pos.x = x;
    pl->pos.y = y;
    pl->color.x = r;
    pl->color.y = g;
    pl->color.z = b;
    pl->color_start.x = r;
    pl->color_start.y = g;
    pl->color_start.z = b;
    pl->lifetime_max = lifetime;
    pl->lifetime = 0.0;

    pl->attenuation.x = 0.85;
    pl->attenuation.y = 0.01 / radius;
    pl->attenuation.z = 0.0001 / radius;

    list_add(lighting_list,pl);

    return lighting_list->count-1;
}

void lighting_point_light_update(double delta_t)
{
    for(int i = lighting_list->count -1; i >= 0; --i)
    {
        PointLight* pl = &point_lights[i];
        if(pl->lifetime_max == 0.0)
            continue;

        pl->lifetime += delta_t;

        float factor = 1.0 - (pl->lifetime / pl->lifetime_max);

        float r = pl->color_start.x*factor;
        float g = pl->color_start.y*factor;
        float b = pl->color_start.z*factor;

        lighting_point_light_update_color(i,r,g,b);

        if(pl->lifetime >= pl->lifetime_max)
        {
            lighting_point_light_remove(i);
        }
    }
}

void lighting_point_light_remove(int point_light)
{
    list_remove(lighting_list,point_light);
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
