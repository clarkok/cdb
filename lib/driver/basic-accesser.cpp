#include <iostream>
#include "basic-accesser.hpp"

using namespace cdb;

using cdb::BlockIndex;

BasicAccesser::BasicAccesser(Driver *drv, BlockAllocator *allocator)
    : DriverAccesser(drv, allocator)
{ }

BasicAccesser::~BasicAccesser()
{ flush(); }

Slice
BasicAccesser::access(BlockIndex index)
{
    auto result = _buffers.emplace(index, BufferWithCount(1, Buffer(Driver::BLOCK_SIZE)));
    if (result.second) {
        _drv->readBlock(index, result.first->second.buffer);
    }
    else {
        result.first->second.count ++;
    }
    // std::cerr << index << " -- " << result.first->second.count;
    return result.first->second.buffer;
}

void
BasicAccesser::release(BlockIndex index)
{
    auto iter = _buffers.find(index);
    if (iter != _buffers.end()) {
        // std::cerr << index << " -- " << iter->second.count - 1;
        if (--(iter->second.count) == 0) {
            _drv->writeBlock(index, iter->second.buffer);
            _buffers.erase(iter);
        }
    }
}

BlockIndex
BasicAccesser::allocateBlocks(Length length, BlockIndex hint)
{ return _allocator->allocateBlocks(length, hint); }

void
BasicAccesser::freeBlocks(BlockIndex index, Length length)
{ _allocator->freeBlocks(index, length); }

void
BasicAccesser::flush()
{
    for (auto &pair : _buffers) {
        _drv->writeBlock(pair.first, pair.second.buffer);
    }
}
