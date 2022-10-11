#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "gfx.h"

#include "rat_math.h"

Matrix IDENTITY_MATRIX = {
    .m = {
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0}
    }
};

void ortho(Matrix* m, float left, float right, float bottom, float top, float znear, float zfar)
{
    memcpy(m,&IDENTITY_MATRIX,sizeof(Matrix));

    m->m[0][0] = 2.0f/(right-left);
    m->m[1][1] = 2.0f/(top-bottom);
    m->m[2][2] = -2.0f/(zfar-znear);
    m->m[0][3] = -(right+left) / (right - left);
    m->m[1][3] = -(top+bottom) / (top-bottom);
    m->m[3][2] = -(zfar+znear) / (zfar-znear);
}

void get_model_transform(Vector3f* pos, Vector3f* rotation, Vector3f* scale, Matrix* model)
{
    Matrix scale_trans            = {0};
    Matrix rotation_trans         = {0};
    Matrix translate_trans        = {0};

    get_scale_transform(&scale_trans, scale);
    get_rotation_transform(&rotation_trans, rotation);
    get_translate_transform(&translate_trans, pos);

    memcpy(model,&IDENTITY_MATRIX,sizeof(Matrix));

    dot_product_mat(*model, translate_trans, model);
    dot_product_mat(*model, rotation_trans,  model);
    dot_product_mat(*model, scale_trans,     model);

}

float calc_angle_rad(float x0, float y0, float x1, float y1)
{
    // printf("x: %f | %f        y: %f | %f\n", x0, x1, y0, y1);
    bool xeq = FEQ(x0, x1);
    bool yeq = FEQ(y0, y1);

    if(xeq && yeq)
    {
        return 0.0f;
    }

    if(xeq)
    {
        if(y1 > y0)
            return PI_OVER_2;
        else
            return PI_OVER_2*3;
    }
    else if(yeq)
    {
        if(x1 > x0)
            return 0;
        else
            return PI;
    }
    else
    {
        if(y1 > y0)
        {
            float opp = y1-y0;
            float adj = x1-x0;
            float a = atanf(opp/adj);
            if(x1 > x0)
            {
                return a;
            }
            return PI+a;
        }

        float opp = x1-x0;
        float adj = y1-y0;
        float a = atanf(opp/adj);
        if(x1 > x0)
        {
            return PI_OVER_2*3-a;
        }
        return PI_OVER_2*3-a;
    }
}

void print_matrix(Matrix* mat)
{
    printf("Matrix:\n");
    for(int i = 0; i < 4; ++i)
    {
        printf("[ %f %f %f %f]"
                ,mat->m[i][0]
                ,mat->m[i][1]
                ,mat->m[i][2]
                ,mat->m[i][3]
              );
        printf("\n");
    }
}

void dot_product_mat(Matrix a, Matrix b, Matrix* result)
{
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 4; ++j)
        {
            result->m[i][j] =
                a.m[i][0] * b.m[0][j] + 
                a.m[i][1] * b.m[1][j] + 
                a.m[i][2] * b.m[2][j] + 
                a.m[i][3] * b.m[3][j];
        }
    }
}


void mult_v2f_mat4(Vector2f* v, Matrix* m, Vector2f* result)
{
    // assuming w is 1.0 for Vector
    result->x = (m->m[0][0] * v->x + m->m[0][1] * v->y + m->m[0][3]);
    result->y = (m->m[1][0] * v->x + m->m[1][1] * v->y + m->m[1][3]);
}


bool onSegment(Vector2f p, Vector2f q, Vector2f r)
{
    if (q.x <= MAX(p.x, r.x) && q.x >= MIN(p.x, r.x) &&
        q.y <= MAX(p.y, r.y) && q.y >= MIN(p.y, r.y))
       return true;
  
    return false;
}
  
int orientation(Vector2f p, Vector2f q, Vector2f r)
{
    int val = (q.y - p.y) * (r.x - q.x) -
              (q.x - p.x) * (r.y - q.y);
  
    if (val == 0) return 0;  // collinear
  
    return (val > 0)? 1: 2; // clock or counterclock wise
}
  
bool doIntersect(Vector2f p1, Vector2f q1, Vector2f p2, Vector2f q2)
{
    // Find the four orientations needed for general and
    // special cases
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);
  
    // General case
    if (o1 != o2 && o3 != o4)
        return true;
  
    // Special Cases
    if (o1 == 0 && onSegment(p1, p2, q1)) return true;
    if (o2 == 0 && onSegment(p1, q2, q1)) return true;
    if (o3 == 0 && onSegment(p2, p1, q2)) return true;
    if (o4 == 0 && onSegment(p2, q1, q2)) return true;
  
    return false; // Doesn't fall in any of the above cases
}

bool is_line_seg_intersecting_rect(LineSeg* l, Rect* r)
{
    // rect segs
    LineSeg r00 = {{r->x,r->y},{r->x+r->w,r->y}}; // top
    LineSeg r01 = {{r->x,r->y},{r->x,r->y+r->h}}; // left
    LineSeg r10 = {{r->x+r->w,r->y},{r->x+r->w,r->y+r->h}}; // right
    LineSeg r11 = {{r->x,r->y+r->h},{r->x+r->w,r->y+r->h}}; // bottom

    bool b00 = doIntersect(r00.a,r00.b,l->a,l->b);//are_line_segs_intersecting(&r00,l);
    if(b00) return true;

    bool b01 = doIntersect(r01.a,r01.b,l->a,l->b);//are_line_segs_intersecting(&r01,l);
    if(b01) return true;

    bool b10 = doIntersect(r10.a,r10.b,l->a,l->b);//are_line_segs_intersecting(&r10,l);
    if(b10) return true;

    bool b11 = doIntersect(r11.a,r11.b,l->a,l->b);//are_line_segs_intersecting(&r11,l);
    if(b11) return true;

    return false;
}

bool are_rects_colliding(Rect* prior_s, Rect* curr_s, Rect* check)
{
    // prior_s is the rect for t-1
    // check rect is the thing we're checking to see if s is intersecting it

    LineSeg s00 = {{prior_s->x, prior_s->y},{curr_s->x, curr_s->y}};
    LineSeg s01 = {{prior_s->x+prior_s->w, prior_s->y},{curr_s->x+curr_s->w, curr_s->y}};
    LineSeg s10 = {{prior_s->x, prior_s->y+prior_s->h},{curr_s->x, curr_s->y+curr_s->h}};
    LineSeg s11 = {{prior_s->x+prior_s->w, prior_s->y+prior_s->h},{curr_s->x+curr_s->w, curr_s->y+curr_s->h}};

    bool b00 = is_line_seg_intersecting_rect(&s00, check);
    if(b00) return true;

    bool b01 = is_line_seg_intersecting_rect(&s01, check);
    if(b01) return true;
    
    bool b10 = is_line_seg_intersecting_rect(&s10, check);
    if(b10) return true;

    bool b11 = is_line_seg_intersecting_rect(&s11, check);
    if(b11) return true;

    return false;
}

void get_scale_transform(Matrix* mat, Vector3f* scale)
{
    memset(mat,0,sizeof(Matrix));

    mat->m[0][0] = scale->x;
    mat->m[1][1] = scale->y;
    mat->m[2][2] = scale->z;
    mat->m[3][3] = 1.0f;
}

void get_rotation_transform(Matrix* mat, Vector3f* rotation)
{
    Matrix rx = {0};
    Matrix ry = {0};
    Matrix rz = {0};

    const float x = RAD(rotation->x);
    const float y = RAD(rotation->y);
    const float z = RAD(rotation->z);

    rx.m[0][0] = 1.0f;
    rx.m[1][1] = cosf(x);
    rx.m[1][2] = -sinf(x);
    rx.m[2][1] = sinf(x);
    rx.m[2][2] = cosf(x);
    rx.m[3][3] = 1.0f;

    ry.m[0][0] = cosf(y);
    ry.m[0][2] = -sinf(y); 
    ry.m[1][1] = 1.0f;  
    ry.m[2][0] = sinf(y);
    ry.m[2][2] = cosf(y);
    ry.m[3][3] = 1.0f;

    rz.m[0][0] = cosf(z);
    rz.m[0][1] = -sinf(z);
    rz.m[1][0] = sinf(z);
    rz.m[1][1] = cosf(z);
    rz.m[2][2] = 1.0f;
    rz.m[3][3] = 1.0f;

    memcpy(mat,&IDENTITY_MATRIX,sizeof(Matrix));

    dot_product_mat(*mat,rz,mat);
    dot_product_mat(*mat,ry,mat);
    dot_product_mat(*mat,rx,mat);
}

void get_translate_transform(Matrix* mat, Vector3f* position)
{
    memset(mat,0,sizeof(Matrix));

    mat->m[0][0] = 1.0f;
    mat->m[0][3] = position->x;
    mat->m[1][1] = 1.0f;
    mat->m[1][3] = position->y;
    mat->m[2][2] = 1.0f;
    mat->m[2][3] = position->z;
    mat->m[3][3] = 1.0f;
}

float vec_magn(Vector3f v)
{
    return sqrt(v.x * v.x + v.y*v.y + v.z*v.z);
}

float vec_dot(Vector3f a, Vector3f b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

float get_angle_between_vectors_rad(Vector3f* a, Vector3f* b)
{
    float ma = vec_magn(*a);
    float mb = vec_magn(*b);

    if(ma == 0.0 || mb == 0.0)
        return 0.0;

    float d  = vec_dot(*a,*b);
    
    float angle = acosf(d/(ma*mb));
    return angle;
}

bool rectangles_colliding(Rect* a, Rect* b)
{
    bool overlap = (
        a->x < (b->x+b->w) && (a->x+a->w) > b->x &&
        a->y < (b->y+b->h) && (a->y+a->h) > b->y
    );

    return overlap;
}