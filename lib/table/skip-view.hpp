#ifndef _DB_TABLE_SKIP_VIEW_H_
#define _DB_TABLE_SKIP_VIEW_H_

#include "view.hpp"
#include "lib/index/skip-table.hpp"

namespace cdb {
    class SkipView : public ModifiableView
    {
        struct IteratorImpl : public View::IteratorImpl
        {
            SkipTable::Iterator impl;

            IteratorImpl(SkipTable::Iterator impl)
                    : impl(impl)
            { }

            virtual ~IteratorImpl() = default;

            virtual void
            next()
            { impl = impl.next(); }

            virtual void
            prev()
            { impl = impl.prev(); }

            virtual Slice
            slice()
            { return *impl; }

            virtual bool
            equal(const View::IteratorImpl &b) const
            { return dynamic_cast<const SkipView::IteratorImpl &>(b).impl == impl; }
        };

        std::unique_ptr<SkipTable> _table;
    public:

        SkipView(Schema *schema, SkipTable *table)
                : ModifiableView(schema), _table(table)
        { }

        virtual ~SkipView() = default;

        virtual ModifiableView *peek(Schema::Column col, const Byte *lower_bound, const Byte *upper_bound);
        virtual ModifiableView *intersect(Iterator b, Iterator e);
        virtual ModifiableView *join(Iterator b, Iterator e);
        virtual Iterator begin();
        virtual Iterator end();
        virtual Iterator lowerBound(const Byte *value);
        virtual Iterator upperBound(const Byte *value);

        static SkipTable::Comparator getIntegerCompareFunc();
        static SkipTable::Comparator getFloatCompareFunc();
        static SkipTable::Comparator getCharCompareFunc();

        static SkipTable::Comparator getCompareFuncForType(Schema::Field::Type type);
    };
}

#endif //_DB_TABLE_SKIP_VIEW_H_
