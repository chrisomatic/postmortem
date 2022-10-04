#pragma once

typedef struct
{
    float  fps;
    float  spf;
    double time_start;
    double time_last;
    double frame_fps;
} Timer;

void timer_begin(Timer* timer);
void timer_set_fps(Timer* timer, float fps);
void timer_wait_for_frame(Timer* timer);
double timer_get_prior_frame_fps(Timer* timer);

double timer_get_elapsed(Timer* timer);
void timer_delay_us(int us);
double timer_get_time();
