#include "slice.hpp"

using namespace cdb;

using cdb::Byte;
using cdb::Length;

Slice::Slice(Byte* content, Length length)
    : content_(content), length_(length)
{ }

ConstSlice::ConstSlice(const Byte* content, Length length)
    : content_(content), length_(length)
{ }
