#include "linear-table-intl.hpp"

using namespace cdb;

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
