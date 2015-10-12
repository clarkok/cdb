#include "driver-accesser.hpp"

using namespace cdb;

using cdb::BlockIndex;

Block::Block(const Block &block)
    : Block(std::move(block._owner->aquire(block._index)))
{ }

Block &
Block::operator = (const Block &block)
{ return this->operator=(std::move(block._owner->aquire(block._index))); }

Block::~Block()
{
    if (_index) {
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

