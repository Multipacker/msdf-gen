global Win32_State win32_state;

internal Win32_Resource *win32_resource_create(Void) {
    Win32_State *state = &win32_state;

    Win32_Resource *result = 0;
    EnterCriticalSection(&state->resource_mutex);

    result = state->resource_freelist;
    if (result) {
        sll_stack_pop(state->resource_freelist);
    } else {
        result = arena_push_struct(state->permanent_arena, Win32_Resource);
    }
    memory_zero_struct(result);

    LeaveCriticalSection(&state->resource_mutex);
    return result;
}

internal Void win32_resource_destroy(Win32_Resource *resource) {
    Win32_State *state = &win32_state;

    EnterCriticalSection(&state->resource_mutex);
    sll_stack_push(state->resource_freelist, resource);
    LeaveCriticalSection(&state->resource_mutex);
}




// NOTE(simon): Helpers for converting between date formats.
internal DateTime win32_date_time_from_system_time(SYSTEMTIME system_time) {
    DateTime result = { 0 };
    result.millisecond = system_time.wMilliseconds;
    result.second      = system_time.wSecond;
    result.minute      = system_time.wMinute;
    result.hour        = system_time.wHour;
    result.day         = system_time.wDay - 1;
    result.month       = system_time.wMonth - 1;
    result.year        = system_time.wYear;
    return result;
}

internal SYSTEMTIME win32_system_time_from_date_time(DateTime date_time) {
    SYSTEMTIME result = { 0 };
    result.wMilliseconds = date_time.millisecond;
    result.wSecond       = date_time.second;
    result.wMinute       = date_time.minute;
    result.wHour         = date_time.hour;
    result.wDay          = 1 + date_time.day;
    result.wMonth        = 1 + date_time.month;
    result.wYear         = date_time.year;
    return result;
}



// NOTE(simon): Memory.
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



// NOTE(simon): Files.
internal B32 os_file_read(Arena *arena, Str8 file_name, Str8 *result) {
    B32 success = true;
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    CStr16 cstr16_file_name = cstr16_from_str8(scratch.arena, file_name);
    HANDLE file = CreateFileW(
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
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr16 file_name_cstr16 = cstr16_from_str8(scratch.arena, file_name);
    HANDLE handle = CreateFileW((WCHAR *) file_name_cstr16, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (handle != INVALID_HANDLE_VALUE) {
        for (Str8Node *node = data.first; node; node = node->next) {
            U8 *ptr = node->string.data;
            U8 *opl = node->string.data + node->string.size;
            while (ptr < opl) {
                U64 bytes_left = (U64) (opl - ptr);
                DWORD write_size = u64_min(bytes_left, megabytes(1));
                DWORD bytes_written = 0;
                BOOL success = WriteFile(handle, ptr, write_size, &bytes_written, 0);
                if (success == 0) {
                    break;
                }

                ptr += bytes_written;
            }
        }

        CloseHandle(handle);
    }
    arena_end_temporary(scratch);
    return false;
}


internal FileProperties os_file_properties(Str8 file_name) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    CStr16 cstr16_file_name = cstr16_from_str8(scratch.arena, file_name);
    HANDLE file = CreateFileW(
        cstr16_file_name,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    FileProperties result = { 0 };

    if (file != INVALID_HANDLE_VALUE) {
        BY_HANDLE_FILE_INFORMATION information = { 0 };
        GetFileInformationByHandle(file, &information);
        if (information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            result.flags |= FILE_PROPERTY_FLAGS_IS_FOLDER;
        }
        result.size = (U64) information.nFileSizeHigh << 32 | (U64) information.nFileSizeLow;

        SYSTEMTIME system_create_time = { 0 };
        FileTimeToSystemTime(&information.ftCreationTime, &system_create_time);
        DateTime create_time = win32_date_time_from_system_time(system_create_time);
        result.create_time = dense_time_from_date_time(&create_time);

        SYSTEMTIME system_modify_time = { 0 };
        FileTimeToSystemTime(&information.ftLastWriteTime, &system_modify_time);
        DateTime modify_time = win32_date_time_from_system_time(system_modify_time);
        result.modify_time = dense_time_from_date_time(&modify_time);

        CloseHandle(file);
    }

    arena_end_temporary(scratch);
    return result;
}

internal B32 os_file_delete(Str8 file_name) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr16 file_name_cstr16 = cstr16_from_str8(scratch.arena, file_name);
    B32 result = DeleteFileW(file_name_cstr16);
    arena_end_temporary(scratch);
    return result;
}

// Moves the file if neccessary and replaces existing files.
// NOTE(simon): This doens't replace existing files.
internal B32 os_file_rename(Str8 old_name, Str8 new_name) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr16 old_name_cstr16 = cstr16_from_str8(scratch.arena, old_name);
    CStr16 new_name_cstr16 = cstr16_from_str8(scratch.arena, new_name);
    B32 result = MoveFileW(old_name_cstr16, new_name_cstr16);
    arena_end_temporary(scratch);
    return result;
}

internal B32 os_file_make_directory(Str8 path) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr16 path_cstr16 = cstr16_from_str8(scratch.arena, path);
    B32 result = CreateDirectoryW(path_cstr16, 0);
    arena_end_temporary(scratch);
    return result;
}

// The directory must be empty.
internal B32 os_file_delete_directory(Str8 path) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr16 path_cstr16 = cstr16_from_str8(scratch.arena, path);
    B32 result = RemoveDirectoryW(path_cstr16);
    arena_end_temporary(scratch);
    return result;
}



// NOTE(simon): File iteration.
internal OS_FileIterator *os_file_iterator_begin(Arena *arena, Str8 path) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    Str8   path_with_wildcard = str8_concatenate(scratch.arena, path, str8_literal("\\*"));
    CStr16 path_cstr16        = cstr16_from_str8(scratch.arena, path_with_wildcard);

    Win32_FileIterator *win32_iterator = (Win32_FileIterator *) arena_push_struct(arena, OS_FileIterator);
    win32_iterator->handle = FindFirstFileExW((WCHAR *) path_cstr16, FindExInfoBasic, &win32_iterator->find_data, FindExSearchNameMatch, 0, FIND_FIRST_EX_LARGE_FETCH);

    arena_end_temporary(scratch);
    return (OS_FileIterator *) win32_iterator;
}

internal B32 os_file_iterator_next(Arena *arena, OS_FileIterator *iterator, OS_FileInfo *info) {
    Win32_FileIterator *win32_iterator = (Win32_FileIterator *) iterator;

    B32 result = false;

    if (!win32_iterator->done && win32_iterator->handle != INVALID_HANDLE_VALUE) {
        do {
            WCHAR *file_name = win32_iterator->find_data.cFileName;
            DWORD attributes = win32_iterator->find_data.dwFileAttributes;

            B32 is_dot    = (file_name[0] == '.' && file_name[1] == 0);
            B32 is_dotdot = (file_name[0] == '.' && file_name[1] == '.' && file_name[2] == 0);

            if (!is_dot && !is_dotdot) {
                info->name = str8_from_str16(arena, str16_cstr16(file_name));
                info->properties.size = (U64) win32_iterator->find_data.nFileSizeHigh << 32 | (U64) win32_iterator->find_data.nFileSizeLow;
                // TODO(simon): Creation time and modification time.
                if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
                    info->properties.flags |= FILE_PROPERTY_FLAGS_IS_FOLDER;
                }

                result = true;

                if (!FindNextFileW(win32_iterator->handle, &win32_iterator->find_data)) {
                    win32_iterator->done = true;
                }

                break;
            }
        } while (FindNextFileW(win32_iterator->handle, &win32_iterator->find_data));
    }

    if (!result) {
        win32_iterator->done = true;
    }

    return result;
}

internal Void os_file_iterator_end(OS_FileIterator *iterator) {
    Win32_FileIterator *win32_iterator = (Win32_FileIterator *) iterator;
    HANDLE zero_handle = { 0 };
    if (!memory_equal(&win32_iterator->handle, &zero_handle, sizeof(zero_handle))) {
        FindClose(win32_iterator->handle);
    }
}



internal Str8 os_current_directory(Arena *arena) {
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    DWORD length = GetCurrentDirectoryW(0, 0);
    U16 *buffer = arena_push_array(scratch.arena, U16, length + 1);
    GetCurrentDirectoryW(length + 1, (WCHAR *) buffer);
    Str8 result = str8_from_str16(arena, str16(buffer, length));

    arena_end_temporary(scratch);
    return result;
}

internal Str8 os_file_path(Arena *arena, OS_SystemPath path) {
    Str8 result = { 0 };
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    switch (path) {
        case OS_SYSTEM_PATH_BINARY: {
            // TODO(simon): Handle insufficient buffer size.
            DWORD size = kilobytes(32);
            U16 *buffer = arena_push_array(scratch.arena, U16, size);
            DWORD length = GetModuleFileNameW(0, (WCHAR *) buffer, size);
            Str8 name = str8_from_str16(scratch.arena, str16(buffer, length));
            Str8 name_chopped = str8_chop_last_slash(name);
            result = str8_copy(arena, name_chopped);
        } break;
        case OS_SYSTEM_PATH_USER_DATA: {
            // TODO(simon): Handle insufficient buffer size.
            DWORD size = kilobytes(32);
            U16 *buffer = arena_push_array(scratch.arena, U16, size);
            if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, (WCHAR *) buffer))) {
                result = str8_from_str16(arena, str16_cstr16(buffer));
            }
        } break;
        case OS_SYSTEM_PATH_TEMPORARY_DATA: {
        } break;
        case OS_SYSTEM_PATH_COUNT: {
        } break;
    }

    arena_end_temporary(scratch);
    return result;
}



// NOTE(simon): Time
internal U64 os_now_nanoseconds(Void) {
    LARGE_INTEGER counter = { 0 };
    QueryPerformanceCounter(&counter);
    U64 result = counter.QuadPart * 1e9 / win32_state.performance_frequency;
    return result;
}

internal DateTime os_now_universal_time(Void) {
    SYSTEMTIME system_time = { 0 };
    GetSystemTime(&system_time);
    DateTime result = win32_date_time_from_system_time(system_time);
    return result;
}

internal DateTime os_local_time_from_universal(DateTime *date_time) {
    SYSTEMTIME system_time = win32_system_time_from_date_time(*date_time);
    FILETIME file_time = { 0 };
    SystemTimeToFileTime(&system_time, &file_time);
    FILETIME file_time_local = { 0 };
    FileTimeToLocalFileTime(&file_time, &file_time_local);
    SYSTEMTIME system_time_local = { 0 };
    FileTimeToSystemTime(&file_time_local, &system_time_local);
    DateTime result = win32_date_time_from_system_time(system_time_local);
    return result;
}

internal DateTime os_universal_time_from_local(DateTime *date_time) {
    SYSTEMTIME system_time_local = win32_system_time_from_date_time(*date_time);
    FILETIME file_time_local = { 0 };
    SystemTimeToFileTime(&system_time_local, &file_time_local);
    FILETIME file_time = { 0 };
    LocalFileTimeToFileTime(&file_time_local, &file_time);
    SYSTEMTIME system_time = { 0 };
    FileTimeToSystemTime(&file_time, &system_time);
    DateTime result = win32_date_time_from_system_time(system_time);
    return result;
}

internal Void os_sleep_milliseconds(U64 time) {
    Sleep(time);
}



internal Void os_get_entropy(Void *data, U64 size) {
    BCryptGenRandom(0, data, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}



internal B32 os_console_run(Str8 program, Str8List arguments) {
    // TODO(simon): Implement
    return false;
}

internal Void os_console_print(Str8 string) {
    Win32_State *state = &win32_state;
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    if (state->standard_output == INVALID_HANDLE_VALUE) {
        // NOTE: In case we are a graphical application that wants to output
        // text. If we already have a console, this will fail.
        AllocConsole();

        state->standard_output = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    Str16 str16 = str16_from_str8(scratch.arena, string);
    DWORD offset = 0;
    do {
        DWORD characters_written = 0;

        BOOL success = WriteConsole(
            state->standard_output,
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



// NOTE(simon): Threads
// TODO(simon): There might be a race condition if you run the following code:
//     OS_Thread thread = os_thread_start(entry_point, data);
//     os_thread_detach(thread);
//     os_thread_start(other_entry_point, other_data);
// If the first thread isn't started before the second call os_thread_start,
// the values in the Linux_Resource will be replaced with new ones, causing
// both threads to use the same entry point with the same data pointer. One
// solution is to store the entry point and data in a separate allocation from
// the thread handle that gets released once the new thread is started. This
// would work as these cannot be accessed through the handle.
internal DWORD os_win32_thread_entry(Void *data) {
    Win32_Resource *thread = (Win32_Resource *) data;
    arena_init_scratch();
    thread->thread.entry_point(thread->thread.data);
    arena_destroy_scratch();
    return 0;
}

internal OS_Thread os_thread_start(OS_ThreadFunction entry_point, Void *data) {
    Win32_Resource *resource = win32_resource_create();
    resource->thread.entry_point = entry_point;
    resource->thread.data = data;
    resource->thread.handle = CreateThread(0, 0, os_win32_thread_entry, resource, 0, &resource->thread.tid);
    OS_Thread result = { 0 };
    result.u64[0] = integer_from_pointer(resource);
    return result;
}

internal Void os_thread_set_name(OS_Thread handle, Str8 name) {
    // TODO(simon): Implement this correctly.
}

internal B32 os_thread_join(OS_Thread handle) {
    Win32_Resource *thread = (Win32_Resource *) pointer_from_integer(handle.u64[0]);
    DWORD wait_result = WaitForSingleObject(thread->thread.handle, INFINITE);
    CloseHandle(thread->thread.handle);
    win32_resource_destroy(thread);
    B32 result = wait_result == WAIT_OBJECT_0;
    return result;
}

internal Void os_thread_detach(OS_Thread handle) {
    Win32_Resource *thread = (Win32_Resource *) pointer_from_integer(handle.u64[0]);
    CloseHandle(thread->thread.handle);
    win32_resource_destroy(thread);
}



// NOTE(simon): Mutexes.
internal OS_Mutex os_mutex_create(Void) {
    Win32_Resource *resource = win32_resource_create();
    InitializeCriticalSection(&resource->mutex);
    OS_Mutex result = { 0 };
    result.u64[0] = integer_from_pointer(resource);
    return result;
}

internal Void os_mutex_destroy(OS_Mutex handle) {
    Win32_Resource *mutex = (Win32_Resource *) pointer_from_integer(handle.u64[0]);
    DeleteCriticalSection(&mutex->mutex);
    win32_resource_destroy(mutex);
}

internal Void os_mutex_lock(OS_Mutex handle) {
    Win32_Resource *mutex = (Win32_Resource *) pointer_from_integer(handle.u64[0]);
    EnterCriticalSection(&mutex->mutex);
}

internal Void os_mutex_unlock(OS_Mutex handle) {
    Win32_Resource *mutex = (Win32_Resource *) pointer_from_integer(handle.u64[0]);
    LeaveCriticalSection(&mutex->mutex);
}



// NOTE(simon): Condition variables.
internal OS_ConditionVariable os_condition_variable_create(Void) {
    Win32_Resource *resource = win32_resource_create();
    InitializeConditionVariable(&resource->condition_variable);
    OS_ConditionVariable result = { 0 };
    result.u64[0] = integer_from_pointer(resource);
    return result;
}

internal Void os_condition_variable_destroy(OS_ConditionVariable handle) {
    Win32_Resource *condition_variable = (Win32_Resource *) pointer_from_integer(handle.u64[0]);
    win32_resource_destroy(condition_variable);
}

internal Void os_condition_variable_signal(OS_ConditionVariable handle) {
    Win32_Resource *condition_variable = (Win32_Resource *) pointer_from_integer(handle.u64[0]);
    WakeConditionVariable(&condition_variable->condition_variable);
}

internal Void os_condition_variable_broadcast(OS_ConditionVariable handle) {
    Win32_Resource *condition_variable = (Win32_Resource *) pointer_from_integer(handle.u64[0]);
    WakeAllConditionVariable(&condition_variable->condition_variable);
}

internal Void os_condition_variable_wait(OS_ConditionVariable condition_variable_handle, OS_Mutex mutex_handle, U64 end_ns) {
    Win32_Resource *condition_variable = (Win32_Resource *) pointer_from_integer(condition_variable_handle.u64[0]);
    Win32_Resource *mutex = (Win32_Resource *) pointer_from_integer(mutex_handle.u64[0]);
    if (end_ns == U64_MAX) {
        SleepConditionVariableCS(&condition_variable->condition_variable, &mutex->mutex, INFINITE);
    } else {
        // TODO(simon): Implement this! Make sure that the we use the same base
        // time as the function we are calling. We probably need to decide on a
        // consistent time base for the entire codebase.
        assert(false);
        //SleepConditionVariableCS(&condition_variable->condition_variable, &mutex->mutex, ...);
    }
}



int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    arena_init_scratch();

    Win32_State *state = &win32_state;

    // NOTE(simon): Get performance counter frequency.
    LARGE_INTEGER frequency = { 0 };
    QueryPerformanceFrequency(&frequency);
    state->performance_frequency = frequency.QuadPart;

    state->standard_output = INVALID_HANDLE_VALUE;

    state->permanent_arena = arena_create();

    for (int i = 0; i < __argc; ++i) {
        Str8 argument = str8_from_str16(state->permanent_arena, str16_cstr16(__wargv[i]));
        str8_list_push(state->permanent_arena, &state->argument_list, argument);
    }

    InitializeCriticalSection(&state->resource_mutex);

    S32 exit_code = os_run(state->argument_list);

    arena_destroy_scratch();

    ExitProcess(exit_code);
}
