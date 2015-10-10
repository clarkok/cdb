#ifndef _DB_DRIVER_DRIVER_ACCESSER_H_
#define _DB_DRIVER_DRIVER_ACCESSER_H_

#include "lib/utils/slice.hpp"

#include "driver.hpp"
#include "block-allocator.hpp"

namespace cdb {

    class DriverAccesser
    {
    protected:
        Driver *_drv;
        BlockAllocator *_allocator;
    public:
        DriverAccesser(Driver *drv, BlockAllocator *allocator);

        virtual ~DriverAccesser() = default;

        virtual Slice access(BlockIndex index) = 0;
        virtual void release(BlockIndex index) = 0;

        virtual BlockIndex allocateBlock(BlockIndex hint = 0);
        virtual BlockIndex allocateBlocks(Length length, BlockIndex hint = 0) = 0;

        virtual void freeBlock(BlockIndex index);
        virtual void freeBlocks(BlockIndex index, Length length) = 0;

        virtual void flush() = 0;
    };

}

#endif // _DB_DRIVER_DRIVER_ACCESSER_H_
