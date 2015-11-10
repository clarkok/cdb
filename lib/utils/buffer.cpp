#include "buffer.hpp"

using namespace cdb;

using cdb::Byte;
using cdb::Length;

Buffer::Buffer(Length length)
    : Buffer(std::make_shared<BufferImpl>(length))
{ }

Buffer::Buffer(const Buffer &buffer)
    : Buffer(std::shared_ptr<BufferImpl>(buffer.pimpl_))
{ }

Buffer::Buffer(Buffer &&buffer)
    : Buffer(std::move(buffer.pimpl_))
{ }

Buffer::Buffer(std::shared_ptr<BufferImpl> &&pimpl)
    : pimpl_(std::move(pimpl)),
      content_(pimpl_->content),
      length_(pimpl_->length)
{ }

void
Buffer::ensureOwnership()
{
    if (!pimpl_.unique()) {
        pimpl_ = std::make_shared<BufferImpl>(pimpl_->copy());
        content_ = pimpl_->content;
        length_ = pimpl_->length;
    }
}
