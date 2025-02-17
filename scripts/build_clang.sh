#!/bin/sh

set -e

for argument in "$@"; do declare $argument='1'; done
if [ ! -v debug ] && [ ! -v release ] && [ ! -v profile ]; then
    echo "No version specified, using debug"
    debug=1
fi
if [ ! -v wayland ] && [ ! -v x11 ]; then
    echo "No backend specified, choosing from XDG_SESSION_TYPE($XDG_SESSION_TYPE)"
    declare $XDG_SESSION_TYPE='1'
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

wayland_libraries=" -lwayland-client -lwayland-egl"
x11_libraries="-lxcb -lxcb-cursor -lxcb-xkb -lxkbcommon-x11"
common_libraries="-lm -lpthread -lEGL -lxkbcommon"

# Choose libraries
if [ -v wayland ]; then
    echo "Wayland backend"
    libraries="${common_libraries} ${wayland_libraries}"
    defines="-DLINUX_WAYLAND=1"

    wayland-scanner client-header < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > src/graphics/wayland/wayland_xdg_shell.generated.h
    wayland-scanner private-code  < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > src/graphics/wayland/wayland_xdg_shell.generated.c
    wayland-scanner client-header < /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml > src/graphics/wayland/wayland_xdg_decoration.generated.h
    wayland-scanner private-code  < /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml > src/graphics/wayland/wayland_xdg_decoration.generated.c
elif [ -v x11 ]; then
    echo "X11 backend"
    libraries="${common_libraries} ${x11_libraries}"
    defines="-DLINUX_X11=1"
fi

common_compiler_flags="-I. ${errors} ${defines}"
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

clang $compiler_flags $linker_flags src/msdf-gen/main.c -o build/msdf-gen
clang $compiler_flags $linker_flags src/msdf-gen/test.c -o build/test
