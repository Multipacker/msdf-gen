#ifndef WIN32_ESSENTIAL_H
#define WIN32_ESSENTIAL_H

#define UNICODE
#define WIN32_LEAN_AND_MEAN

#pragma warning(push, 0)
#include <Windows.h>
#include <Shlobj.h>
#include <bcrypt.h>
#pragma warning(pop)

typedef struct {
    HANDLE handle;
    WIN32_FIND_DATAW find_data;
    B32 done;
} Win32_FileIterator;
static_assert(sizeof(Win32_FileIterator) <= sizeof(OS_FileIterator));

typedef struct Win32_Resource Win32_Resource;
struct Win32_Resource {
    Win32_Resource *next;
    union {
        struct {
            OS_ThreadFunction *entry_point;
            Void  *data;
            HANDLE handle;
            DWORD  tid;
        } thread;
        CRITICAL_SECTION mutex;
        CONDITION_VARIABLE condition_variable;
    };
};

typedef struct Win32_State Win32_State;
struct Win32_State {
    Arena *permanent_arena;

    Str8List argument_list;
    S64 performance_frequency;

    HANDLE standard_output;

    Win32_Resource *volatile resource_freelist;
    CRITICAL_SECTION         resource_mutex;
};

// NOTE(simon): Helpers for converting between date formats.
internal DateTime   win32_date_time_from_system_time(SYSTEMTIME system_time);
internal SYSTEMTIME win32_system_time_from_date_time(DateTime date_time);

#endif // WIN32_ESSENTIAL_H
