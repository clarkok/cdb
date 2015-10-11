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
        Comparator _equal;

        BlockIndex _root_index;
        Block _root;

        Length _key_size;
        Length _value_size;

        inline Length nodeEntrySize() const;
        inline Length leafEntrySize() const;

        inline Length maximumEntryPerNode() const;
        inline Length maximumEntryPerLeaf() const;

        inline NodeMark *getMarkFromNode(Slice node);
        inline LeafMark *getMarkFromLeaf(Slice leaf);
        inline NodeHeader *getHeaderFromNode(Slice node);

        inline Byte *getFirstEntryInNode(Slice node);
        inline Byte *nextEntryInNode(Byte *entry);

        inline Byte *getFirstEntryInLeaf(Slice leaf);
        inline Byte *nextEntryInLeaf(Byte *entry);

        inline BlockIndex getIndexFromNodeEntry(Byte *entry);
        inline Slice      getValueFromLeafEntry(Byte *entry);

        inline Byte* getKeyFromNodeEntry(Byte *entry);
        inline Byte* getKeyFromLeafEntry(Byte *entry);

        inline BlockIndex findInNode(Block &node, ConstSlice key);
        inline Iterator   findInLeaf(Block &leaf, ConstSlice key);

        inline void keepTracingToLeaf(ConstSlice key, std::stack<Block> &path);

        /**
         * @return new key to split
         */
        inline Slice splitLeaf(Slice leaf, BlockIndex &new_index, ConstSlice key, ConstSlice value);
        inline Slice splitNode(Slice node, BlockIndex &new_index, ConstSlice key, BlockIndex index);

        inline void insertInLeaf(Slice leaf, ConstSlice key, ConstSlice value);
        inline void insertInNode(Slice node, ConstSlice key, BlockIndex index);

        inline BlockIndex newRoot(ConstSlice split_key, BlockIndex before, BlockIndex after);

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

        Iterator find(ConstSlice key);
        Iterator insert(ConstSlice key, ConstSlice value);

        void reset();
    };

}

#endif // _DB_INDEX_BTREE_H_
