#ifndef _DB_INDEX_SKIPTABLE_H_
#define _DB_INDEX_SKIPTABLE_H_

#include <functional>
#include <iostream>

#include "lib/utils/buffer.hpp"
#include "lib/utils/slice.hpp"

namespace cdb {

    /**
     * SkipTable is actually a Skip List.
     *
     * Objects of this class will exists only in memory, and are used as temp tables.
     *
     * Time complexity of finding, inserting and erasing a single element will be 
     * O(logN). And for iterating, time complexity will be O(1).
     *
     * Values can be equal by Comparator in this SkipTable.
     *
     * Values is mutable in a SkipTable, but one should not modify the order of a value
     * in a SkipTable, or the behavior is undefined.
     */
    class SkipTable
    {
        struct Node;
        struct Leaf;
        struct NonLeaf;

    public:
        /**
         * Bidirection iterator, which dereference to a Slice pointing to the actual
         * record
         */
        struct Iterator
        { 
        private:
            SkipTable *_owner;
            Leaf *_ptr;
            mutable Slice _slice;   // prefetched when constructed

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

            inline
            operator bool () const
            { return _ptr; }

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
        typedef const Byte * Key;
        typedef std::function<bool(Key, Key)> Comparator;

    private:
        /**
         * Get a Iterator pointing to the next element this Iterator pointing to
         *
         * @param iter the Iterator to reference
         * @return a new Iterator
         * @see prevIterator
         */
        Iterator nextIterator(const Iterator &iter);
        /**
         * Get a Iterator pointing to the previous element this Iterator pointing to
         *
         * @param iter the Iterator to reference
         * @return a new Iterator
         * @see nextIterator
         */
        Iterator prevIterator(const Iterator &iter);

        int _key_offset;
        Comparator _less;

        Node *_root = nullptr;
        Length _size = 0;

        /**
         * Find lower bound of value, until it meets a leaf, or nullptr
         *
         * @param value the value to look up
         * @return the leaf if found, or nullptr
         * @see upperBoundInLeaf
         */
        inline Leaf* lowerBoundInLeaf(Key key);

        /**
         * Find upper bound of value, until it meets a leaf, or nullptr
         *
         * @param value the value to look up
         * @return the leaf if found, or nullptr
         * @see lowerBoundInLeaf
         */
        inline Leaf* upperBoundInLeaf(Key key);

        /**
         * Construct a leaf node
         *
         * @param parent the parent of this leaf
         * @param next the next leaf of this leaf
         * @param prev the previous leaf of this leaf
         * @param value the value of this leaf
         * @return the new constructed Leaf
         * @see newNonLeaf
         */
        inline Leaf*    newLeaf(NonLeaf *parent, Leaf *next, Leaf *prev, ConstSlice value) const;
        /**
         * Construct a non-leaf node
         *
         * @param parent the parent of this node
         * @param leaf the value-leaf of this node
         * @param next the next node of this node
         * @param prev the prev node of this node
         * @param child the child node of this node
         * @return ths new consturcted Node
         * @see newLeaf
         */
        inline NonLeaf* newNonLeaf(NonLeaf *parent, Key key, Node *next, Node *prev, Node *child) const;

        /**
         * Get the first leaf of this SkipTable
         *
         * @return the first leaf, or nullptr if this table is empty
         * @see last
         */
        inline Leaf* first() const;
        /**
         * Get the last leaf of this SkipTable
         *
         * @return the last leaf, or nullptr if this table is empty
         * @see first
         */
        inline Leaf* last() const;

        inline Key getKeyOfValue(ConstSlice value) const;

        // SkipTable not copiable
        SkipTable(const Iterator &) = delete;
        SkipTable &operator = (const Iterator &) = delete;

    public:
        SkipTable(int key_offset, Comparator less);
        ~SkipTable();

        inline Length
        size() const
        { return _size; }

        /**
         * Find the lower bound of the given value
         *
         * @param value the value to find
         * @return an Iterator pointing to the lower bound element
         * @see upperBound
         */
        Iterator lowerBound(Key key);

        /**
         * Find the upper bound of the given value
         *
         * @param value the value to find
         * @return an Iterator pointing to the upper bound element
         * @see lowerBound
         */
        Iterator upperBound(Key key);

        /**
         * Insert a value to this SkipTable
         *
         * @param value the value to insert
         * @return an Iterator pointing to the new inserted element
         */
        Iterator insert(ConstSlice value);

        /**
         * Erase a value from this SkipTable
         *
         * @param pos the Iteartor pointing to the element to be erased
         * @return an Iterator pointing to the next element in this SkipTable
         */
        Iterator erase(Iterator pos);

        /**
         * Get the first Iterator in this SkipTable
         *
         * @return the first Iterator
         * @see end
         */
        Iterator begin();

        /**
         * Get the Iterator after the last Iterator in this SkipTable
         *
         * The returned Iterator is invalid
         *
         * @return the Iterator after the last
         * @see begin
         */
        Iterator end();

        /**
         * Remove all the elements in this SkipTable
         */
        void clear();

        /**
         * For debug ONLY, output all the levels in this SkipTable
         *
         * @param os the output stream
         * @param print the printing function used to print a single value to the os
         * @return the os
         */
        std::ostream &__debug_output(std::ostream &os, std::function<void(std::ostream &, Key)> print);
    };
}

#endif // _DB_INDEX_SKIPTABLE_H_
