#ifndef _DB_TABLE_INDEX_VIEW_H_
#define _DB_TABLE_INDEX_VIEW_H_

#include <memory>

#include "lib/index/btree.hpp"
#include "view.hpp"

namespace cdb {
    class IndexView : public View
    {
        struct IteratorImpl : public View::IteratorImpl
        {
            BTree::Iterator impl;

            IteratorImpl(BTree::Iterator &&impl)
                    : impl(std::move(impl))
            { }

            virtual ~IteratorImpl() = default;

            virtual void
            next()
            { impl.next(); }

            virtual void
            prev()
            { impl.prev(); }

            virtual Slice
            slice()
            { return impl.getValue(); }

            virtual bool
            equal(const View::IteratorImpl &b) const
            { return dynamic_cast<const IndexView::IteratorImpl &>(b).impl == impl; }
        };

        std::unique_ptr<BTree> _tree;
    public:
        IndexView(Schema *schema, BTree *tree)
                : View(schema), _tree(tree)
        { }

        virtual ~IndexView() = default;

        virtual ModifiableView *peek(Schema::Column col, const Byte *lower_bound, const Byte *upper_bound);

        virtual Iterator begin();
        virtual Iterator end();
        virtual Iterator lowerBound(const Byte *key);
        virtual Iterator upperBound(const Byte *key);
    };
}

#endif //_DB_TABLE_INDEX_VIEW_H_
