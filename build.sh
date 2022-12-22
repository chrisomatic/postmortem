#!/bin/sh
rm -rf bin
mkdir bin

cd src

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
    core/glist.c \
    main.c \
    world.c \
    player.c \
    zombie.c \
    effects.c \
    gui.c \
    net.c \
    projectile.c \
    item.c \
    entity.c \
    editor.c \
    console.c \
    -Icore \
    -lglfw -lGLU -lGLEW -lGL -lm -O2 \
    -o ../bin/postmortem
