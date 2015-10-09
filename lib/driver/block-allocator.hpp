#ifndef _DB_DRIVER_BLOCK_ALLOCATOR_H_
#define _DB_DRIVER_BLOCK_ALLOCATOR_H_

#include <cstdint>

#include "driver.hpp"

namespace cdb {
    typedef std::uint32_t BlockIndex;

    class BlockAllocator
    {
    protected:
        Driver *_drv = nullptr;
        BlockIndex _start_at = 0;
    public:
        BlockAllocator(Driver *drv, BlockIndex start_at);
        virtual ~BlockAllocator() = default;

        virtual BlockIndex allocateBlock(BlockIndex hint = 0);
        virtual BlockIndex allocateBlocks(Length length, BlockIndex hint = 0) = 0;

        virtual void freeBlock(BlockIndex index);
        virtual void freeBlocks(BlockIndex index, Length length) = 0;

        virtual void reset() = 0;
        virtual void flush() = 0;
    };
}

#endif // _DB_DRIVER_BLOCK_ALLOCATOR_H_
