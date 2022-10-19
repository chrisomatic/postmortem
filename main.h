#pragma once
#include "timer.h"

typedef enum
{
    ROLE_LOCAL,
    ROLE_CLIENT,
    ROLE_SERVER,
} GameRole;

const char* game_role_to_str(GameRole _role);

extern Timer game_timer;
extern GameRole role;
