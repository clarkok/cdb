#ifndef _DB_INDEX_LINEAR_TABLE_INTL_H_
#define _DB_INDEX_LINEAR_TABLE_INTL_H_

#include "linear-table.hpp"

namespace cdb {
    struct LinearTable::Header
    {
        Length block_count;
        Length record_count;
        BlockIndex direct[3];   // direct blocks
        BlockIndex primary;     // primary index block
        BlockIndex secondary;   // secondary index block
        BlockIndex tertiary;    // tertiary index block
    };

    struct LinearTable::BlockHeader
    {
        Length record_count;
    };
}

using namespace cdb;

LinearTable::Header*
LinearTable::getHeaderOfHead(Block &head)
{ return reinterpret_cast<Header*>(head.content()); }

Length
LinearTable::getMaximumRecordCountByBlockIndex(BlockIndex index) const
{
    return
        (index == _head.index()) ?
            getMaximumRecordCountInHead() :
            getMaximumRecordCountInNormal();
}

Length
LinearTable::getMaximumRecordCountInHead() const
{ return (Driver::BLOCK_SIZE - sizeof(Header) - sizeof(BlockHeader)) / _value_size; }

Length
LinearTable::getMaximumRecordCountInNormal() const
{ return (Driver::BLOCK_SIZE - sizeof(BlockHeader)) / _value_size; }

Block
LinearTable::fetchBlockByIndex(BlockIndex index)
{
    if (index == 0) {
        return _head;
    }
    index -= 1;

    if (index < directBlockCount()) {
        return _accesser->aquire(_header->direct[index]);
    }
    index -= directBlockCount();

    if (index < primaryBlockCount()) {
        return fetchPrimaryIndexedBlock(_header->primary, index);
    }
    index -= primaryBlockCount();

    if (index < secondaryBlockCount()) {
        return fetchSecondaryIndexedBlock(_header->secondary, index);
    }
    index -= secondaryBlockCount();

    return fetchTertiaryIndexedBlock(_header->tertiary, index);
}

constexpr Length
LinearTable::directBlockCount()
{ return sizeof(Header::direct) / sizeof(Header::direct[0]); }

constexpr Length
LinearTable::primaryBlockCount()
{ return Driver::BLOCK_SIZE / sizeof(BlockIndex); }

constexpr Length
LinearTable::secondaryBlockCount()
{ return primaryBlockCount() * (Driver::BLOCK_SIZE / sizeof(BlockIndex)); }

constexpr Length
LinearTable::maximumIndexPerBlock()
{ return Driver::BLOCK_SIZE / sizeof(BlockIndex); }

Block
LinearTable::fetchPrimaryIndexedBlock(BlockIndex primary_index, BlockIndex offset)
{
    Block primary = _accesser->aquire(primary_index);
    return _accesser->aquire(
            reinterpret_cast<BlockIndex*>(primary.content())[offset]);
}

Block
LinearTable::fetchSecondaryIndexedBlock(BlockIndex secondary_index, BlockIndex offset)
{
    Block secondary = _accesser->aquire(secondary_index);
    BlockIndex primary_index = 
        reinterpret_cast<BlockIndex*>(secondary.content())
            [offset / maximumIndexPerBlock()];
    return fetchPrimaryIndexedBlock(
            primary_index,
            offset % maximumIndexPerBlock()
        );
}

Block
LinearTable::fetchTertiaryIndexedBlock(BlockIndex tertiary_index, BlockIndex offset)
{
    Block tertiary = _accesser->aquire(tertiary_index);
    BlockIndex secondary_index =
        reinterpret_cast<BlockIndex*>(tertiary.content())
            [offset / (maximumIndexPerBlock() * maximumIndexPerBlock())];
    return fetchSecondaryIndexedBlock(
            secondary_index,
            offset % (maximumIndexPerBlock() * maximumIndexPerBlock())
        );
}

#endif // _DB_INDEX_LINEAR_TABLE_INTL_H_
