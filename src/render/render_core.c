internal Render_Rectangle *render_rectangle_list_push(Arena *arena, Render_RectangleList *rectangles) {
    Render_RectangleChunk *chunk = rectangles->last;

    if (!chunk || chunk->count == chunk->capacity) {
        chunk = arena_push_struct_zero(arena,  Render_RectangleChunk);
        chunk->capacity = 512;
        chunk->rectangles = arena_push_array_zero(arena, Render_Rectangle, chunk->capacity);
        sll_queue_push(rectangles->first, rectangles->last, chunk);
        ++rectangles->chunk_count;
    }

    Render_Rectangle *result = &chunk->rectangles[chunk->count];
    ++chunk->count;
    ++rectangles->rectangle_count;

    return result;
}
