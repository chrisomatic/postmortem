#!/bin/sh
rm -rf bin
mkdir bin

gcc main.c \
    rfx/gfx.c \
    rfx/shader.c \
    rfx/timer.c \
    rfx/rat_math.c \
    rfx/window.c \
    -Irfx \
    -lglfw -lGLU -lGLEW -lGL -lm \
    -o bin/example
