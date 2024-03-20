internal B32 points_are_collinear(V2F32 a, V2F32 b, V2F32 c) {
    F32 epsilon = 0.0001f;
    V2F32 ba = v2f32_subtract(a, b);
    V2F32 bc = v2f32_subtract(c, b);

    return (f32_abs(v2f32_cross(ba, bc)) < epsilon);
}

internal Void quadratic_bezier_split(V2F32 p0, V2F32 p1, V2F32 p2, F32 t, V2F32 *result_ps) {
    // De Casteljau's algorithm.
    V2F32 a = v2f32_add(p0, v2f32_scale(v2f32_subtract(p1, p0), t));
    V2F32 b = v2f32_add(p1, v2f32_scale(v2f32_subtract(p2, p1), t));
    V2F32 c = v2f32_add(a, v2f32_scale(v2f32_subtract(b, a), t));

    result_ps[0] = p0;
    result_ps[1] = a;
    result_ps[2] = c;

    result_ps[3] = c;
    result_ps[4] = b;
    result_ps[5] = p2;
}
