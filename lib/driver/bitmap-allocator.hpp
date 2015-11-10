#ifndef _DB_DRIVER_BITMAP_ALLOCATOR_H_
#define _DB_DRIVER_BITMAP_ALLOCATOR_H_

#include <vector>

#include "driver.hpp"
#include "block-allocator.hpp"

namespace cdb {
    /**
     * Block allocator using bitmap allocating algorithm.
     *
     * There are two kind of special block used in this allocator, which are:
     *  - count block
     *  - bitmap block
     *
     * There is only one count block in one bitmap allocator, which is stored rigth after
     * reserved blocks by the holder of allocator, aka. the index of the count block is 
     * always `start_at`.
     *
     * The file is separated to several sections, and each section has only one bitmap
     * block. So there are several bitmap blocks in one bitmap allocator. The bitmap
     * block is located at the first block of the last operation unit of a section. when
     * a block is of 1024 bytes size, a operation unit is of 4 bytes size, then the 
     * bitmap block is located at the index of (1024 * 8) - (4 * 8) = 8160.
     *
     * When allocating blocks with a hint, the algorithm will start at the hinting 
     * operation unit and check the following one by one, by comparing the leading zero 
     * of each operation unit and the required length. Once find, length of bits starting 
     * from the least important bit will be set. It is the reason why that index of 
     * bitmap block is chosen.
     *
     * If following operation units from the hinting operation unit in the hinting 
     * section cannot feed the request, then operation units before the hinting operation
     * unit in the same section would be checked in revert order.
     *
     * If the request cannot be feeded in the hinting section, sections after the hinting
     * section would be searched first. If still not found, the sections before the 
     * hinting would be searched in reverted order. If still not found, a new section 
     * would be created. Hinting position would be ignored, when searching in sections
     * other than the hinting section.
     *
     * NOTE: length must not be greater than 8 * sizeof(OperationUnit) in one allocation
     */
    class BitmapAllocator : public BlockAllocator
    {
        /**
         * internal repersentation of bitmap inside class BitmapAllocator
         */
        struct Bitmap
        {
            BlockIndex index;   /** index of this bitmap */
            Buffer bitmap;      /** data of this bitmap */
            Length count;       /** number blocks allocated in this bitmap */
            bool dirty;         /** ture if this bitmap is dirty */
        };

        typedef std::vector<Bitmap> BitmapVector;

        /**
         * Search in a bitmap (aka. a section) is operated one unit a time. so it is able
         * to check sizeof(OperatorUnit) bits one time. It is much faster.
         */
        typedef std::uint32_t OperationUnit;

        static const Length BLOCK_PER_SECTION = Driver::BLOCK_SIZE * 8;
        static const Length MAX_SECTION_COUNT = Driver::BLOCK_SIZE / sizeof(Length);
        static const Length BLOCK_PER_UNIT = sizeof(OperationUnit) * 8;
        static const Length MAX_UNIT_COUNT = Driver::BLOCK_SIZE / sizeof(OperationUnit);

        /**
         * Bitmaps in memory, all bitmaps is read into memory when constructing, and
         * would be flushed back to disk when flushing
         */
        BitmapVector _bitmaps;

        /**
         * Interpreted as an array of Length. Last element in the array indicate the 
         * total number of sections in this allocator. Other elements is the allocated 
         * blocks in each section.
         *
         * NOTE: when in memory, the count of allocated blocks of each section is stored
         * and operated in struct Bitmap instead of _count_block. Couning infomation 
         * would be gathered to _count_block and writed back to disk when flushing.
         */
        Buffer _count_block;

        /**
         * Calculate the index of each bitmap block by the index of each bitmap
         *
         * @param bitmap_index the index of the bitmap
         * @return the block index of bitmap block
         */
        inline BlockIndex calculateBitmapBlockIndex(BlockIndex bitmap_index);

        /**
         * Calculate the index of the count block. Always return start_at_, currently.
         *
         * @return the block index of count block
         */
        inline BlockIndex calculateCountBlockIndex();

        /**
         * Append a new section and initialize a new bitmap at the end of _bitmaps.
         */
        inline void appendSection();

        /**
         * Reserve a block in the allocator. Used when resetting the allocator and 
         * reserving the blocks before `start_at_`. Also used when reserving the bitmap
         * block itself
         *
         * @param index the index of block to reserve
         */
        inline void reserve(BlockIndex index);

        /**
         * Set a bit in the bitmap on
         *
         * @param bitmap the bitmap to operate
         * @param offset the offset of the bit in the bitmap to set on
         * @see setBitmapOnRange
         */
        inline void setBitmapOn(Bitmap &bitmap, BlockIndex offset);

        /**
         * Set a range of bits in the bitmap on
         *
         * NOTE: bits must not cross over the bound of operation units
         *
         * @param bitmap the bitmap to operate
         * @param offset the offset of the first bit to set on
         * @param length count of bits to set on
         * @see setBitmapOn
         */
        inline void setBitmapOnRange(Bitmap &bitmap, BlockIndex offset, Length length);

        /**
         * Set a bit in the bitmap off
         *
         * @param bitmap the bitmap to operate
         * @param offset the offset of the bit in the bitmap to set off
         * @see setBitmapOffRange
         */
        inline void setBitmapOff(Bitmap &bitmap, BlockIndex offset);

        /**
         * Set a range of bits in the bitmap off
         *
         * NOTE: bits must not cross over the bound of operatioff units
         *
         * @param bitmap the bitmap to operate
         * @param offset the offset of the first bit to set off
         * @param length count of bits to set off
         * @see setBitmapOff
         */
        inline void setBitmapOffRange(Bitmap &bitmap, BlockIndex offset, Length length);

        /**
         * Allocate blocks in one section
         *
         * NOTE: length of blocks to allocate must not be greater than
         * sizeof(OperationUnit)
         *
         * @param bitmap the bitmap to operate
         * @param length the length of blocks to allocate
         * @param section_hint hint offset in this section
         * @param result [out] the result
         * @return true if succeed, otherwise false
         */
        inline bool allocateBlocksInSection(
                Bitmap &bitmap,
                Length length,
                BlockIndex section_hint,
                BlockIndex &result
            );
    public:
        /**
         * Constructor, read necessery infomation from disk
         *
         * @param drv the drv to read from and write to
         * @param start_at count of reserved block at the beginning of the driver
         */
        BitmapAllocator(Driver *drv, BlockIndex start_at);

        /**
         * Flush the infomation, and free all memory used internal
         */
        virtual ~BitmapAllocator();

        /**
         * Reset the allocator, rebuild the count block and first bitmap block, then flush
         */
        virtual void reset();

        /**
         * Flush all the changes back to the disk, only dirty bitmaps will be written
         */
        virtual void flush();

        /**
         * Allocate a series of blocks from the allocator, with of without hint
         *
         * NOTE: the length must not be greater than sizeof(OperationUnit), which is 32,
         * currently
         *
         * @param length the count of blocks required
         * @param hint hint to the allocator
         * @return the index of first block allocated
         */
        virtual BlockIndex allocateBlocks(Length length, BlockIndex hint = 0);

        /**
         * Free a series of block
         *
         * NOTE: the blocks to free should always be allocated, allocator would never 
         * check for it
         *
         * @param index the index of first block to free
         * @param length the number of blocks to free
         */
        virtual void freeBlocks(BlockIndex index, Length length);
    };
}

#endif // _DB_DRIVER_BITMAP_ALLOCATOR_H_
