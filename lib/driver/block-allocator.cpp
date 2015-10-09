#include "block-allocator.hpp"

using namespace cdb;

using cdb::BlockIndex;

BlockAllocator::BlockAllocator(Driver *drv, BlockIndex start_at)
    : _drv(drv), _start_at(start_at)
{ }

BlockIndex
BlockAllocator::allocateBlock(BlockIndex hint)
{ return allocateBlocks(1, hint); }

void
BlockAllocator::freeBlock(BlockIndex index)
{ freeBlocks(index, 1); }
