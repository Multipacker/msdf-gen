#ifndef GEOMETRY_H
#define GEOMETRY_H

internal B32 points_are_collinear(V2F32 a, V2F32 b, V2F32 c);

internal Void quadratic_bezier_split(V2F32 p0, V2F32 p1, V2F32 p2, F32 t, V2F32 *result_ps);

#endif // GEOMETRY_H
