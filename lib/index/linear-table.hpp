#ifndef _DB_INDEX_LINEAR_TABLE_H_
#define _DB_INDEX_LINEAR_TABLE_H_

#include "lib/driver/driver-accesser.hpp"

namespace cdb {
    class LinearTable
    {
        struct Header;
        struct BlockHeader;
        DriverAccesser *_accesser;
        Block _head;
        Length _value_size;

        Header *_header;

    public:
        class Iterator
        {
            LinearTable *_owner;
            Block _block;
            Byte *_entry;

            Iterator(LinearTable *owner, Block block, Byte *entry);

            Iterator(const Iterator &) = delete;
            Iterator &operator = (const Iterator &) = delete;

            friend class LinearTable;

        public:
            ~Iterator() = default;

            Iterator(Iterator &&iter)
                : _owner(iter._owner), _block(std::move(iter._block)), _entry(iter._entry)
            { }

            Iterator &
            operator = (Iterator &&iter)
            {
                _owner = iter._owner;
                _block = std::move(iter._block);
                _entry = iter._entry;
            }

            inline Slice
            operator *() const
            { return Slice(_entry, _owner->_value_size); }

            inline Iterator &
            operator ++()
            { return (*this = _owner->nextIterator(*this)); }

            inline Iterator &
            operator --()
            { return (*this = _owner->prevIterator(*this)); }

            inline Iterator
            operator ++(int)
            {
                auto ret = _owner->nextIterator(*this);
                std::swap(ret, *this);
                return ret;
            }

            inline Iterator
            operator --(int)
            {
                auto ret = _owner->nextIterator(*this);
                std::swap(ret, *this);
                return ret;
            }

            inline bool
            operator == (const Iterator &b)
            {
                return 
                    (_block.index() == b._block.index()) &&
                    (_entry == b._entry) &&
                    (_owner == b._owner);
            }

            inline bool
            operator != (const Iterator &b)
            { return !this->operator==(b); }

        };
    private:
        inline Header *getHeaderOfHead(Block &head);
        inline Length getMaximumRecordCountByBlockIndex(BlockIndex index) const;
        inline Length getMaximumRecordCountInHead() const;
        inline Length getMaximumRecordCountInNormal() const;

        // index in this table
        inline Block fetchBlockByIndex(BlockIndex index);
        static inline constexpr Length directBlockCount();
        static inline constexpr Length primaryBlockCount();
        static inline constexpr Length secondaryBlockCount();
        static inline constexpr Length maximumIndexPerBlock();
        inline Block fetchPrimaryIndexedBlock(BlockIndex primary_index, BlockIndex offset);
        inline Block fetchSecondaryIndexedBlock(BlockIndex secondary_index, BlockIndex offset);
        inline Block fetchTertiaryIndexedBlock(BlockIndex tertiary_index, BlockIndex offset);

        inline Byte* getFirstEntry(Block &block);
        inline Byte* getLimitEntry(Block &block);

        Iterator nextIterator(const Iterator &i);
        Iterator prevIterator(const Iterator &i);
    public:
        LinearTable(
                DriverAccesser *accesser,
                BlockIndex head_index,
                Length value_size
            );

        void reset();

        Iterator begin();
        Iterator end();
    };
}

#endif // _DB_INDEX_LINEAR_TABLE_H_
