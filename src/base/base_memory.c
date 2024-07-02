#define THREAD_SCRATCH_ARENA_POOL_SIZE 2
thread_local Arena *thread_scratch_arenas[THREAD_SCRATCH_ARENA_POOL_SIZE];

internal Arena *arena_create_reserve(U64 reserve_size) {
    U8 *memory         = os_memory_reserve(reserve_size);
    U64 initial_commit = u64_max(u64_ceil_to_power_of_2(sizeof(Arena)), ARENA_COMMIT_BLOCK_SIZE);
    os_memory_commit(memory, initial_commit);

    Arena *result = (Arena *) memory;

    result->memory           = memory;
    result->capacity         = reserve_size;
    result->position         = sizeof(Arena);
    result->commit_position  = initial_commit;

    return result;
}

internal Arena *arena_create(Void) {
    return arena_create_reserve(ARENA_DEFAULT_RESERVE_SIZE);
}

internal Void arena_destroy(Arena *arena) {
    os_memory_release(arena->memory, arena->capacity);
}

internal Void *arena_push(Arena *arena, U64 size) {
    Void *result = 0;

    if (arena->position + size <= arena->capacity) {
        result           = arena->memory + arena->position;
        arena->position += size;

        if (arena->position > arena->commit_position) {
            U64 position_aligned     = u64_round_up_to_power_of_2(arena->position, ARENA_COMMIT_BLOCK_SIZE);
            U64 next_commit_position = u64_min(position_aligned, arena->capacity);
            U64 commit_size          = next_commit_position - arena->commit_position;
            os_memory_commit(arena->memory + arena->commit_position, commit_size);
            arena->commit_position = next_commit_position;
        }
    }

    return result;
}

internal Void arena_pop_to(Arena *arena, U64 position) {
    position = u64_max(sizeof(Arena), position);

    if (position < arena->position) {
        arena->position = position;

        U64 position_aligned     = u64_round_up_to_power_of_2(arena->position, ARENA_COMMIT_BLOCK_SIZE);
        U64 next_commit_position = u64_min(position_aligned, arena->capacity);
        if (next_commit_position < arena->commit_position) {
            U64 decommit_size = arena->commit_position - next_commit_position;
            os_memory_decommit(arena->memory + next_commit_position, decommit_size);
            arena->commit_position = next_commit_position;
        }
    }
}

internal Void arena_pop_amount(Arena *arena, U64 amount) {
    arena_pop_to(arena, arena->position - amount);
}

internal Void arena_reset(Arena *arena) {
    arena_pop_to(arena, sizeof(Arena));
}

internal Void *arena_push_zero(Arena *arena, U64 size) {
    Void *result = arena_push(arena, size);
    memory_zero(result, size);
    return result;
}

internal Void arena_align(Arena *arena, U64 power) {
    U64 position_aligned = u64_round_up_to_power_of_2(arena->position, power);
    U64 align = position_aligned - arena->position;
    if (align) {
        arena_push(arena, align);
    }
}

internal Void arena_align_zero(Arena *arena, U64 power) {
    U64 position_aligned = u64_round_up_to_power_of_2(arena->position, power);
    U64 align = position_aligned - arena->position;
    if (align) {
        arena_push_zero(arena, align);
    }
}

internal Arena_Temporary arena_begin_temporary(Arena *arena) {
    Arena_Temporary result;
    result.arena = arena;
    result.position = arena->position;

    return result;
}

internal Void arena_end_temporary(Arena_Temporary temporary) {
    arena_pop_to(temporary.arena, temporary.position);
}

internal Void arena_init_scratch(Void) {
    for (U32 i = 0; i < array_count(thread_scratch_arenas); ++i) {
        thread_scratch_arenas[i] = arena_create();
    }
}

internal Void arena_destroy_scratch(Void) {
    for (U32 i = 0; i < array_count(thread_scratch_arenas); ++i) {
        arena_destroy(thread_scratch_arenas[i]);
    }
}

internal Arena_Temporary arena_get_scratch(Arena **conflicts, U32 count) {
    Arena *selected = 0;

    for (U32 i = 0; i < array_count(thread_scratch_arenas); ++i) {
        Arena *arena = thread_scratch_arenas[i];

        B32 is_non_conflicting = true;
        for (U32 j = 0; j < count; ++j) {
            if (arena == conflicts[j]) {
                is_non_conflicting = false;
                break;
            }
        }

        if (is_non_conflicting) {
            selected = arena;
            break;
        }
    }

    return arena_begin_temporary(selected);
}
