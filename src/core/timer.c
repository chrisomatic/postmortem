#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "timer.h"

static struct
{
    bool monotonic;
    uint64_t  frequency;
    uint64_t  offset;
} _timer;

static uint64_t get_timer_value(void)
{
#if defined(_POSIX_TIMERS) && defined(_POSIX_MONOTONIC_CLOCK)
    if (_timer.monotonic)
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t) ts.tv_sec * (uint64_t) 1000000000 + (uint64_t) ts.tv_nsec;
    }
    else
#endif
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (uint64_t) tv.tv_sec * (uint64_t) 1000000 + (uint64_t) tv.tv_usec;
    }
}

void init_timer(void)
{
#if defined(_POSIX_TIMERS) && defined(_POSIX_MONOTONIC_CLOCK)
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        _timer.monotonic = true;
        _timer.frequency = 1000000000;
    }
    else
#endif
    {
        _timer.monotonic = false;
        _timer.frequency = 1000000;
    }
    _timer.offset = get_timer_value();
}


static double get_time()
{
    return (double) (get_timer_value() - _timer.offset) / _timer.frequency;
}

void timer_begin(Timer* timer)
{
    timer->time_start = get_time();
    timer->time_last = timer->time_start;
    timer->frame_fps = 0.0f;
}

double timer_get_time()
{
    return get_time();
}

void timer_set_fps(Timer* timer, float fps)
{
    timer->fps = fps;
    timer->spf = 1.0f / fps;
}

void timer_wait_for_frame(Timer* timer)
{
    double now;
    for(;;)
    {
        now = get_time();
        if(now >= timer->time_last + timer->spf)
            break;
        usleep(100);
    }

    timer->frame_fps = 1.0f / (now - timer->time_last);
    timer->time_last = now;
}

double timer_get_elapsed(Timer* timer)
{
    double time_curr = get_time();
    return time_curr - timer->time_start;
}

double timer_get_prior_frame_fps(Timer* timer)
{
    return timer->frame_fps;
}

void timer_delay_us(int us)
{
    usleep(us);
}
