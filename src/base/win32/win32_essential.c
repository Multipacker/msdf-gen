global Arena    *win32_permanent_arena;
global Str8List win32_argument_list;
global HANDLE   win32_standard_output = INVALID_HANDLE_VALUE;

internal Void *os_memory_reserve(U64 size) {
    Void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return(result);
}

internal Void os_memory_commit(Void *pointer, U64 size) {
    VirtualAlloc(pointer, size, MEM_COMMIT, PAGE_READWRITE);
}

internal Void os_memory_decommit(Void *pointer, U64 size) {
    VirtualFree(pointer, size, MEM_DECOMMIT);
}

internal Void os_memory_release(Void *pointer, U64 size) {
    VirtualFree(pointer, 0, MEM_RELEASE);
}


internal B32 os_file_read(Arena *arena, Str8 file_name, Str8 *result) {
    B32 success = true;
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    CStr16 cstr16_file_name = cstr16_from_str8(scratch.arena, file_name);
    HANDLE file = CreateFile(
        cstr16_file_name,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    if (file != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER file_size = { 0 };
        if (GetFileSizeEx(file, &file_size)) {
            result->data = arena_push_array(arena, U8, (U64) file_size.QuadPart);
            result->size = file_size.QuadPart;

            U64 offset = 0;
            do {
                DWORD bytes_read = 0;
                success &= ReadFile(
                    file,
                    &result->data[offset],
                    result->size - offset,
                    &bytes_read,
                    0
                );

                offset += bytes_read;

                if (!success) {
                    arena_pop_amount(arena, result->size);
                    // TODO: Error
                    break;
                }
            } while (offset != result->size);
        } else {
            // TODO: Error
            success = false;
        }

        CloseHandle(file);
    } else {
        // TODO: Error
        success = false;
    }

    arena_end_temporary(scratch);
    return success;
}

internal B32 os_file_write(Str8 file_name, Str8List data) {
    // TODO: Implement
    return false;
}


internal FileProperties os_file_properties(Str8 file_name) {
    // TODO: Implement
    return (FileProperties) { 0 };
}


internal B32 os_file_delete(Str8 file_name) {
    // TODO: Implement
    return false;
}

// Moves the file if neccessary and replaces existing files.
internal B32 os_file_rename(Str8 old_name, Str8 new_name) {
    // TODO: Implement
    return false;
}

internal B32 os_file_make_directory(Str8 path) {
    // TODO: Implement
    return false;
}

// The directory must be empty.
internal B32 os_file_delete_directory(Str8 path) {
    // TODO: Implement
    return false;
}


internal Void os_file_iterator_initialize(OS_FileIterator *iterator, Str8 path) {
    // TODO: Implement
}

internal B32 os_file_iterator_next(Arena *arena, OS_FileIterator *iterator, Str8 *name_out, FileProperties *properties_out) {
    // TODO: Implement
    return false;
}

internal Void os_file_iterator_end(OS_FileIterator *iterator) {
    // TODO: Implement
}


internal Str8 os_file_path(Arena *arena, OS_SystemPath path) {
    // TODO: Implement
    return (Str8) { 0 };
}


internal DateTime os_now_universal_time(Void) {
    // TODO: Implement
    return (DateTime) { 0 };
}

internal DateTime os_local_time_from_universal(DateTime *date_time) {
    // TODO: Implement
    return (DateTime) { 0 };
}

internal DateTime os_universal_time_from_local(DateTime *date_time) {
    // TODO: Implement
    return (DateTime) { 0 };
}


internal U64 os_now_nanoseconds(Void) {
    LARGE_INTEGER counter = { 0 };
    QueryPerformanceCounter(&counter);
    // TODO: Implement
    return 0;
}

internal Void os_sleep_milliseconds(U64 time) {
    // TODO: Implement
}


internal Void os_get_entropy(Void *data, U64 size) {
    // TODO: Implement
}


internal B32 os_console_run(Str8 program, Str8List arguments) {
    return false;
}

internal Void os_console_print(Str8 string) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    if (win32_standard_output == INVALID_HANDLE_VALUE) {
        // NOTE: In case we are a graphical application that wants to output
        // text. If we already have a console, this will fail.
        AllocConsole();

        win32_standard_output = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    Str16 str16 = str16_from_str8(scratch.arena, string);
    DWORD offset = 0;
    do {
        DWORD characters_written = 0;

        BOOL success = WriteConsole(
            win32_standard_output,
            &str16.data[offset],
            str16.size - offset,
            &characters_written,
            0
        );
        offset += characters_written;

        if (!success) {
            // TODO: Handle errors properly.
            break;
        }
    } while (offset != str16.size);

    arena_end_temporary(scratch);
}


internal Void os_restart_self(Void) {
    // TODO: Implement
}

internal Void os_exit(S32 exit_code) {
    ExitProcess(exit_code);
}

int main(int argument_count, char *arguments[]) {
    arena_init_scratch();

    win32_permanent_arena = arena_create();

    for (int i = 0; i < argument_count; ++i) {
        Str8 argument = str8_cstr(arguments[i]);
        str8_list_push(win32_permanent_arena, &win32_argument_list, argument);
    }

    S32 exit_code = os_run(win32_argument_list);

    arena_destroy_scratch();

    ExitProcess(exit_code);
}
