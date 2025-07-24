internal Void draw_begin_frame(Void) {
    prof_function_begin();
    Draw_Context *draw = &global_draw_context;
    if (!draw->arena) {
        draw->arena = arena_create();
    }

    arena_reset(draw->arena);

    draw->list_stack = 0;
    prof_function_end();
}

internal Void draw_submit_list(Gfx_Window graphics_window, Render_Window render_window, Draw_List *list) {
    render_window_submit(graphics_window, render_window, list->batches);
}



internal Draw_List *draw_list_create(Void) {
    Draw_Context *draw = &global_draw_context;
    Draw_List *result = arena_push_struct(draw->arena, Draw_List);

    Draw_M3F32StackNode *transform_node = arena_push_struct(draw->arena, Draw_M3F32StackNode);
    transform_node->item = m3f32_identity();
    sll_stack_push(result->transform_stack.top, transform_node);

    Draw_R2F32StackNode *clip_node = arena_push_struct(draw->arena, Draw_R2F32StackNode);
    clip_node->item = r2f32(-10000.0f, -10000.0f, 10000.0f, 10000.0f);
    sll_stack_push(result->clip_stack.top, clip_node);

    Draw_FilteringStackNode *filtering_node = arena_push_struct(draw->arena, Draw_FilteringStackNode);
    filtering_node->item = Render_Filtering_Linear;
    sll_stack_push(result->filtering_stack.top, filtering_node);

    return result;
}

internal Void draw_list_push(Draw_List *list) {
    Draw_Context *draw = &global_draw_context;
    sll_stack_push(draw->list_stack, list);
}

internal Void draw_list_pop(Void) {
    Draw_Context *draw = &global_draw_context;
    sll_stack_pop(draw->list_stack);
}

internal Draw_List *draw_list_top(Void) {
    Draw_Context *draw = &global_draw_context;
    Draw_List *result = draw->list_stack;
    return result;
}



internal Render_Shape *draw_rectangle(R2F32 rectangle, V4F32 color, F32 radius, F32 thickness, F32 softness) {
    Arena *arena = global_draw_context.arena;
    Draw_List *list = draw_list_top();

    Render_Batch *batch = list->batches.last;
    if (!batch || list->batch_generation != list->stack_generation) {
        batch = arena_push_struct(arena,  Render_Batch);
        batch->texture   = render_texture_null();
        batch->clip      = draw_clip_top();
        batch->transform = draw_transform_top();
        batch->filtering = draw_filtering_top();

        sll_queue_push(list->batches.first, list->batches.last, batch);
        ++list->batches.count;
        list->batch_generation = list->stack_generation;
    }

    Render_Shape *result = render_shape_list_push(arena, &batch->shapes);

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
    Arena *arena = global_draw_context.arena;
    Draw_List *list = draw_list_top();

    Render_Batch *batch = list->batches.last;
    if (!batch || list->batch_generation != list->stack_generation) {
        batch = arena_push_struct(arena,  Render_Batch);
        batch->texture   = render_texture_null();
        batch->clip      = draw_clip_top();
        batch->transform = draw_transform_top();
        batch->filtering = draw_filtering_top();

        sll_queue_push(list->batches.first, list->batches.last, batch);
        ++list->batches.count;
        list->batch_generation = list->stack_generation;
    }

    Render_Shape *result = render_shape_list_push(arena, &batch->shapes);

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
    Arena *arena = global_draw_context.arena;
    Draw_List *list = draw_list_top();

    Render_Batch *batch = list->batches.last;
    if (batch && render_texture_equal(batch->texture, render_texture_null()) && list->batch_generation == list->stack_generation) {
        batch->texture = texture;
    } else if (!batch || (!render_texture_equal(batch->texture, render_texture_null()) && !render_texture_equal(batch->texture, texture)) || list->batch_generation != list->stack_generation) {
        batch = arena_push_struct(arena,  Render_Batch);
        batch->texture   = texture;
        batch->clip      = draw_clip_top();
        batch->transform = draw_transform_top();
        batch->filtering = draw_filtering_top();

        sll_queue_push(list->batches.first, list->batches.last, batch);
        ++list->batches.count;
        list->batch_generation = list->stack_generation;
    }

    Render_Shape *result = render_shape_list_push(arena, &batch->shapes);

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
    Arena *arena = global_draw_context.arena;
    Draw_List *list = draw_list_top();

    Render_Batch *batch = list->batches.last;
    if (!batch || list->batch_generation != list->stack_generation) {
        batch = arena_push_struct(arena,  Render_Batch);
        batch->texture   = render_texture_null();
        batch->clip      = draw_clip_top();
        batch->transform = draw_transform_top();
        batch->filtering = draw_filtering_top();

        sll_queue_push(list->batches.first, list->batches.last, batch);
        ++list->batches.count;
        list->batch_generation = list->stack_generation;
    }

    Render_Shape *result = render_shape_list_push(arena, &batch->shapes);

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

internal Void draw_sub_list(Draw_List *sub_list) {
    Arena *arena = global_draw_context.arena;
    Draw_List *list = draw_list_top();

    for (Render_Batch *src_batch = sub_list->batches.first; src_batch; src_batch = src_batch->next) {
        Render_Batch *batch = arena_push_struct(arena, Render_Batch);
        batch->shapes    = src_batch->shapes;
        batch->texture   = src_batch->texture;
        batch->filtering = src_batch->filtering;
        batch->transform = m3f32_multiply_m3f32(draw_transform_top(), src_batch->transform);
        batch->clip      = r2f32_intersect(draw_clip_top(), src_batch->clip);

        sll_queue_push(list->batches.first, list->batches.last, batch);

        ++list->batches.count;
    }

    // NOTE(simon): We cannot modify any batches that we have merged. This
    // forces us to allocate a new batch if we add more draw commands to the
    // sub-list.
    ++sub_list->batch_generation;
}
