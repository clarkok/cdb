#ifndef _DB_DRIVER_DRIVER_ACCESSER_H_
#define _DB_DRIVER_DRIVER_ACCESSER_H_

#include "lib/utils/slice.hpp"

#include <limits>
#include "driver.hpp"
#include "block-allocator.hpp"

namespace cdb {

    class DriverAccesser;

    class Block
    {
        DriverAccesser *_owner;
        BlockIndex _index;
        Slice _slice;
        mutable bool _dirty = false;

        Block(DriverAccesser *owner, BlockIndex index, Slice slice)
            : _owner(owner), _index(index), _slice(slice)
        { }

        friend class DriverAccesser;
    public:
        Block(Block &&block)
            : _owner(block._owner), _index(block._index), _slice(block._slice), _dirty(block._dirty)
        { block._index = std::numeric_limits<BlockIndex>::max(); }

        Block &operator = (Block &&block);

        // copying means aquire again
        Block(const Block &block);
        Block &operator = (const Block &block);

        ~Block();

        inline BlockIndex
        index() const
        { return _index; }

        inline ConstSlice
        constSlice() const
        { return _slice; }

        inline Slice
        slice() const
        { 
            _dirty = true;
            return _slice;
        }

        inline Length
        length() const
        { return constSlice().length(); }

        inline const Byte*
        content() const
        { return constSlice().content(); }

        inline Byte*
        content()
        { 
            _dirty = true;
            return slice().content();
        }

        inline Slice::ConstSliceIterator
        cbegin() const
        { return constSlice().cbegin(); }

        inline Slice::ConstSliceIterator
        cend() const
        { return constSlice().cend(); }

        inline Slice::SliceIterator
        begin()
        { 
            _dirty = true;
            return slice().begin();
        }

        inline Slice::SliceIterator
        end()
        { 
            _dirty = true;
            return slice().end();
        }

        inline
        operator Slice()
        { 
            _dirty = true;
            return slice();
        }

        inline
        operator ConstSlice()
        { return constSlice(); }
    };

    class DriverAccesser
    {
    protected:
        Driver *_drv;
        BlockAllocator *_allocator;

        virtual void release(BlockIndex block, bool dirty = true) = 0;
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
