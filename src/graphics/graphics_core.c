// TODO(simon): There is a lot more to do here, but this will do for a *very*
// basic implementation.
internal Uri uri_from_string(Str8 string) {
    Uri uri = { 0 };

    // NOTE(simon): Scheme
    {
        U64 colon_index = str8_first_index_of(string, ':');
        uri.scheme = str8_prefix(string, colon_index);
        string = str8_skip(string, colon_index + 1);
    }

    // TODO(simon): Validation of shceme according to https://datatracker.ietf.org/doc/html/rfc3986#section-3.1

    // NOTE(simon): Hier-part

    // NOTE(simon): Authority
    if (string.size >= 2 && string.data[0] == '/' && string.data[1] == '/') {
        string = str8_skip(string, 2);
        uri.flags |= UriFlag_HasAuthority;

        U64 slash_index         = str8_first_index_of(string, '/');
        U64 question_mark_index = str8_first_index_of(string, '?');
        U64 number_sign_index   = str8_first_index_of(string, '#');
        U64 authority_end       = u64_min(u64_min(slash_index, question_mark_index), number_sign_index);

        uri.authority = str8_prefix(string, authority_end);
        string = str8_skip(string, authority_end);

        // TODO(simon): Validate according to https://datatracker.ietf.org/doc/html/rfc3986#section-3.2
    }

    // NOTE(simon): Path
    {
        U64 question_mark_index = str8_first_index_of(string, '?');
        U64 number_sign_index   = str8_first_index_of(string, '#');
        U64 path_end            = u64_min(question_mark_index, number_sign_index);

        uri.path = str8_prefix(string, path_end);
        string = str8_skip(string, path_end);

        // TODO(simon): Validation according to https://datatracker.ietf.org/doc/html/rfc3986#section-3.3
    }

    // NOTE(simon): Query
    if (string.size >= 1 && string.data[0] == '?') {
        string = str8_skip(string, 1);
        uri.flags |= UriFlag_HasQuery;

        U64 number_sign_index = str8_first_index_of(string, '#');
        uri.query = str8_prefix(string, number_sign_index);
        string = str8_skip(string, number_sign_index);

        // TODO(simon): Validation according to https://datatracker.ietf.org/doc/html/rfc3986#section-3.4
    }

    // NOTE(simon): Fragment
    if (string.size >= 1 && string.data[0] == '#') {
        string = str8_skip(string, 1);
        uri.flags |= UriFlag_HasFragment;

        uri.fragment = string;

        // TODO(simon): Validation according to https://datatracker.ietf.org/doc/html/rfc3986#section-3.5
    }

    return uri;
}
