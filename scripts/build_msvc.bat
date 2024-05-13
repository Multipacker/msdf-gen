@echo off

rem 4201: nonstandard extension used: nameless struct/union
rem 4152: nonstandard extension used: function/data ptr conversion in expression
rem 4100: unreferenced formal parameter
rem 4189: local variable is initialized but not referenced
rem 4101: unreferenced local variable
rem 4310: cast truncates constant value
rem 4061: enum case not explicitly handled
rem 4820: n bytes padding added after data member x
rem 5045: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
rem 4711: x selected for automatic inline expansion
rem 4710: function not inlined
rem 4242: 4242 conversion from U32 to U16
rem 4244: 4244 conversion from U32 to U8

rem -- Common flags --

set disabled_warnings=-wd4201 -wd4152 -wd4100 -wd4189 -wd4101 -wd4310 -wd4061 -wd4820 -wd4191 -wd5045 -wd4711 -wd4710 -wd4242 -wd4244
set compiler_flags=-nologo -FC -Wall -MP -WX %disabled_warnings% -I..
set libs=
set linker_flags=%libs% -incremental:no

if not exist build mkdir build
pushd build

cl ../src/msdf-gen/main.c %compiler_flags% -link %linker_flags%

popd
