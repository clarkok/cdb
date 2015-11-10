#include <cassert>

#include "bitmap-allocator.hpp"

using namespace cdb;

using cdb::Length;
using cdb::BlockIndex;

#if defined __GNUC__
    #define cdb_count_leading_zero_32(x) (x ? __builtin_clz(x) : 32)
#elif defined __clang__
    #define cdb_count_leading_zero_32(x) (x ? __builtin_clz(x) : 32)
#elif defined _MSC_VER
    #include <intrin.h>
    #pragma intrinsic(_BitScanReverse)

    #define cdb_count_leading_zero_32(x) \
        __count_leading_zero_32_helper(static_cast<std::uint32_t>(x))

    static inline int __count_leading_zero_32_helper(std::uint32_t value) {
        unsigned long index;
        if (_BitScanReverse(&index, value)) {
            return index;
        }
        else {
            return 32;
        }
    }
#endif

BitmapAllocator::BitmapAllocator(Driver *drv, BlockIndex start_at)
    : BlockAllocator(drv, start_at), _count_block(Driver::BLOCK_SIZE)
{ 
    // read count block
    _drv->readBlock(_start_at, _count_block);

    // read all bitmaps
    Length *count_ptr = reinterpret_cast<Length*>(_count_block.content());
    auto bitmap_count = count_ptr[MAX_SECTION_COUNT - 1];

    if (_bitmaps.size()) {
        _bitmaps.clear();
    }

    for (BlockIndex i = 0; i < bitmap_count; ++i, ++count_ptr) {
        Buffer bitmap_buf(Driver::BLOCK_SIZE);
        _drv->readBlock(calculateBitmapBlockIndex(i), bitmap_buf);
        _bitmaps.emplace_back(Bitmap{i, bitmap_buf, *count_ptr, false});
    }
}

BitmapAllocator::~BitmapAllocator()
{ flush(); }

inline void
BitmapAllocator::flush()
{
    // mean this bitmap is not valid
    if (!_bitmaps.size()) {
        return;
    }

    // restore count && flush dirty bitmap
    Length* count_ptr = reinterpret_cast<Length*>(_count_block.content());
    for (auto &bitmap : _bitmaps) {
        *count_ptr ++ = bitmap.count;

        if (bitmap.dirty) {
            _drv->writeBlock(calculateBitmapBlockIndex(bitmap.index), bitmap.bitmap);
        }
    }

    // write back count block
    _drv->writeBlock(calculateCountBlockIndex(), _count_block);

    _drv->flush();
}

inline BlockIndex
BitmapAllocator::calculateBitmapBlockIndex(BlockIndex bitmap_index)
{ return (bitmap_index + 1) * BLOCK_PER_SECTION - BLOCK_PER_UNIT; } // for fast look up

inline BlockIndex
BitmapAllocator::calculateCountBlockIndex()
{ return _start_at; }                                   // on the first block allocable

void
BitmapAllocator::reset()
{
    // initialize the count block and clear the bitmaps
    std::fill(_count_block.begin(), _count_block.end(), 0);
    if (_bitmaps.size()) {
        _bitmaps.clear();
    }

    // append the first section
    appendSection();

    // reserve all the reserved
    for (BlockIndex i = 0; i < _start_at; ++i) {
        reserve(i);
    }

    // reserve the count block
    reserve(_start_at);

    flush();
}

void
BitmapAllocator::appendSection()
{
    BlockIndex new_bitmap_index = _bitmaps.size();
    Buffer new_bitmap_buf(Driver::BLOCK_SIZE);
    std::fill(new_bitmap_buf.begin(), new_bitmap_buf.end(), 0);
    _bitmaps.emplace_back(Bitmap{new_bitmap_index, new_bitmap_buf, 0, false});

    // reserve for the bitmap itself
    reserve(calculateBitmapBlockIndex(new_bitmap_index));

    // modify section count
    Length *count_ptr = reinterpret_cast<Length*>(_count_block.content());
    count_ptr[MAX_SECTION_COUNT - 1] ++;
}

void
BitmapAllocator::reserve(BlockIndex index)
{
    BlockIndex bitmap_index = index / BLOCK_PER_SECTION;
    BlockIndex block_offset = index % BLOCK_PER_SECTION;
    auto &bitmap = _bitmaps[bitmap_index];
    setBitmapOn(bitmap, block_offset);
}

void
BitmapAllocator::setBitmapOn(Bitmap &bitmap, BlockIndex offset)
{ setBitmapOnRange(bitmap, offset, 1); }

void
BitmapAllocator::setBitmapOnRange(Bitmap &bitmap, BlockIndex offset, Length length)
{
    assert(length + (offset % BLOCK_PER_UNIT) <= BLOCK_PER_UNIT);

    OperationUnit *unit_ptr = reinterpret_cast<OperationUnit*>(bitmap.bitmap.content());
    Length unit_index = offset / BLOCK_PER_UNIT;
    Length unit_offset = offset % BLOCK_PER_UNIT;

    Length mask = (~((~(static_cast<OperationUnit>(0))) << length)) << unit_offset;
    unit_ptr[unit_index] |= mask;
    bitmap.dirty = true;
    bitmap.count += length;
}

void
BitmapAllocator::setBitmapOff(Bitmap &bitmap, BlockIndex offset)
{ setBitmapOffRange(bitmap, offset, 1); }

void
BitmapAllocator::setBitmapOffRange(Bitmap &bitmap, BlockIndex offset, Length length)
{
    OperationUnit *unit_ptr = reinterpret_cast<OperationUnit*>(bitmap.bitmap.content());
    Length unit_index = offset / BLOCK_PER_UNIT;
    Length unit_offset = offset % BLOCK_PER_UNIT;

    Length mask = ~((~((~(static_cast<OperationUnit>(0))) << length)) << unit_offset);
    unit_ptr[unit_index] &= mask;
    bitmap.dirty = true;
    bitmap.count -= length;
}

BlockIndex
BitmapAllocator::allocateBlocks(Length length, BlockIndex hint)
{
    assert(length);
    assert(length <= BLOCK_PER_UNIT);

    BlockIndex hint_section = hint / BLOCK_PER_SECTION;
    BlockIndex section_hint = hint % BLOCK_PER_SECTION;

    BlockIndex ret;

    while (_bitmaps.size() <= hint_section) {
        appendSection();
    }

    // no enough space in the hinting section
    if (_bitmaps[hint_section].count <= BLOCK_PER_SECTION - length) {
        if (allocateBlocksInSection(_bitmaps[hint_section], length, section_hint, ret)) {
            assert((ret + hint_section * BLOCK_PER_SECTION) > _start_at);
            return ret + hint_section * BLOCK_PER_SECTION;
        }
    }

    // try to allocate before the hinting section
    if (hint_section) {
        BlockIndex section = hint_section - 1;
        do {
            if ((_bitmaps[section].count <= BLOCK_PER_SECTION - length) &&
                    allocateBlocksInSection(_bitmaps[section], length, 0, ret)) {
                return ret + section * BLOCK_PER_SECTION;
            }
        } while(section--);
    }

    // try to allocate after the hinting section
    Length section_count = _bitmaps.size();
    for (BlockIndex section = hint_section + 1; section < section_count; ++section) {
        if ((_bitmaps[section].count <= BLOCK_PER_SECTION - length) &&
                allocateBlocksInSection(_bitmaps[section], length, 0, ret)) {
            return ret + section * BLOCK_PER_SECTION;
        }
    }

    // append a new section
    appendSection();
    bool last_allocation_result = allocateBlocksInSection(_bitmaps.back(), length, 0, ret);
    assert(last_allocation_result);

    return ret + (_bitmaps.size() - 1) * BLOCK_PER_SECTION;
}

bool
BitmapAllocator::allocateBlocksInSection(
        Bitmap &bitmap,
        Length length,
        BlockIndex section_hint,
        BlockIndex &result
    )
{
    BlockIndex hint_unit = section_hint / BLOCK_PER_UNIT;
    OperationUnit *unit_start = reinterpret_cast<OperationUnit*>(bitmap.bitmap.content());
    OperationUnit *unit_limit = unit_start + MAX_UNIT_COUNT;
    OperationUnit *unit_hint_ptr = unit_start + hint_unit;

    // find begin at hinting point
    for (
            OperationUnit *unit_ptr = unit_hint_ptr;
            unit_ptr != unit_limit;
            ++unit_ptr
    ) {
        Length leading_size = cdb_count_leading_zero_32(*unit_ptr);

        if (leading_size >= length) {
            result = ((unit_ptr - unit_start) * BLOCK_PER_UNIT) + (32 - leading_size);
            setBitmapOnRange(bitmap, result, length);
            return true;
        }
    }

    // find from start if hinted
    if (section_hint) {
        for (
                OperationUnit *unit_ptr = unit_hint_ptr;
                unit_ptr >= unit_start;
                --unit_ptr
        ) {
            Length leading_size = cdb_count_leading_zero_32(*unit_ptr);

            if (leading_size >= length) {
                result = ((unit_ptr - unit_start) * BLOCK_PER_UNIT) + (32 - leading_size);
                setBitmapOnRange(bitmap, result, length);
                return true;
            }
        }
    }

    // not found
    return false;
}

void
BitmapAllocator::freeBlocks(BlockIndex index, Length length)
{
    BlockIndex section = index / BLOCK_PER_SECTION;
    BlockIndex offset = index % BLOCK_PER_SECTION;
    setBitmapOffRange(_bitmaps[section], offset, length);
}

