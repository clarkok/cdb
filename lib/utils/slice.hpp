#ifndef _DB_UTILS_BUFFER_VIEW_H_
#define _DB_UTILS_BUFFER_VIEW_H_

#include <cassert>
#include <algorithm>
#include <iterator>

#include "byte_iterator.hpp"
#include "buffer.hpp"

namespace cdb {

    /**
     * @class Slice
     * this class hold weak reference of a piece of memory
     */
    class Slice
    {
        Byte* content_;
        Length length_;

    public:
        typedef _Iterator::Iterator<Byte> SliceIterator;
        typedef _Iterator::Iterator<const Byte> ConstSliceIterator;

        Slice(Byte* content, Length length);

        Slice(Buffer &buffer)
            : Slice(buffer.content(), buffer.length())
        { }

        inline Length
        length() const
        { return length_; }

        inline const Byte*
        content() const
        { return content_; }

        inline Byte*
        content()
        {
            return const_cast<Byte*>(
                    const_cast<const Slice*>(this)->content());
        }

        inline ConstSliceIterator
        cbegin() const
        { return ConstSliceIterator(content(), content(), content() + length()); }

        inline ConstSliceIterator
        cend() const
        { return ConstSliceIterator(content() + length(), content(), content() + length()); }

        inline SliceIterator
        begin()
        { return SliceIterator(content(), content(), content() + length()); }

        inline SliceIterator
        end()
        { return SliceIterator(content() + length(), content(), content() + length()); }

        inline Slice
        subSlice(Length index)
        {
            assert(index <= length());
            return Slice(content() + index, length() - index);
        }

        inline Slice
        subSlice(Length index, Length length)
        {
            assert(index <= this->length());
            return Slice(content() + index, std::min(length, this->length() - index));
        }
    };

    class ConstSlice
    {
        const Byte* content_;
        Length length_;
    public:
        typedef _Iterator::Iterator<const Byte> ConstSliceIterator;

        ConstSlice(const Byte* content_, Length length);

        ConstSlice(const Buffer &buffer)
            : ConstSlice(buffer.content(), buffer.length())
        { }

        ConstSlice(const Slice &slice)
            : ConstSlice(slice.content(), slice.length())
        { }

        inline Length
        length() const
        { return length_; }

        inline const Byte*
        content() const
        { return content_; }

        inline ConstSliceIterator
        cbegin() const
        { return ConstSliceIterator(content(), content(), content() + length()); }

        inline ConstSliceIterator
        cend() const
        { return ConstSliceIterator(content() + length(), content(), content() + length()); }

        inline ConstSlice
        subSlice(Length index)
        {
            assert(index <= length());
            return ConstSlice(content() + index, length() - index);
        }

        inline ConstSlice
        subSlice(Length index, Length length)
        {
            assert(index <= this->length());
            return ConstSlice(content() + index, std::min(length, this->length() - index));
        }
    };
}

#endif // _DB_UTILS_BUFFER_VIEW_H_
