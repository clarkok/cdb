#include "hash.hpp"

using namespace cdb;

HashResult
FNVHasher::hash(const Byte *src, Length length)
{
    static const HashResult HASH_SEED = 16777619;
    static const HashResult FNV_PRIME = 2166136261;
    static const HashResult TAILING_MASK[] = {0, 0xFF, 0xFFFF, 0xFFFFFF};

    const Byte *limit = src + length;
    const Byte *limit_32 = limit - sizeof(HashResult);

    HashResult result = HASH_SEED;

    for (; src < limit_32; src += sizeof(HashResult)) {
        result *= FNV_PRIME;
        result ^= *reinterpret_cast<const HashResult*>(src);
    }

    result *= FNV_PRIME;
    result ^= 
        *reinterpret_cast<const HashResult*>(src) & 
        (TAILING_MASK[limit - src]);

    return result;
}
