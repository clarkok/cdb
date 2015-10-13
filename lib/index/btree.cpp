#include <cassert>
#include <stack>

#include "btree.hpp"

using namespace cdb;

namespace cdb {
    struct BTree::NodeHeader
    {
        bool node_is_leaf : 1;
        unsigned int node_length : 15;
        unsigned int entry_count : 16;
        BlockIndex prev;
        BlockIndex next;
        BlockIndex parent;
    };

    struct BTree::NodeMark
    {
        NodeHeader header;
        BlockIndex before;
    };

    struct BTree::LeafMark
    {
        NodeHeader header;
    };
}

BTree::BTree(
        DriverAccesser *accesser,
        Comparator less,
        BlockIndex root_index,
        Length key_size,
        Length value_size
    )
    : _accesser(accesser),
      _less(less),
      _root(accesser->aquire(root_index)),
      _key_size(key_size),
      _value_size(value_size)
{
    // replace first & last leaf
    auto *header = getHeaderFromNode(_root);
    _first_leaf = header->prev;
    _last_leaf = header->next;

    header->prev = header->next = 0;
}

BTree::~BTree()
{
    // restore first & last leaf
    auto *header = getHeaderFromNode(_root);
    header->prev = _first_leaf;
    header->next = _last_leaf;
}

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

void
BTree::reset()
{
    // TODO release all other blocks

    auto *header = getHeaderFromNode(_root);
    *header = {true, 1, 0, 0, 0, 0};

    _first_leaf = _last_leaf = _root.index();
}

BTree::Iterator
BTree::lowerBound(ConstSlice key)
{
    assert(key.length() >= _key_size);
    std::stack<Block> path;
    leafLowerBound(key, path);
    return lowerBoundInLeaf(path.top(), key);
}

BTree::Iterator
BTree::upperBound(ConstSlice key)
{
    assert(key.length() >= _key_size);
    std::stack<Block> path;
    leafUpperBound(key, path);
    return nextIterator(upperBoundInLeaf(path.top(), key));
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
BTree::leafLowerBound(ConstSlice key, std::stack<Block> &path)
{
    BlockIndex parent = 0;
    path.push(_root);

    auto *header = getHeaderFromNode(path.top());

    while (!header->node_is_leaf) {
        header->parent = parent;
        path.push(_accesser->aquire(lowerBoundInNode(path.top(), key)));
        header = getHeaderFromNode(path.top());
        parent = path.top().index();
    }
}

void
BTree::leafUpperBound(ConstSlice key, std::stack<Block> &path)
{
    BlockIndex parent = 0;
    path.push(_root);

    auto *header = getHeaderFromNode(path.top());

    while (!header->node_is_leaf) {
        header->parent = parent;
        path.push(_accesser->aquire(upperBoundInNode(path.top(), key)));
        header = getHeaderFromNode(path.top());
        parent = path.top().index();
    }
}

BTree::NodeHeader *
BTree::getHeaderFromNode(Slice node)
{ return reinterpret_cast<NodeHeader*>(node.content()); }

BlockIndex
BTree::lowerBoundInNode(Block &node, ConstSlice key)
{
    auto *entry_limit = getLimitEntryInNode(node);
    auto *entry = getFirstEntryInNode(node);

    auto ret = getMarkFromNode(node)->before;

    for (; entry < entry_limit; entry = nextEntryInNode(entry)) {
        if (_less(key.content(), getKeyFromNodeEntry(entry))) {
            return ret;
        }
        ret = *getIndexFromNodeEntry(entry);
    }

    return ret;
}

BlockIndex
BTree::upperBoundInNode(Block &node, ConstSlice key)
{
    auto *entry_start = getFirstEntryInNode(node);

    for (
            auto *entry = prevEntryInNode(getLimitEntryInNode(node));
            entry >= entry_start;
            entry = prevEntryInNode(entry)
    ) {
        if (!_less(key.content(), getKeyFromNodeEntry(entry))) {
            return *getIndexFromNodeEntry(entry);
        }
    }

    return getMarkFromNode(node)->before;
}

BTree::NodeMark *
BTree::getMarkFromNode(Slice node)
{ return reinterpret_cast<NodeMark*>(node.content()); }

Byte *
BTree::getFirstEntryInNode(Block &node)
{ return node.content() + sizeof(NodeMark); }

Byte *
BTree::getLimitEntryInNode(Block &node)
{ return getFirstEntryInNode(node) + getHeaderFromNode(node)->entry_count * nodeEntrySize(); }

Byte *
BTree::nextEntryInNode(Byte *entry)
{ return entry + nodeEntrySize(); }

Byte *
BTree::prevEntryInNode(Byte *entry)
{ return entry - nodeEntrySize(); }

BlockIndex *
BTree::getIndexFromNodeEntry(Byte *entry)
{ return reinterpret_cast<BlockIndex*>(entry + _key_size); }

BTree::Iterator
BTree::lowerBoundInLeaf(Block &leaf, ConstSlice key)
{
    // to check if there are some of entry same with key in prev leaf
    if (leaf.index() != _first_leaf) {
        auto prev_leaf = _accesser->aquire(getHeaderFromNode(leaf)->prev);
        auto *last_entry = prevEntryInLeaf(getLimitEntryInLeaf(prev_leaf));
        if (!_less(getKeyFromLeafEntry(last_entry), key.content())) {
            auto *entry = prevEntryInLeaf(last_entry);
            while (!_less(getKeyFromLeafEntry(entry), key.content())) {
                last_entry = entry;
                entry = prevEntryInLeaf(entry);
            }
            auto offset = last_entry - prev_leaf.begin();
            return Iterator(this, std::move(prev_leaf), offset);
        }
    }

    auto *entry_limit = getLimitEntryInLeaf(leaf);
    auto *entry = getFirstEntryInLeaf(leaf);

    for (; entry < entry_limit; entry = nextEntryInLeaf(entry)) {
        if (!_less(getKeyFromLeafEntry(entry), key.content())) {
            return Iterator(this, leaf, entry - leaf.begin());
        }
    }

    return Iterator(this, _accesser->aquire(getHeaderFromNode(leaf)->next), getFirstEntryOffset());
}

BTree::Iterator
BTree::upperBoundInLeaf(Block &leaf, ConstSlice key)
{
    auto *entry_start = getFirstEntryInLeaf(leaf);

    for (
            auto *entry = prevEntryInLeaf(getLimitEntryInLeaf(leaf));
            entry >= entry_start;
            entry = prevEntryInLeaf(entry)
    ) {
        if (!_less(key.content(), getKeyFromLeafEntry(entry))) {
            return Iterator(this, leaf, entry - leaf.begin());
        }
    }

    return Iterator(this, leaf, getFirstEntryOffset());
}

BTree::LeafMark *
BTree::getMarkFromLeaf(Slice leaf)
{ return reinterpret_cast<LeafMark*>(leaf.content()); }

Byte *
BTree::getFirstEntryInLeaf(Block &leaf)
{ return leaf.content() + sizeof(LeafMark); }

Byte *
BTree::getLimitEntryInLeaf(Block &leaf)
{ return getFirstEntryInLeaf(leaf) + getHeaderFromNode(leaf)->entry_count * leafEntrySize(); }

Byte *
BTree::nextEntryInLeaf(Byte *entry)
{ return entry + leafEntrySize(); }

Byte *
BTree::prevEntryInLeaf(Byte *entry)
{ return entry - leafEntrySize(); }

BTree::Iterator
BTree::insert(ConstSlice key)
{
    assert(key.length() >= _key_size);

    std::stack<Block> path;
    leafLowerBound(key, path);

    if (getHeaderFromNode(path.top())->entry_count >= maximumEntryPerLeaf()) {
        Iterator ret = end();

        Length split_offset = getHeaderFromNode(path.top())->entry_count / 2;
        Block new_node = splitLeaf(path.top(), split_offset);

        ConstSlice split_key(
                getKeyFromLeafEntry(getFirstEntryInLeaf(new_node)),
                _key_size
            );

        if (maximumEntryPerLeaf() > 1 && _less(key.content(), split_key.content())) {
            ret = insertInLeaf(path.top(), key);
        }
        else {
            ret = insertInLeaf(new_node, key);
        }

        path.pop();

        while (!path.empty() && 
                getHeaderFromNode(path.top())->entry_count >= maximumEntryPerNode()) {

            auto new_index = new_node.index();
            split_offset = getHeaderFromNode(path.top())->entry_count / 2;
            new_node = splitNode(path.top(), split_offset);

            split_key = ConstSlice(
                    getKeyFromNodeEntry(getFirstEntryInNode(new_node)),
                    _key_size
                );

            if (_less(key.content(), split_key.content())) {
                insertInNode(path.top(), key, new_index);
            }
            else {
                insertInNode(new_node, key, new_index);
            }
            path.pop();
        }

        if (path.empty()) {
            _root = newRoot(split_key, _root, new_node);
        }
        else {
            insertInNode(path.top(), split_key, new_node.index());
        }

        return ret;
    }
    else {
        return insertInLeaf(path.top(), key);
    }
}

Byte *
BTree::getEntryInLeafByIndex(Block &leaf, Length index)
{ return getFirstEntryInLeaf(leaf) + index * leafEntrySize(); }

Byte *
BTree::getEntryInNodeByIndex(Block &node, Length index)
{ return getFirstEntryInNode(node) + index * nodeEntrySize(); }

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

    return new_leaf;
}

Block
BTree::splitNode(Block &old_node, Length split_offset)
{
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
    getMarkFromNode(new_node)->before   = old_node.index();

    old_header->entry_count    -= new_header->entry_count;
    old_header->prev            = new_node.index();

    return new_node;
}

BTree::Iterator
BTree::insertInLeaf(Block &leaf, ConstSlice key)
{
    auto *header = getHeaderFromNode(leaf);
    auto *current = getLimitEntryInLeaf(leaf);
    auto *insert_point = current;
    auto *entry_start = getFirstEntryInLeaf(leaf);

    for (
            current = prevEntryInLeaf(current); 
            current >= entry_start;
                insert_point = current,
                current = prevEntryInLeaf(current)
    ) {
        if (_less(getKeyFromLeafEntry(current), key.content())) {
            break;
        }
    }

    std::copy_backward(
            insert_point,
            getLimitEntryInLeaf(leaf),
            nextEntryInLeaf(getLimitEntryInLeaf(leaf))
        );

    std::copy(
            key.cbegin(),
            key.cend(),
            getKeyFromLeafEntry(insert_point)
        );

    ++header->entry_count;

    return Iterator(this, leaf, insert_point - leaf.begin());
}

void
BTree::insertInNode(Block &node, ConstSlice key, BlockIndex index)
{
    auto *header = getHeaderFromNode(node);
    auto *current = getLimitEntryInNode(node);
    auto *insert_point = current;
    auto *entry_start = getFirstEntryInNode(node);

    for (
            current = prevEntryInNode(current);
            current >= entry_start;
                insert_point = current,
                current = prevEntryInNode(current)
    ) {
        if (_less(getKeyFromNodeEntry(current), key.content())) {
            break;
        }
    }

    std::copy_backward(
            insert_point,
            getLimitEntryInNode(node),
            nextEntryInNode(getLimitEntryInNode(node))
        );

    std::copy(
            key.cbegin(),
            key.cend(),
            getKeyFromNodeEntry(insert_point)
        );

    *getIndexFromNodeEntry(insert_point) = index;

    ++header->entry_count;
}

Block
BTree::newRoot(ConstSlice split_key, Block &before, Block &after)
{
    Block ret = _accesser->aquire(_accesser->allocateBlock(_root.index()));
    auto *mark = getMarkFromNode(ret);

    mark->header.next = mark->header.prev = 0;
    mark->header.entry_count = 1;
    mark->header.node_length = 1;
    mark->header.node_is_leaf = false;

    mark->before = before.index();

    auto *entry = getFirstEntryInNode(ret);

    std::copy(
            split_key.cbegin(),
            split_key.cend(),
            getKeyFromNodeEntry(entry)
        );

    *getIndexFromNodeEntry(entry) = after.index();

    return ret;
}

BTree::Iterator
BTree::begin()
{
    return Iterator(this, _accesser->aquire(_first_leaf), getFirstEntryOffset());
}

BTree::Iterator
BTree::end()
{
    Block last_leaf = _accesser->aquire(_last_leaf);
    return Iterator(this, last_leaf, getLimitEntryOffset(last_leaf));
}

Length
BTree::getFirstEntryOffset()
{ return sizeof(LeafMark); }

Length
BTree::getLimitEntryOffset(Block &leaf)
{ 
    auto *header = getHeaderFromNode(leaf);
    return getFirstEntryOffset() + header->entry_count * leafEntrySize();
}

void
BTree::forEach(Iterator b, Iterator e, Operator op)
{
    while (b != e) {
        op(b);
        b = nextIterator(std::move(b));
    }
}
