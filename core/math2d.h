#pragma once

#include <stdbool.h>

#define PI        3.14159265358f
#define PI_OVER_2 1.57079632679f
#define SQRT2OVER2 0.707106781187

#define RAD(x) (((x) * PI) / 180.0f)
#define DEG(x) (((x) * 180.0f) / PI)
#define ABS(x) ((x) < 0 ? -1*(x) : (x))

#define RAND_RANGE(min, max) ((rand()%((max)-(min))) + (min))
#define RAND_FLOAT(min, max) ((float)((double)rand()/(double)(RAND_MAX/((max)-(min))))+(min))
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define RANGE(x,y,z) MIN(MAX((x),(y)),(z))

#define SQ(x) ((x)*(x))
#define FEQ(a, b) (ABS(a-b) <= 0.00001f)

#define BOUND(a,l,u)    MAX(MIN(a,u),l)
#define IS_BIT_SET(x,b) (((x) & (b)) == (b))

#define IS_RECT_EMPTY(r) (FEQ((r)->w,0.0) || FEQ((r)->h,0.0))

typedef struct
{
    float x,y;
} Vector2f;

typedef struct
{
    int x,y;
} Vector2i;

typedef struct
{
    Vector2f a;
    Vector2f b;
} LineSeg;

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
    Vector2f pos;
    Vector3f color;
} LinePoint;

typedef struct
{
    float m[4][4];
} Matrix;

typedef struct
{
    float x,y;  //center
    float w,h;
} Rect;

typedef struct
{
    float x[4];
    float y[4];
} RectXY;


extern Matrix IDENTITY_MATRIX;

void get_model_transform(Vector3f* pos, Vector3f* rotation, Vector3f* scale, Matrix* model);
void ortho(Matrix* m, float left, float right, float bottom, float top, float znear, float zfar);
float calc_angle_deg(float x0, float y0, float x1, float y1);
float calc_angle_rad(float x0, float y0, float x1, float y1);
float dist_squared(float x0, float y0, float x1, float y1);
float dist(float x0, float y0, float x1, float y1);
float magn(Vector2f v);
void normalize(Vector2f* v);
void get_scale_transform(Matrix* mat, Vector3f* scale);
void get_rotation_transform(Matrix* mat, Vector3f* rotation);
void get_translate_transform(Matrix* mat, Vector3f* position);
void dot_product_mat(Matrix a, Matrix b, Matrix* result);
void mult_mat4(Matrix* m1, Matrix* m2, Matrix* result);
void print_matrix(Matrix* mat);

float get_angle_between_vectors_rad(Vector3f* a, Vector3f* b);

bool are_line_segs_intersecting(LineSeg* l1, LineSeg* l2);
bool is_line_seg_intersecting_rect(LineSeg* l, Rect* r);
bool are_rects_colliding(Rect* prior_s, Rect* curr_s, Rect* check);
bool rectangles_colliding(Rect* a, Rect* b);

int angle_sector(float angle, int num_sectors);
float rangef(float arr[], int n, float* fmin, float* fmax);

void rotate_rect(Rect* in, float rotation, float rotation_x, float rotation_y, RectXY* out);
void rect_to_rectxy(Rect* in, RectXY* out);
void rectxy_to_rect(RectXY* in, Rect* out);
void print_rect(Rect* r);
void print_rectxy(RectXY* r);

float lerp(float a, float b, float t);
Vector2f lerp2f(Vector2f* a, Vector2f* b, float t);

float rand_float_between(float lower, float upper);
