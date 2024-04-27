#!/bin/sh

mkdir -p build

# $1 holds the name of API
# $2 holds the name of file
build_wayland_protocol () {
    wayland-scanner client-header /usr/share/wayland-protocols/$1/$2.xml src/graphics/wayland/$2.h
    wayland-scanner private-code  /usr/share/wayland-protocols/$1/$2.xml src/graphics/wayland/$2.c
}

build_wayland_protocol "stable/xdg-shell" "xdg-shell"
build_wayland_protocol "unstable/xdg-decoration" "xdg-decoration-unstable-v1"

glslc src/graphics/shaders/sdf.frag -g -o build/sdf.frag.spv
glslc src/graphics/shaders/sdf.vert -g -o build/sdf.vert.spv

arguments="-I. -DLINUX_WAYLAND=1"
libraries="-lm -lwayland-client -lxkbcommon -lvulkan -lSDL2"
errors="-Werror -Wall -Wextra -pedantic"
exclude_errors="-Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-extra-semi -Wno-gnu-zero-variadic-macro-arguments"

debug_options="-g -DENABLE_ASSERT=1 -DDEBUG_BUILD=1"

clang src/msdf-gen/main.c $arguments $debug_options $errors $exclude_errors $libraries -o build/msdf-gen
