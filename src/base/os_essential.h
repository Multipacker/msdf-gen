#ifndef OS_ESSENTIAL_H
#define OS_ESSENTIAL_H

typedef struct OS_FileIterator OS_FileIterator;
struct OS_FileIterator {
    U8 data[700];
};

typedef enum {
    OS_SYSTEM_PATH_BINARY,
    OS_SYSTEM_PATH_USER_DATA,
    OS_SYSTEM_PATH_TEMPORARY_DATA,
    OS_SYSTEM_PATH_COUNT,
} OS_SystemPath;

typedef Void OS_ThreadFunction(Void *data);

typedef struct OS_Thread OS_Thread;
struct OS_Thread {
    U64 u64[1];
};

typedef struct OS_Mutex OS_Mutex;
struct OS_Mutex {
    U64 u64[1];
};

typedef struct OS_ConditionVariable OS_ConditionVariable;
struct OS_ConditionVariable {
    U64 u64[1];
};

typedef struct OS_FileInfo OS_FileInfo;
struct OS_FileInfo {
    Str8           name;
    FileProperties properties;
};

// NOTE(simon): Memory.
internal Void *os_memory_reserve(U64 size);
internal Void  os_memory_commit(Void *pointer, U64 size);
internal Void  os_memory_decommit(Void *pointer, U64 size);
internal Void  os_memory_release(Void *pointer, U64 size);

// NOTE(simon): Files.
internal B32 os_file_read(Arena *arena, Str8 file_name, Str8 *result);
internal B32 os_file_write(Str8 file_name, Str8List data);

internal FileProperties os_file_properties(Str8 file_name);

internal B32 os_file_delete(Str8 file_name);
// Moves the file if neccessary and replaces existing files.
internal B32 os_file_rename(Str8 old_name, Str8 new_name);
internal B32 os_file_make_directory(Str8 path);
// The directory must be empty.
internal B32 os_file_delete_directory(Str8 path);

// NOTE(simon): File iteration.
internal OS_FileIterator *os_file_iterator_begin(Arena *arena, Str8 path);
internal B32              os_file_iterator_next(Arena *arena, OS_FileIterator *iterator, OS_FileInfo *info);
internal Void             os_file_iterator_end(OS_FileIterator *iterator);

internal Str8 os_current_directory(Arena *arena);
internal Str8 os_file_path(Arena *arena, OS_SystemPath path);

// NOTE(simon): Time.
internal U64      os_now_nanoseconds(Void);
internal DateTime os_now_universal_time(Void);
internal DateTime os_local_time_from_universal(DateTime *date_time);
internal DateTime os_universal_time_from_local(DateTime *date_time);
internal Void     os_sleep_milliseconds(U64 time);

internal Void os_get_entropy(Void *data, U64 size);

internal B32  os_console_run(Str8 program, Str8List arguments);
internal Void os_console_print(Str8 string);

internal Void os_restart_self(Void);
internal Void os_exit(S32 exit_code);

// NOTE: Called by the os layer on startup.
internal S32 os_run(Str8List arguments);

// NOTE(simon): Threads
internal OS_Thread os_thread_start(OS_ThreadFunction entry_point, Void *data);
internal Void      os_thread_set_name(OS_Thread handle, Str8 name);
internal B32       os_thread_join(OS_Thread thread);
internal Void      os_thread_detach(OS_Thread handle);

// NOTE(simon): Mutexes.
internal OS_Mutex os_mutex_create(Void);
internal Void     os_mutex_destroy(OS_Mutex mutex);
internal Void     os_mutex_lock(OS_Mutex mutex);
internal Void     os_mutex_unlock(OS_Mutex mutex);
#define os_mutex_scope(mutex) defer_loop(os_mutex_lock(mutex), os_mutex_unlock(mutex))

// NOTE(simon): Condition variables.
internal OS_ConditionVariable os_condition_variable_create(Void);
internal Void                 os_condition_variable_destroy(OS_ConditionVariable condition_variable);
internal Void                 os_condition_variable_signal(OS_ConditionVariable condition_variable);
internal Void                 os_condition_variable_broadcast(OS_ConditionVariable condition_variable);
// NOTE(simon): A end_ns of U64_MAX means wait forever.
internal Void                 os_condition_variable_wait(OS_ConditionVariable condition_variable, OS_Mutex mutex, U64 end_ns);

#endif // OS_ESSENTIAL_H
