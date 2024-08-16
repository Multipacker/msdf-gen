internal Void draw_begin_frame(Void) {
    Draw_Context *draw = &global_draw_context;
    if (!draw->arena) {
        draw->arena = arena_create();
    }

    arena_pop_to(draw->arena, 0);

    draw->clip_stack.top      = 0;
    draw->clip_stack.freelist = 0;

    draw->batches.first = 0;
    draw->batches.last  = 0;
    draw->batches.count = 0;
}

internal Void draw_submit(Void) {
    Draw_Context *draw = &global_draw_context;
    render_submit(draw->batches);
}

internal Render_Shape *draw_rectangle(R2F32 rectangle, V4F32 color, F32 radius, F32 thickness, F32 softness) {
    Draw_Context *draw = &global_draw_context;

    Render_Batch *batch = draw->batches.last;
    if (!batch) {
        batch = arena_push_struct_zero(draw->arena,  Render_Batch);
        batch->texture = render_texture_null();
        batch->clip    = draw_clip_top();

        sll_queue_push(draw->batches.first, draw->batches.last, batch);
        ++draw->batches.count;
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
    if (!batch) {
        batch = arena_push_struct_zero(draw->arena,  Render_Batch);
        batch->texture = render_texture_null();
        batch->clip    = draw_clip_top();

        sll_queue_push(draw->batches.first, draw->batches.last, batch);
        ++draw->batches.count;
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

internal Render_Shape *draw_texture(R2F32 rectangle, R2F32 uvs, Render_Texture texture, V4F32 color, F32 radius, F32 thickness, F32 softness, Render_ShapeFlags flags) {
    Draw_Context *draw = &global_draw_context;

    Render_Batch *batch = draw->batches.last;
    if (batch && render_texture_equal(batch->texture, render_texture_null())) {
        batch->texture = texture;
    } else if (!batch || (!render_texture_equal(batch->texture, render_texture_null()) && !render_texture_equal(batch->texture, texture))) {
        batch = arena_push_struct_zero(draw->arena,  Render_Batch);
        batch->texture = texture;
        batch->clip    = draw_clip_top();

        sll_queue_push(draw->batches.first, draw->batches.last, batch);
        ++draw->batches.count;
    }

    Render_Shape *result = render_shape_list_push(draw->arena, &batch->shapes);

    result->position  = rectangle;
    result->uvs       = uvs;
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

internal Render_Shape *draw_image(R2F32 rectangle, R2F32 uvs, Render_Texture texture, V4F32 color, F32 radius, F32 thickness, F32 softness) {
    Render_Shape *result = draw_texture(rectangle, uvs, texture, color, radius, thickness, softness, Render_ShapeFlag_Texture);
    return result;
}

internal Render_Shape *draw_glyph(R2F32 rectangle, R2F32 uvs, Render_Texture atlas, V4F32 color) {
    Render_Shape *result = draw_texture(rectangle, uvs, atlas, color, 0.0f, 0.0f, 0.0f, Render_ShapeFlag_AlphaMask);
    return result;
}

internal Render_Shape *draw_msdf(R2F32 rectangle, R2F32 uvs, Render_Texture atlas, V4F32 color) {
    Render_Shape *result = draw_texture(rectangle, uvs, atlas, color, 0.0f, 0.0f, 0.0f, Render_ShapeFlag_MSDF);
    return result;
}

internal Render_Shape *draw_line(V2F32 p0, V2F32 p1, V4F32 color, F32 radius, F32 thickness, F32 softness) {
    Draw_Context *draw = &global_draw_context;

    Render_Batch *batch = draw->batches.last;
    if (!batch) {
        batch = arena_push_struct_zero(draw->arena,  Render_Batch);
        batch->texture = render_texture_null();
        batch->clip    = draw_clip_top();

        sll_queue_push(draw->batches.first, draw->batches.last, batch);
        ++draw->batches.count;
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
