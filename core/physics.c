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


bool physics_rect_collision2(Rect* prior_box, Rect* curr_box, Rect* check, float delta_x_pos, float delta_y_pos)
{
    // float xdelta = curr_box->x - prior_box->x;
    // float ydelta = curr_box->y - prior_box->y;

    int xd = 0;
    int yd = 0;

    if(!FEQ(delta_x_pos, 0.0))
    {
        xd = ABS(delta_x_pos)/delta_x_pos;
    }

    if(!FEQ(delta_y_pos, 0.0))
    {
        yd = ABS(delta_y_pos)/delta_y_pos;
    }

    int xd2 = 0;
    int yd2 = 0;
    if(prior_box->x < check->x)
        xd2 = 1;    // left
    else
        xd2 = -1;   // right

    if(prior_box->y < check->y)
        yd2 = 1;   // top
    else
        yd2 = -1;    // bottom


    // int xd = ABS(xdelta)/xdelta;
    // int yd = ABS(ydelta)/ydelta;

    if(are_rects_colliding(prior_box, curr_box, check))
    {
        printf("collision: %d, %d   %d, %d  | delta: %.2f, %.2f | x: %.2f -> %.2f | y: %.2f -> %.2f\n", xd, yd, xd2, yd2, delta_x_pos, delta_y_pos, prior_box->x, curr_box->x, prior_box->y, curr_box->y);

        if(ABS(delta_x_pos) > ABS(delta_y_pos))
        {
            if(xd2 == 1)
            {
                printf("left\n");
                curr_box->x = (check->x - check->w/2.0) - curr_box->w/2.0 - 1.0;
            }
            else
            {
                printf("right\n");
                curr_box->x = (check->x + check->w/2.0) + curr_box->w/2.0 + 1.0;
            }


            // if(xd == 1)
            // {
            //     curr_box->x = (check->x - check->w/2.0) - curr_box->w/2.0 - 1.0;
            // }
            // else if(xd == -1)
            // {
            //     curr_box->x = (check->x + check->w/2.0) + curr_box->w/2.0 + 1.0;
            // }
        }
        else
        {

            if(yd2 == 1)
            {
                printf("top\n");
                curr_box->y = (check->y - check->h/2.0) - curr_box->h/2.0 - 1.0;
            }
            else
            {
                printf("bottom\n");
                curr_box->y = (check->y + check->h/2.0) + curr_box->h/2.0 + 1.0;
            }

            // if(yd == 1)
            // {
            //     curr_box->y = (check->y - check->h/2.0) - curr_box->h/2.0 - 1.0;
            // }
            // else if(yd == -1)
            // {
            //     curr_box->y = (check->y + check->h/2.0) + curr_box->h/2.0 + 1.0;
            // }
        }



        return true;

    }

    return false;
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


bool physics_rect_collision(Rect* prior_box, Rect* curr_box, Rect* check, float delta_x_pos, float delta_y_pos)
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

        return true;
    }

    return false;


}
