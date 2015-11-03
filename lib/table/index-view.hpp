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
            virtual ~IteratorImpl();

            virtual void next();
            virtual void prev();
            virtual Slice slice();
        };

        std::unique_ptr<BTree> _tree;
    public:
        IndexView(Schema *schema, BTree *tree)
                : View(schema), _tree(tree)
        { }

        virtual ~IndexView() = default;

        virtual View *peek(Schema::Column col, const Byte *lower_bound, const Byte *upper_bound);
        virtual View *intersect(Iterator a, Iterator b);
        virtual View *join(Iterator a, Iterator b);

        virtual Iterator begin();
        virtual Iterator end();
        virtual Iterator lowerBound(const Byte *key);
        virtual Iterator upperBound(const Byte *key);
    };
}

#endif //_DB_TABLE_INDEX_VIEW_H_
