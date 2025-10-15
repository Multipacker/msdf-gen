thread_local Log *thread_active_log;

// NOTE(simon): Logs
internal Log *log_create(Void) {
    Arena *arena = arena_create();
    Log *log = arena_push_struct(arena, Log);
    log->arena = arena;
    return log;
}

internal Void log_destroy(Log *log) {
    if (thread_active_log == log) {
        thread_active_log = 0;
    }

    arena_destroy(log->arena);
}

internal Void log_select(Log *log) {
    thread_active_log = log;
}



// NOTE(simon): Scopes
internal Void log_scope_begin(Void) {
    if (thread_active_log) {
        U64 position = thread_active_log->arena->position;
        LogScope *scope = arena_push_struct(thread_active_log->arena, LogScope);
        scope->arena_position = position;
        sll_stack_push(thread_active_log->top_scope, scope);
    }
}

internal LogScopeResult log_scope_end(Arena *arena) {
    LogScopeResult result = { 0 };

    if (thread_active_log && thread_active_log->top_scope) {
        LogScope *scope = thread_active_log->top_scope;
        sll_stack_pop(thread_active_log->top_scope);

        if (arena) {
            for (LogMessageKind kind = 0; kind < Log_MessageKind_COUNT; ++kind) {
                result.strings[kind] = str8_join(arena, scope->strings[kind]);
            }
        }

        arena_pop_to(thread_active_log->arena, scope->arena_position);
    }

    return result;
}



// NOTE(simon): Messages
internal Void log_message(LogMessageKind kind, Str8 string) {
    if (thread_active_log && thread_active_log->top_scope) {
        Str8 string_copy = str8_copy(thread_active_log->arena, string);
        str8_list_push(thread_active_log->arena, &thread_active_log->top_scope->strings[kind], string_copy);
    }
}

internal Void log_message_format(LogMessageKind kind, CStr format, ...) {
    if (thread_active_log && thread_active_log->top_scope) {
        va_list arguments;
        va_start(arguments, format);
        Str8 string_copy = str8_format_list(thread_active_log->arena, format, arguments);
        va_end(arguments);

        str8_list_push(thread_active_log->arena, &thread_active_log->top_scope->strings[kind], string_copy);
    }
}
