#include "linear-table-intl.hpp"

using namespace cdb;

LinearTable::Iterator::Iterator(
        LinearTable *owner,
        Block block,
        Byte *entry,
        BlockIndex block_index
    ) : _owner(owner),
        _block(block),
        _entry(entry),
        _block_index(block_index)
{ }

LinearTable::LinearTable(
        DriverAccesser *accesser,
        BlockIndex head_index,
        Length value_size
    ) : _accesser(accesser),
        _head(_accesser->aquire(head_index)),
        _value_size(value_size),
        _header(getHeaderOfHead(_head))
{ }

void 
LinearTable::reset()
{
    // TODO free all other blocks
    *_header = {
        1,              // block_count
        0,              // record_count
        {0, 0, 0},      // direct
        0,              // primary
        0,              // secondary
        0               // tertiary
    };
}

LinearTable::Iterator
LinearTable::begin()
{
    Block block = _head;
    Byte *entry = getFirstEntry(block);
    return Iterator(this, std::move(block), entry, 0);
}

LinearTable::Iterator
LinearTable::end()
{
    Block block = fetchBlockByIndex(_header->block_count - 1);
    Byte *entry = getLimitEntry(block);
    return Iterator(this, std::move(block), entry, _header->block_count - 1);
}
