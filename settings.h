#pragma once

#define VIEW_WIDTH   1200
#define VIEW_HEIGHT  800

#define ASPECT_NUM 16.0f
#define ASPECT_DEM  9.0f
#define ASPECT_RATIO (ASPECT_NUM / ASPECT_DEM)

#define BPP 4 // bits per pixel
#define TARGET_FPS     60.0f
#define TARGET_SPF     (1.0f/TARGET_FPS) // seconds per frame

extern int window_width;
extern int window_height;
