#ifndef _DB_INDEX_BTREE_H_
#define _DB_INDEX_BTREE_H_

#include <gtest/gtest_prod.h>

#include <cassert>
#include <vector>
#include <stack>
#include <iterator>
#include <functional>

#include "lib/driver/driver-accesser.hpp"

namespace cdb {

    /**
     * BTree is actually a B+ tree.
     *
     * Each node of this B+ tree is a block in the disk. 
     *
     * There two kinds of node in this B+ tree, they are:
     *  - Leaf node
     *  - Non-leaf node
     *
     * Each kind of node has is own type of `Mark', but those tow kind of mark share one
     * single structure called `Header', which is located at the begining of a block. The
     * Header and both Marks is defined in `btree-intl.hpp'.
     *
     * The structure of a leaf node is as following:
     *     size     offset                      usage
     * +----------+  0
     * |    12    |                             General header, the size is 12 currently
     * +----------+  12
     * | key_size |                             0th key in this leaf
     * +----------+  12 + key_size
     * | val_size |                             0th value in this leaf
     * +----------+  12 + key_size + val_size
     * | key_size |                             1st key in this leaf
     * +----------+
     * | val_size |                             1st value in this leaf
     * +----------+
     *     ....
     *
     * Each leaf node contains (BLOCK_SIZE - sizeof(LeafMark)) / (key_size + value_size) 
     * records. Currently LeafMark only contains a Header, and linear searching is 
     * performed in the leaf node.
     *
     * The structure of a non-leaf node is as following:
     *     size     offset                      usage
     * +----------+  0
     * |    12    |                             General header, the size is 12 currnetly
     * +----------+  12 
     * |    4     |                             `Before' field in this non-leaf node
     * +----------+  16                         Until here is the total NodeMark
     * | key_size |                             0th key in this node
     * +----------+  16 + key_size
     * |    4     |                             0th block index in this node, 32bits int
     * +----------+  16 + key_size + 4
     * | key_size |                             1st key in this node
     * +----------+
     * |    4     |                             1st block index in thie node
     * +----------+
     *     ....
     *
     * Each non-leaf node contains (BLOCK_SIZE - sizeof(NodeMark)) / (key_size + 4) 
     * records. Compared to LeafMark, a `before' field is added to NodeMark. So all 
     * records with a key not less than nth key is stored in a subtree indexed by nth 
     * index. Currently linear searching is performed in the non-leaf node. However
     * all entries is kept increasingly 
     *
     * The root node can be either a leaf node or a non-leaf node. A BTree in disk is
     * identificated by the index of root, so user of BTree should notice the change of
     * root node.
     *
     * The root node is always in memory when a BTree is constructed.
     *
     * NOTE: all key in the BTree should be unique
     */
    class BTree
    {
    public:

        /**
         * Iterator used to pointing to record in a BTree.
         *
         * The Iterator object only exists in memory, and each object hold a strong 
         * reference to the disk block the record on. The time used to exec `begin()',
         * `end()', `next()' and `prev()', are all O(1). However, it is expensive to copy
         * a Block, so `next()' and `prev()' is not public, and can used only in 
         * `forEach()' and `forEachReverse()'.
         */
        class Iterator
        {
        private:
            BTree *_owner;
            Block _block;
            Length _offset;

            /**
             * Constructor can only be accessed in @class BTree
             */
            Iterator(BTree *owner, const Block &block, Length offset)
                : _owner(owner), _block(block), _offset(offset)
            { }

            // Copy constructor and copy assignment is disabled
            Iterator(const Iterator &) = delete;
            Iterator &operator = (const Iterator &) = delete;

            /**
             * get the entry this Iterator pointing to
             *
             * @return start byte of this entry
             */
            inline Byte *
            getEntry() const
            { return _block.slice().content() + _offset; }

            friend class BTree;
        public:
            // Only move constructor is public
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

        /** 
         * The Comparator is used in comparation when operating the BTree, less and 
         * equal is required 
         */
        typedef std::function<bool(const Byte *, const Byte *)> Comparator;

        /**
         * The Operator is used in operating records when invoking `forEach()' and 
         * `forEachReverse()'
         */
        typedef std::function<void(const Iterator &)> Operator;

    private:
        struct NodeHeader;
        struct NodeMark;
        struct LeafMark;

        struct Key
        {
            union {
                const Byte *pointer;
                long value;
            };
        };

        // used for std::lower_bound
        struct NodeEntryIterator : public std::iterator<std::random_access_iterator_tag, Key>
        {
            Byte *entry = nullptr;
            BTree *owner = nullptr;

            NodeEntryIterator() = default;
            ~NodeEntryIterator() = default;

            inline bool
            operator == (const NodeEntryIterator &i) const
            { return owner == i.owner && entry == i.entry; }

            inline bool
            operator != (const NodeEntryIterator &i) const
            { return !this->operator==(i); }

            inline bool
            operator < (const NodeEntryIterator &i) const
            { return entry < i.entry; }

            inline bool
            operator <= (const NodeEntryIterator &i) const
            { return entry <= i.entry; }

            inline bool
            operator > (const NodeEntryIterator &i) const
            { return entry > i.entry; }

            inline bool
            operator >= (const NodeEntryIterator &i) const
            { return entry >= i.entry; }

            inline Key operator* () const;

            inline NodeEntryIterator &operator +=(int);
            inline NodeEntryIterator &operator -=(int);

            inline NodeEntryIterator 
            operator ++(int)
            { 
                auto ret = *this;
                this->operator+=(1);
                return ret;
            };

            inline NodeEntryIterator &
            operator ++()
            { return this->operator+=(1); }

            inline NodeEntryIterator 
            operator --(int)
            {
                auto ret = *this;
                this->operator-=(1);
                return ret;
            }

            inline NodeEntryIterator &
            operator --()
            { return this->operator-=(1); }

            inline int operator -(const NodeEntryIterator &b) const;

            inline Key 
            operator[] (int offset) const
            {
                auto tmp = *this;
                return *(tmp += offset);
            }
        };

        typedef std::stack<Block> BlockStack;

        DriverAccesser *_accesser;
        Comparator _less;
        Comparator _equal;

        /** 
         * When on disk, `prev' and `next' field of the root node is used to store 
         * `_first_leaf' and `_last_leaf', but when in memory, both fields should be zero
         */
        Block _root;
        BlockIndex _first_leaf;
        BlockIndex _last_leaf;

        /**
         * Currently _key_size + _value_size should less than 
         * BLOCK_SIZE - sizeof(LeafMark)
         */
        Length _key_size;
        Length _value_size;

        /**
         * @return _key_size + 4
         */
        inline Length nodeEntrySize() const;

        /**
         * @retrn _key_size + _value_size
         */
        inline Length leafEntrySize() const;

        /**
         * To get maximum number of entries per node
         * 
         * @return maximum number
         */
        inline Length maximumEntryPerNode() const;

        /**
         * To get maximum number of enties per leaf
         *
         * @return maximum number
         */
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

        /**
         * Find the needed block index in a node
         *
         * Return index in an entry whose key should be less than or equal to @key, and 
         * whose next entry's key should be greater than @key. Or the `before' field in 
         * the header
         *
         * @param node the node to find in
         * @param key the key to find
         * @return found index
         */
        inline BlockIndex findInNode(Block &node, Key key);

        /**
         * Find the key in a leaf.
         *
         * If the key is found, then an Iterator pointing to it's entry is returned
         * If the key is not found, then an Iterator pointer to the first entry whose key
         * is greater than the @key is returned
         *
         * @param leaf the leaf node to find in
         * @param key the key to find
         * @parma found Iterator
         */
        inline Iterator   findInLeaf(Block &leaf, Key key);

        /**
         * Find the leaf from the root, keep trace the whole path
         * 
         * @param key the key to find
         * @param path [out] record all node from root to the leaf, FILO
         */
        inline void keepTracingToLeaf(Key key, BlockStack &path);

        /**
         * Split a leaf into two
         *
         * After the operation, all entries indexed less than split_offset is kept 
         * untouched in old_leaf, all entries indexed greater than or equal to split_key
         * is copied to a new leaf, which is to be returned.
         *
         * All fields in the header of old_leaf and the new leaf is updated.
         *
         * @param old_leaf old leaf to copy from
         * @param split_offset the position to split
         * @return a new leaf which is next to old_leaf
         * @see splitNode
         */
        inline Block splitLeaf(Block &old_leaf, Length split_offset);

        /**
         * Split a node into two
         *
         * Like splitLeaf, all entries indexed less than split_offset is kept untouched
         * in old_node, all entries index greater than or equal to split_key is copied to
         * a new node, which is to be returned.
         *
         * All fields in both headers is updated.
         *
         * @param old_node old node to copy from
         * @param split_offset the position to split
         * @return a new node which is next to old_node
         * @see splitLeaf
         */
        inline Block splitNode(Block &old_node, Length split_offset);

        /**
         * Insert a entry into a leaf, initialized with the key
         */
        inline Iterator insertInLeaf(Block &leaf, Key key);
        inline void     insertInNode(Block &node, Key key, BlockIndex index);

        inline Block newRoot(Key split_key, Block &before, Block &after);

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

        inline void eraseInLeaf(Block &leaf, Key key);
        inline void eraseInNode(Block &leaf, Key key);
        inline void mergeLeaf(Block &leaf, Block &next_leaf);
        inline void mergeNode(Block &node, Block &next_node);

        inline void updateKey(Block &node, Key new_key, BlockIndex index);

        inline void updateLinkBeforeFreeLeaf(Block &leaf);
        inline void updateLinkBeforeFreeNode(Block &node);

        inline const Byte* getPointerOfKey(Key &key);
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

        Iterator lowerBound(Key key);
        Iterator upperBound(Key key);
        Iterator insert(Key key);
        void erase(Key key);

        Iterator begin();
        Iterator end();

        inline void forEach(Operator op)
        { forEach(begin(), end(), op); }

        inline void forEachReverse(Operator op)
        { forEachReverse(begin(), end(), op); }

        void forEach(Iterator b, Iterator e, Operator op);
        void forEachReverse(Iterator b, Iterator e, Operator op);

        void reset();

        template<typename T>
        Key makeKey(const T *p)
        {
            assert(sizeof(T) >= _key_size);

            Key ret;

            if (_key_size > sizeof(Key::value)) {
                ret.pointer = reinterpret_cast<const Byte*>(p);
            }
            else {
                *reinterpret_cast<T*>(&ret.value) = *p;
            }

            return ret;
        }

        template<typename T>
        Key makeKey(const T *p, int length)
        {
            assert(sizeof(T) * length >= _key_size);

            Key ret;

            if (_key_size > sizeof(Key::value)) {
                ret.pointer = reinterpret_cast<const Byte*>(p);
            }
            else {
                std::copy(
                        p,
                        p + length,
                        reinterpret_cast<T*>(&ret.value)
                    );
            }

            return ret;
        }

        friend class BTreeTest;
    };

}

#endif // _DB_INDEX_BTREE_H_
