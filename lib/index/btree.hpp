#ifndef _DB_INDEX_BTREE_H_
#define _DB_INDEX_BTREE_H_

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
     * records. Currently LeafMark only contains a Header, and binary searching is 
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
     * index. Currently bineay searching is performed in the non-leaf node, so all entries
     * is kept increasingly 
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

        /**
         * Target pointer for Key
         *
         * When _key_size is not greater than sizeof(Key::value), then the value of key 
         * is stored directly in `value'. Otherwise, the pointer to the actually key is
         * stored in `pointer'
         */
        struct Key
        {
            union {
                const Byte *pointer;
                long value;
            };
        };

        /**
         * Iterator used when performing binary search (currently using `std::upper_bound'
         * in `findInNode'
         *
         * NOTE: this iterator is not a fully implemented standerd random access iterator,
         * and it should not be used in other place.
         */
        struct NodeEntryIterator : public std::iterator<std::random_access_iterator_tag, Key>
        {
            Byte *entry = nullptr;
            BTree *owner = nullptr;

            NodeEntryIterator(Byte *entry, BTree *owner)
                : entry(entry), owner(owner)
            { }

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

            inline NodeEntryIterator 
            operator +(int n) const
            {
                auto ret = *this;
                return ret += n;
            }

            inline NodeEntryIterator
            operator -(int n) const
            {
                auto ret = *this;
                return ret -= n;
            }

            inline Key 
            operator[] (int offset) const
            {
                auto tmp = *this;
                return *(tmp += offset);
            }
        };

        /**
         * Iterator used when performing binary search (currently using `std::lower_bound'
         * in `findInLeaf'
         *
         * NOTE: this iterator is not a fully implemented standerd random access iterator,
         * and it should not be used in other place.
         */
        struct LeafEntryIterator : public std::iterator<std::random_access_iterator_tag, Key>
        {
            Byte *entry = nullptr;
            BTree *owner = nullptr;

            LeafEntryIterator(Byte *entry, BTree *owner)
                : entry(entry), owner(owner)
            { }

            inline bool
            operator == (const LeafEntryIterator &i) const
            { return owner == i.owner && entry == i.entry; }

            inline bool
            operator != (const LeafEntryIterator &i) const
            { return !this->operator==(i); }

            inline bool
            operator < (const LeafEntryIterator &i) const
            { return entry < i.entry; }

            inline bool
            operator <= (const LeafEntryIterator &i) const
            { return entry <= i.entry; }

            inline bool
            operator > (const LeafEntryIterator &i) const
            { return entry > i.entry; }

            inline bool
            operator >= (const LeafEntryIterator &i) const
            { return entry >= i.entry; }

            inline Key operator* () const;

            inline LeafEntryIterator &operator +=(int);
            inline LeafEntryIterator &operator -=(int);

            inline LeafEntryIterator 
            operator ++(int)
            { 
                auto ret = *this;
                this->operator+=(1);
                return ret;
            };

            inline LeafEntryIterator &
            operator ++()
            { return this->operator+=(1); }

            inline LeafEntryIterator 
            operator --(int)
            {
                auto ret = *this;
                this->operator-=(1);
                return ret;
            }

            inline LeafEntryIterator &
            operator --()
            { return this->operator-=(1); }

            inline int operator -(const LeafEntryIterator &b) const;

            inline LeafEntryIterator 
            operator +(int n) const
            {
                auto ret = *this;
                return ret += n;
            }

            inline LeafEntryIterator
            operator -(int n) const
            {
                auto ret = *this;
                return ret -= n;
            }

            inline Key 
            operator[] (int offset) const
            {
                auto tmp = *this;
                return *(tmp += offset);
            }
        };

        /** this type is used to keep path to a leaf when search. @see keepTracingToLeaf */
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
        inline NodeHeader *getHeaderFromNode(Slice node);   /** for both leaf and non-leaf */

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
         * Insert a entry into a leaf, initialize it with the key
         *
         * If a key already exists in the tree, then an Iterator pointing to that record
         * is returned instead of inserting a new record
         *
         * @param leaf the leaf to insert in
         * @param key the key to be inserted
         * @return an Iterator pointing to a new/old record whose key equal to `key'
         * @see insertInNode
         */
        inline Iterator insertInLeaf(Block &leaf, Key key);

        /**
         * Insert a entry into a node, initialize it with the key and it's index
         *
         * NOTE: the key should not exist in the original node
         *
         * @param node the node to insert in
         * @param key the key to be inserted
         * @param index the index to be inserted
         * @see insertInLeaf
         */
        inline void insertInNode(Block &node, Key key, BlockIndex index);

        /**
         * Construct a new root for the tree, with old root and the node after old root
         *
         * @param split_key the key split old root and the node after old root
         * @param before old root
         * @param after the node after old root
         * @return new root
         */
        inline Block newRoot(Key split_key, Block &before, Block &after);

        inline BlockIndex *getIndexFromNodeEntry(Byte *entry);

        inline Slice getValueFromLeafEntry(Byte *entry)
        { return Slice(entry + _key_size, _value_size); }

        inline Byte* getKeyFromNodeEntry(Byte *entry)
        { return entry; }

        inline Byte* getKeyFromLeafEntry(Byte *entry)
        { return entry; }

        inline Length getFirstEntryOffset();
        inline Length getLimitEntryOffset(Block &leaf);

        /**
         * Get the Iterator pointing to the record next to `iter'
         *
         * If the `iter' is pointing to the last record in the tree, then `end()' is 
         * returned. If `iter' is already equal to `end()' then `end()' is returned.
         *
         * @param iter the original Iterator
         * @return a new Iterator pointing to next record
         * @see prevIterator
         * @see begin
         * @see end
         */
        inline Iterator nextIterator(Iterator iter);

        /**
         * Get the Iterator pointing to the record previous to `iter'
         *
         * If the `iter' is pointing to the first record in the tree, then `end()' is
         * returned. If `iter' is equal to `end()', then a Iterator pointing to the last
         * record is returned
         *
         * @param iter the original Iterator
         * @return a new Iterator pointing to previous record
         * @see nextIterator
         * @see begin
         * @see end
         */
        inline Iterator prevIterator(Iterator iter);

        /**
         * Erase an entry with `key' in a leaf node
         *
         * If the required entry is not found, then this method do nothing
         *
         * @param leaf the leaf to erase in
         * @param key the key to be erased
         * @see eraseInNode
         */
        inline void eraseInLeaf(Block &leaf, Key key);

        /**
         * Erase an entry with `key' in a non-leaf node
         *
         * If the required entry is not found, then this method do nothing
         *
         * @param node the node to erase in
         * @param key the key to be erased
         * @see eraseInLeaf
         */
        inline void eraseInNode(Block &node, Key key);

        /**
         * Merge all entries in `next_leaf' to `leaf'
         *
         * @param leaf the leaf to contain all entries
         * @param next_leaf the leaf to move all entries from
         * @see mergeNode
         */
        inline void mergeLeaf(Block &leaf, Block &next_leaf);

        /**
         * Merge all entries in `next_node' to `node'
         *
         * @param node the node to contain all entires
         * @param next_node the node to move all entries from
         * @see mergeLeaf
         */
        inline void mergeNode(Block &node, Block &next_node);

        /**
         * Update key whose index specified with a new key
         *
         * @param node the node to update
         * @param new_key
         * @param index
         */
        inline void updateKey(Block &node, Key new_key, BlockIndex index);

        /**
         * Update all linking information in a leaf before free it
         *
         * This method will not free a leaf actually
         *
         * @param leaf the leaf to free
         * @see updateLinkBeforeFreeNode
         */
        inline void updateLinkBeforeFreeLeaf(Block &leaf);
        
        /**
         * Update all linking infomation in a node before free it
         *
         * This method will not free a leaf actually
         *
         * @param node the node to free
         * @see updateLinkBeforeFreeLeaf
         */
        inline void updateLinkBeforeFreeNode(Block &node);

        /**
         * Get a pointer to the content of a key
         *
         * If _key_size < sizeof(Key::value) then the pointer returned points to 
         * `key.value', otherwise the pointer returned is `key.pointer'
         *
         * @param key the key to retrive infomation from
         * @return the pointer pointing to the content of the key
         */
        inline const Byte* getPointerOfKey(const Key &key);

        /**
         * Clean a node and its subtree, free all blocks
         */
        void cleanNodeRecursive(Block &node);
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

        /**
         * Get index of root node
         *
         * This method should be called to get the latest infomation before flush upper 
         * structure.
         *
         * @return the index of root node
         */
        BlockIndex getRootIndex() const
        { return _root.index(); }

        /**
         * Find the lower bound of key
         *
         * An Iterator pointing to the first record whose key is not less than `key' is 
         * returned. If not found, `end()' is returned.
         *
         * @param key the key to find
         * @return an Iterator pointing to the result
         * @see upperBound
         */
        Iterator lowerBound(Key key);

        /**
         * Find the upper bound of key
         * 
         * An Iterator pointing to the first record whose key is greater than `key' is 
         * returned. If not found, `end()' is returned.
         *
         * @param key the key to find
         * @return an Iterator pointing to the result
         * @see lowerBound
         */
        Iterator upperBound(Key key);

        /**
         * Insert a key to the tree
         *
         * If the key is already existing in the tree, then an Iterator pointing to it
         * is returned directly, instead of inserting a new one.
         *
         * @param key the key to insert
         * @return an Iterator pointing to the record
         */
        Iterator insert(Key key);

        /**
         * Erase a key in the tree
         * 
         * If the key is not found, this method will do nothing.
         *
         * @param key the key to erase
         */
        void erase(Key key);

        /**
         * Return an Iterator pointing to the first record in the tree.
         *
         * If the tree is empty, `end()' is returned
         *
         * @return the Iterator
         * @see end
         */
        Iterator begin();

        /**
         * Return an Iterator pointing to the record after the last record in the tree
         *
         * @return the Iterator
         * @see begin
         */
        Iterator end();

        /**
         * Invoke `op' on every records in the tree, in increasingly order
         *
         * @param op the Operator used to visit records
         * @see forEach(Iterator _begin, Iterator last, Operator op)
         * @see forEachReverse(Operator op)
         * @see forEachReverse(Iterator first, Iterator last, Operator op)
         */
        inline void forEach(Operator op)
        { forEach(begin(), end(), op); }

        /**
         * Invoke `op' on every records in the tree, in reverse order
         *
         * @param op the Operator used to visit records
         * @see forEach(Operator op)
         * @see forEach(Iterator first, Iterator last, Operator op)
         * @see forEachReverse(Iterator first, Iterator last, Operator op)
         */
        inline void forEachReverse(Operator op)
        { forEachReverse(begin(), end(), op); }

        /**
         * Invoke `op' on every records between [first, last)
         *
         * @param first
         * @param last
         * @param op
         * @see forEach(Operator op)
         * @see forEachReverse(Operator op)
         * @see forEachReverse(Iterator first, Iterator last, Operator op)
         */
        void forEach(Iterator first, Iterator last, Operator op);

        /**
         * Invoke `op' on every records between [first, last) in reverse order
         * 
         * @param first
         * @param last
         * @param op
         * @see forEach(Operator op)
         * @see forEach(Iterator first, Iterator last, Operator op)
         * @see forEachReverse(Operator op)
         */
        void forEachReverse(Iterator first, Iterator last, Operator op);

        /**
         * Reset the whole tree
         */
        void reset();

        /**
         * Remove the whole tree
         */
        void clean();

        /**
         * Make a key from a pointer
         *
         * @param p pointer to the key object
         * @return a Key
         * @see makeKey(const T *p, int length)
         */
        template<typename T>
        Key makeKey(const T *p)
        {
            assert(sizeof(T) <= _key_size);

            Key ret;

            if (_key_size > sizeof(Key::value)) {
                ret.pointer = reinterpret_cast<const Byte*>(p);
            }
            else {
                *reinterpret_cast<T*>(&ret.value) = *p;
            }

            return ret;
        }

        /**
         * Make a key from a pointer, which pointing to an array
         *
         * @param p pointer to the key array
         * @param length the length of key array
         * @return a Key
         * @see makeKey(const T *p)
         */
        template<typename T>
        Key makeKey(const T *p, int length)
        {
            assert(sizeof(T) * length <= _key_size);

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

        // used to test
        friend class BTreeTest;
    };

}

#endif // _DB_INDEX_BTREE_H_
