internal U64 u64_hash(U64 x) {
    x ^= u64_rotate_right(x, 25) ^ u64_rotate_right(x, 50);
    x *= 0xA24BAED4963EE407UL;
    x ^= u64_rotate_right(x, 24) ^ u64_rotate_right(x, 49);
    x *= 0x9FB21C651E98DF25UL;
    x = x ^ x >> 28;
    return x;
}

internal U64 s64_hash(S64 x) {
    U64 result = u64_hash((U64) x);
    return result;
}

// TODO: Better hash function
internal U64 str8_hash(Str8 string) {
    U64 hash = 5381;

    for (U64 i = 0; i < string.size; ++i) {
        hash = ((hash << 5) + hash) + string.data[i];
    }

    return hash;
}

internal U64 hash_combine(U64 a, U64 b) {
    U64 result = a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    return result;
}
