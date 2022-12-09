#include <stdint.h>
#include "log.h"
#include "physics.h"

// static float xlim[2] = {0,1};
// static float ylim[2] = {0,1};

// void physics_set_pos_limits(Rect* limits)
// {
//     xlim[0] = limits->x - limits->w/2.0;
//     xlim[1] = xlim[0] + limits->w;
//     ylim[0] = limits->y - limits->h/2.0;
//     ylim[1] = ylim[0] + limits->h;
// }

void physics_begin(Physics* phys)
{
    phys->accel.x = 0.0;
    phys->accel.y = 0.0;
}

void physics_add_force(Physics* phys, float x, float y)
{
    phys->accel.x += x;
    phys->accel.y += y;
}
void physics_add_friction(Physics* phys, float mu)
{
    if(phys->accel.x == 0.0)
    {
        // apply friction in x direction
        float abs_vel_x = ABS(phys->vel.x);

        if(abs_vel_x > 0.0)
        {
            float val_x = MIN(mu, abs_vel_x);
            phys->vel.x += (phys->vel.x > 0 ? -val_x : val_x);
        }
    }

    if(phys->accel.y == 0.0)
    {
        // apply friction in y direction
        float abs_vel_y = ABS(phys->vel.y);
        if(abs_vel_y > 0.0)
        {
            float val_y = MIN(mu, abs_vel_y);
            phys->vel.y += (phys->vel.y > 0 ? -val_y : val_y);
        }
    }
}

void physics_print(Physics* phys, bool force)
{
    if(force || phys->vel.x != 0 || phys->vel.y != 0)
    {
        LOGI("A: %6.4f %6.4f V: %6.4f %6.4f P: %6.4f %6.4f",
                phys->accel.x, phys->accel.y,
                phys->vel.x, phys->vel.y,
                phys->pos.x, phys->pos.y
        );
    }

}

void physics_apply_pos_offset(Physics* phys, float offset_x, float offset_y)
{
    phys->actual_pos.x += offset_x;
    phys->actual_pos.y += offset_y;

    phys->hit.x += offset_x;
    phys->hit.y += offset_y;

    phys->collision.x += offset_x;
    phys->collision.y += offset_y;
}

void physics_set_pos_offset(Physics* phys, float offset_x, float offset_y)
{
    //physics_apply_pos_offset(phys, -phys->pos_offset.x, -phys->pos_offset.y);

    phys->actual_pos.x -= phys->pos_offset.x;
    phys->actual_pos.y -= phys->pos_offset.y;

    phys->pos_offset.x = offset_x;
    phys->pos_offset.y = offset_y;

    phys->actual_pos.x += phys->pos_offset.x;
    phys->actual_pos.y += phys->pos_offset.y;

    //physics_apply_pos_offset(phys, phys->pos_offset.x, phys->pos_offset.y);
}

void physics_simulate(Physics* phys, Rect* limit, float delta_t)
{
    phys->vel.x += phys->accel.x;
    phys->vel.y += phys->accel.y;

    phys->vel.x = RANGE(phys->vel.x, -phys->max_linear_vel, phys->max_linear_vel);
    phys->vel.y = RANGE(phys->vel.y, -phys->max_linear_vel, phys->max_linear_vel);

    float dx = delta_t * phys->vel.x;
    float dy = delta_t * phys->vel.y;

    phys->pos.x += dx;
    phys->pos.y += dy;

    physics_apply_pos_offset(phys, dx, dy);

    if(limit != NULL)
    {
        Rect r = phys->pos;
        physics_limit_pos(limit, &r);
        float adjx = r.x - phys->pos.x;
        float adjy = r.y - phys->pos.y;

        if(!FEQ(adjx,0.0) || !FEQ(adjy,0.0))
        {
            phys->pos.x += adjx;
            phys->pos.y += adjy;
            physics_apply_pos_offset(phys, adjx, adjy);
            // phys->actual_pos.x += adjx;
            // phys->actual_pos.y += adjy;
        }
    }

}




/*
     x0,y0 ------------------ x1,y0
          |                  |
          |                  |
          |                  |
          |                  |
          |                  |
          |                  |
     x0,y1 ------------------ x1,y1
*/


// pos is contained inside of limit
void physics_limit_pos(Rect* limit, Rect* pos)
{
    // printf("-------------------------------------\n");
    // printf("map: "); print_rect(limit);
    // printf("before: "); print_rect(pos);
    float lx0 = limit->x - limit->w/2.0;
    float lx1 = lx0 + limit->w;
    float ly0 = limit->y - limit->h/2.0;
    float ly1 = ly0 + limit->h;

    float px0 = pos->x - pos->w/2.0;
    float px1 = px0 + pos->w;
    float py0 = pos->y - pos->h/2.0;
    float py1 = py0 + pos->h;

    if(px0 < lx0)
        pos->x = lx0+pos->w/2.0;
    if(px1 > lx1)
        pos->x = lx1-pos->w/2.0;
    if(py0 < ly0)
        pos->y = ly0+pos->h/2.0;
    if(py1 > ly1)
        pos->y = ly1-pos->h/2.0;
    // printf("after: "); print_rect(pos);
}
void physics_handle_collision(Physics* phys1, Physics* phys2, double delta_t)
{
    // check for collision
    bool colliding = rectangles_colliding(&phys1->collision, &phys2->collision);

    if(!colliding)
        return;

    // correct collision
    float m1 = phys1->mass;
    float m2 = phys2->mass;

    Vector2f u1 = {phys1->vel.x,phys1->vel.y};
    Vector2f u2 = {phys2->vel.x,phys2->vel.y};

    Vector2f v1 = {0.0,0.0};
    Vector2f v2 = {0.0,0.0};

    float denom = m1+m2;

    if(ABS(m1-m2) >= 9999.0)
    {
        // immoveable
        if(m1 > m2)
        {
            v2.x = 1.0;
            v2.y = 1.0;
        }
        else
        {
            v1.x = 1.0;
            v1.y = 1.0;
        }
    }
    else if(m1 == m2)
    {
        if(u2.x == 0.0 && u2.y == 0.0)
        {
            v1.x = 1.0;
            v1.y = 1.0;
        }
        else
        {
            v1.x = u2.x;
            v1.y = u2.y;
        }

        if(u2.x == 0.0 && u2.y == 0.0)
        {
            v2.x = 1.0;
            v2.y = 1.0;
        }
        else
        {
            v2.x = u1.x;
            v2.y = u1.y;
        }
    }
    else if(denom > 0.0)
    {
        float mf11 = (m1-m2)/denom;
        float mf12 = (2.0*m2)/denom;
        float mf21 = (m2-m1)/denom;
        float mf22 = (2.0*m1)/denom;

        v1.x = mf11*u1.x+mf12*u2.x;
        v1.y = mf11*u1.y+mf12*u2.y;

        v2.x = mf22*u1.x+mf21*u2.x;
        v2.y = mf22*u1.y+mf21*u2.y;
    }
    else
    {
        v1.x = 1.0;
        v1.y = 1.0;

        v2.x = 1.0;
        v2.y = 1.0;
    }

    float ux = magn_fast(v1);
    float uy = magn_fast(v2);

    Vector2f ratio = {ABS(ux),ABS(uy)};
    normalize(&ratio);

    if(1.0 - ratio.x < 0.001) ratio.x = 1.0;
    if(1.0 - ratio.y < 0.001) ratio.y = 1.0;

    float dx = phys1->collision.x - phys2->collision.x;
    float dy = phys1->collision.y - phys2->collision.y;

    Vector2f adj1 = {0.0,0.0};
    Vector2f adj2 = {0.0,0.0};

    if(ABS(dx) > ABS(dy))
    {
        if(phys1->collision.x > phys2->collision.x)
        {
            // move phys1 right, phys2 left
            adj1.x = 1.0;
            adj2.x = -1.0;
        }
        else
        {
            // move phys1 right, phys2 left
            adj1.x = -1.0;
            adj2.x = 1.0;
        }
    }
    else
    {
        if(phys1->collision.y > phys2->collision.y)
        {
            // move phys1 down, phys2 up
            adj1.y = 1.0;
            adj2.y = -1.0;
        }
        else
        {
            // move phys1 up, phys2 down
            adj1.y = -1.0;
            adj2.y = 1.0;
        }
    }

    adj1.x *= ratio.x;
    adj1.y *= ratio.x;

    adj2.x *= ratio.y;
    adj2.y *= ratio.y;

    const int max_loops = 10;
    int num_loops = 0;

    for(;;)
    {
        num_loops++;
        if(num_loops >= max_loops)
            break;

        phys1->pos.x += adj1.x;
        phys1->pos.y += adj1.y;
        phys2->pos.x += adj2.x;
        phys2->pos.y += adj2.y;

        //printf("loop %d: adj1: %f %f, adj2: %f %f\n",num_loops,adj1.x,adj1.y,adj2.x,adj2.y);

        physics_apply_pos_offset(phys1, adj1.x, adj1.y);
        physics_apply_pos_offset(phys2, adj2.x, adj2.y);

        colliding = rectangles_colliding(&phys1->collision, &phys2->collision);
        if(!colliding)
            break;
    }

    //printf("v1: %f %f, v2: %f %f, ratio: %f %f, num_loops: %d\n",v1.x,v1.y,v2.x,v2.y, ratio.x,ratio.y, num_loops);

    //phys1->vel.x = 0.0;
    //phys1->vel.y = 0.0;

    //phys2->vel.x = 0.0;
    //phys2->vel.y = 0.0;
}
