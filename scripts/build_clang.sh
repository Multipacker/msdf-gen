#!/bin/sh

set -e

for argument in "$@"; do declare $argument='1'; done
if [ ! -v release ] && [ ! -v profile ]; then
    debug=1
fi

# Common flags

errors=""
errors+="-Werror "
errors+="-Wall "
errors+="-Wextra "
errors+="-pedantic "
errors+="-Wshadow "
errors+="-Wconversion "
errors+="-Wsign-compare "
errors+="-Wsign-conversion "
errors+="-Wtautological-compare "
errors+="-Wtype-limits "
errors+="-Wno-extra-semi "
errors+="-Wno-gnu-zero-variadic-macro-arguments "
errors+="-Wno-initializer-overrides "
errors+="-Wno-unused-but-set-variable "
errors+="-Wno-unused-function "
errors+="-Wno-unused-local-typedef "
errors+="-Wno-unused-parameter "
errors+="-Wno-unused-value "
errors+="-Wno-unused-variable "
errors+="-Wno-c23-extensions"

if [ -v error_limit ]; then
    errors+="-ferror-limit=5 "
fi

libraries="-lm -lSDL2 -lxcb -lxcb-cursor -lxcb-xkb -lxkbcommon -lxkbcommon-x11 -lEGL -lpthread"
common_compiler_flags="-I. ${errors}"
common_linker_flags="${libraries}"

# Debug flags
debug_compiler_flags="${common_compiler_flags} -g -DENABLE_ASSERT=1 -DDEBUG_BUILD=1"
debug_linker_flags="${common_linker_flags}"

# Release flags
release_compiler_flags="${common_compiler_flags} -O3 -march=native -DRELEASE_BUILD=1"
release_linker_flags="${common_linker_flags}"

# Profile flags
profile_compiler_flags="${common_compiler_flags} -g -O3 -march=native -DPROFILE_BUILD=1 -DTRACY_ENABLE=1"
profile_linker_flags="-fuse-ld=mold ${common_linker_flags} -lstdc++ -lTracyClient"

# Choose options
if [ -v debug ]; then
    echo "Debug build"
    compiler_flags="${debug_compiler_flags}"
    linker_flags="${debug_linker_flags}"
fi
if [ -v release ]; then
    echo "Release build"
    compiler_flags="${release_compiler_flags}"
    linker_flags="${release_linker_flags}"
fi
if [ -v profile ]; then
    echo "Profile build"
    compiler_flags="${profile_compiler_flags}"
    linker_flags="${profile_linker_flags}"
fi

# Build

mkdir -p build

#clang $compiler_flags $linker_flags src/msdf-gen/main.c -o build/msdf-gen
clang $compiler_flags $linker_flags src/msdf-gen/test.c -o build/test
