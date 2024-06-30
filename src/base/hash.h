#ifndef HASH_H
#define HASH_H

internal U64 u64_hash(U64 x);
internal U64 s64_hash(S64 x);

internal U64 str8_hash(Str8 string);

internal U64 hash_combine(U64 a, U64 b);

#endif // HASH_H
