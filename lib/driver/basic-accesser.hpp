#ifndef _DB_DRIVER_BASIC_ACCESSER_H_
#define _DB_DRIVER_BASIC_ACCESSER_H_

#include <map>

#include "driver-accesser.hpp"

namespace cdb {

    class BasicAccesser : public DriverAccesser
    {
        struct BufferWithCount
        {
            int count;
            Buffer buffer;

            BufferWithCount(int count, const Buffer &buffer)
                : count(count), buffer(buffer)
            { }
        };

        std::map<BlockIndex, BufferWithCount> _buffers;

        virtual Slice access(BlockIndex index);
        virtual void release(BlockIndex index);
    public:
        BasicAccesser(Driver *drv, BlockAllocator *allocator);

        virtual ~BasicAccesser();

        virtual BlockIndex allocateBlocks(Length length, BlockIndex hint = 0);
        virtual void freeBlocks(BlockIndex index, Length length);

        virtual void flush();
    };
}

#endif // _DB_DRIVER_BASIC_ACCESSER_H_
