#ifndef _DB_DRIVER_BITMAP_ALLOCATOR_H_
#define _DB_DRIVER_BITMAP_ALLOCATOR_H_

#include <vector>

#include "driver.hpp"
#include "block-allocator.hpp"

namespace cdb {
    class BitmapAllocator : public BlockAllocator
    {
        struct Bitmap
        {
            BlockIndex index;
            Buffer bitmap;
            Length count;
            bool dirty;
        };

        typedef std::vector<Bitmap> BitmapVector;
        typedef BitmapVector::iterator BitmapIterator;
        typedef std::uint32_t OperatorUnit;

        static const Length BLOCK_PER_SECTION = Driver::BLOCK_SIZE * 8;
        static const Length MAX_SECTION_COUNT = Driver::BLOCK_SIZE / sizeof(Length);
        static const Length BLOCK_PER_UNIT = sizeof(OperatorUnit) * 8;
        static const Length MAX_UNIT_COUNT = Driver::BLOCK_SIZE / sizeof(OperatorUnit);

        BitmapVector _bitmaps;
        Buffer _count_block;

        inline void initialize();
        inline BlockIndex calculateBitmapBlockIndex(BlockIndex bitmap_index);
        inline BlockIndex calculateCountBlockIndex();

        inline void appendSection();
        inline void reserve(BlockIndex index);

        inline void setBitmapOn(Bitmap &bitmap, BlockIndex offset);
        inline void setBitmapOnRange(Bitmap &bitmap, BlockIndex offset, Length length);

        inline void setBitmapOff(Bitmap &bitmap, BlockIndex offset);
        inline void setBitmapOffRange(Bitmap &bitmap, BlockIndex offset, Length length);

        inline bool allocateBlocksInSection(
                Bitmap &bitmap,
                Length length,
                BlockIndex section_hint,
                BlockIndex &result
            );
    public:
        BitmapAllocator(Driver *drv, BlockIndex start_at);

        virtual ~BitmapAllocator();

        virtual void reset();
        virtual void flush();

        virtual BlockIndex allocateBlocks(Length length, BlockIndex hint = 0);
        virtual void freeBlocks(BlockIndex index, Length length);
    };
}

#endif // _DB_DRIVER_BITMAP_ALLOCATOR_H_
