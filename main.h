#pragma once
#include <stdint.h>
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

// strings
#define STR_EMPTY(x)      (x == 0 || strlen(x) == 0)
#define STR_EQUAL(x,y)    (strncmp((x),(y),strlen((x))) == 0 && strlen(x) == strlen(y))
#define STRN_EQUAL(x,y,n) (strncmp((x),(y),(n)) == 0)

#define FREE(p) do{ if(p != NULL) {free(p); p = NULL;} }while(0)

#define DEBUG_PRINT()   printf("%d %s %s()\n", __LINE__, __FILE__, __func__)

extern Timer game_timer;
extern GameRole role;
extern Vector2f aim_camera_offset;

#define CONSOLE_MSG_MAX  10
#define CONSOLE_TEXT_MAX 100
#define CONSOLE_HIST_MAX 10 //input history
#define CONSOLE_PROMPT  "cmd> "
typedef struct
{
    char msg[CONSOLE_TEXT_MAX+1];
    uint32_t color;
} ConsoleMessage;

extern ConsoleMessage console_msg[CONSOLE_MSG_MAX];
extern int console_msg_count;

extern char console_text[CONSOLE_TEXT_MAX+1];
extern char console_text_hist[CONSOLE_HIST_MAX][CONSOLE_TEXT_MAX+1];
extern int console_text_hist_index;
extern int console_text_hist_selection;
extern bool console_enabled;
extern bool debug_enabled;

const char* game_role_to_str(GameRole _role);

char* string_split_index(char* str, const char* delim, int index, int* ret_len, bool split_past_index);
char* string_split_index_copy(char* str, const char* delim, int index, bool split_past_index);

void console_message_add(uint32_t color, char* fmt, ...);
void console_text_hist_add(char* text);
int console_text_hist_get(int direction);
void run_console_command(char* text);
void handle_backspace_timer();

// lists
glist* list_create(void* buf, int max_count, int item_size);
void list_delete(glist* list);
bool list_add(glist* list, void* item);
bool list_remove(glist* list, int index);
void* list_get(glist* list, int index);

void limit_pos(Rect* limit, Rect* pos);
