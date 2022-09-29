#pragma once

#include <stdint.h>

typedef struct
{
    float x,y;
} Vector2f;

typedef struct
{
    float x,y,z;
} Vector3f;

typedef struct
{
    Vector2f position;
    Vector2f tex_coord;
} Vertex;
