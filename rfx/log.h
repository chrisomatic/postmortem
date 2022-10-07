#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "timer.h"


#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_BROWN   "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR_WHITE   "37"
#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define LOG_RESET_COLOR   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D       LOG_COLOR(LOG_COLOR_PURPLE)
#define LOG_COLOR_V       LOG_COLOR(LOG_COLOR_CYAN)
#define LOG_COLOR_N       LOG_COLOR(LOG_COLOR_WHITE)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)


#if defined(WIN32)

#define LOG_FMT_START(letter)   #letter " [" "%-10.10s:%4d " "%7.2f ]: "
#define LOG_FMT_END()           "\n"
#define LOG_FMT(letter, format) LOG_FMT_START(letter) format LOG_FMT_END()

#else

#define LOG_FMT_START(letter)     LOG_COLOR_ ## letter #letter LOG_RESET_COLOR " [" LOG_COLOR(LOG_COLOR_BLUE) "%-10.10s:%4d " LOG_RESET_COLOR "%7.2f ]: " LOG_COLOR_ ## letter
#define LOG_FMT_END()           LOG_RESET_COLOR "\n"
#define LOG_FMT(letter, format) LOG_FMT_START(letter) format LOG_FMT_END()

#endif


static Timer log_timer = {0};
static void log_init(int log_level)
{
    timer_begin(&log_timer);
}

static void print_log(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

#define LOG(format, ...) print_log(format, __FILENAME__, __LINE__, timer_get_elapsed(&log_timer), ##__VA_ARGS__)

#define LOGE(format,...) LOG(LOG_FMT(E, format), ##__VA_ARGS__) // error
#define LOGW(format,...) LOG(LOG_FMT(W, format), ##__VA_ARGS__) // warning
#define LOGI(format,...) LOG(LOG_FMT(I, format), ##__VA_ARGS__) // info
#define LOGV(format,...) LOG(LOG_FMT(V, format), ##__VA_ARGS__) // verbose
#define LOGN(format,...) LOG(LOG_FMT(N, format), ##__VA_ARGS__) // network
