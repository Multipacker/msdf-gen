#ifndef OS_ESSENTIAL_H
#define OS_ESSENTIAL_H

typedef struct {
    U8 data[512];
} OS_FileIterator;

typedef enum {
    OS_SYSTEM_PATH_CURRENT_DIRECTORY,
    OS_SYSTEM_PATH_BINARY,
    OS_SYSTEM_PATH_USER_DATA,
    OS_SYSTEM_PATH_TEMPORARY_DATA,
    OS_SYSTEM_PATH_COUNT,
} OS_SystemPath;

internal Void *os_memory_reserve(U64 size);
internal Void  os_memory_commit(Void *pointer, U64 size);
internal Void  os_memory_decommit(Void *pointer, U64 size);
internal Void  os_memory_release(Void *pointer, U64 size);

internal B32 os_file_read(Arena *arena, Str8 file_name, Str8 *result);
internal B32 os_file_write(Str8 file_name, Str8List data);

internal FileProperties os_file_properties(Str8 file_name);

internal B32 os_file_delete(Str8 file_name);
// Moves the file if neccessary and replaces existing files.
internal B32 os_file_rename(Str8 old_name, Str8 new_name);
internal B32 os_file_make_directory(Str8 path);
// The directory must be empty.
internal B32 os_file_delete_directory(Str8 path);

internal Void os_file_iterator_initialize(OS_FileIterator *iterator, Str8 path);
internal B32  os_file_iterator_next(Arena *arena, OS_FileIterator *iterator, Str8 *name_out, FileProperties *properties_out);
internal Void os_file_iterator_end(OS_FileIterator *iterator);

internal Str8 os_file_path(Arena *arena, OS_SystemPath path);

internal DateTime os_now_universal_time(Void);
internal DateTime os_local_time_from_universal(DateTime *date_time);
internal DateTime os_universal_time_from_local(DateTime *date_time);

internal U64  os_now_nanoseconds(Void);
internal Void os_sleep_milliseconds(U64 time);

internal Void os_get_entopy(Void *data, U64 size);

internal B32  os_console_run(Str8 program, Str8List arguments);
internal Void os_console_print(Str8 string);

internal Void os_restart_self(Void);
internal Void os_exit(S32 exit_code);

// NOTE: Called by the os layer on startup.
internal S32 os_run(Str8List arguments);

#endif // OS_ESSENTIAL_H
