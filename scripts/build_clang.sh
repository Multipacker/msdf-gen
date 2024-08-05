#!/bin/sh

set -e

for argument in "$@"; do declare $argument='1'; done
if [ ! -v release ]; then
    debug=1
fi

if [ -v debug ]; then
    echo "Debug build"
fi
if [ -v release ]; then
    echo "Release build"
fi

# Common flags

errors=""
errors+="-Werror "
errors+="-Wall "
errors+="-Wextra "
errors+="-pedantic "
errors+="-Wshadow "
errors+="-Wno-unused-parameter "
errors+="-Wno-unused-variable "
errors+="-Wno-unused-function "
errors+="-Wno-unused-but-set-variable "
errors+="-Wno-extra-semi "
errors+="-Wno-gnu-zero-variadic-macro-arguments "
errors+="-Wno-initializer-overrides "

libraries="-lm -lSDL2"
common_compiler_flags="-I. ${errors}"
common_linker_flags="${libraries}"

# Debug flags

debug_compiler_flags="${common_compiler_flags} -g -DENABLE_ASSERT=1 -DDEBUG_BUILD=1"
debug_linker_flags="${common_linker_flags}"

# Release flags

release_compiler_flags="${common_compiler_flags} -O3 -DRELEASE_BUILD=1"
release_linker_flags="${common_linker_flags}"

# Choose options

if [ -v debug ]; then
    compiler_flags="${debug_compiler_flags}"
    linker_flags="${debug_linker_flags}"
fi
if [ -v release ]; then
    compiler_flags="${release_compiler_flags}"
    linker_flags="${release_linker_flags}"
fi

# Build

mkdir -p build

clang $compiler_flags $linker_flags src/msdf-gen/main.c -o build/msdf-gen
