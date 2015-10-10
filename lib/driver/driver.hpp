#ifndef _DB_DRIVER_DRIVER_H_
#define _DB_DRIVER_DRIVER_H_

#include <cstdint>

#include "lib/utils/slice.hpp"

namespace cdb {
    typedef std::uint32_t BlockIndex;

    /**
     * Class to interact with external storage, maybe a single file, or web resource, etc.
     */
    class Driver
    {
    public:
        /**
         * Minimum unit to operate with
         */
        static const Length BLOCK_SIZE = 1024;

        virtual ~Driver() = default;

        /**
         * read a single block from external storage
         *
         * @param index the index of block to read
         * @param dest the dest to read to 
         * @see readBlocks(BlockIndex index, Length count, Slice dest)
         */
        virtual void readBlock(BlockIndex index, Slice dest) = 0;

        /**
         * read a series of blocks from external storage
         *
         * @param index the index of first block to read
         * @param count the count of blocks to read
         * @param dest the dest to read to
         * @see readBlock(BlockIndex index, Slice dest)
         */
        virtual void readBlocks(BlockIndex index, Length count, Slice dest);

        /**
         * write a single block to external storage
         *
         * @param index the index of block to write to
         * @param src the data to write
         * @see writeBlocks(BlockIndex index, Length count, ConstSlice src)
         */
        virtual void writeBlock(BlockIndex index, ConstSlice src) = 0;

        /**
         * write a series of blocks to external storage
         *
         * @param index the index of forst block to write to
         * @param count the count of blocks to write to
         * @param src the data to write
         * @see writeBlock(BlockIndex index, ConstSlice src)
         */
        virtual void writeBlocks(BlockIndex index, Length count, ConstSlice src);

        /**
         * Flush content to disk, immediately
         */
        virtual void flush() = 0;
    };
}

#endif // _DB_DRIVER_DRIVER_H_
