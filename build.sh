#!/bin/sh
rm -rf bin
mkdir bin

gcc core/gfx.c \
    core/shader.c \
    core/timer.c \
    core/physics.c \
    core/math2d.c \
    core/camera.c \
    core/lighting.c \
    core/window.c \
    core/socket.c \
    core/bitpack.c \
    core/imgui.c \
    core/particles.c \
    main.c \
    world.c \
    player.c \
    zombie.c \
    gui.c \
    net.c \
    projectile.c \
    -Icore \
    -lglfw -lGLU -lGLEW -lGL -lm \
    -o bin/postmortem
