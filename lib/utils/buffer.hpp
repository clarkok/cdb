#ifndef _DB_UTILS_BUFFER_H_
#define _DB_UTILS_BUFFER_H_

#include <cstdint>
#include <memory>

namespace cdb {
    typedef std::int8_t     Byte;
    typedef std::uint32_t   Length;

    /**
     * @struct BufferImpl
     * the defination of this struct is hidden
     */
    struct BufferImpl;

    /**
     * @class Buffer
     * this class hold strong reference of a piece of memory
     */
    class Buffer
    {
        std::shared_ptr<BufferImpl> pimpl_; /** The impl of this buffer         */
        Byte* content_;                     /** cache content of this Buffer    */
        Length length_;                     /** cache length of this Buffer     */

        void ensureOwnership();

        Buffer(std::shared_ptr<BufferImpl> &&pimpl);

    public:
        Buffer(Length length);
        Buffer(const Buffer &buffer);
        Buffer(const Byte* cbegin, const Byte* cend);
        Buffer(Buffer &&buffer);

        inline Length
        length() const
        { return length_; }

        inline const Byte*
        content() const
        { return content_; }

        inline Byte*
        content()
        { 
            ensureOwnership();
            return const_cast<Byte*>(
                    const_cast<const Buffer*>(this)->content());
        }

        inline const Byte*
        cbegin() const
        { return content(); }

        inline const Byte*
        cend() const
        { return content() + length(); }

        inline Byte*
        begin()
        {
            ensureOwnership();
            return const_cast<Byte*>(cbegin());
        }

        inline Byte*
        end()
        {
            ensureOwnership();
            return const_cast<Byte*>(cend());
        }
    };
}

#endif // _DB_UTILS_BUFFER_H_
