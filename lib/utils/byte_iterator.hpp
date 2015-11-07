#ifndef _DB_UTILS_BYTE_ITERATOR_H_
#define _DB_UTILS_BYTE_ITERATOR_H_

#include <cassert>

namespace cdb {
    namespace _Iterator {
        template <typename T>
        struct Iterator : std::iterator<std::random_access_iterator_tag, T>
        {
            T *start_ = nullptr;
            T *lower_limit_ = nullptr;
            T *upper_limit_ = nullptr;

            Iterator() = default;
            Iterator(const Iterator &) = default;
            Iterator(T *start, T *lower_limit, T *upper_limit)
                : start_(start), lower_limit_(lower_limit), upper_limit_(upper_limit)
            { 
                assert(start_ >= lower_limit_);
                assert(start_ <= upper_limit_);
            }
            Iterator & operator = (const Iterator &) = default;

            ~Iterator() = default;

            inline bool
            operator == (const Iterator &i) const
            {
                return start_ == i.start_ &&
                    lower_limit_ == i.lower_limit_ &&
                    upper_limit_ == i.upper_limit_;
            }

            inline bool
            operator != (const Iterator &i) const
            { return !this->operator==(i); }

            inline bool
            operator > (const Iterator &i) const
            {
                assert(lower_limit_ == i.lower_limit_);
                assert(upper_limit_ == i.upper_limit_);
                return start_ > i.start_;
            }

            inline bool
            operator >= (const Iterator &i) const
            {
                assert(lower_limit_ == i.lower_limit_);
                assert(upper_limit_ == i.upper_limit_);
                return start_ >= i.start_;
            }

            inline bool
            operator < (const Iterator &i) const
            {
                assert(lower_limit_ == i.lower_limit_);
                assert(upper_limit_ == i.upper_limit_);
                return start_ < i.start_;
            }

            inline bool
            operator <= (const Iterator &i) const
            {
                assert(lower_limit_ == i.lower_limit_);
                assert(upper_limit_ == i.upper_limit_);
                return start_ <= i.start_;
            }

            inline T &
            operator *() const
            { 
                assert(start_ >= lower_limit_);
                assert(start_ < upper_limit_);
                return *start_;
            }

            inline T *
            operator ->() const
            {
                assert(start_ >= lower_limit_);
                assert(start_ < upper_limit_);
                return start_;
            }

            inline Iterator &
            operator ++()
            {
                assert(++start_ <= upper_limit_);
                return *this;
            }

            inline Iterator
            operator ++(int)
            {
                Iterator ret(*this);
                assert(++start_ <= upper_limit_);
                return ret;
            }

            inline Iterator &
            operator --()
            {
                assert(--start_ >= lower_limit_);
                return *this;
            }

            inline Iterator
            operator --(int)
            {
                Iterator ret(*this);
                assert(--start_ >= lower_limit_);
                return ret;
            }

            inline int
            operator -(const Iterator &b) const
            {
                assert(lower_limit_ == b.lower_limit_);
                assert(upper_limit_ == b.upper_limit_);
                return start_ - b.start_;
            }
        };

        template <typename T>
        Iterator<T> operator + (const Iterator<T> &i, int delta)
        { return Iterator<T>(i.start_ + delta, i.lower_limit_, i.upper_limit_); }

        template <typename T>
        Iterator<T> operator - (const Iterator<T> &i, int delta)
        { return Iterator<T>(i.start_ - delta, i.lower_limit_, i.upper_limit_); }

        template <typename T>
        Iterator<T> &operator += (Iterator<T> &i, int delta)
        { return i = i + delta; }

        template <typename T>
        Iterator<T> &operator -= (Iterator<T> &i, int delta)
        { return i = i + delta; }
    }
}

#endif // _DB_UTILS_BYTE_ITERATOR_H_
