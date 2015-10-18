#ifndef _DB_UTILS_HASH_H_
#define _DB_UTILS_HASH_H_

#include <cstdint>

#include "buffer.hpp"

namespace cdb {
    typedef std::uint32_t HashResult;

    class FNVHasher
    {
    public:
        static HashResult hash(const Byte *src, Length length);
    };
}

#endif // _DB_UTILS_HASH_H_
