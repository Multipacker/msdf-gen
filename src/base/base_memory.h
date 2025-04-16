#ifndef MEMORY_H
#define MEMORY_H

typedef struct {
    U8 *memory;
    U64 capacity;
    U64 position;
    U64 commit_position;
} Arena;

typedef struct {
    Arena *arena;
    U64 position;
} Arena_Temporary;

#define ARENA_DEFAULT_RESERVE_SIZE gigabytes(1)
#define ARENA_COMMIT_BLOCK_SIZE    kilobytes(64)

internal Arena *arena_create_reserve(U64 reserve_size);
internal Arena *arena_create(Void);
internal Void   arena_destroy(Arena *arena);

internal Void *arena_push_no_zero(Arena *arena, U64 size, U64 alignment);
internal Void  arena_pop_to(Arena *arena, U64 position);
internal Void  arena_pop_amount(Arena *arena, U64 amount);
internal Void  arena_reset(Arena *arena);

internal Void *arena_push(Arena *arena, U64 size, U64 alignment);

internal Void arena_align_no_zero(Arena *arena, U64 power);
internal Void arena_align(Arena *arena, U64 power);

#define arena_push_struct_no_zero(arena, type)       ((type *) arena_push_no_zero((arena), sizeof(type), _Alignof(type)))
#define arena_push_array_no_zero(arena, type, count) ((type *) arena_push_no_zero((arena), sizeof(type) * (count), _Alignof(type)))

#define arena_push_struct(arena, type)       ((type *) arena_push((arena), sizeof(type), _Alignof(type)))
#define arena_push_array(arena, type, count) ((type *) arena_push((arena), sizeof(type) * (count), _Alignof(type)))

internal Arena_Temporary arena_begin_temporary(Arena *arena);
internal Void            arena_end_temporary(Arena_Temporary temporary);

internal Void            arena_init_scratch(Void);
internal Void            arena_destroy_scratch(Void);
internal Arena_Temporary arena_get_scratch(Arena **conflicts, U32 count);

// NOTE(simon): These assume your buffer is a power of 2 in size.
internal U64 circular_buffer_read(Void *buffer, U64 buffer_size, U64 buffer_position, Void *destination, U64 destination_size);
internal U64 circular_buffer_write(Void *buffer, U64 buffer_size, U64 buffer_position, Void *source, U64 source_size);

#define circular_buffer_write_type(buffer, buffer_size, buffer_position, type) circular_buffer_write(buffer, buffer_size, buffer_position, type, sizeof(type));
#define circular_buffer_read_type(buffer, buffer_size, buffer_position, type)  circular_buffer_read(buffer, buffer_size, buffer_position, type, sizeof(type));

#endif // MEMORY_H
