#include <cassert>

#include "basic-driver.hpp"

using namespace cdb;

using cdb::Byte;
using cdb::Length;

BasicDriver::BasicDriver(const char *path)
    : _fd(std::fopen(path, "r+"))
{
    if (!_fd) {
        _fd = std::fopen(path, "w+");
    }

    assert(_fd);
}

BasicDriver::~BasicDriver()
{ fclose(_fd); }

void
BasicDriver::readBlocks(BlockIndex index, Length count, Slice dest)
{
    assert(dest.length() >= BLOCK_SIZE * count);

    std::fseek(_fd, index * BLOCK_SIZE, SEEK_SET);

    auto ret = std::fread(dest.content(), BLOCK_SIZE, count, _fd);
    if (ret != count) {
        std::fill(dest.begin() + ret * BLOCK_SIZE, dest.begin() + count * BLOCK_SIZE, 0);
    }
}

void
BasicDriver::writeBlocks(BlockIndex index, Length count, ConstSlice src)
{
    assert(src.length() >= BLOCK_SIZE * count);

    std::fseek(_fd, index * BLOCK_SIZE, SEEK_SET);

    auto ret =  std::fwrite(src.content(), BLOCK_SIZE, count, _fd);
    assert(ret == count);
}

void
BasicDriver::flush()
{ std::fflush(_fd); }
