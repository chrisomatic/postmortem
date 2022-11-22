#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "log.h"
#include "timer.h"
#include "math2d.h"

typedef enum
{
    ROLE_LOCAL,
    ROLE_CLIENT,
    ROLE_SERVER,
} GameRole;

// strings
#define STR_EMPTY(x)      (x == 0 || strlen(x) == 0)
#define STR_EQUAL(x,y)    (strncmp((x),(y),strlen((x))) == 0 && strlen(x) == strlen(y))
#define STRN_EQUAL(x,y,n) (strncmp((x),(y),(n)) == 0)

#define FREE(p) do{ if(p != NULL) {free(p); p = NULL;} }while(0)

#define DEBUG_PRINT()   printf("%d %s %s()\n", __LINE__, __FILE__, __func__)

extern Timer game_timer;
extern GameRole role;
extern Vector2f aim_camera_offset;
extern Vector2f recoil_camera_offset;

extern bool debug_enabled;
extern bool editor_enabled;

const char* game_role_to_str(GameRole _role);

char* string_split_index(char* str, const char* delim, int index, int* ret_len, bool split_past_index);
char* string_split_index_copy(char* str, const char* delim, int index, bool split_past_index);

void handle_backspace_timer();

// used for players
void get_actual_pos(float draw_x, float draw_y, float scale, int img_w, int img_h, Rect* visible_rect, Rect* ret);
void limit_pos(Rect* limit, Rect* pos, Rect* phys_pos);

Rect calc_box(Rect* pos, float wscale, float hscale, int location);
