#ifndef _DB_DRIVER_DRIVER_ACCESSER_H_
#define _DB_DRIVER_DRIVER_ACCESSER_H_

#include <limits>
#include "lib/utils/slice.hpp"

#include "driver.hpp"
#include "block-allocator.hpp"

#include <map>
#include <iostream>

namespace cdb {

    std::map<BlockIndex, int> &_get_debug_counter();

    class DriverAccesser;

    class Block
    {
        DriverAccesser *_owner;
        BlockIndex _index;
        Slice _slice;
        Block(DriverAccesser *owner, BlockIndex index, Slice slice)
            : _owner(owner), _index(index), _slice(slice)
        {
            auto count = ++_get_debug_counter()[index];
            // std::cerr << "ctor " << index << " | " << count << std::endl;
        }

        friend class DriverAccesser;
    public:
        static const BlockIndex NON_BLOCK = std::numeric_limits<BlockIndex>::max();

        Block(Block &&block)
            : _owner(block._owner), _index(block._index), _slice(block._slice)
        {
            block._index = NON_BLOCK;
            // std::cerr << "move ctor " << _index << " | " << _get_debug_counter()[_index] << std::endl;
        }

        Block &operator = (Block &&block);

        // copying means acquire again
        Block(const Block &block);
        Block &operator = (const Block &block);

        ~Block();

        inline BlockIndex
        index() const
        { return _index; }

        inline Slice
        slice() const
        { return _slice; }

        inline Length
        length() const
        { return slice().length(); }

        inline const Byte*
        content() const
        { return slice().content(); }

        inline Byte*
        content()
        { return slice().content(); }

        inline const Byte*
        cbegin() const
        { return slice().cbegin(); }

        inline const Byte*
        cend() const
        { return slice().cend(); }

        inline Byte*
        begin()
        { return slice().begin(); }

        inline Byte*
        end()
        { return slice().end(); }

        inline
        operator Slice()
        { return slice(); }
    };

    class DriverAccesser
    {
    protected:
        Driver *_drv;
        BlockAllocator *_allocator;

        virtual void release(BlockIndex block) = 0;
        virtual Slice access(BlockIndex index) = 0;

        friend class Block;

    public:
        DriverAccesser(Driver *drv, BlockAllocator *allocator);

        virtual ~DriverAccesser() = default;

        inline Block aquire(BlockIndex index)
        { return Block(this, index, access(index)); }

        virtual BlockIndex allocateBlock(BlockIndex hint = 0);
        virtual BlockIndex allocateBlocks(Length length, BlockIndex hint = 0) = 0;

        virtual void freeBlock(BlockIndex index);
        virtual void freeBlocks(BlockIndex index, Length length) = 0;

        virtual void flush() = 0;
    };

}

#endif // _DB_DRIVER_DRIVER_ACCESSER_H_
