#ifndef _DB_INDEX_BTREE_H_
#define _DB_INDEX_BTREE_H_

#include <stack>
#include <iterator>

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

            Iterator &inc();
            Iterator &dec();

            inline const Byte *
            getKey() const
            { return _owner->getKeyFromLeafEntry(getEntry()); }

            Slice getValue() const
            { return _owner->getValueFromLeafEntry(getEntry()); }
        };

        typedef bool (*Comparator)(const Byte *, const Byte *);

    private:
        struct NodeHeader;
        struct NodeMark;
        struct LeafMark;

        DriverAccesser *_accesser;
        Comparator _less;

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

        inline Byte *getFirstEntryInLeaf(Block &leaf);
        inline Byte *getLimitEntryInLeaf(Block &leaf);
        inline Byte *nextEntryInLeaf(Byte *entry);
        inline Byte *prevEntryInLeaf(Byte *entry);
        inline Byte *getEntryInLeafByIndex(Block &node, Length index);

        inline BlockIndex findInNode(Block &node, ConstSlice key);
        inline Iterator   findInLeaf(Block &leaf, ConstSlice key);

        inline void keepTracingToLeaf(ConstSlice key, std::stack<Block> &path);

        /**
         * @return new block
         */
        inline Block splitLeaf(Block &old_leaf, Length split_offset);
        inline Block splitNode(Block &old_node, Length split_offset);

        inline Iterator insertInLeaf(Block &leaf, ConstSlice key);
        inline void     insertInNode(Block &node, ConstSlice key, BlockIndex index);

        inline Block newRoot(ConstSlice split_key, BlockIndex before, BlockIndex after);

        inline BlockIndex *getIndexFromNodeEntry(Byte *entry);

        inline Slice      getValueFromLeafEntry(Byte *entry)
        { return Slice(entry + _key_size, _value_size); }

        inline Byte* getKeyFromNodeEntry(Byte *entry)
        { return entry; }

        inline Byte* getKeyFromLeafEntry(Byte *entry)
        { return entry; }

        inline Length getFirstEntryOffset();
        inline Length getLimitEntryOffset(Block &leaf);

    public:
        BTree(
                DriverAccesser *accesser,
                Comparator less,
                BlockIndex root_index,
                Length key_size,
                Length value_size
            );

        ~BTree();

        BlockIndex getRootIndex() const
        { return _root.index(); }

        Iterator find(ConstSlice key);
        Iterator insert(ConstSlice key);

        Iterator begin();
        Iterator end();

        void reset();
    };

}

#endif // _DB_INDEX_BTREE_H_
