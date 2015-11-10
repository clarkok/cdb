#ifndef _DB_UTILS_BUFFER_H_
#define _DB_UTILS_BUFFER_H_

#include <cstdint>
#include <memory>
#include <algorithm>

#include "byte_iterator.hpp"

namespace cdb {
    typedef std::int8_t     Byte;
    typedef std::uint32_t   Length;

    /**
     * this class hold strong reference of a piece of memory
     */
    class Buffer
    {
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

        std::shared_ptr<BufferImpl> pimpl_; /** The impl of this buffer         */
        Byte* content_;                     /** cache content of this Buffer    */
        Length length_;                     /** cache length of this Buffer     */

        void ensureOwnership();

        Buffer(std::shared_ptr<BufferImpl> &&pimpl);

    public:
        typedef _Iterator::Iterator<Byte> BufferIterator;
        typedef _Iterator::Iterator<const Byte> ConstBufferIterator;

        Buffer(Length length);
        Buffer(const Buffer &buffer);
        Buffer(Buffer &&buffer);

        template <typename T>
        Buffer(T b, T e)
            : Buffer(std::make_shared<BufferImpl>(e - b))
        { std::copy(b, e, content_); }

        inline Length
        length() const
        { return length_; }

        inline const Byte*
        content() const
        { return content_; }

        inline decltype(pimpl_.use_count())
        useCount() const
        { return pimpl_.use_count(); }

        inline Byte*
        content()
        { 
            ensureOwnership();
            return const_cast<Byte*>(
                    const_cast<const Buffer*>(this)->content());
        }

        inline ConstBufferIterator
        cbegin() const
        { return ConstBufferIterator(content(), content(), content() + length()); }

        inline ConstBufferIterator
        cend() const
        { return ConstBufferIterator(content() + length(), content(), content() + length()); }

        inline BufferIterator
        begin()
        {
            ensureOwnership();
            return BufferIterator(content(), content(), content() + length());
        }

        inline BufferIterator
        end()
        {
            ensureOwnership();
            return BufferIterator(content() + length(), content(), content() + length());
        }
    };
}

#endif // _DB_UTILS_BUFFER_H_
