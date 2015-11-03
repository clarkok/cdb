#ifndef _DB_TABLE_VIEW_H_
#define _DB_TABLE_VIEW_H_

#include <memory>
#include <arpa/nameser.h>

#include "schema.hpp"

namespace cdb {
    class View
    {
    protected:
        std::unique_ptr<Schema> _schema;

        struct IteratorImpl {
            virtual ~IteratorImpl() = default;

            virtual void next() = 0;
            virtual void prev() = 0;
            virtual Slice slice() = 0;
        };

    public:
        /**
         * Iterator is should not be inherited
         */
        struct Iterator
        {
            Iterator(View *owner, IteratorImpl *pimpl)
                    : _owner(owner), _pimpl(pimpl)
            { }
        public:
            Iterator(const Iterator &) = default;
            Iterator(Iterator &&) = default;

            View *_owner;
            std::unique_ptr<IteratorImpl> _pimpl;

            Schema *
            getSchema() const
            { return _owner->_schema.get(); }

            void
            next()
            { _pimpl->next(); }

            void
            prev()
            { _pimpl->prev(); }

            Slice
            slice()
            { return _pimpl->slice(); }

            template <typename T>
            static Iterator make(View *owner, T* pimpl)
            {
                static_assert(std::is_base_of<IteratorImpl, T>::value,
                              "pimpl must be a class devired from IteratorImpl");
                return Iterator(owner, pimpl);
            }

            friend class View;
        };

        virtual ~View() = default;

        /**
         * filter records with range in this View
         *
         * NOTE: the returned pointer may pointing to the original View
         *
         * @param col column in this View to filter
         * @param lower_bound of column in this View
         * @param upper_bound of column in this View
         * @return the pointer of the new View
         * @see View *filter(Schema::Column, Iterator, Iterator, Schema::Column)
         */
        virtual View *filter(Schema::Column col, const Byte *lower_bound, const Byte *upper_bound) = delete;

        /**
         * filter records with another View
         *
         * NOTE: the returned pointer may pointing to the original View
         *
         * @param col column in this View to filter
         * @param b beginning Iterator of the other View
         * @param e ending Iterator of the other View
         * @param sel column in the other View to filter with
         * @return the pointer of the new View
         * @see View *filter(Schema::Column col, const Byte *, const Byte *)
         */
        virtual View *filter(Schema::Column col, Iterator b, Iterator e, Schema::Column sel) = delete;

        /**
         * get the beginning Iterator of this View
         *
         * @return the beginning Iterator
         * @see end()
         */
        virtual Iterator begin() = delete;

        /**
         * get the ending Iterator of this View
         *
         * @return the ending Iterator
         * @see begin()
         */
        virtual Iterator end() = delete;

        /**
         * find the lower bound of primary key in this View
         *
         * @param primary_value the value to find
         * @return the Iterator pointing to that row
         * @see upperBound
         */
        virtual Iterator lowerBound(const Byte *primary_value) = delete;

        /**
         * find the upper bound of primary key in this View
         *
         * @param primary_value the value to find
         * @return the Iterator pointing to the row after that row
         * @see lowerBound
         */
        virtual Iterator upperBound(const Byte *primary_value) = delete;
    };
}

#endif // _DB_TABLE_VIEW_H_
