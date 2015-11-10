#ifndef _DB_DRIVER_CACHED_ACCESSER_H_
#define _DB_DRIVER_CACHED_ACCESSER_H_

#include <exception>
#include <list>
#include "driver-accesser.hpp"

namespace cdb {
    struct CachedNotFoundException : public std::exception
    {
        BlockIndex index;

        CachedNotFoundException(BlockIndex index)
            : index(index)
        { }

        const char *what () const noexcept
        { return ("Cached for " + std::to_string(index) + " not found").c_str(); }
    };

    class CachedAccesser : public DriverAccesser
    {
        constexpr static Length CACHE_BLOCK_SIZE = 1024 * 1024; // 1MB
        constexpr static Length CACHE_MAX_BLOCK_COUNT = 100;    // 100MB cache
        constexpr static Length TOTAL_CACHED = CACHE_BLOCK_SIZE * CACHE_MAX_BLOCK_COUNT;
        constexpr static Length BLOCK_PER_CACHE = CACHE_BLOCK_SIZE / Driver::BLOCK_SIZE;

        static constexpr BlockIndex calcTagByBlockIndex(BlockIndex index)
        { return index / BLOCK_PER_CACHE; }

        static constexpr BlockIndex calcOffsetByBlockIndex(BlockIndex index)
        { return Driver::BLOCK_SIZE * (index % BLOCK_PER_CACHE); }

        static constexpr BlockIndex calcIndexByTag(BlockIndex tag)
        { return tag * BLOCK_PER_CACHE; }

        struct CacheBlock
        {
            Length count;
            Length accessed;
            BlockIndex tag;
            Buffer content;

            CacheBlock(Length count, Length accessed, BlockIndex tag, Buffer &&content)
                : count(count), accessed(accessed), tag(tag), content(std::move(content))
            { }

            CacheBlock(CacheBlock &&cache_block)
                : count(cache_block.count),
                  accessed(cache_block.accessed),
                  tag(cache_block.tag),
                  content(std::move(cache_block.content))
            { }
        };

        std::list<CacheBlock> _record;

        std::list<CacheBlock>::iterator findRecordByBlockIndexAndIncCount(BlockIndex index);
        std::list<CacheBlock>::iterator findRecordByBlockIndexOnly(BlockIndex index);

    protected:
        virtual void release(BlockIndex block, bool dirty = true);
        virtual Slice access(BlockIndex index);

    public:
        CachedAccesser(Driver *drv, BlockAllocator *allocator)
            : DriverAccesser(drv, allocator)
        { }

        virtual ~CachedAccesser() = default;

        virtual BlockIndex allocateBlocks(Length length, BlockIndex hint = 0);
        virtual void freeBlocks(BlockIndex index, Length length);
        virtual void flush();
    };

}

#endif // _DB_DRIVER_CACHED_ACCESSER_H_
