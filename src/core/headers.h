#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN      // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#pragma comment(lib, "User32.lib")
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>
#include <errno.h> 
#include <time.h>
#include <unistd.h>
