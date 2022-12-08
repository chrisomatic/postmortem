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

void physic_apply_pos_offset(Physics* phys, float offset_x, float offset_y)
{
    phys->actual_pos.x += offset_x;
    phys->actual_pos.y += offset_y;

    phys->hit.x += offset_x;
    phys->hit.y += offset_y;

    phys->collision.x += offset_x;
    phys->collision.y += offset_y;
}

void physic_set_pos_offset(Physics* phys, float offset_x, float offset_y)
{
    physic_apply_pos_offset(phys, -phys->pos_offset.x, -phys->pos_offset.y);

    phys->pos_offset.x = offset_x;
    phys->pos_offset.y = offset_y;

    physic_apply_pos_offset(phys, phys->pos_offset.x, phys->pos_offset.y);
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

    physic_apply_pos_offset(phys, dx, dy);

    if(limit != NULL)
    {
        Rect r = (*phys).pos;
        physics_limit_pos(limit, &r);
        float adjx = r.x - phys->pos.x;
        float adjy = r.y - phys->pos.y;
        if(!FEQ(adjx,0.0) || !FEQ(adjy,0.0))
        {
            phys->pos.x += adjx;
            phys->pos.y += adjy;
            physic_apply_pos_offset(phys, adjx, adjy);
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

    const int max_loops = 10;
    int num_loops = 0;

    Vector2f total_adj1 = {0.0,0.0};
    Vector2f total_adj2 = {0.0,0.0};

    for(;;)
    {
        num_loops++;
        if(num_loops >= max_loops)
            break;

        total_adj1.x += adj1.x;
        total_adj1.y += adj1.y;
        total_adj2.x += adj2.x;
        total_adj2.y += adj2.y;

        phys1->collision.x += adj1.x;
        phys1->collision.y += adj1.y;
        phys2->collision.x += adj2.x;
        phys2->collision.y += adj2.y;

        colliding = rectangles_colliding(&phys1->collision, &phys2->collision);
        if(!colliding)
            break;
    }

    phys1->pos.x += total_adj1.x;
    phys1->pos.y += total_adj1.y;
    phys2->pos.x += total_adj2.x;
    phys2->pos.y += total_adj2.y;

    physic_apply_pos_offset(phys1, total_adj1.x, total_adj1.y);
    physic_apply_pos_offset(phys2, total_adj2.x, total_adj2.y);

    //printf("num_loops: %d\n",num_loops);
}

void physics_handle_collision_old_attempt(Physics* phys1, Physics* phys2, double delta_t) // dont use
{
    // check for collision
    bool colliding = rectangles_colliding(&phys1->pos, &phys2->pos);

    if(!colliding)
        return;

    // correct collision
    double check_t = delta_t;
    double ddt = delta_t;

    int dir = -1;
    int num_loops = 0;

    const int max_loops = 10;

    Rect pos1 = {phys1->pos.x, phys1->pos.y, phys1->pos.w, phys1->pos.h};
    Rect pos2 = {phys2->pos.x, phys2->pos.y, phys2->pos.w, phys2->pos.h};

    Vector2f pos1_prior = {0.0,0.0};
    Vector2f pos2_prior = {0.0,0.0};

    printf("==============\n");
    printf("Colliding with frame time: %f!!!\n", delta_t);
    printf("v1: %f %f, v2: %f %f\n", phys1->vel.x, phys1->vel.y, phys2->vel.x, phys2->vel.y);
    printf("size1: %f %f, size2: %f %f\n", phys1->pos.w, phys1->pos.h, phys2->pos.w, phys2->pos.h);

    for(;;)
    {
        if(num_loops >= max_loops)
        {
            break;
        }

        // update positions of objects
        ddt /= 1.4;
        check_t += (dir*ddt);

        Vector2f adj1 = { phys1->vel.x*check_t, phys1->vel.y*check_t};
        Vector2f adj2 = { phys2->vel.x*check_t, phys2->vel.y*check_t};

        pos1_prior.x = pos1.x;
        pos1_prior.y = pos1.y;
        pos2_prior.x = pos2.x;
        pos2_prior.y = pos2.y;

        pos1.x = phys1->pos.x - adj1.x;
        pos1.y = phys1->pos.y - adj1.y;
        pos2.x = phys2->pos.x - adj2.x;
        pos2.y = phys2->pos.y - adj2.y;

        float d1x = ABS(pos1.x - pos1_prior.x);
        float d1y = ABS(pos1.y - pos1_prior.y);
        float d2x = ABS(pos2.x - pos2_prior.x);
        float d2y = ABS(pos2.y - pos2_prior.y);

        // check colliding again
        colliding = rectangles_colliding(&pos1, &pos2);
        if(colliding)
        {
            dir = +1;
        }
        else
        {
            dir = -1;

            if(d1x + d1y + d2x + d2y < 2.0)
            {
                break;
            }
        }

        printf("   loop %d: check_time: %f, colliding: %s, deltas: %f %f %f %f\n",num_loops,check_t, colliding ? "true" : "false", d1x,d1y,d2x,d2y);
            
        ++num_loops;
    }

    if(colliding)
    {
        // completely reverse collision
        phys1->pos.x -= (phys1->vel.x*delta_t);
        phys1->pos.y -= (phys1->vel.y*delta_t);
        phys2->pos.x -= (phys2->vel.x*delta_t);
        phys2->pos.y -= (phys2->vel.y*delta_t);
    }
    else
    {
        phys1->pos.x = pos1.x;
        phys1->pos.y = pos1.y;
        phys2->pos.x = pos2.x;
        phys2->pos.y = pos2.y;
    }

    // update velocities

    float m1 = phys1->mass;
    float m2 = phys2->mass;

    Vector2f u1 = {phys1->vel.x,phys1->vel.y};
    Vector2f u2 = {phys2->vel.x,phys2->vel.y};

    float denom = m1+m2;
    if(denom != 0.0)
    {
        float mf11 = (m1-m2)/denom;
        float mf12 = (2.0*m2)/denom;
        float mf21 = (m2-m1)/denom;
        float mf22 = (2.0*m1)/denom;

        phys1->vel.x = mf11*u1.x+mf12*u2.x;
        phys1->vel.y = mf11*u1.y+mf12*u2.y;

        phys2->vel.x = mf22*u1.x+mf21*u2.x;
        phys2->vel.y = mf22*u1.y+mf21*u2.y;
    }
    else
    {
        phys1->vel.x = 0.0;
        phys1->vel.y = 0.0;

        phys2->vel.x = 0.0;
        phys2->vel.y = 0.0;
    }


    //

    //printf("num_loops: %d\n",num_loops);
    //printf("==============\n");


    //printf("num_loops: %d\n",num_loops);
}


// collision | pos: RIGHT,    - | box:  LEFT,    - | delta pos: 2.14, 0.00 | delta box: -31.50, 7.33
// block collision index: 1
// collision | pos: RIGHT,    - | box:  LEFT,    - | delta pos: 2.18, 0.00 | delta box: -31.50, 7.33
// block collision index: 1
// collision | pos: RIGHT,    - | box:  LEFT,    - | delta pos: 2.25, 0.00 | delta box: -31.50, 7.33
// block collision index: 1
// prior box is inside check!
// collision | pos: RIGHT,    - | box:     -,    - | delta pos: 2.15, 0.00 | delta box: 31.00, 7.43
// block collision index: 0
// collision | pos: RIGHT,    - | box:  LEFT,    - | delta pos: 2.14, 0.00 | delta box: -33.00, 7.43
// block collision index: 0

bool physics_rect_collision(Rect* prior_box, Rect* curr_box, Rect* check, float delta_x_pos, float delta_y_pos, rect_collision_data_t* data)
{

    #define UP      -1
    #define DOWN    1
    #define LEFT    -1
    #define RIGHT   1

    #define STR_LR(_xd) (_xd == LEFT ? "LEFT" : (_xd == RIGHT ? "RIGHT" : "-" ))
    #define STR_UD(_yd) (_yd == UP ? "UP" : (_yd == DOWN ? "DOWN" : "-" ))


    // physic position:
    // what direction is the entity moving
    int xd = 0;
    int yd = 0;

    if(!FEQ(delta_x_pos, 0.0))
    {
        if(delta_x_pos < 0.0)
            xd = LEFT;
        else
            xd = RIGHT;
    }

    if(!FEQ(delta_y_pos, 0.0))
    {
        if(delta_y_pos < 0.0)
            yd = UP;
        else
            yd = DOWN;
    }


    // prior box:
    // what side of the box
    float delta_x = prior_box->x - check->x;
    float delta_y = prior_box->y - check->y;
    int xo = 0;
    int yo = 0;

    if(rectangles_colliding(prior_box, check))
    {
        // printf("prior box is inside check!\n");
        if(ABS(delta_x) > ABS(delta_y))
        {
            if(prior_box->x > check->x)
                xo = RIGHT;
            else
                xo = LEFT;
        }
        else
        {
            if(prior_box->y > check->y)
                yo = DOWN;
            else
                yo = UP;
        }

    }
    else if((prior_box->x + prior_box->w/2.0) < (check->x - check->w/2.0))
    {
        xo = LEFT;
    }
    else if((prior_box->x - prior_box->w/2.0) > (check->x + check->w/2.0))
    {
        xo = RIGHT;
    }

    if((prior_box->y + prior_box->h/2.0) < (check->y - check->h/2.0))
    {
        yo = UP;
    }
    else if((prior_box->y - prior_box->h/2.0) > (check->y + check->h/2.0))
    {
        yo = DOWN;
    }


    float adj = 1.0;
    float x_left_correction = (check->x - check->w/2.0) - curr_box->w/2.0 - adj;
    float x_right_correction = (check->x + check->w/2.0) + curr_box->w/2.0 + adj;
    float y_down_correction = (check->y + check->h/2.0) + curr_box->h/2.0 + adj;
    float y_up_correction = (check->y - check->h/2.0) - curr_box->h/2.0 - adj;

    bool collide = rectangles_colliding(curr_box, check);
    if(!collide)
    {
        collide = are_rects_colliding(prior_box, curr_box, check);
    }

    if(collide)
    {
        // printf("collision | pos: %5s, %4s | box: %5s, %4s | delta pos: %.2f, %.2f | delta box: %.2f, %.2f\n" , STR_LR(xd), STR_UD(yd), STR_LR(xo), STR_UD(yo), delta_x_pos, delta_y_pos, delta_x, delta_y);

        if(xo == LEFT)
        {
            curr_box->x = x_left_correction;
        }
        else if(xo == RIGHT)
        {
            curr_box->x = x_right_correction;
        }
        else if(yo == UP)
        {
            curr_box->y = y_up_correction;
        }
        else if(yo == DOWN)
        {
            curr_box->y = y_down_correction;
        }
        else
        {

            LOGE("help");

            // moving left or right
            if(ABS(delta_x_pos) > ABS(delta_y_pos))
            {
                if(delta_x_pos < 0.0)   // moving to the left
                    curr_box->x = x_right_correction;
                else
                    curr_box->x = x_left_correction;
            }
            else
            {
                if(delta_y_pos < 0.0)   // moving up
                    curr_box->y = y_down_correction;
                else
                    curr_box->y = y_up_correction;
            }
        }

        data->collide = true;

        return true;
    }

    return false;


}


//needs works, not used yet
bool physics_rect_collision2(Rect* prior_box, Rect* curr_box, Rect* check, float delta_x_pos, float delta_y_pos, rect_collision_data_t* data)
{

    bool collide = rectangles_colliding(curr_box, check);
    if(!collide)
    {
        collide = are_rects_colliding(prior_box, curr_box, check);
    }
    if(!collide) return false;


    #define UP      -1
    #define DOWN    1
    #define LEFT    -1
    #define RIGHT   1

    #define STR_LR(_xd) (_xd == LEFT ? "LEFT" : (_xd == RIGHT ? "RIGHT" : "-" ))
    #define STR_UD(_yd) (_yd == UP ? "UP" : (_yd == DOWN ? "DOWN" : "-" ))



    int xo = 0;
    int yo = 0;
    // prior box:
    // what side of the box
    float delta_x = prior_box->x - check->x;
    float delta_y = prior_box->y - check->y;


    if(rectangles_colliding(prior_box, check))
    {
        // printf("prior box is inside check!\n");
        if(ABS(delta_x) > ABS(delta_y))
        {
            if(prior_box->x > check->x)
                xo = RIGHT;
            else
                xo = LEFT;
        }
        else
        {
            if(prior_box->y > check->y)
                yo = DOWN;
            else
                yo = UP;
        }

    }
    else if((prior_box->x + prior_box->w/2.0) < (check->x - check->w/2.0))
    {
        xo = LEFT;
    }
    else if((prior_box->x - prior_box->w/2.0) > (check->x + check->w/2.0))
    {
        xo = RIGHT;
    }

    else if((prior_box->y + prior_box->h/2.0) < (check->y - check->h/2.0))
    {
        yo = UP;
    }
    else if((prior_box->y - prior_box->h/2.0) > (check->y + check->h/2.0))
    {
        yo = DOWN;
    }
    else
    {
        LOGE("help");

        // moving left or right
        if(ABS(delta_x_pos) > ABS(delta_y_pos))
        {
            if(delta_x_pos < 0.0)   // moving to the left
            {
                // curr_box->x = x_right_correction;
                xo = RIGHT;
            }
            else
            {
                // curr_box->x = x_left_correction;
                xo = LEFT;
            }
        }
        else
        {
            if(delta_y_pos < 0.0)   // moving up
            {
                // curr_box->y = y_down_correction;
                yo = DOWN;
            }
            else
            {
                // curr_box->y = y_up_correction;
                yo = UP;
            }
        }
    }

    if(data->collide)
    {
        xo = data->xo;
        yo = data->yo;
    }



    float adj = 0.5;
    float x_left_correction = (check->x - check->w/2.0) - curr_box->w/2.0 - adj;
    float x_right_correction = (check->x + check->w/2.0) + curr_box->w/2.0 + adj;
    float y_down_correction = (check->y + check->h/2.0) + curr_box->h/2.0 + adj;
    float y_up_correction = (check->y - check->h/2.0) - curr_box->h/2.0 - adj;

    printf("%s collision | check: (%.1f,%.1f) | box: %5s, %4s | delta pos: %.2f, %.2f\n" , data->collide ? "another" : "first", check->x, check->y, STR_LR(xo), STR_UD(yo), delta_x_pos, delta_y_pos);


    if(xo == LEFT)
    {
        curr_box->x = x_left_correction;
    }
    else if(xo == RIGHT)
    {
        curr_box->x = x_right_correction;
    }
    else if(yo == UP)
    {
        curr_box->y = y_up_correction;
    }
    else if(yo == DOWN)
    {
        curr_box->y = y_down_correction;
    }

    data->collide = true;
    data->check = *check;
    data->xo = xo;
    data->yo = yo;

    return true;

}
