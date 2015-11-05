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
            bool select_key = false;

            IteratorImpl(BTree::Iterator &&iter, bool select_key)
                    : impl(std::move(iter)), select_key(select_key)
            { }

            virtual ~IteratorImpl() = default;

            virtual void
            next()
            { impl.next(); }

            virtual void
            prev()
            { impl.prev(); }

            virtual ConstSlice
            constSlice()
            { return select_key ? impl.getKey() : impl.getValue(); }

            virtual Slice
            slice()
            {
                assert(!select_key);
                return impl.getValue();
            }

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
