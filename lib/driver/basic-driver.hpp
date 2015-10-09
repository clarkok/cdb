#ifndef _DB_DRIVER_BASIC_DRIVER_H_
#define _DB_DRIVER_BASIC_DRIVER_H_

#include <cstdio>

#include "driver.hpp"

namespace cdb {
    class BasicDriver : public Driver
    {
        FILE* _fd;

        // not copiable
        BasicDriver(const BasicDriver &) = delete;
        BasicDriver &operator = (const BasicDriver &) = delete;

    public:
        BasicDriver(const char *path);
        virtual ~BasicDriver();

        virtual void readBlock(BlockIndex index, Slice dest)
        { readBlocks(index, 1, dest); }
        virtual void readBlocks(BlockIndex index, Length count, Slice dest);

        virtual void writeBlock(BlockIndex index, ConstSlice src)
        { writeBlocks(index, 1, src); }
        virtual void writeBlocks(BlockIndex index, Length count, ConstSlice src);

        virtual void flush();
    };
}

#endif // _DB_DRIVER_BASIC_DRIVER_H_
