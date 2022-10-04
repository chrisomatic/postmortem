#pragma once

#define PI        3.14159265358f
#define PI_OVER_2 1.57079632679f

#define RAD(x) (((x) * PI) / 180.0f)
#define DEG(x) (((x) * 180.0f) / PI)
#define ABS(x) ((x) < 0 ? -1*(x) : (x))

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define SQ(x) ((x)*(x))

#define BOUND(a,l,u)    MAX(MIN(a,u),l)

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

typedef struct 
{
    float m[4][4];
} Matrix;

void ortho(Matrix* m, float left, float right, float bottom, float top);
void get_model_transform(Vector3f* pos, Vector3f* rotation, Vector3f* scale, Matrix* model);
void print_matrix(Matrix* mat);
