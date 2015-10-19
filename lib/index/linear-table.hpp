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
    public:
        LinearTable(
                DriverAccesser *accesser,
                BlockIndex head_index,
                Length value_size
            );

        void reset();
    };
}

#endif // _DB_INDEX_LINEAR_TABLE_H_
