#pragma once

#include "main.h"

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

void console_message_add(uint32_t color, char* fmt, ...);
void console_text_hist_add(char* text);
int console_text_hist_get(int direction);
void run_console_command(char* text);

void gui_init();
void gui_draw();
