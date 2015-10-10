#ifndef _DB_DRIVER_BASIC_DRIVER_H_
#define _DB_DRIVER_BASIC_DRIVER_H_

#include <cstdio>

#include "driver.hpp"

namespace cdb {
    /**
     * Basic single file driver, read or write blocks directly.
     */
    class BasicDriver : public Driver
    {
        /** internal file descriptor */
        FILE* _fd;

        // not copiable
        BasicDriver(const BasicDriver &) = delete;
        BasicDriver &operator = (const BasicDriver &) = delete;

    public:
        /**
         * Construct a BasicDriver with file path.
         *
         * If the file exists, open it and update it. If the file doesn't exists, create it and update it.
         *
         * @param path path of the single file
         */
        BasicDriver(const char *path);

        /** Close the file when destructing. */
        virtual ~BasicDriver();

        /**
         * Read a block from disk, directly.
         *
         * There should always enough space in dest, or it would assert failed
         *
         * @param index index of block
         * @param dest a piece of memory where the driver reads to 
         * @see readBlocks(BlockIndex index, Length count, Slice dest)
         */
        virtual void readBlock(BlockIndex index, Slice dest)
        { readBlocks(index, 1, dest); }

        /**
         * Read a series of blocks from disk, directly.
         *
         * There should always enough space in dest, or it would assert failed
         *
         * @param index index of first block
         * @param count number of blocks to read
         * @param dest a piece of memory where the driver reads to
         * @see readBlock(BlockIndex index, Slice dest)
         */
        virtual void readBlocks(BlockIndex index, Length count, Slice dest);

        /**
         * Write a block to disk, directly.
         *
         * There should always enough data in src, or it would assert failed
         *
         * @param index index of block
         * @param src a piece of memory which the driver write to disk
         * @see writeBlocks(BlockIndex index, Length length, ConstSlice src)
         */
        virtual void writeBlock(BlockIndex index, ConstSlice src)
        { writeBlocks(index, 1, src); }

        /**
         * Write a series of block to disk, directly.
         *
         * There should always enough data in src, or it would assert failed
         *
         * @param index index of first block
         * @param count number of blocks to read
         * @param src a piece of memory which the driver write to disk
         * @see writeBlock(BlockIndex index, ConstSlice src)
         */
        virtual void writeBlocks(BlockIndex index, Length count, ConstSlice src);

        /** Flush all data to disk. */
        virtual void flush();
    };
}

#endif // _DB_DRIVER_BASIC_DRIVER_H_
