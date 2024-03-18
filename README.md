# MSDF-gen

The basic idea for taking TTF-files and generating MSDFs from them goes like this:

TTF encoded contours -> expanded contours -> non-overlapping contours ->
non-selfintersecting contours -> normlized winding -> MSDF

The paper describing how to turn vector graphics into MSDFs makes three
assumptions that TTF-files do not guarantee:

1. Contours do not overlapp each other.
2. Contours do not self-intersect.
3. A contours winding number in isolation determines if it is adds or removes
   area to the shape.

These contraints turn out to be usefull when generating MSDFs, but are annoying
when designing fonts, so we have to convert between the two forms. This is done
by first taking the TTF-encoded contours and loading them in to a uniform
format without any implied curves or control points. Then we Take any contours
that overlap, cut then up and shuffle around their pieces so that any contour
only ever intersects with itself. After that, we make sure that contours that
intersect themselves are split into multiple non-self-intersecting contours.

At this point we have a contour that represents the exact same thing as the
original one, just without any overlapp or sefl-intersections. TrueType allows
winding numbers of contours to combine in order to cut out holes and combine
shapes (non 0 fill rule). The MSDF algorithm considers contours in isolation
and thus we need to correct the winding numbers. After this pre-processing, the
contours can be sent of for MSDF generation.
