#ifndef BASE_LOG_H
#define BASE_LOG_H

typedef enum {
    Log_MessageKind_Information,
    Log_MessageKind_Warning,
    Log_MessageKind_Error,
    Log_MessageKind_COUNT,
} LogMessageKind;

typedef struct LogScope LogScope;
struct LogScope {
    LogScope *next;
    U64       arena_position;
    Str8List  strings[Log_MessageKind_COUNT];
};

typedef struct LogScopeResult LogScopeResult;
struct LogScopeResult {
    Str8 strings[Log_MessageKind_COUNT];
};

typedef struct Log Log;
struct Log {
    Arena *arena;
    LogScope *top_scope;
};

// NOTE(simon): Logs
internal Log *log_create(Void);
internal Void log_destroy(Log *log);
internal Void log_select(Log *log);

// NOTE(simon): Scopes
internal Void           log_scope_begin(Void);
internal LogScopeResult log_scope_end(Arena *arena);

// NOTE(simon): Messages
internal Void log_message(LogMessageKind kind, Str8 string);
internal Void log_message_format(LogMessageKind kind, CStr format, ...);
#define log_info(string)             log_message(Log_MessageKind_Information, (string))
#define log_info_format(format, ...) log_message_format(Log_MessageKind_Information, (format), __VA_ARGS__)
#define log_warning(string)             log_message(Log_MessageKind_Warning, (string))
#define log_warning_format(format, ...) log_message_format(Log_MessageKind_Warning, (format), __VA_ARGS__)
#define log_error(string)             log_message(Log_MessageKind_Error, (string))
#define log_error_format(format, ...) log_message_format(Log_MessageKind_Error, (format), __VA_ARGS__)


#endif // BASE_LOG_H
