internal Render_Shape *render_shape_list_push(Arena *arena, Render_ShapeList *shapes) {
    Render_ShapeChunk *chunk = shapes->last;

    if (!chunk || chunk->count == chunk->capacity) {
        chunk = arena_push_struct(arena, Render_ShapeChunk);
        chunk->capacity = 32;
        chunk->shapes = arena_push_array_no_zero(arena, Render_Shape, chunk->capacity);
        sll_queue_push(shapes->first, shapes->last, chunk);
        ++shapes->chunk_count;
    }

    Render_Shape *result = &chunk->shapes[chunk->count];
    memory_zero_struct(result);
    ++chunk->count;
    ++shapes->shape_count;

    return result;
}
