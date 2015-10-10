#ifndef _DB_DRIVER_BLOCK_ALLOCATOR_H_
#define _DB_DRIVER_BLOCK_ALLOCATOR_H_

#include <cstdint>

#include "driver.hpp"

namespace cdb {
    typedef std::uint32_t BlockIndex;

    /**
     * Allocator to allocate block(s) on a driver
     */
    class BlockAllocator
    {
    protected:
        Driver *_drv = nullptr;     /** the driver to read from and to write to */
        BlockIndex _start_at = 0;   /** reserved blocks in the allocator */
    public:
        /**
         * Constructor, initialize protected members
         *
         * @param drv the driver to read from and to write to
         * @param start_at reserved blocks in the allocator
         */
        BlockAllocator(Driver *drv, BlockIndex start_at);

        virtual ~BlockAllocator() = default;

        /**
         * Allocate one block in the allocator
         *
         * NOTE: the hint is just a suggestion, the allocator will try to allocate block 
         * near the hint, but it is not necesserily.
         *
         * @param hint the suggestion to the allocator
         * @return the index of the block allocated
         * @see allocateBlocks(Length length, BlockIndex hint)
         */
        virtual BlockIndex allocateBlock(BlockIndex hint = 0);

        /**
         * Allocate a series of blocks in the allocator, the blocks will always be 
         * allocated in a row
         *
         * NOTE: some of allocators have limitation of length, make length less than 32
         *
         * @param length the count of blocks required
         * @param hint the suggestion to the allocator
         * @return the index of first block allocated
         * @see allocateBlock(BlockIndex hint)
         */
        virtual BlockIndex allocateBlocks(Length length, BlockIndex hint = 0) = 0;

        /**
         * Free one block in the allocator
         *
         * NOTE: the block to free must be block allocated in the allocator, or the 
         * behavior is undefined
         *
         * @param index the index of block to free
         * @see freeBlocks(BlockIndex index, Length length)
         */
        virtual void freeBlock(BlockIndex index);

        /**
         * Free a series of blocks in the allocator
         *
         * NOTE: the block to free must be block allocated in the allocator, or the 
         * behavior is undefined
         *
         * @param index the index of first block to free
         * @param length the number of blocks to free
         * @see freeBlock(BlockIndex index)
         */
        virtual void freeBlocks(BlockIndex index, Length length) = 0;

        /**
         * Reset the allocator
         */
        virtual void reset() = 0;

        /**
         * Flush back to driver
         */
        virtual void flush() = 0;
    };
}

#endif // _DB_DRIVER_BLOCK_ALLOCATOR_H_
