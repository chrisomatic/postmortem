#!/bin/sh
rmdir -rf bin
mkdir bin

gcc main.c \
    gfx.c \
    shader.c \
    timer.c \
    rat_math.c \
    window.c \
    -lglfw -lGLU -lGLEW -lGL -lm \
    -o bin/example
