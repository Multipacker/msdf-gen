# MSDF-gen

This project is meant as a reference implementation of MSDF generation, all
code relevant for it can be found in `src/font/msdf.c` and `src/font/msdf.h`.
Any other primitives that are used can be found in `src/base`. The Windows
backend was hacked together rather quickly, but will be improved later.

MSDFs are generated using the direct multi-channel distance field construction
method described in [Viktor Chlumskys master's thesis][thesis]. I haven't
implemented the collision correction they describe yet, and so there are still
some artifacts.

The approach described in the thesis makes three assumptions that TTF-files do
not satisfy:

1. Contours do not overlap each other.
2. Contours do not self-intersect.
3. A contours winding number in isolation determines if it is adds or removes
   area to the shape.

These constraints turn out to be useful when generating MSDFs, but are annoying
when designing fonts, so we have to convert between the two forms. This is done
by first taking the TTF-encoded contours and loading them in to a uniform
format without any implied control points. Then we take any contours that
overlap, cut then up and shuffle around their pieces so that any contour only
ever intersects with itself. After that, we make sure that contours that
intersect themselves are split into multiple non-self-intersecting contours.

At this point we have contours that represents the exact same thing as the
original one, just without any overlap or self-intersections. TrueType allows
winding numbers of contours to combine in order to cut out holes and combine
shapes (non 0 fill rule). The MSDF algorithm considers contours in isolation
and thus we need to correct the winding numbers. After this pre-processing, the
contours can be sent of for MSDF generation.



## Building and Running

### Windows

Search for `x64 Native Tools Command Prompt for VS <year>` in the Windows Start
Menu. Navigate to the project root and run `scripts\build_msvc.bat`. You can
now run the program with `build\msdf-gen <TTF-file>`.

### Linux

Make sure you have SDL2 installed.

Open a terminal and navigate to the project root. From there, run
`scripts/build_clang.sh`. You can now run the program with `build/msdf-gen
<TTF-file>`.

### MacOS

Not supported for now.



## Usage

You start the program from the command line and provide it with a TTF-file to
load. From there, it will generate an MSDF atlas with all ASCII characters from
the font and display it. You can drag around the atlas with the left mouse
button, and zoom in and out using the scroll wheel. Press tab to switch between
displaying the raw texture and the MSDF render.



## What are the weird lines between the glyphs?

They are the result of texture bleeding. At the edges and corners of entries in
the atlas, we will sample information from up to 4 different glyphs. Due to the
way that MSDFs are rendered, the sudden jumps in the samples will produce these
artefacts. If the glyphs are rendered one by one, the artefacts won't show up.

[thesis]: https://github.com/Chlumsky/msdfgen/files/3050967/thesis.pdf
