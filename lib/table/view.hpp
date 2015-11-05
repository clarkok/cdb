#ifndef _DB_TABLE_VIEW_H_
#define _DB_TABLE_VIEW_H_

#include <functional>
#include <memory>
#include <arpa/nameser.h>

#include "schema.hpp"

namespace cdb {
    class ModifiableView;

    class View
    {
    protected:
        std::unique_ptr<Schema> _schema;

        struct IteratorImpl {
            virtual ~IteratorImpl() = default;

            virtual void next() = 0;
            virtual void prev() = 0;
            virtual ConstSlice constSlice() = 0;
            virtual Slice slice() = 0;
            virtual bool equal(const IteratorImpl &b) const = 0;
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
            Iterator(const Iterator &) = delete;
        public:
            Iterator(Iterator &&) = default;

            View *_owner;
            mutable std::unique_ptr<IteratorImpl> _pimpl;

            inline Schema *
            getSchema() const
            { return _owner->_schema.get(); }

            inline void
            next()
            { _pimpl->next(); }

            inline void
            prev()
            { _pimpl->prev(); }

            inline Slice
            slice() const
            { return _pimpl->slice(); }

            inline ConstSlice
            constSlice() const
            { return _pimpl->constSlice(); }

            inline bool
            operator == (const Iterator &b) const
            { return _pimpl->equal(*b._pimpl); }

            inline bool
            operator != (const Iterator &b) const
            { return !_pimpl->equal(*b._pimpl); }

            template <typename T>
            static Iterator make(View *owner, T* pimpl)
            {
                static_assert(std::is_base_of<IteratorImpl, T>::value,
                              "pimpl must be a class devired from IteratorImpl");
                return Iterator(owner, pimpl);
            }

            friend class View;
        };

        typedef std::function<bool(const Schema*, ConstSlice)> Filter;

        static Filter getDefaultFilter()
        {
            static Filter ret = [](const Schema *, ConstSlice) -> bool { return true; };
            return ret;
        }

        View(Schema *schema)
                : _schema(schema)
        { }

        virtual ~View() = default;

        Schema *
        getSchema() const
        { return _schema.get(); }

        /**
         * Select some columns in this view with filter
         *
         * the result of copying will be stored in memory only, and this View remains unchanged
         *
         * @param schema the schema to select
         * @param filter the filter return true if want this record selected
         * @return a new View, must in memory
         * @see selectRange
         * @see selectIndexed
         */
        virtual ModifiableView *select(Schema *schema, Filter filter = getDefaultFilter());

        /**
         * Select rows in this view
         *
         * the result of copying will be stored in memory only, and this View remains unchanged
         *
         * @param schema the schema to copy
         * @param b beginning Iterator of this View
         * @param e ending Iterator of this View
         * @param filter
         * @return a new View, must in memory
         * @see select
         * @see selectIndexed
         */
        virtual ModifiableView *selectRange(
                Schema *schema,
                Iterator b,
                Iterator e,
                Filter filter = getDefaultFilter()
        );

        /**
         * Select some columns in this view with filter, using index
         *
         * the result of copying will be stored in memory only, and this View remains unchanged
         *
         * @param schema the schema to select
         * @param b beginning Iterator of index
         * @param e ending Iterator of index
         * @param filter
         * @return a new View, must in memory
         * @see select
         * @see selectRnage
         */
        virtual ModifiableView *selectIndexed(
                Schema *schema,
                Iterator b,
                Iterator e,
                Filter filter = getDefaultFilter()
        );

        /**
         * peek records with column in the given range in this View
         *
         * the result of peek should contains primary keys only, and it be placed on a new View,
         * while this View remains unchanged
         *
         * @param col column in this View to filter
         * @param lower_bound of column in this View
         * @param upper_bound of column in this View
         * @return the result of peeking
         * @see View *filter(Schema::Column, Iterator, Iterator, Schema::Column)
         */
        virtual ModifiableView *peek(Schema::Column col, const Byte *lower_bound, const Byte *upper_bound) = 0;

        /**
         * get the beginning Iterator of this View
         *
         * @return the beginning Iterator
         * @see end()
         */
        virtual Iterator begin() = 0;

        /**
         * get the ending Iterator of this View
         *
         * @return the ending Iterator
         * @see begin()
         */
        virtual Iterator end() = 0;

        /**
         * find the lower bound of primary key in this View
         *
         * @param primary_value the value to find
         * @return the Iterator pointing to that row
         * @see upperBound
         */
        virtual Iterator lowerBound(const Byte *primary_value) = 0;

        /**
         * find the upper bound of primary key in this View
         *
         * @param primary_value the value to find
         * @return the Iterator pointing to the row after that row
         * @see lowerBound
         */
        virtual Iterator upperBound(const Byte *primary_value) = 0;
    };

    class ModifiableView : public View
    {
    public:
        ModifiableView(Schema *schema)
                : View(schema)
        { };

        virtual ~ModifiableView() = default;

        /**
         * Get count of rows in this view
         *
         * @return count
         */
        virtual Length count() const = 0;

        /**
         * intersect this view by primary key
         *
         * the result of should performed on this View
         *
         * @param col column in this View to filter
         * @param b beginning Iterator of the other View
         * @param e ending Iterator of the other View
         * @param sel column in the other View to filter with
         * @return the result of filter
         * @see View *filter(Schema::Column col, const Byte *, const Byte *)
         */
        virtual ModifiableView *intersect(Iterator b, Iterator e) = 0;

        /**
         * join another View to this, by primary key
         *
         * the result of join should performed on this View
         *
         * @param b the beginning Iterator of the other View
         * @param e the ending Iterator of the other View
         * @return the result of join
         */
        virtual ModifiableView *join(Iterator b, Iterator e) = 0;

    };
}

#endif // _DB_TABLE_VIEW_H_
