#ifndef _DB_INDEX_SKIPTABLE_H_
#define _DB_INDEX_SKIPTABLE_H_

#include <functional>
#include <iostream>

#include "lib/utils/buffer.hpp"
#include "lib/utils/slice.hpp"

namespace cdb {

    /**
     * Index in memory only
     */
    class SkipTable
    {
        struct Node;
        struct Leaf;
        struct NonLeaf;
        typedef std::function<bool(ConstSlice, ConstSlice)> Comparator;

    public:
        struct Iterator
        { 
            SkipTable *_owner;
            Leaf *_ptr;
            mutable Slice _slice;

            Iterator(SkipTable *owner, Leaf *ptr);
        public:
            Iterator(const Iterator &) = default;
            Iterator &operator = (const Iterator &) = default;
            ~Iterator() = default;

            inline bool
            operator == (const Iterator &b) const
            { return (b._ptr == _ptr && b._owner == _owner); }

            inline bool
            operator != (const Iterator &b) const
            { return !operator ==(b); }

            inline Iterator
            next() const
            { return _owner->nextIterator(*this); }

            inline Iterator
            prev() const
            { return _owner->prevIterator(*this); }

            inline Slice &
            operator *() const
            { return _slice; }

            inline Slice *
            operator ->() const
            { return &_slice; }

            friend class SkipTable;
        };

    private:
        Iterator nextIterator(const Iterator &iter);
        Iterator prevIterator(const Iterator &iter);

        Comparator _less;

        Node *_root = nullptr;

        inline Leaf* lowerBoundInLeaf(ConstSlice value);
        inline Leaf* upperBoundInLeaf(ConstSlice value);

        inline Leaf*    newLeaf(NonLeaf *parent, Leaf *next, Leaf *prev, ConstSlice value) const;
        inline NonLeaf* newNonLeaf(NonLeaf *parent, Leaf *leaf, Node *next, Node *prev, Node *child) const;

        inline Leaf* first() const;
        inline Leaf* last() const;

        SkipTable(const Iterator &) = delete;
        SkipTable &operator = (const Iterator &) = delete;

    public:
        SkipTable(Comparator less);
        ~SkipTable();

        Iterator lowerBound(ConstSlice value);
        Iterator upperBound(ConstSlice value);

        Iterator insert(ConstSlice value);

        Iterator begin();
        Iterator end();

        void clear();

        // for debug
        std::ostream &__debug_output(std::ostream &os);
    };
}

#endif // _DB_INDEX_SKIPTABLE_H_
