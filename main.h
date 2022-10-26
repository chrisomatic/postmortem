#pragma once
#include "timer.h"
#include "math2d.h"

typedef enum
{
    ROLE_LOCAL,
    ROLE_CLIENT,
    ROLE_SERVER,
} GameRole;

typedef struct
{
    int max_count;
    int count;
    int item_size;
    void* buf;
} glist;

#define DEBUG_PRINT()   printf("%d %s %s()\n", __LINE__, __FILE__, __func__)

extern Timer game_timer;
extern GameRole role;
extern Vector2f aim_camera_offset;

#define CONSOLE_TEXT_MAX 250
extern char console_text[CONSOLE_TEXT_MAX+1];
extern bool console_enabled;
extern bool debug_enabled;

const char* game_role_to_str(GameRole _role);

void parse_console_command(char* command);
void handle_backspace_timer();

// lists
glist* list_create(void* buf, int max_count, int item_size);
void list_delete(glist* list);
bool list_add(glist* list, void* item);
bool list_remove(glist* list, int index);
void* list_get(glist* list, int index);

void limit_pos(Rect* limit, Rect* pos);
