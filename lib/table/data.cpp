#include "data.hpp"

using namespace cdb;

Data::Data(const BufferView &content)
    : _content(content)
{}

BufferView
Data::getView() const
{ return _content; }

void
Data::setView(const BufferView &content)
{ _content = content; }
