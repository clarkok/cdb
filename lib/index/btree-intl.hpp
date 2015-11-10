#ifndef _DB_INDEX_BTREE_INTL_H_
#define _DB_INDEX_BTREE_INTL_H_

#include "btree.hpp"

using namespace cdb;

namespace cdb {
    /**
     * Gerneral header for each node
     */
    struct BTree::NodeHeader
    {
        bool node_is_leaf : 1;
        unsigned int node_length : 7;   /** currently all node_length is 1 */
        unsigned int entry_count : 24;
        BlockIndex prev;                /** only first node in each level whose `prev' can be zero */
        BlockIndex next;                /** only last node in each level whose `next' can be zero */
    };

    /**
     * Mark for Node
     */
    struct BTree::NodeMark
    {
        NodeHeader header;
        BlockIndex before;  /** only first node in each level has a `before' */
    };

    struct BTree::LeafMark
    { NodeHeader header; };

    struct BTree::NodeEntryIterator : public std::iterator<std::random_access_iterator_tag, Key>
    {
        Slice::SliceIterator entry;
        BTree *owner = nullptr;

        NodeEntryIterator(Slice::SliceIterator entry, BTree *owner)
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

    struct BTree::LeafEntryIterator : public std::iterator<std::random_access_iterator_tag, Key>
    {
        Slice::SliceIterator entry;
        BTree *owner = nullptr;

        LeafEntryIterator(Slice::SliceIterator entry, BTree *owner)
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
}

BTree::Key
BTree::NodeEntryIterator::operator * () const
{
    return owner->makeKey(
            owner->getKeyFromNodeEntry(entry).start(),
            owner->_key_size
        );
}

BTree::NodeEntryIterator &
BTree::NodeEntryIterator::operator += (int offset)
{
    entry += owner->nodeEntrySize() * offset;
    return *this;
}

BTree::NodeEntryIterator &
BTree::NodeEntryIterator::operator -= (int offset)
{
    entry -= owner->nodeEntrySize() * offset;
    return *this;
}

int
BTree::NodeEntryIterator::operator - (const NodeEntryIterator &b) const
{ return (entry - b.entry) / owner->nodeEntrySize(); }

BTree::Key
BTree::LeafEntryIterator::operator * () const
{
    return owner->makeKey(
            owner->getKeyFromLeafEntry(entry).start(),
            owner->_key_size
        );
}

BTree::LeafEntryIterator &
BTree::LeafEntryIterator::operator += (int offset)
{
    entry += owner->leafEntrySize() * offset;
    return *this;
}

BTree::LeafEntryIterator &
BTree::LeafEntryIterator::operator -= (int offset)
{
    entry -= owner->leafEntrySize() * offset;
    return *this;
}

int
BTree::LeafEntryIterator::operator - (const LeafEntryIterator &b) const
{ return (entry - b.entry) / owner->leafEntrySize(); }

Length
BTree::maximumEntryPerNode() const
{ return (Driver::BLOCK_SIZE - sizeof(NodeMark)) / nodeEntrySize(); }

Length
BTree::nodeEntrySize() const
{ return _key_size + sizeof(BlockIndex); }

Length
BTree::maximumEntryPerLeaf() const
{ return (Driver::BLOCK_SIZE - sizeof(LeafMark)) / leafEntrySize(); }

Length
BTree::leafEntrySize() const
{ return _key_size + _value_size; }

BTree::NodeMark *
BTree::getMarkFromNode(Slice node)
{ return reinterpret_cast<NodeMark*>(node.content()); }

Slice::SliceIterator
BTree::getFirstEntryInNode(Block &node)
{ return node.begin() + sizeof(NodeMark); }

Slice::SliceIterator
BTree::getLastEntryInNode(Block &node)
{ return prevEntryInNode(getLimitEntryInNode(node)); }

Slice::SliceIterator
BTree::getLimitEntryInNode(Block &node)
{ return getFirstEntryInNode(node) + getHeaderFromNode(node)->entry_count * nodeEntrySize(); }

Slice::SliceIterator
BTree::nextEntryInNode(Slice::SliceIterator entry)
{ return entry + nodeEntrySize(); }

Slice::SliceIterator
BTree::prevEntryInNode(Slice::SliceIterator entry)
{ return entry - nodeEntrySize(); }

BlockIndex *
BTree::getIndexFromNodeEntry(Slice::SliceIterator entry)
{ return reinterpret_cast<BlockIndex*>((entry + _key_size).start()); }

BTree::LeafMark *
BTree::getMarkFromLeaf(Slice leaf)
{ return reinterpret_cast<LeafMark*>(leaf.content()); }

Slice::SliceIterator
BTree::getFirstEntryInLeaf(Block &leaf)
{ return leaf.begin() + sizeof(LeafMark); }

Slice::SliceIterator
BTree::getLimitEntryInLeaf(Block &leaf)
{ return getFirstEntryInLeaf(leaf) + getHeaderFromNode(leaf)->entry_count * leafEntrySize(); }

Slice::SliceIterator
BTree::nextEntryInLeaf(Slice::SliceIterator entry)
{ return entry + leafEntrySize(); }

Slice::SliceIterator
BTree::prevEntryInLeaf(Slice::SliceIterator entry)
{ return entry - leafEntrySize(); }

BTree::NodeHeader *
BTree::getHeaderFromNode(Slice node)
{ return reinterpret_cast<NodeHeader*>(node.content()); }

Length
BTree::getFirstEntryOffset()
{ return sizeof(LeafMark); }

Length
BTree::getLimitEntryOffset(Block &leaf)
{ 
    auto *header = getHeaderFromNode(leaf);
    return getFirstEntryOffset() + header->entry_count * leafEntrySize();
}

Slice::SliceIterator
BTree::getEntryInLeafByIndex(Block &leaf, Length index)
{ return getFirstEntryInLeaf(leaf) + index * leafEntrySize(); }

Slice::SliceIterator
BTree::getEntryInNodeByIndex(Block &node, Length index)
{ return getFirstEntryInNode(node) + index * nodeEntrySize(); }

BlockIndex
BTree::findInNode(Block &node, Key key)
{
    assert(getHeaderFromNode(node)->prev ^ getMarkFromNode(node)->before);

    auto entry_limit = getLimitEntryInNode(node);
    auto entry = getFirstEntryInNode(node);

    if (_less(getPointerOfKey(key), getKeyFromNodeEntry(entry).start())) {
        return getMarkFromNode(node)->before;
    }

    auto iter = std::upper_bound(
            NodeEntryIterator(entry, this),
            NodeEntryIterator(entry_limit, this),
            key,
            [&] (const Key &a, const Key &b) {
                return _less(getPointerOfKey(a), getPointerOfKey(b));
            }
        );

    --iter;

    return *getIndexFromNodeEntry(iter.entry);
}

BTree::Iterator
BTree::findInLeaf(Block &leaf, Key key)
{
    auto entry_limit = getLimitEntryInLeaf(leaf);
    auto entry = getFirstEntryInLeaf(leaf);

    auto iter = std::lower_bound(
            LeafEntryIterator(entry, this),
            LeafEntryIterator(entry_limit, this),
            key,
            [&] (const Key &a, const Key &b) {
                return _less(getPointerOfKey(a), getPointerOfKey(b));
            }
        );

    if (iter.entry == entry_limit) {
        if (getHeaderFromNode(leaf)->next) {
            return Iterator(
                    this,
                    _accesser->aquire(getHeaderFromNode(leaf)->next),
                    getFirstEntryOffset()
                );
        }
        else {
            return end();
        }
    }
    else {
        return Iterator(this, leaf, iter.entry - leaf.begin());
    }
}

void
BTree::keepTracingToLeaf(Key key, BlockStack &path)
{
    path.push(_root);

    auto *header = getHeaderFromNode(path.top());

    assert(path.size());    // make path.size() contained in object

    while (!header->node_is_leaf) {
        path.push(_accesser->aquire(findInNode(path.top(), key)));
        header = getHeaderFromNode(path.top());
    }
}

Block
BTree::splitLeaf(Block &old_leaf, Length split_offset)
{
    // TODO to handle large record
    Block new_leaf              = _accesser->aquire(_accesser->allocateBlock(old_leaf.index()));
    auto *new_header            = getHeaderFromNode(new_leaf);
    auto *old_header            = getHeaderFromNode(old_leaf);

    std::copy(
            getEntryInLeafByIndex(old_leaf, split_offset),
            getLimitEntryInLeaf(old_leaf),
            getFirstEntryInLeaf(new_leaf)
        );

    new_header->node_is_leaf    = true;
    new_header->node_length     = old_header->node_length;
    new_header->entry_count     = old_header->entry_count - split_offset;
    new_header->prev            = old_leaf.index();
    new_header->next            = old_header->next;

    old_header->entry_count    -= new_header->entry_count;
    old_header->next            = new_leaf.index();

    if (new_header->next == 0) {
        _last_leaf = new_leaf.index();
    }
    else {
        Block new_next_leaf = _accesser->aquire(new_header->next);
        auto *new_next_header = getHeaderFromNode(new_next_leaf);
        new_next_header->prev = new_leaf.index();
    }

    return new_leaf;
}

Block
BTree::splitNode(Block &old_node, Length split_offset)
{
    assert(split_offset);

    Block new_node = _accesser->aquire(_accesser->allocateBlock(old_node.index()));
    auto *new_header = getHeaderFromNode(new_node);
    auto *old_header = getHeaderFromNode(old_node);

    std::copy(
            getEntryInNodeByIndex(old_node, split_offset),
            getLimitEntryInNode(old_node),
            getFirstEntryInNode(new_node)
        );

    new_header->node_is_leaf    = false;
    new_header->node_length     = old_header->node_length;
    new_header->entry_count     = old_header->entry_count - split_offset;
    new_header->prev            = old_node.index();
    new_header->next            = old_header->next;

    // this should never be used
    getMarkFromNode(new_node)->before   = 0;

    old_header->entry_count    -= new_header->entry_count;
    old_header->next            = new_node.index();

    assert(new_header->entry_count);
    assert(old_header->entry_count);

    if (new_header->next) {
        Block new_next_node = _accesser->aquire(new_header->next);
        auto *new_next_header = getHeaderFromNode(new_next_node);
        new_next_header->prev = new_node.index();
    }

    return new_node;
}

BTree::Iterator
BTree::insertInLeaf(Block &leaf, Key key)
{
    auto *header = getHeaderFromNode(leaf);
    auto current = getLimitEntryInLeaf(leaf);
    auto insert_point = current;
    auto entry_start = getFirstEntryInLeaf(leaf);

    if (current > entry_start) {
        for (
                current = prevEntryInLeaf(current); 
                current >= entry_start;
                    insert_point = current,
                    current = prevEntryInLeaf(current)
        ) {
            if (_less(getKeyFromLeafEntry(current).start(), getPointerOfKey(key))) {
                break;
            }
        }

        if (
                current >= entry_start && 
                _equal(getKeyFromLeafEntry(current).start(), getPointerOfKey(key))
        ) {
            return Iterator(this, leaf, current - leaf.begin());
        }
    }
    else {
        insert_point = current;
    }

    std::copy_backward(
            insert_point,
            getLimitEntryInLeaf(leaf),
            nextEntryInLeaf(getLimitEntryInLeaf(leaf))
        );

    ++header->entry_count;

    std::copy(
            getPointerOfKey(key),
            getPointerOfKey(key) + _key_size,
            getKeyFromLeafEntry(insert_point)
        );

    return Iterator(this, leaf, insert_point - leaf.begin());
}

void
BTree::insertInNode(Block &node, Key key, BlockIndex index)
{
    auto *header = getHeaderFromNode(node);
    auto current = getLimitEntryInNode(node);
    auto insert_point = current;
    auto entry_start = getFirstEntryInNode(node);

    for (
            current = prevEntryInNode(current);
            current >= entry_start;
                insert_point = current,
                current = prevEntryInNode(current)
    ) {
        if (_less(getKeyFromNodeEntry(current).start(), getPointerOfKey(key))) {
            break;
        }
    }

    std::copy_backward(
            insert_point,
            getLimitEntryInNode(node),
            nextEntryInNode(getLimitEntryInNode(node))
        );

    std::copy(
            getPointerOfKey(key),
            getPointerOfKey(key) + _key_size,
            getKeyFromNodeEntry(insert_point)
        );

    *getIndexFromNodeEntry(insert_point) = index;

    ++header->entry_count;
}

Block
BTree::newRoot(Key split_key, Block &before, Block &after)
{
    Block ret = _accesser->aquire(_accesser->allocateBlock(_root.index()));
    auto *mark = getMarkFromNode(ret);

    mark->header.next = mark->header.prev = 0;
    mark->header.entry_count = 1;
    mark->header.node_length = 1;
    mark->header.node_is_leaf = false;

    mark->before = before.index();

    auto entry = getFirstEntryInNode(ret);

    std::copy(
            getPointerOfKey(split_key),
            getPointerOfKey(split_key) + _key_size,
            getKeyFromNodeEntry(entry)
        );

    *getIndexFromNodeEntry(entry) = after.index();

    return ret;
}

BTree::Iterator
BTree::nextIterator(Iterator iter)
{
    auto limit_offset = getLimitEntryOffset(iter._block);

    if ((iter._offset += leafEntrySize()) >= limit_offset) {
        // already on end()
        if (iter._block.index() == _last_leaf) {
            return iter;
        }

        auto *header = getHeaderFromNode(iter._block);
        iter._block = _accesser->aquire(header->next);
        iter._offset = getFirstEntryOffset();
    }

    return iter;
}

BTree::Iterator
BTree::prevIterator(Iterator iter)
{
    auto start_offset = getFirstEntryOffset();

    // already on begin()
    if (iter._block.index() == _first_leaf && iter._offset == start_offset) {
        return end();
    }

    if ((iter._offset -= leafEntrySize()) < start_offset) {
        auto *header = getHeaderFromNode(iter._block);
        iter._block = _accesser->aquire(header->prev);
        iter._offset = getLimitEntryOffset(iter._block) - leafEntrySize();
    }

    return iter;
}

void
BTree::eraseInLeaf(Block &leaf, Key key)
{
    auto entry_limit = getLimitEntryInLeaf(leaf);
    auto entry = getFirstEntryInLeaf(leaf);

    for (;
            entry < entry_limit;
            entry = nextEntryInLeaf(entry)
    ) {
        if (!_less(getKeyFromLeafEntry(entry).start(), getPointerOfKey(key))) {
            break;
        }
    }

    if (!_equal(getKeyFromLeafEntry(entry).start(), getPointerOfKey(key))) {
        // key not found
        return;
    }

    auto *header = getHeaderFromNode(leaf);
    header->entry_count--;

    std::copy(
            nextEntryInLeaf(entry),
            entry_limit,
            entry
        );
}

void
BTree::eraseInNode(Block &node, Key key)
{
    auto entry_limit = getLimitEntryInNode(node);
    auto entry = getFirstEntryInNode(node);

    for (;
            entry < entry_limit;
            entry = nextEntryInNode(entry)
    ) {
        if (!_less(getKeyFromNodeEntry(entry).start(), getPointerOfKey(key))) {
            break;
        }
    }

    auto *header = getHeaderFromNode(node);

    assert(
            _equal(getKeyFromNodeEntry(entry).start(), getPointerOfKey(key)) ||
            header->prev == 0
        );

    if (!_equal(getKeyFromNodeEntry(entry).start(), getPointerOfKey(key)) && header->prev == 0) {
        // remove before
        assert(entry == getFirstEntryInNode(node));
        getMarkFromNode(node)->before = *getIndexFromNodeEntry(entry);
    }

    header->entry_count--;

    std::copy(
            nextEntryInNode(entry),
            entry_limit,
            entry
        );
}

void
BTree::mergeLeaf(Block &leaf, Block &next_leaf)
{
    auto *header = getHeaderFromNode(leaf);
    auto *next_header = getHeaderFromNode(next_leaf);

    std::copy(
            getFirstEntryInLeaf(next_leaf),
            getLimitEntryInLeaf(next_leaf),
            getLimitEntryInLeaf(leaf)
        );

    header->entry_count += next_header->entry_count;
    header->next = next_header->next;
    if (header->next) {
        Block new_next_leaf = _accesser->aquire(header->next);
        getHeaderFromNode(new_next_leaf)->prev = leaf.index();
    }
    else {
        _last_leaf = leaf.index();
    }
}

void
BTree::mergeNode(Block &node, Block &next_node)
{
    auto *header = getHeaderFromNode(node);
    auto *next_header = getHeaderFromNode(next_node);

    std::copy(
            getFirstEntryInNode(next_node),
            getLimitEntryInNode(next_node),
            getLimitEntryInNode(node)
        );

    header->entry_count += next_header->entry_count;
    header->next = next_header->next;
    if (header->next) {
        Block new_next_node = _accesser->aquire(header->next);
        getHeaderFromNode(new_next_node)->prev = node.index();
    }
}

void
BTree::updateKey(Block &node, Key new_key, BlockIndex index)
{
    auto entry_limit = getLimitEntryInNode(node);
    for (
            auto entry = getFirstEntryInNode(node);
            entry < entry_limit;
            entry = nextEntryInNode(entry)
    ) {
        if (index == *getIndexFromNodeEntry(entry)) {
            std::copy(
                    getPointerOfKey(new_key),
                    getPointerOfKey(new_key) + _key_size,
                    getKeyFromNodeEntry(entry)
                );
            break;
        }
    }
}

void
BTree::updateLinkBeforeFreeLeaf(Block &leaf)
{
    auto *header = getHeaderFromNode(leaf);
    assert(header->prev);
    if (header->prev) {
        Block prev_leaf = _accesser->aquire(header->prev);
        getHeaderFromNode(prev_leaf)->next = header->next;
    }

    if (header->next) {
        Block next_leaf = _accesser->aquire(header->next);
        getHeaderFromNode(next_leaf)->prev = header->prev;
    }
    else {
        _last_leaf = header->prev;
    }
}

void
BTree::updateLinkBeforeFreeNode(Block &node)
{
    auto *header = getHeaderFromNode(node);
    assert(header->prev);
    if (header->prev) {
        Block prev_node = _accesser->aquire(header->prev);
        getHeaderFromNode(prev_node)->next = header->next;
    }
    if (header->next) {
        Block next_node = _accesser->aquire(header->next);
        getHeaderFromNode(next_node)->prev = header->prev;
    }
}

const Byte *
BTree::getPointerOfKey(const Key &key)
{
    if (sizeof(Key::value) < _key_size) {
        return key.pointer;
    }
    else {
        return reinterpret_cast<const Byte*>(&key.value);
    }
}

#endif // _DB_INDEX_BTREE_INTL_H_
