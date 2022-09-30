#!/bin/sh
gcc main.c \
    gfx.c \
    shader.c \
    timer.c \
    window.c \
    -lglfw -lGLU -lGLEW -lGL -lm \
    -o bin/example
