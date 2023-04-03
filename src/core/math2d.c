#include "headers.h"
#include "gfx.h"

#include "math2d.h"

Matrix IDENTITY_MATRIX = {
    .m = {
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0}
    }
};

float lerp(float a, float b, float t)
{
    t = RANGE(t,0.0,1.0);
    float r = (1.0-t)*a+(t*b);
    return r;
}

Vector2f lerp2f(Vector2f* a, Vector2f* b, float t)
{
    float rx = lerp(a->x,b->x,t);
    float ry = lerp(a->y,b->y,t);

    Vector2f r = {rx,ry};
    return r;
}

Vector3f lerp3f(Vector3f* a, Vector3f* b, float t)
{
    float rx = lerp(a->x,b->x,t);
    float ry = lerp(a->y,b->y,t);
    float rz = lerp(a->z,b->z,t);

    Vector3f r = {rx,ry,rz};
    return r;
}

void ortho(Matrix* m, float left, float right, float bottom, float top, float znear, float zfar)
{
    memcpy(m,&IDENTITY_MATRIX,sizeof(Matrix));

    m->m[0][0] = 2.0f/(right-left);
    m->m[1][1] = 2.0f/(top-bottom);
    m->m[2][2] = -2.0f/(zfar-znear);
    m->m[0][3] = -(right+left) / (right - left);
    m->m[1][3] = -(top+bottom) / (top-bottom);
    m->m[3][2] = (zfar+znear) / (zfar-znear);
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
float calc_angle_deg(float x0, float y0, float x1, float y1)
{
    return DEG(calc_angle_rad(x0,y0,x1,y1));
}


float calc_angle_rad(float x0, float y0, float x1, float y1)
{

    // printf("%.2f, %.2f   %.2f, %.2f", x0, y0, x1, y1);
    y0 = 10000.0-y0;
    y1 = 10000.0-y1;
    // printf("--->    %.2f, %.2f   %.2f, %.2f\n", x0, y0, x1, y1);
    bool xeq = FEQ(x0, x1);
    bool yeq = FEQ(y0, y1);

    if(xeq && yeq)
    {
        // printf("ret: %d\n",__LINE__);
        return 0.0f;
    }

    if(xeq)
    {
        if(y1 > y0)
        {
            // printf("ret: %d\n",__LINE__);
            return PI_OVER_2;
        }
        else
        {
            // printf("ret: %d\n",__LINE__);
            return PI_OVER_2*3;
        }
    }
    else if(yeq)
    {
        if(x1 > x0)
        {
            // printf("ret: %d\n",__LINE__);
            return 0;
        }
        else
        {
            // printf("ret: %d\n",__LINE__);
            return PI;
        }
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
                // printf("opp: %.2f, adj: %.2f\n", opp, adj);
                // printf("ret: %d\n",__LINE__);
                return a;
            }
            // printf("ret: %d\n",__LINE__);
            return PI+a;
        }

        float opp = x1-x0;
        float adj = y1-y0;
        float a = atanf(opp/adj);
        if(x1 > x0)
        {
            // printf("opp: %.2f, adj: %.2f\n", opp, adj);
            // printf("ret: %d\n",__LINE__);
            return PI_OVER_2*3-a;
        }
        // printf("ret: %d\n",__LINE__);
        return PI_OVER_2*3-a;
    }
}

float dist_squared(float x0, float y0, float x1, float y1)
{
    Vector2f s = {
        x1 - x0,
        y1 - y0
    };

    float d_sq = (s.x * s.x) + (s.y * s.y);
    return d_sq;
}

float dist(float x0, float y0, float x1, float y1)
{
    float d = sqrt(dist_squared(x0,y0,x1,y1));
    return d;
}

static float Q_rsqrt(float number)
{
	union {
		float    f;
		uint32_t i;
	} conv = { .f = number };
	conv.i  = 0x5f3759df - (conv.i >> 1);
	conv.f *= 1.5F - (number * 0.5F * conv.f * conv.f);
	return conv.f;
}

float magn_fast(Vector2f v)
{
    return (0.96*v.x + 0.40*v.y);
}

float magn(Vector2f v)
{
    return sqrt(v.x * v.x + v.y*v.y);
}

void normalize(Vector2f* v)
{
    float magn_squared = v->x*v->x + v->y*v->y;
    float r = Q_rsqrt(magn_squared);

    v->x *= r;
    v->y *= r;
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
    float x0 = r->x - r->w/2.0;
    float x1 = r->x + r->w/2.0;
    float y0 = r->y - r->h/2.0;
    float y1 = r->y + r->h/2.0;
    // rect segs
    LineSeg r00 = {{x0,y0},{x1,y0}}; // top
    LineSeg r01 = {{x0,y0},{x0,y1}}; // left
    LineSeg r10 = {{x1,y0},{x1,y1}}; // right
    LineSeg r11 = {{x0,y1},{x1,y1}}; // bottom

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


bool are_line_segs_intersecting_rect(LineSeg* segs, int seg_count, Rect* check)
{
    for(int i = 0; i < seg_count; ++i)
    {
        if(is_line_seg_intersecting_rect(&segs[i], check))
        {
            return true;
        }
    }
    return false;
}


void rects_to_ling_segs(Rect* a, Rect* b, LineSeg out[5])
{
    float px = a->x;
    float py = a->y;
    float px0 = a->x - a->w/2.0;
    float px1 = a->x + a->w/2.0;
    float py0 = a->y - a->h/2.0;
    float py1 = a->y + a->h/2.0;

    float cx = b->x;
    float cy = b->y;
    float cx0 = b->x - b->w/2.0;
    float cx1 = b->x + b->w/2.0;
    float cy0 = b->y - b->h/2.0;
    float cy1 = b->y + b->h/2.0;

    LineSeg sc =  {{px, py},{cx, cy}};
    LineSeg s00 = {{px0, py0},{cx0, cy0}};
    LineSeg s01 = {{px1, py0},{cx1, cy0}};
    LineSeg s10 = {{px0, py1},{cx0, cy1}};
    LineSeg s11 = {{px1, py1},{cx1, cy1}};

    out[0] = sc;
    out[1] = s00;
    out[2] = s01;
    out[3] = s10;
    out[4] = s11;
}


bool are_rects_colliding(Rect* prior_s, Rect* curr_s, Rect* check)
{
    LineSeg segs[5] = {0};
    rects_to_ling_segs(prior_s, curr_s, segs);
    return are_line_segs_intersecting_rect(segs, 5, check);
}

bool rectangles_colliding(Rect* a, Rect* b)
{
    bool isOverlapping(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

    int x1 = a->x-(a->w/2.0);
    int y1 = a->y-(a->h/2.0);
    int w1 = a->w;
    int h1 = a->h;

    int x2 = b->x-(b->w/2.0);
    int y2 = b->y-(b->h/2.0);
    int w2 = b->w;
    int h2 = b->h;

    // Check if one rectangle is on the left side of the other
    if (x1 + w1 <= x2 || x2 + w2 <= x1) {
        return false;
    }

    // Check if one rectangle is above the other
    if (y1 + h1 <= y2 || y2 + h2 <= y1) {
        return false;
    }

    // If neither of the above checks are true, then the
    // two rectangles must be overlapping.
    return true;
}

bool rectangles_colliding2(Rect* a, Rect* b)
{
    bool isOverlapping(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

    int x1 = a->x;
    int y1 = a->y;
    int w1 = a->w;
    int h1 = a->h;

    int x2 = b->x;
    int y2 = b->y;
    int w2 = b->w;
    int h2 = b->h;

    // Check if one rectangle is on the left side of the other
    if (x1 + w1 <= x2 || x2 + w2 <= x1) {
        return false;
    }

    // Check if one rectangle is above the other
    if (y1 + h1 <= y2 || y2 + h2 <= y1) {
        return false;
    }

    // If neither of the above checks are true, then the
    // two rectangles must be overlapping.
    return true;
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

float vec_dot3(Vector3f a, Vector3f b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

float vec_dot(Vector2f a, Vector2f b)
{
    return (a.x * b.x) + (a.y * b.y);
}

float get_angle_between_vectors_rad(Vector3f* a, Vector3f* b)
{
    float ma = vec_magn(*a);
    float mb = vec_magn(*b);

    if(ma == 0.0 || mb == 0.0)
        return 0.0;

    float d  = vec_dot3(*a,*b);
    
    float angle = acosf(d/(ma*mb));
    return angle;
}

// angle_deg: 0-360
int angle_sector(float angle_deg, int num_sectors)
{
    if(num_sectors <= 1) return 0;
    float sector_range = 360.0 / num_sectors;
    int sector = (angle_deg) / sector_range;
    // printf("Num sectors: %d (%.1f), angle: %.1f, sector: %d\n", num_sectors, sector_range, angle, sector);
    return sector;
}

Vector2f angle_sector_range(int num_sectors, int sector)
{
    Vector2f angle_range = {0};
    float sector_range = 360.0 / num_sectors;

    angle_range.x = sector * sector_range;
    angle_range.y = (sector+1) * sector_range;
    return angle_range;
}

float rangef(float arr[], int n, float* fmin, float* fmax)
{
    *fmin = arr[0];
    *fmax = arr[0];
    for(int i = 0; i < n; ++i)
    {
        if(arr[i] < *fmin)
            *fmin = arr[i];
        if(arr[i] > *fmax)
            *fmax = arr[i];
    }
    return (*fmax-*fmin);
}

void rotate_rect(Rect* in, float rotation, float rotation_x, float rotation_y, RectXY* out)
{
    float x0 = in->x - in->w/2.0;
    float x1 = in->x + in->w/2.0;
    float y0 = in->y - in->h/2.0;
    float y1 = in->y + in->h/2.0;
    // top left, top right, bottom right, bottom left
    float xcoords[4] = {x0, x1, x1, x0};
    float ycoords[4] = {y0, y0, y1, y1};

    float a = RAD(360-rotation);
    float xa = cos(a);
    float ya = sin(a);

    for(int i = 0; i < 4; ++i)
    {
        out->x[i] = (xa * (xcoords[i] - rotation_x) - ya * (ycoords[i] - rotation_y)) + rotation_x;
        out->y[i] = (ya * (xcoords[i] - rotation_x) - xa * (ycoords[i] - rotation_y)) + rotation_y;
    }
}

void rect_to_rectxy(Rect* in, RectXY* out)
{
    float x0 = in->x - in->w/2.0;
    float x1 = in->x + in->w/2.0;
    float y0 = in->y - in->h/2.0;
    float y1 = in->y + in->h/2.0;
    // top left, top right, bottom right, bottom left
    float xcoords[4] = {x0, x1, x1, x0};
    float ycoords[4] = {y0, y0, y1, y1};
    memcpy(out->x, xcoords, 4*sizeof(out->x[0]));
    memcpy(out->y, ycoords, 4*sizeof(out->y[0]));
}

void rectxy_to_rect(RectXY* in, Rect* out)
{
    float xmin,xmax,ymin,ymax;
    float xrange = rangef(in->x, 4, &xmin, &xmax);
    float yrange = rangef(in->y, 4, &ymin, &ymax);
    out->w = xrange;
    out->h = yrange;
    out->x = xmin + out->w/2.0;
    out->y = ymin + out->h/2.0;
}

bool rects_equal(Rect* r1, Rect* r2)
{
    if(!FEQ(r1->x, r2->x)) return false;
    if(!FEQ(r1->y, r2->y)) return false;
    if(!FEQ(r1->w, r2->w)) return false;
    if(!FEQ(r1->h, r2->h)) return false;
    return true;
}

void print_rect(Rect* r)
{
    printf("Rectangle (x,y,w,h): %.3f, %.3f, %.3f, %.3f\n", r->x, r->y, r->w, r->h);
}

void print_rectxy(RectXY* r)
{
    printf("Rectangle XY:\n");

    printf("  x: ");
    for(int i = 0; i < 4; ++i)
        printf("%.2f%s", r->x[i], i != 3 ? ", " : "\n");

    printf("  y: ");
    for(int i = 0; i < 4; ++i)
        printf("%.2f%s", r->y[i], i != 3 ? ", " : "\n");
}

// onyl works with positives
float rand_float_between(float lower, float upper)
{
    float range = upper - lower;
    float value = ((float)rand()/(float)(RAND_MAX)) * range + lower;
    return value;
}
