#include "cached-accesser.hpp"

using namespace cdb;

BlockIndex
CachedAccesser::allocateBlocks(Length length, BlockIndex hint)
{ return _allocator->allocateBlocks(length, hint); }

void
CachedAccesser::freeBlocks(BlockIndex index, Length length)
{ _allocator->freeBlocks(index, length); }

std::list<CachedAccesser::CacheBlock>::iterator
CachedAccesser::findRecordByBlockIndexOnly(BlockIndex index)
{
    auto tag = calcTagByBlockIndex(index);

    for (auto i = _record.begin(); i != _record.end(); ++i) {
        if (i->tag == tag) {
            return i;
        }
    }

    throw CachedNotFoundException(index);
}

std::list<CachedAccesser::CacheBlock>::iterator
CachedAccesser::findRecordByBlockIndexAndIncCount(BlockIndex index)
{
    auto tag = calcTagByBlockIndex(index);

    for (auto i = _record.begin(); i != _record.end(); ++i) {
        if (i->tag == tag) {
            ++(i->count);
            ++(i->accessed);
            if (i == _record.begin()) {
                return i;
            }
            _record.emplace(_record.begin(), std::move(*i));
            _record.erase(i);
            return _record.begin();
        }
    }

    if (_record.size() == CACHE_MAX_BLOCK_COUNT) {
        for (auto i = _record.rbegin(); i != _record.rend(); ++i) {
            if (i->count == 0) {
                _record.erase(i.base());
                break;
            }
        }
    }

    assert(_record.size() != CACHE_MAX_BLOCK_COUNT);

    _record.emplace(_record.begin(), CacheBlock{1, 1, tag, Buffer(CACHE_BLOCK_SIZE)});
    _drv->readBlocks(calcIndexByTag(tag), BLOCK_PER_CACHE, _record.begin()->content);
    return _record.begin();
}

void
CachedAccesser::release(BlockIndex block, bool dirty)
{
    auto iter = findRecordByBlockIndexOnly(block);
    assert(iter->count);
    iter->count--;

    if (dirty) {
        _drv->writeBlock(
                block,
                ConstSlice(
                    const_cast<const Buffer &>(iter->content).content() + calcOffsetByBlockIndex(block),
                    Driver::BLOCK_SIZE
                )
        );
    }
}

Slice
CachedAccesser::access(BlockIndex index)
{
    auto iter = findRecordByBlockIndexAndIncCount(index);
    return Slice(
            iter->content.content() + calcOffsetByBlockIndex(index),
            Driver::BLOCK_SIZE
        );
}

void
CachedAccesser::flush()
{ }
