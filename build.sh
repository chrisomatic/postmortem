#!/bin/sh
rm -rf bin
mkdir bin

gcc rfx/gfx.c \
    rfx/shader.c \
    rfx/timer.c \
    rfx/physics.c \
    rfx/math2d.c \
    rfx/camera.c \
    rfx/window.c \
    main.c \
    world.c \
    player.c \
    zombie.c \
    gun.c \
    gui.c \
    projectile.c \
    -Irfx \
    -lglfw -lGLU -lGLEW -lGL -lm \
    -o bin/postmortem
