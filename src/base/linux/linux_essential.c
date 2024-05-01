#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/syscall.h>

global Arena *linux_permanent_arena;
global Str8List linux_argument_list;

internal DateTime linux_date_time_from_tm_and_milliseconds(struct tm *time, U16 milliseconds) {
    DateTime result = { 0 };
    result.millisecond = milliseconds;
    result.second      = time->tm_sec;
    result.minute      = time->tm_min;
    result.hour        = time->tm_hour;
    result.day         = time->tm_mday - 1;
    result.month       = time->tm_mon;
    result.year        = time->tm_year + 1900;

    return result;
}

internal struct tm linux_tm_from_date_time(DateTime *date_time) {
    struct tm result = { 0 };
    result.tm_sec    = date_time->second;
    result.tm_min    = date_time->minute;
    result.tm_hour   = date_time->hour;
    result.tm_mday   = date_time->day + 1;
    result.tm_mon    = date_time->month;
    result.tm_year   = date_time->year - 1900;
    return result;
}

internal Void linux_file_properties_from_stat(FileProperties *properties, struct stat *metadata) {
    properties->size = metadata->st_size;
    properties->flags = 0;
    if (S_ISDIR(metadata->st_mode)) {
        properties->flags |= FILE_PROPERTY_FLAGS_IS_FOLDER;
    }
    properties->access |= ((metadata->st_mode & S_IRUSR) || (metadata->st_mode & S_IRGRP) || (metadata->st_mode & S_IROTH) ? DATA_ACCESS_FLAGS_READ : 0);
    properties->access |= ((metadata->st_mode & S_IWUSR) || (metadata->st_mode & S_IWGRP) || (metadata->st_mode & S_IWOTH) ? DATA_ACCESS_FLAGS_WRITE : 0);
    properties->access |= ((metadata->st_mode & S_IXUSR) || (metadata->st_mode & S_IXGRP) || (metadata->st_mode & S_IXOTH) ? DATA_ACCESS_FLAGS_EXECUTE : 0);
    properties->create_time = 0; // TODO: Figure out how to acquire creation time.

    struct tm deconstructed_modify_time = { 0 };
    if (gmtime_r(&metadata->st_mtim.tv_sec, &deconstructed_modify_time) == &deconstructed_modify_time) {
        DateTime modify_date_time = linux_date_time_from_tm_and_milliseconds(&deconstructed_modify_time, metadata->st_mtim.tv_nsec / 1000000);
        properties->modify_time = dense_time_from_date_time(&modify_date_time);
    } else {
        properties->modify_time = 0;
    }
}

internal Void *os_memory_reserve(U64 size) {
    Void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return result;
}

internal Void os_memory_commit(Void *pointer, U64 size) {
    mprotect(pointer, size, PROT_READ | PROT_WRITE);
}

internal Void os_memory_decommit(Void *pointer, U64 size) {
    mprotect(pointer, size, PROT_NONE);
    madvise(pointer, size, MADV_DONTNEED);
}

internal Void os_memory_release(Void *pointer, U64 size) {
    munmap(pointer, size);
}

internal B32 os_file_read(Arena *arena, Str8 file_name, Str8 *result) {
    // Open a file_descriptor
    Arena_Temporary scratch_restore = arena_begin_temporary(arena);
    CStr file_name_c = cstr_from_str8(arena, file_name);
    S32 file_descriptor = open(file_name_c, O_RDONLY);
    arena_end_temporary(scratch_restore);

    B32 success = false;

    if (file_descriptor != -1) {
        struct stat metadata = { 0 };
        if (fstat(file_descriptor, &metadata) != -1) {
            U64 total_size = metadata.st_size;
            Arena_Temporary restore_point = arena_begin_temporary(arena);
            U8 *buffer = arena_push_array(arena, U8, total_size);
            U8 *ptr = buffer;
            U8 *opl = buffer + total_size;

            success = true;
            while (ptr < opl) {
                U64 total_to_read = u64_min((U64) (opl - ptr), SSIZE_MAX);
                S64 actual_read = read(file_descriptor, ptr, total_to_read);
                if (actual_read == -1) {
                    success = false;
                    break;
                }

                ptr += actual_read;
            }

            if (success) {
                result->data = buffer;
                result->size = total_size;
            } else {
                arena_end_temporary(restore_point);
            }
        }

        close(file_descriptor);
    }

    return success;
}

internal B32 os_file_write(Str8 file_name, Str8List data) {
    // Open a file_descriptor
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr file_name_c = cstr_from_str8(scratch.arena, file_name);
    S32 file_descriptor = creat(file_name_c, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    arena_end_temporary(scratch);

    B32 result = false;
    if (file_descriptor != -1) {
        result = true;

        for (Str8Node *node = data.first; node; node = node->next) {
            U8 *ptr = node->string.data;
            U8 *opl = node->string.data + node->string.size;

            while (ptr < opl) {
                U64 to_write = u64_min((U64) (opl - ptr), SSIZE_MAX);
                S64 acutal_write = write(file_descriptor, ptr, to_write);
                if (acutal_write == -1) {
                    result = false;
                    goto finished_write;
                }

                ptr += acutal_write;
            }
        }
        finished_write:;

        close(file_descriptor);
    }

    return result;
}

internal FileProperties os_file_properties(Str8 file_name) {
    FileProperties result = { 0 };

    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr file_name_c = cstr_from_str8(scratch.arena, file_name);
    struct stat metadata = { 0 };
    if (stat(file_name_c, &metadata) != -1) {
        linux_file_properties_from_stat(&result, &metadata);
    }

    arena_end_temporary(scratch);

    return result;
}

internal B32 os_file_delete(Str8 file_name) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr file_name_c = cstr_from_str8(scratch.arena, file_name);

    B32 success = unlink(file_name_c) == 0;

    arena_end_temporary(scratch);
    return success;
}

internal B32 os_file_rename(Str8 old_name, Str8 new_name) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr old_name_c = cstr_from_str8(scratch.arena, old_name);
    CStr new_name_c = cstr_from_str8(scratch.arena, new_name);

    B32 success = rename(old_name_c, new_name_c) == 0;

    return success;
}

internal B32 os_file_make_directory(Str8 path) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr path_c = cstr_from_str8(scratch.arena, path);

    B32 success = mkdir(path_c, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0;

    arena_end_temporary(scratch);
    return success;
}

internal B32 os_file_delete_directory(Str8 path) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr path_c = cstr_from_str8(scratch.arena, path);

    B32 success = rmdir(path_c) == 0;

    arena_end_temporary(scratch);
    return success;
}

internal Void os_file_iterator_initialize(OS_FileIterator *iterator, Str8 path) {
    memory_zero_struct(iterator);
    Linux_FileIterator *linux_iterator = (Linux_FileIterator *) iterator;

    Arena_Temporary scratch = arena_get_scratch(0, 0);
    CStr path_c = cstr_from_str8(scratch.arena, path);
    linux_iterator->file_descriptor = open(path_c, O_RDONLY | O_DIRECTORY);
    arena_end_temporary(scratch);
}

internal B32 os_file_iterator_next(Arena *arena, OS_FileIterator *iterator, Str8 *name_out, FileProperties *properties_out) {
    B32 result = false;

    Linux_FileIterator *linux_iterator = (Linux_FileIterator *) iterator;

    while (linux_iterator->file_descriptor != -1 && !linux_iterator->done) {
        U64 one_past_last_byte = linux_iterator->first_needed_byte + sizeof(Linux_DirentHeader);
        if (one_past_last_byte <= linux_iterator->bytes_read) {
            Linux_DirentHeader *header = (Linux_DirentHeader *) &linux_iterator->buffer[linux_iterator->first_needed_byte];
            assert(header->record_length < array_count(linux_iterator->buffer));

            one_past_last_byte = linux_iterator->first_needed_byte + header->record_length;
        }

        while (one_past_last_byte > linux_iterator->bytes_read) {
            if (one_past_last_byte > array_count(linux_iterator->buffer)) {
                memory_move(
                    linux_iterator->buffer,
                    &linux_iterator->buffer[linux_iterator->first_needed_byte],
                    linux_iterator->bytes_read - linux_iterator->first_needed_byte
                );
                linux_iterator->bytes_read -= linux_iterator->first_needed_byte;
                one_past_last_byte -= linux_iterator->first_needed_byte;
                linux_iterator->first_needed_byte = 0;
            }

            U64 bytes_to_read = array_count(linux_iterator->buffer) - linux_iterator->bytes_read;

            S64 bytes_read = getdents64(linux_iterator->file_descriptor, &linux_iterator->buffer[linux_iterator->bytes_read], bytes_to_read);
            if (bytes_read == -1 && errno == EINVAL) {
                memory_move(
                    linux_iterator->buffer,
                    &linux_iterator->buffer[linux_iterator->first_needed_byte],
                    linux_iterator->bytes_read - linux_iterator->first_needed_byte
                );
                linux_iterator->bytes_read -= linux_iterator->first_needed_byte;
                one_past_last_byte -= linux_iterator->first_needed_byte;
                linux_iterator->first_needed_byte = 0;
            } else if (bytes_read == -1 || bytes_read == 0) {
                linux_iterator->done = true;
                break;
            } else {
                linux_iterator->bytes_read += bytes_read;

                if (one_past_last_byte <= linux_iterator->bytes_read) {
                    Linux_DirentHeader *header = (Linux_DirentHeader *) &linux_iterator->buffer[linux_iterator->first_needed_byte];
                    assert(header->record_length < array_count(linux_iterator->buffer));

                    one_past_last_byte = linux_iterator->first_needed_byte + header->record_length;
                }
            }
        }

        if (!linux_iterator->done) {
            // Read the dirent header
            Linux_DirentHeader *header = (Linux_DirentHeader *) &linux_iterator->buffer[linux_iterator->first_needed_byte];
            linux_iterator->first_needed_byte += header->record_length;

            // Check for . and ..
            B32 is_dot    = (header->name[0] == '.' && header->name[1] == 0);
            B32 is_dotdot = (header->name[0] == '.' && header->name[1] == '.' && header->name[2] == 0);

            if (!is_dot && !is_dotdot) {
                *name_out = str8_copy_cstr(arena, (U8 *) header->name);
                struct stat metadata = { 0 };
                if (fstatat(linux_iterator->file_descriptor, header->name, &metadata, 0) != -1) {
                    linux_file_properties_from_stat(properties_out, &metadata);
                } else {
                    memory_zero_struct(properties_out);
                }
                result = true;
                break;
            }
        }
    }

    return result;
}

internal Void os_file_iterator_end(OS_FileIterator *iterator) {
    Linux_FileIterator *linux_iterator = (Linux_FileIterator *) iterator;
    if (linux_iterator->file_descriptor != -1) {
        close(linux_iterator->file_descriptor);
    }
}

internal Str8 os_file_path(Arena *arena, OS_SystemPath path) {
    Str8 result = { 0 };

    switch (path) {
        case OS_SYSTEM_PATH_CURRENT_DIRECTORY: {
            Arena_Temporary scratch = arena_get_scratch(&arena, 1);

            U64 buffer_size = 256;
            U8 *buffer = arena_push_array(scratch.arena, U8, buffer_size);

            while (!getcwd((CStr) buffer, buffer_size)) {
                if (errno == ERANGE) {
                    arena_end_temporary(scratch);
                    scratch = arena_begin_temporary(scratch.arena);

                    buffer_size *= 2;
                    buffer       = arena_push_array(scratch.arena, U8, buffer_size);
                } else {
                    // TODO: Handle error
                }
            }

            result = str8_copy_cstr(arena, buffer);
            arena_end_temporary(scratch);
        } break;
        case OS_SYSTEM_PATH_BINARY: {
            Arena_Temporary scratch = arena_get_scratch(&arena, 1);

            U64     buffer_size = 256;
            ssize_t read        = 0;
            U8     *buffer      = arena_push_array(scratch.arena, U8, buffer_size);

            // NOTE: Readlink truncates the result to fit in the buffer, so as
            // long as the number of bytes read is the same as the buffer size,
            // expand the buffer and try again.
            for (;;) {
                read = readlink("/proc/self/exe", (CStr) buffer, buffer_size);
                if (read == -1) {
                    // TODO: Handle error
                } else if ((U64) read == buffer_size) {
                    arena_end_temporary(scratch);
                    scratch = arena_begin_temporary(scratch.arena);

                    buffer_size *= 2;
                    buffer       = arena_push_array(scratch.arena, U8, buffer_size);
                } else {
                    break;
                }
            }

            Str8 path = str8(buffer, read);

            U64 index = 0;
            if (str8_last_index_of(path, '/', &index)) {
                path = str8_prefix(path, index);
            }

            result = str8_copy(arena, path);
        } break;
        case OS_SYSTEM_PATH_USER_DATA: {
            struct passwd *user_data = getpwuid(geteuid());
            if (user_data) {
                Str8List config_path_list = { 0 };
                Str8Node nodes[2];
                str8_list_push_explicit(&config_path_list, str8_cstr(user_data->pw_dir), &nodes[0]);
                str8_list_push_explicit(&config_path_list, str8_literal("/.config"), &nodes[1]);
                result = str8_join(arena, &config_path_list);
            } else {
                // TODO: Handle error
            }
        } break;
        case OS_SYSTEM_PATH_TEMPORARY_DATA: {
            struct passwd *user_data = getpwuid(geteuid());
            if (user_data) {
                Str8List cache_path_list = { 0 };
                Str8Node nodes[2];
                str8_list_push_explicit(&cache_path_list, str8_cstr(user_data->pw_dir), &nodes[0]);
                str8_list_push_explicit(&cache_path_list, str8_literal("/.cache"), &nodes[1]);
                result = str8_join(arena, &cache_path_list);
            } else {
                // TODO: Handle error
            }
        } break;
        case OS_SYSTEM_PATH_COUNT: {
        } break;
    }

    return result;
}

internal DateTime os_now_universal_time(Void) {
    struct timeval time = { 0 };
    if (gettimeofday(&time, 0) == -1) {
        // TODO: Handle error
        return (DateTime) { 0 };
    }

    struct tm deconstructed_time = { 0 };
    if (gmtime_r(&time.tv_sec, &deconstructed_time) != &deconstructed_time) {
        // TODO: Handle error
    }

    return linux_date_time_from_tm_and_milliseconds(&deconstructed_time, time.tv_usec / 1000);
}

internal DateTime os_local_time_from_universal(DateTime *date_time) {
    struct tm universal_tm   = linux_tm_from_date_time(date_time);
    time_t    universal_time = timegm(&universal_tm);

    struct tm local_tm = { 0 };
    if (localtime_r(&universal_time, &local_tm) != &local_tm) {
        // TODO: Handle error
    }
    DateTime  local_date_time = linux_date_time_from_tm_and_milliseconds(&local_tm, date_time->millisecond);
    return local_date_time;
}

internal DateTime os_universal_time_from_local(DateTime *date_time) {
    struct tm local_tm   = linux_tm_from_date_time(date_time);
    time_t    local_time = timelocal(&local_tm);

    struct tm universal_tm = { 0 };
    if (gmtime_r(&local_time, &universal_tm) != &universal_tm) {
        // TODO: Handle error
    }
    DateTime universal_date_time = linux_date_time_from_tm_and_milliseconds(&universal_tm, date_time->millisecond);

    return universal_date_time;
}

internal U64 os_now_nanoseconds(Void) {
    struct timespec time = { 0 };
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &time) == -1) {
        // TODO: Handle error
    }
    U64 nanoseconds = time.tv_sec * 1000000000 + time.tv_nsec;
    return nanoseconds;
}

internal Void os_sleep_milliseconds(U64 time) {
    struct timespec sleep     = { 0 };
    struct timespec remainder = { 0 };

    sleep.tv_sec  = time / 1000;
    sleep.tv_nsec = (time % 1000) * 1000000;

    while (nanosleep(&sleep, &remainder) == -1) {
        sleep = remainder;
        remainder.tv_sec  = 0;
        remainder.tv_nsec = 0;
    }
}

internal Void os_get_entopy(Void *data, U64 size) {
    int file_descriptor = open("/dev/urandom", O_RDONLY);
    if (file_descriptor != -1) {
        U8 *ptr = data;
        U8 *opl = (U8 *) data + size;

        while (ptr < opl) {
            U64 total_to_read = u64_min((U64) (opl - ptr), SSIZE_MAX);
            S64 actual_read = read(file_descriptor, ptr, total_to_read);
            if (actual_read == -1) {
                break;
            }

            ptr += actual_read;
        }

        close(file_descriptor);
    }
}

internal B32 os_console_run(Str8 program, Str8List arguments) {
    B32 success = false;

    Arena_Temporary scratch = arena_get_scratch(0, 0);

    U32   argument_count  = 1 + arguments.node_count + 1;
    CStr *arguments_array = arena_push_array(scratch.arena, CStr, argument_count);

    arguments_array[0] = cstr_from_str8(scratch.arena, program);
    U32 argument_index = 1;
    for (Str8Node *node = arguments.first; node; node = node->next, ++argument_index) {
        arguments_array[argument_index] = cstr_from_str8(scratch.arena, node->string);
    }
    arguments_array[argument_count - 1] = 0;

    pid_t child_pid = fork();
    if (child_pid < 0) {
        // TODO: Error, could not fork.
    } else if (child_pid == 0) {
        // Child
        if (execvp(arguments_array[0], arguments_array) == -1) {
            // TODO: Error, could not exec.
            syscall(SYS_exit_group, -1);
        }
    } else {
        // Parent
        int status = 0;
        waitpid(child_pid, &status, 0);
        success = (WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

    return success;
}

internal Void os_console_print(Str8 string) {
    write(1, string.data, string.size);
}

internal Void os_restart_self(Void) {
    U32   argument_count  = linux_argument_list.node_count + 1;
    CStr *arguments_array = arena_push_array(linux_permanent_arena, CStr, argument_count);

    U32 argument_index = 0;
    for (Str8Node *node = linux_argument_list.first; node; node = node->next, ++argument_index) {
            arguments_array[argument_index] = cstr_from_str8(linux_permanent_arena, node->string);
    }
    arguments_array[argument_count - 1] = 0;

    if (execvp(arguments_array[0], arguments_array) == -1) {
        // TODO: Error, could not exec.
        syscall(SYS_exit_group, -1);
    }
}

internal Void os_exit(S32 exit_code) {
    syscall(SYS_exit_group, exit_code);
}

int main(int argument_count, char *arguments[]) {
    arena_init_scratch();

    linux_permanent_arena = arena_create();

    for (int i = 0; i < argument_count; ++i) {
        Str8 argument = str8_cstr(arguments[i]);
        str8_list_push(linux_permanent_arena, &linux_argument_list, argument);
    }

    S32 return_value = os_run(linux_argument_list);

    arena_destroy_scratch();

    return return_value;
}
