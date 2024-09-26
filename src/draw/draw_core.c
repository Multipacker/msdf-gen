internal Void draw_begin_frame(Void) {
    Draw_Context *draw = &global_draw_context;
    if (!draw->arena) {
        draw->arena = arena_create();
    }

    arena_pop_to(draw->arena, 0);

    draw->clip_stack.top      = 0;
    draw->clip_stack.freelist = 0;
    draw->transform_stack.top      = 0;
    draw->transform_stack.freelist = 0;

    draw->stack_generation = 0;
    draw->batch_generation = 0;

    draw->batches.first = 0;
    draw->batches.last  = 0;
    draw->batches.count = 0;

    draw_transform_push(m3f32_identity());
}

internal Void draw_submit(Void) {
    Draw_Context *draw = &global_draw_context;
    render_submit(draw->batches);
}

internal Render_Shape *draw_rectangle(R2F32 rectangle, V4F32 color, F32 radius, F32 thickness, F32 softness) {
    Draw_Context *draw = &global_draw_context;

    Render_Batch *batch = draw->batches.last;
    if (!batch || draw->batch_generation != draw->stack_generation) {
        batch = arena_push_struct_zero(draw->arena,  Render_Batch);
        batch->texture   = render_texture_null();
        batch->clip      = draw_clip_top();
        batch->transform = draw_transform_top();

        sll_queue_push(draw->batches.first, draw->batches.last, batch);
        ++draw->batches.count;
        draw->batch_generation = draw->stack_generation;
    }

    Render_Shape *result = render_shape_list_push(draw->arena, &batch->shapes);

    result->position  = rectangle;
    result->colors[0] = color;
    result->colors[1] = color;
    result->colors[2] = color;
    result->colors[3] = color;
    result->radies[0] = radius;
    result->radies[1] = radius;
    result->radies[2] = radius;
    result->radies[3] = radius;
    result->thickness = thickness;
    result->softness  = softness;
    result->flags     = 0;

    return result;
}

internal Render_Shape *draw_circle(V2F32 center, F32 radius, V4F32 color, F32 thickness, F32 softness) {
    Draw_Context *draw = &global_draw_context;

    Render_Batch *batch = draw->batches.last;
    if (!batch || draw->batch_generation != draw->stack_generation) {
        batch = arena_push_struct_zero(draw->arena,  Render_Batch);
        batch->texture   = render_texture_null();
        batch->clip      = draw_clip_top();
        batch->transform = draw_transform_top();

        sll_queue_push(draw->batches.first, draw->batches.last, batch);
        ++draw->batches.count;
        draw->batch_generation = draw->stack_generation;
    }

    Render_Shape *result = render_shape_list_push(draw->arena, &batch->shapes);

    result->position = r2f32(
        center.x - radius, center.y - radius,
        center.x + radius, center.y + radius
    );
    result->colors[0] = color;
    result->colors[1] = color;
    result->colors[2] = color;
    result->colors[3] = color;
    result->radies[0] = radius;
    result->radies[1] = radius;
    result->radies[2] = radius;
    result->radies[3] = radius;
    result->thickness = thickness;
    result->softness  = softness;
    result->flags     = 0;

    return result;
}

internal Render_Shape *draw_texture(R2F32 rectangle, R2F32 source, Render_Texture texture, V4F32 color, F32 radius, F32 thickness, F32 softness, Render_ShapeFlags flags) {
    Draw_Context *draw = &global_draw_context;

    Render_Batch *batch = draw->batches.last;
    if (batch && render_texture_equal(batch->texture, render_texture_null()) && draw->batch_generation == draw->stack_generation) {
        batch->texture = texture;
    } else if (!batch || (!render_texture_equal(batch->texture, render_texture_null()) && !render_texture_equal(batch->texture, texture)) || draw->batch_generation != draw->stack_generation) {
        batch = arena_push_struct_zero(draw->arena,  Render_Batch);
        batch->texture   = texture;
        batch->clip      = draw_clip_top();
        batch->transform = draw_transform_top();

        sll_queue_push(draw->batches.first, draw->batches.last, batch);
        ++draw->batches.count;
        draw->batch_generation = draw->stack_generation;
    }

    Render_Shape *result = render_shape_list_push(draw->arena, &batch->shapes);

    result->position  = rectangle;
    result->source    = source;
    result->colors[0] = color;
    result->colors[1] = color;
    result->colors[2] = color;
    result->colors[3] = color;
    result->radies[0] = 0;
    result->radies[1] = 0;
    result->radies[2] = 0;
    result->radies[3] = 0;
    result->thickness = thickness;
    result->softness  = softness;
    result->flags     = flags;

    return result;
}

internal Render_Shape *draw_image(R2F32 rectangle, R2F32 source, Render_Texture texture, V4F32 color, F32 radius, F32 thickness, F32 softness) {
    Render_Shape *result = draw_texture(rectangle, source, texture, color, radius, thickness, softness, Render_ShapeFlag_Texture);
    return result;
}

internal Render_Shape *draw_glyph(R2F32 rectangle, R2F32 source, Render_Texture atlas, V4F32 color) {
    Render_Shape *result = draw_texture(rectangle, source, atlas, color, 0.0f, 0.0f, 0.0f, Render_ShapeFlag_AlphaMask);
    return result;
}

internal Render_Shape *draw_msdf(R2F32 rectangle, R2F32 source, Render_Texture atlas, V4F32 color) {
    Render_Shape *result = draw_texture(rectangle, source, atlas, color, 0.0f, 0.0f, 0.0f, Render_ShapeFlag_MSDF);
    return result;
}

internal Render_Shape *draw_line(V2F32 p0, V2F32 p1, V4F32 color, F32 radius, F32 thickness, F32 softness) {
    Draw_Context *draw = &global_draw_context;

    Render_Batch *batch = draw->batches.last;
    if (!batch || draw->batch_generation != draw->stack_generation) {
        batch = arena_push_struct_zero(draw->arena,  Render_Batch);
        batch->texture   = render_texture_null();
        batch->clip      = draw_clip_top();
        batch->transform = draw_transform_top();

        sll_queue_push(draw->batches.first, draw->batches.last, batch);
        ++draw->batches.count;
        draw->batch_generation = draw->stack_generation;
    }

    Render_Shape *result = render_shape_list_push(draw->arena, &batch->shapes);

    result->position.min = p0;
    result->position.max = p1;
    result->colors[0]    = color;
    result->colors[1]    = color;
    result->colors[2]    = color;
    result->colors[3]    = color;
    result->radies[0]    = radius;
    result->radies[1]    = radius;
    result->radies[2]    = radius;
    result->radies[3]    = radius;
    result->thickness    = thickness;
    result->softness     = softness;
    result->flags        = Render_ShapeFlag_Line;

    return result;
}

// TODO(simon): Iterative version
internal Void draw_bezier(V2F32 p0, V2F32 p1, V2F32 p2, V4F32 color, F32 radius, F32 thickness, F32 softness) {
    F32 error = 0.01f;
    F32 lx = 2.0f * f32_abs(p2.x - 2.0f * p1.x + p0.x);
    F32 ly = 2.0f * f32_abs(p2.y - 2.0f * p1.y + p0.y);
    U32 r = (U32) f32_max(0.0f, 0.25f * f32_log2(lx * lx + ly * ly) + 4.0f * error);

    if (r == 0) {
        draw_line(p0, p2, color, radius, thickness, softness);
    } else {
        // NOTE(simon): Split the curve in the middle.
	V2F32 a = v2f32_add(p0, v2f32_scale(v2f32_subtract(p1, p0), 0.5f));
	V2F32 b = v2f32_add(p1, v2f32_scale(v2f32_subtract(p2, p1), 0.5f));
	V2F32 c = v2f32_add(a, v2f32_scale(v2f32_subtract(b, a), 0.5f));

        draw_bezier(p0, a,  c, color, radius, thickness, softness);
        draw_bezier(c,  b, p2, color, radius, thickness, softness);
    }
}
