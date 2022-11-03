#pragma once

#define MAX_POINT_LIGHTS 16

typedef struct
{
    Vector2f pos;
    Vector3f color;
    Vector3f attenuation;
} PointLight;

extern PointLight point_lights[MAX_POINT_LIGHTS];
extern int point_light_count;

int lighting_point_light_add(float x, float y, float r, float g, float b, float radius);
void lighting_point_light_move(int index, float x, float y);
void lighting_point_light_update_color(int index, float r, float g, float b);
void lighting_point_light_update_radius(int index, float radius);
