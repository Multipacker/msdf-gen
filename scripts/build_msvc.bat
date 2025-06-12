@echo off
setlocal

for %%a in (%*) do set "%%a=1"
if not "%release%" == "1" if not "%profile%" == "1" set debug=1

:: 4201: nonstandard extension used: nameless struct/union
:: 4152: nonstandard extension used: function/data ptr conversion in expression
:: 4100: unreferenced formal parameter
:: 4189: local variable is initialized but not referenced
:: 4101: unreferenced local variable
:: 4310: cast truncates constant value
:: 4061: enum case not explicitly handled
:: 4820: n bytes padding added after data member x
:: 5045: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
:: 4711: x selected for automatic inline expansion
:: 4710: function not inlined
:: 4242: conversion from U32 to U16
:: 4244: conversion from U32 to U8
:: 4668: undefined preprocessor macros

:: Common flags

set disabled_warnings=-wd4201 -wd4152 -wd4100 -wd4189 -wd4101 -wd4310 -wd4061 -wd4820 -wd4191 -wd5045 -wd4711 -wd4710 -wd4242 -wd4244 -wd4668

set libs=User32.lib Opengl32.lib Gdi32.lib Shell32.lib Bcrypt.lib Shell32.lib
set common_compiler_flags=-nologo -FC -Wall -MP -WX %disabled_warnings% -I.. -diagnostics:caret -diagnostics:color
set common_linker_flags=%libs% -incremental:no

:: Debug flags
set debug_compiler_flags=%common_compiler_flags% -RTC1 -MTd -Zi -DCONSOLE=1 -DENABLE_ASSERT=1 -DDEBUG_BUILD=1
set debug_linker_flags=%common_linker_flags%

:: Release flags
set release_compiler_flags=%common_compiler_flags% -MT -O2 -Oi -EHsc -GS- -DRELEASE_BUILD=1
set release_linker_flags=%common_linker_flags% -fixed -opt:icf -opt:ref -subsystem:windows libvcruntime.lib

set render_libraries=opengl.o

:: Choose options
if "%debug%" == "1" (
    echo Debug build
    set compiler_flags=%debug_compiler_flags%
    set linker_flags=%debug_linker_flags%
)
if "%release%" == "1" (
    echo Release build
    set compiler_flags=%release_compiler_flags%
    set linker_flags=%release_linker_flags%
)
if "%profile%" == "1" (
    echo Profile build
    echo NOT IMPLEMENTED YET
    exit 1
)

if not exist build mkdir build

:: Build and run metaprogram
pushd build
cl ../src/meta/main.c %compiler_flags% -link %linker_flags% -out:"meta.exe" || exit /b 1
popd
build\meta || exit /b 1

pushd build
cl ../src/msdf-gen/main.c %compiler_flags% -link %linker_flags% %render_libraries% msdf-gen.o -out:"msdf-gen.exe" || exit /b 1
popd
