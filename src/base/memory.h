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
#define ARENA_COMMIT_BLOCK_SIZE    megabytes(64)

internal Arena arena_create_reserve(U64 reserve_size);
internal Arena arena_create(Void);

internal Void arena_destroy(Arena *arena);

internal Void *arena_push(Arena *arena, U64 size);
internal Void  arena_pop_to(Arena *arena, U64 position);
internal Void  arena_pop_amount(Arena *arena, U64 amount);

internal Void *arena_push_zero(Arena *arena, U64 size);

internal Void arena_align(Arena *arena, U64 power);
internal Void arena_align_zero(Arena *arena, U64 power);

#define arena_push_struct(arena, type)            ((type *) arena_push((arena), sizeof(type)))
#define arena_push_array(arena, type, count)      ((type *) arena_push((arena), sizeof(type) * (count)))

#define arena_push_struct_zero(arena, type)       ((type *) arena_push_zero((arena), sizeof(type)))
#define arena_push_array_zero(arena, type, count) ((type *) arena_push_zero((arena), sizeof(type) * (count)))

internal Arena_Temporary arena_begin_temporary(Arena *arena);
internal Void            arena_end_temporary(Arena_Temporary temporary);

internal Void            arena_init_scratch(Void);
internal Void            arena_destroy_scratch(Void);
internal Arena_Temporary arena_get_scratch(Arena **conflicts, U32 count);

#endif // MEMORY_H
