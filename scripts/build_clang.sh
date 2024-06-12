#!/bin/sh

mkdir -p build

arguments="-I."
libraries="-lm -lSDL2"
errors="-Werror -Wall -Wextra -pedantic"
exclude_errors="-Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-extra-semi -Wno-gnu-zero-variadic-macro-arguments -Wno-initializer-overrides -Wshadow"

debug_options="-g -DENABLE_ASSERT=1 -DDEBUG_BUILD=1"

clang src/msdf-gen/main.c $arguments $debug_options $errors $exclude_errors $libraries -o build/msdf-gen
