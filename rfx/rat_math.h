#pragma once

#define PI        3.14159265358f
#define PI_OVER_2 1.57079632679f

#define RAD(x) (((x) * PI) / 180.0f)
#define DEG(x) (((x) * 180.0f) / PI)
#define ABS(x) ((x) < 0 ? -1*(x) : (x))

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define RANGE(x,y,z) MIN(MAX((x),(y)),(z))

#define SQ(x) ((x)*(x))
#define FEQ(a, b) (ABS(a-b) <= 0.00001f)

#define BOUND(a,l,u)    MAX(MIN(a,u),l)
#define IS_BIT_SET(x,b) (((x) & (b)) == (b))

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

extern Matrix IDENTITY_MATRIX;

void get_model_transform(Vector3f* pos, Vector3f* rotation, Vector3f* scale, Matrix* model);
void ortho(Matrix* m, float left, float right, float bottom, float top, float znear, float zfar);
float calc_angle_rad(float x0, float y0, float x1, float y1);
void get_scale_transform(Matrix* mat, Vector3f* scale);
void get_rotation_transform(Matrix* mat, Vector3f* rotation);
void get_translate_transform(Matrix* mat, Vector3f* position);
void dot_product_mat(Matrix a, Matrix b, Matrix* result);
void print_matrix(Matrix* mat);
float get_angle_between_vectors_rad(Vector3f* a, Vector3f* b);

