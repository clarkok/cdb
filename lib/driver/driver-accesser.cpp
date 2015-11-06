#include <cassert>
#include "driver-accesser.hpp"

using namespace cdb;

using cdb::BlockIndex;

Block::Block(const Block &block)
    : _owner(block._owner), _index(block._index), _slice(_owner->access(_index))
{
    auto count = ++_get_debug_counter()[_index];
    // std::cerr << "copy ctor " << _index << " | " << count << std::endl;
}

Block &
Block::operator = (const Block &block)
{
    // std::cerr << "copy assign " << block._index << " -> " << _index << std::endl;

    if (this == &block) {
        return *this;
    }

    // std::cerr << block._index << " | " << ++_get_debug_counter()[block._index] << std::endl;
    // std::cerr << _index << " | " << --_get_debug_counter()[_index] << std::endl;

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
    // std::cerr << "move assign " << block._index << " -> " << _index << std::endl;

    if (this == &block) {
        return *this;
    }

    // std::cerr << block._index << " | " << _get_debug_counter()[block._index] << std::endl;
    // std::cerr << _index << " | " << --_get_debug_counter()[_index] << std::endl;

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
    // std::cerr << "dtor " << _index << std::endl;
    if (_index != NON_BLOCK) {
        _owner->release(_index);
        // std::cerr << _index << " | " << --_get_debug_counter()[_index] << std::endl;
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

std::map<BlockIndex, int> &
cdb::_get_debug_counter()
{
    static std::map<BlockIndex, int> counter;
    return counter;
}
