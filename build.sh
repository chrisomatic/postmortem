#!/bin/sh
rm -rf bin
mkdir bin

gcc rfx/gfx.c \
    rfx/shader.c \
    rfx/timer.c \
    rfx/rat_math.c \
    rfx/camera.c \
    rfx/window.c \
    main.c \
    world.c \
    player.c \
    -Irfx \
    -lglfw -lGLU -lGLEW -lGL -lm \
    -o bin/example
