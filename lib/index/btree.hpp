#ifndef _DB_INDEX_BTREE_H_
#define _DB_INDEX_BTREE_H_

#include <gtest/gtest_prod.h>

#include <stack>
#include <iterator>
#include <functional>

#include "lib/driver/driver-accesser.hpp"

namespace cdb {

    class BTree
    {
    public:
        class Iterator
        {
        private:
            BTree *_owner;
            Block _block;
            Length _offset;

            Iterator(BTree *owner, const Block &block, Length offset)
                : _owner(owner), _block(block), _offset(offset)
            { }

            Iterator(const Iterator &) = delete;
            Iterator &operator = (const Iterator &) = delete;

            inline Byte *
            getEntry() const
            { return _block.slice().content() + _offset; }

            friend class BTree;
        public:
            Iterator(Iterator &&iter)
                : _owner(iter._owner), _block(std::move(iter._block)), _offset(iter._offset)
            { }

            ~Iterator() = default;

            inline Iterator &
            operator = (Iterator &&iter)
            {
                _owner = iter._owner;
                _block = std::move(iter._block);
                _offset = iter._offset;

                return *this;
            }

            inline bool
            operator == (const Iterator &iter)
            { 
                return _owner == iter._owner &&
                     _block.index() == iter._block.index() &&
                     _offset == iter._offset;
            }

            inline bool
            operator != (const Iterator &iter)
            { return !this->operator==(iter); }

            inline const Byte *
            getKey() const
            { return _owner->getKeyFromLeafEntry(getEntry()); }

            Slice getValue() const
            { return _owner->getValueFromLeafEntry(getEntry()); }
        };

        typedef std::function<bool(const Byte *, const Byte *)> Comparator;
        typedef std::function<void(const Iterator &)> Operator;

    private:
        struct NodeHeader;
        struct NodeMark;
        struct LeafMark;

        DriverAccesser *_accesser;
        Comparator _less;
        Comparator _equal;

        // _root's before & end record to first child and last leaf
        Block _root;
        BlockIndex _first_leaf;
        BlockIndex _last_leaf;

        Length _key_size;
        Length _value_size;

        inline Length nodeEntrySize() const;
        inline Length leafEntrySize() const;

        inline Length maximumEntryPerNode() const;
        inline Length maximumEntryPerLeaf() const;

        inline NodeMark *getMarkFromNode(Slice node);
        inline LeafMark *getMarkFromLeaf(Slice leaf);
        inline NodeHeader *getHeaderFromNode(Slice node);

        inline Byte *getFirstEntryInNode(Block &node);
        inline Byte *getLimitEntryInNode(Block &node);
        inline Byte *nextEntryInNode(Byte *entry);
        inline Byte *prevEntryInNode(Byte *entry);
        inline Byte *getEntryInNodeByIndex(Block &node, Length index);
        inline Byte *getLastEntryInNode(Block &node);

        inline Byte *getFirstEntryInLeaf(Block &leaf);
        inline Byte *getLimitEntryInLeaf(Block &leaf);
        inline Byte *nextEntryInLeaf(Byte *entry);
        inline Byte *prevEntryInLeaf(Byte *entry);
        inline Byte *getEntryInLeafByIndex(Block &node, Length index);

        inline BlockIndex lowerBoundInNode(Block &node, ConstSlice key);
        inline Iterator   lowerBoundInLeaf(Block &leaf, ConstSlice key);

        inline BlockIndex upperBoundInNode(Block &node, ConstSlice key);
        inline Iterator   upperBoundInLeaf(Block &leaf, ConstSlice key);

        inline void leafLowerBound(ConstSlice key, std::stack<Block> &path);
        inline void leafUpperBound(ConstSlice key, std::stack<Block> &path);

        /**
         * @return new block
         */
        inline Block splitLeaf(Block &old_leaf, Length split_offset);
        inline Block splitNode(Block &old_node, Length split_offset);

        inline Iterator insertInLeaf(Block &leaf, ConstSlice key);
        inline void     insertInNode(Block &node, ConstSlice key, BlockIndex index);

        inline Block newRoot(ConstSlice split_key, Block &before, Block &after);

        inline BlockIndex *getIndexFromNodeEntry(Byte *entry);

        inline Slice      getValueFromLeafEntry(Byte *entry)
        { return Slice(entry + _key_size, _value_size); }

        inline Byte* getKeyFromNodeEntry(Byte *entry)
        { return entry; }

        inline Byte* getKeyFromLeafEntry(Byte *entry)
        { return entry; }

        inline Length getFirstEntryOffset();
        inline Length getLimitEntryOffset(Block &leaf);

        inline Iterator nextIterator(Iterator iter);
        inline Iterator prevIterator(Iterator iter);

        inline void eraseInLeaf(Block &leaf, ConstSlice key);
        inline void eraseInNode(Block &leaf, ConstSlice key);
        inline void mergeLeaf(Block &leaf, Block &next_leaf);
        inline void mergeNode(Block &node, Block &next_node);

        inline void updateKey(Block &node, Byte *new_key, BlockIndex index);

        inline void updateLinkBeforeFreeLeaf(Block &leaf);
        inline void updateLinkBeforeFreeNode(Block &node);
    public:
        BTree(
                DriverAccesser *accesser,
                Comparator less,
                Comparator equal,
                BlockIndex root_index,
                Length key_size,
                Length value_size
            );

        ~BTree();

        BlockIndex getRootIndex() const
        { return _root.index(); }

        Iterator lowerBound(ConstSlice key);
        Iterator upperBound(ConstSlice key);
        Iterator insert(ConstSlice key);
        void erase(ConstSlice key);

        Iterator begin();
        Iterator end();

        inline void forEach(Operator op)
        { forEach(begin(), end(), op); }

        inline void forEachReverse(Operator op)
        { forEachReverse(begin(), end(), op); }

        void forEach(Iterator b, Iterator e, Operator op);
        void forEachReverse(Iterator b, Iterator e, Operator op);

        void reset();

        FRIEND_TEST(BTreeTest, InternalTest);
        friend class BTreeTest;
    };

}

#endif // _DB_INDEX_BTREE_H_
