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

    phys->total_adj.x += offset_x;
    phys->total_adj.y += offset_y;
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

    memcpy(&phys->prior_collision, &phys->collision,sizeof(Rect));

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

static bool is_colliding(Physics* phys1, Physics* phys2, bool* slow, double delta_t)
{
    bool colliding = rectangles_colliding(&phys1->collision, &phys2->collision);

    if(slow)
        *slow = colliding;

    if(!IS_RECT_EMPTY(&phys1->prior_collision) && !IS_RECT_EMPTY(&phys1->collision))
    {
        float distance = dist(phys1->prior_collision.x, phys1->prior_collision.y, phys1->collision.x,phys1->collision.y);

        if(distance >= 30.0)
        {
            if(slow)
                *slow = false;

            // more hardcore check
            //LOGI("Entity is moving more than 10 pixels!\n");
            if(!colliding)
                colliding = are_rects_colliding(&phys1->prior_collision, &phys1->collision, &phys2->collision);
        }
    }

    return colliding;
}

bool physics_check_collisions(Physics* phys1, Physics* phys2, double delta_t)
{
    bool slow;
    bool colliding = is_colliding(phys1, phys2,&slow, delta_t);

    if(!colliding) return false;

    if(phys1->num_colliding_entities >= MAX_COLLIDING_ENTITIES)
    {
        //LOGW("Too many colliding entities");
    }
    else
    {
        phys1->colliding_entities[phys1->num_colliding_entities] = (void*)phys2;
        phys1->collision_overlap[phys1->num_colliding_entities] = slow;
        phys1->num_colliding_entities++;
    }

    return true;

}

typedef struct
{
    Physics* phys;
    float dist;
    bool slow;
} PhysicsSortObj;

static void sort_physics_obj(PhysicsSortObj lst[MAX_COLLIDING_ENTITIES], int count)
{
    // insertion sort
    int i, j;
    PhysicsSortObj key;
    for (i = 1; i < count; i++) 
    {
        memcpy(&key, &lst[i], sizeof(PhysicsSortObj));
        j = i - 1;

        while (j >= 0 && lst[j].dist > key.dist)
        {
            memcpy(&lst[j+1], &lst[j], sizeof(PhysicsSortObj));
            j = j - 1;
        }
        memcpy(&lst[j+1], &key, sizeof(PhysicsSortObj));
    }

}

void physics_resolve_collisions(Physics* phys1, double delta_t)
{
    int num_collisions = phys1->num_colliding_entities;
    if(num_collisions == 0)
        return;

    PhysicsSortObj obj[MAX_COLLIDING_ENTITIES] = {0};

    if(num_collisions == 1)
    {
        obj[0].phys = (Physics*)phys1->colliding_entities[0];
        obj[0].slow = phys1->collision_overlap;
    }
    else
    {
        for(int i = 0; i < num_collisions; ++i)
        {
            obj[i].phys = (Physics*)phys1->colliding_entities[i];
            obj[i].dist = dist(phys1->prior_collision.x, phys1->prior_collision.y, obj[i].phys->prior_collision.x,obj[i].phys->prior_collision.y);
            obj[i].slow = phys1->collision_overlap[i];
        }

        sort_physics_obj(obj,num_collisions);
    }

    for(int i = 0; i < num_collisions; ++i)
    {
        Physics* phys2 = obj[i].phys;
        
        bool slow = obj[i].slow;

        // correct collision
        float m1 = phys1->mass;
        float m2 = phys2->mass;

        Vector2f u1 = {phys1->vel.x,phys1->vel.y};
        Vector2f u2 = {phys2->vel.x,phys2->vel.y};

        Vector2f v1 = {0.0,0.0};
        Vector2f v2 = {0.0,0.0};

        // make sure we don't have zero mass
        if(m1 <= 0.0) m1 = 1.0;
        if(m2 <= 0.0) m2 = 1.0;

        float denom = m1+m2;

        float mf11 = (m1-m2)/denom;
        float mf12 = (2.0*m2)/denom;
        float mf21 = (m2-m1)/denom;
        float mf22 = (2.0*m1)/denom;

        v1.x = mf11*u1.x+mf12*u2.x;
        v1.y = mf11*u1.y+mf12*u2.y;

        v2.x = mf22*u1.x+mf21*u2.x;
        v2.y = mf22*u1.y+mf21*u2.y;

        if(m1 >= 10000.0)
        {
            // immovable
            v1.x = 0.0;
            v1.y = 0.0;
        }
            
        if(m2 >= 10000.0)
        {
            // immovable
            v2.x = 0.0;
            v2.y = 0.0;
        }

        float r1 = magn_fast(v1);
        float r2 = magn_fast(v2);

        r1 = ABS(r1);
        r2 = ABS(r2);
        
        if(r1 == 0.0 && r2 == 0.0)
        {
            if(m1 >= 10000.0)
            {
                r1 = 0.0;
                r2 = 1.0;
            }
            else if(m2 >= 10000.0)
            {
                r1 = 1.0;
                r2 = 0.0;
            }
            else
            {
                r1 = 0.5;
                r2 = 0.5;
            }
        }

        float total = r1 + r2;

        r1 = r1 / total;
        r2 = r2 / total;

        if(!slow)
        {
            // fast entity, try and prevent running through walls
            Vector2f d = {phys1->collision.x - phys1->prior_collision.x,phys1->collision.y - phys1->prior_collision.y};

            phys1->pos.x -= d.x;
            phys1->pos.y -= d.y;
            physics_apply_pos_offset(phys1, -d.x, -d.y);

            normalize(&d);
            d.x *= (phys1->collision.w/2.0);
            d.y *= (phys1->collision.h/2.0);

            int num_loops = 0;
            const int max_loops = 16;
            
            for(;;)
            {
                num_loops++;
                if(num_loops >= max_loops)
                    break;

                phys1->pos.x += d.x;
                phys1->pos.y += d.y;
                physics_apply_pos_offset(phys1, d.x, d.y);

                bool colliding = rectangles_colliding(&phys1->collision, &phys2->collision);
                if(colliding)
                    break;
            }

            //printf("num_loops: %d\n",num_loops);

            slow = true;
        }

        float ax = MAX(phys1->collision.x-phys1->collision.w/2.0,phys2->collision.x-phys2->collision.w/2.0);
        float bx = MIN(phys1->collision.x+phys1->collision.w/2.0,phys2->collision.x+phys2->collision.w/2.0);

        float ay = MAX(phys1->collision.y-phys1->collision.h/2.0,phys2->collision.y-phys2->collision.h/2.0);
        float by = MIN(phys1->collision.y+phys1->collision.h/2.0,phys2->collision.y+phys2->collision.h/2.0);

        Vector2f overlap = {
            .x = MAX(0.0,bx - ax),
            .y = MAX(0.0,by - ay)
        };

        Vector2f adj1 = {0.0,0.0};
        Vector2f adj2 = {0.0,0.0};

        if(ABS(overlap.y) > ABS(overlap.x))
        {
            if(phys1->prior_collision.x > phys2->prior_collision.x)
            {
                // move phys1 right, phys2 left
                adj1.x = 1.0;
                adj2.x = -1.0;
            }
            else
            {
                // move phys1 left, phys2 right
                adj1.x = -1.0;
                adj2.x = 1.0;
            }
        }
        else
        {
            if(phys1->prior_collision.y > phys2->prior_collision.y)
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

        adj1.x *= (r1*overlap.x);
        adj1.y *= (r1*overlap.y);

        adj2.x *= (r2*overlap.x);
        adj2.y *= (r2*overlap.y);

        memcpy(&phys1->prior_collision, &phys1->collision,sizeof(Rect));
        memcpy(&phys2->prior_collision, &phys2->collision,sizeof(Rect));

        phys1->pos.x += adj1.x;
        phys1->pos.y += adj1.y;
        phys2->pos.x += adj2.x;
        phys2->pos.y += adj2.y;

        physics_apply_pos_offset(phys1, adj1.x, adj1.y);
        physics_apply_pos_offset(phys2, adj2.x, adj2.y);
    }


    phys1->num_colliding_entities = 0;

    //printf("v1: %f %f, v2: %f %f, ratio: %f %f, num_loops: %d\n",v1.x,v1.y,v2.x,v2.y, ratio.x,ratio.y, num_loops);
    return;
}

