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

void physics_simulate(Physics* phys, float delta_t)
{
    phys->vel.x += phys->accel.x;
    phys->vel.y += phys->accel.y;

    phys->vel.x = RANGE(phys->vel.x, -phys->max_linear_vel,phys->max_linear_vel);
    phys->vel.y = RANGE(phys->vel.y, -phys->max_linear_vel,phys->max_linear_vel);

    phys->pos.x += delta_t*phys->vel.x;
    phys->pos.y += delta_t*phys->vel.y;
}

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
