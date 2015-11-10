#include <cassert>
#include "driver-accesser.hpp"

using namespace cdb;

using cdb::BlockIndex;

Block::Block(const Block &block)
    : Block(std::move(block._owner->aquire(block._index)))
{ }

Block &
Block::operator = (const Block &block)
{ return this->operator=(std::move(block._owner->aquire(block._index))); }

Block &
Block::operator = (Block &&block)
{
    assert(this != &block);

    if (_index != std::numeric_limits<BlockIndex>::max()) {
        _owner->release(_index, _dirty);
    }

    _owner = block._owner;
    _index = block._index;
    _slice = block._slice;
    _dirty = block._dirty;
    block._index = std::numeric_limits<BlockIndex>::max();
    return *this;
}

Block::~Block()
{
    if (_index != std::numeric_limits<BlockIndex>::max()) {
        _owner->release(_index, _dirty);
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

