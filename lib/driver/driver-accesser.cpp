#include <cassert>
#include "driver-accesser.hpp"

using namespace cdb;

using cdb::BlockIndex;

Block::Block(const Block &block)
    : _owner(block._owner), _index(block._index), _slice(_owner->access(_index))
{ }

Block &
Block::operator = (const Block &block)
{
    if (this == &block) {
        return *this;
    }

    if (_index != NON_BLOCK) {
        _owner->release(_index);
    }

    _owner = block._owner;
    _index = block._index;
    _slice = _owner->access(_index);

    return *this;
}

Block &
Block::operator = (Block &&block)
{
    if (this == &block) {
        return *this;
    }

    if (_index != NON_BLOCK) {
        _owner->release(_index);
    }

    _owner = block._owner;
    _index = block._index;
    _slice = block._slice;
    block._index = NON_BLOCK;
    return *this;
}

Block::~Block()
{
    if (_index != NON_BLOCK) {
        _owner->release(_index);
    }
}


DriverAccesser::DriverAccesser(Driver *drv, BlockAllocator *allocator)
    : _drv(drv), _allocator(allocator)
{ }

BlockIndex
DriverAccesser::allocateBlock(BlockIndex hint)
{ return allocateBlocks(1, hint); }

void
DriverAccesser::freeBlock(BlockIndex index)
{ freeBlocks(index, 1); }

