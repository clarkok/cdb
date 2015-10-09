#include <cassert>

#include "driver.hpp"

using namespace cdb;

void
Driver::readBlocks(BlockIndex index, Length count, Slice dest)
{
    while (count) {
        assert(dest.length() >= BLOCK_SIZE);

        readBlock(index, dest);
        ++index;
        --count;
        dest = dest.subSlice(BLOCK_SIZE);
    }
}

void
Driver::writeBlocks(BlockIndex index, Length count, ConstSlice dest)
{
    while (count) {
        assert(dest.length() >= BLOCK_SIZE);

        writeBlock(index, dest);
        ++index;
        --count;
        dest = dest.subSlice(BLOCK_SIZE);
    }
}
