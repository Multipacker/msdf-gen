thread_local Str8 thread_error_message;

internal Void error_emit(Str8 message) {
    thread_error_message = message;
}

internal Str8 error_get_error_message(Void) {
    return thread_error_message;
}
