#ifndef _DB_DRIVER_DRIVER_H_
#define _DB_DRIVER_DRIVER_H_

#include <cstdint>

#include "lib/utils/slice.hpp"

namespace cdb {
    typedef std::uint32_t BlockIndex;

    class Driver
    {
    public:
        static const Length BLOCK_SIZE = 1024;

        virtual ~Driver() = default;

        virtual void readBlock(BlockIndex index, Slice dest) = 0;
        virtual void readBlocks(BlockIndex index, Length count, Slice dest);

        virtual void writeBlock(BlockIndex index, ConstSlice src) = 0;
        virtual void writeBlocks(BlockIndex index, Length count, ConstSlice src);

        virtual void flush() = 0;
    };
}

#endif // _DB_DRIVER_DRIVER_H_
