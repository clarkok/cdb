#include "buffer.hpp"

using namespace cdb;

using cdb::Byte;
using cdb::Length;

namespace cdb {
    struct BufferImpl
    {
        Length length;
        Byte* content;

        BufferImpl(Length length)
            : length(length), content(new Byte[length])
        { }

        BufferImpl(BufferImpl &&impl)
            : length(impl.length), content(impl.content)
        { impl.content = nullptr; }

        BufferImpl
        copy() const
        {
            BufferImpl ret(length);
            std::copy(content, content + length, ret.content);

            return ret;
        }

        ~BufferImpl()
        { delete[] content; }
    };
}

Buffer::Buffer(Length length)
    : Buffer(std::make_shared<BufferImpl>(length))
{ }

Buffer::Buffer(const Buffer &buffer)
    : Buffer(std::shared_ptr<BufferImpl>(buffer.pimpl_))
{ }

Buffer::Buffer(const Byte* cbegin, const Byte* cend)
    : Buffer(cend - cbegin)
{ std::copy(cbegin, cend, begin()); }

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
