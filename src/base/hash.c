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

// TODO: SipHash
internal U64 str8_hash(Str8 string) {
    U64 hash = 0;
    U64 p = 0x10FFFF; // 0x10FFFF (the largest codepoint in unicode) happens to be a prime number.
    U64 m = 1000000009;
    U64 p_power = 1;

    U8 *ptr = string.data;
    U8 *opl = string.data + string.size;

    while (ptr < opl) {
        StringDecode decode = string_decode_utf8(ptr, (U64) (ptr - opl));

        hash = (hash + decode.codepoint * p_power) % m;
        p_power = (p_power * p) % m;

        ptr += decode.size;
    }

    return hash;
}
