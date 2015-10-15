#include <cassert>
#include <stack>

#include "btree.hpp"

using namespace cdb;

BTree::BTree(
        DriverAccesser *accesser,
        Comparator less,
        Comparator equal,
        BlockIndex root_index,
        Length key_size,
        Length value_size
    )
    : _accesser(accesser),
      _less(less),
      _equal(equal),
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
    *header = {
        true,   // node_is_leaf
        1,      // node_length
        0,      // entry_count
        0,      // prev
        0       // next
    };

    _first_leaf = _last_leaf = _root.index();
}

BTree::Iterator
BTree::lowerBound(ConstSlice key)
{
    assert(key.length() >= _key_size);
    std::stack<Block> path;
    keepTracingToLeaf(key, path);
    return findInLeaf(path.top(), key);
}

BTree::Iterator
BTree::upperBound(ConstSlice key)
{ 
    auto iter = lowerBound(key);
    if (_equal(iter.getKey(), key.content())) {
        return nextIterator(lowerBound(key));
    }
    else {
        return iter;
    }
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
BTree::keepTracingToLeaf(ConstSlice key, std::stack<Block> &path)
{
    path.push(_root);

    auto *header = getHeaderFromNode(path.top());

    while (!header->node_is_leaf) {
        path.push(_accesser->aquire(findInNode(path.top(), key)));
        header = getHeaderFromNode(path.top());
    }
}

BTree::NodeHeader *
BTree::getHeaderFromNode(Slice node)
{ return reinterpret_cast<NodeHeader*>(node.content()); }

BlockIndex
BTree::findInNode(Block &node, ConstSlice key)
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

    assert(getHeaderFromNode(node)->prev ^ getMarkFromNode(node)->before);
    assert(ret);

    return ret;
}

BTree::NodeMark *
BTree::getMarkFromNode(Slice node)
{ return reinterpret_cast<NodeMark*>(node.content()); }

Byte *
BTree::getFirstEntryInNode(Block &node)
{ return node.content() + sizeof(NodeMark); }

Byte *
BTree::getLastEntryInNode(Block &node)
{ return prevEntryInNode(getLimitEntryInNode(node)); }

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
BTree::findInLeaf(Block &leaf, ConstSlice key)
{
    auto *entry_limit = getLimitEntryInLeaf(leaf);
    auto *entry = getFirstEntryInLeaf(leaf);

    for (; entry < entry_limit; entry = nextEntryInLeaf(entry)) {
        if (!_less(getKeyFromLeafEntry(entry), key.content())) {
            return Iterator(this, leaf, entry - leaf.begin());
        }
    }

    if (leaf.index() != _last_leaf) {
        return Iterator(this, _accesser->aquire(getHeaderFromNode(leaf)->next), getFirstEntryOffset());
    }
    else {
        return end();
    }
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
    keepTracingToLeaf(key, path);

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

            Block node_to_insert = std::move(new_node);
            auto index_to_insert = node_to_insert.index();
            auto split_offset = getHeaderFromNode(path.top())->entry_count / 2;
            new_node = splitNode(path.top(), split_offset);

            if (_less(split_key.content(), getKeyFromNodeEntry(getFirstEntryInNode(new_node)))) {
                insertInNode(path.top(), split_key, index_to_insert);
            }
            else {
                insertInNode(new_node, split_key, index_to_insert);
            }

            split_key = ConstSlice(
                    getKeyFromNodeEntry(getFirstEntryInNode(new_node)),
                    _key_size
                );
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

    if (current >= entry_start && _equal(getKeyFromLeafEntry(current), key.content())) {
        return Iterator(this, leaf, current - leaf.begin());
    }

    ++header->entry_count;

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

void
BTree::forEachReverse(Iterator b, Iterator e, Operator op)
{
    while (b != e) {
        e = prevIterator(std::move(e));
        op(e);
    }
}

void
BTree::erase(ConstSlice key)
{
    assert(_first_leaf == 1);
    assert(key.length() >= _key_size);

    Buffer removal_key(key.cbegin(), key.cend());

    std::stack<Block> path;
    keepTracingToLeaf(key, path);

    Block leaf = std::move(path.top()); path.pop();
    auto *header = getHeaderFromNode(leaf);
    eraseInLeaf(leaf, key);

    if (leaf.index() == _root.index()) {
        return;
    }

    if (header->entry_count) {
        Block &parent = path.top();
        auto *parent_last_entry = getLastEntryInNode(parent);
        auto *parent_last_index = getIndexFromNodeEntry(parent_last_entry);

        updateKey(path.top(), getKeyFromLeafEntry(getFirstEntryInLeaf(leaf)), leaf.index());

        if (*parent_last_index == leaf.index()) {
            return;
        }

        Block next_leaf = _accesser->aquire(header->next);
        auto *next_header = getHeaderFromNode(next_leaf);

        if (next_header->entry_count + header->entry_count > maximumEntryPerLeaf()) {
            return;
        }

        std::copy(
                getKeyFromLeafEntry(getFirstEntryInLeaf(next_leaf)),
                getKeyFromLeafEntry(getFirstEntryInLeaf(next_leaf)) + _key_size,
                removal_key.begin()
            );

        mergeLeaf(leaf, next_leaf);
        _accesser->freeBlock(next_leaf.index());
    }
    else {
        updateLinkBeforeFreeLeaf(leaf);
        _accesser->freeBlock(leaf.index());
    }

    while (true) {
        Block node = std::move(path.top()); path.pop();
        auto *header = getHeaderFromNode(node);
        eraseInNode(node, removal_key);

        if (node.index() == _root.index()) {
            if (header->entry_count == 0) {
                auto prev_root_index = _root.index();
                _root = _accesser->aquire(getMarkFromNode(node)->before);
                _accesser->freeBlock(prev_root_index);
                return;
            }
            return;
        }

        if (header->entry_count) {
            Block &parent = path.top();
            auto *parent_last_entry = getLastEntryInNode(parent);
            auto *parent_last_index = getIndexFromNodeEntry(parent_last_entry);

            updateKey(parent, getKeyFromNodeEntry(getFirstEntryInNode(node)), node.index());

            if (*parent_last_index == node.index()) {
                return;
            }

            Block next_node = _accesser->aquire(header->next);
            auto *next_header = getHeaderFromNode(next_node);

            if (next_header->entry_count + header->entry_count > maximumEntryPerNode()) {
                return;
            }

            std::copy(
                    getKeyFromNodeEntry(getFirstEntryInNode(next_node)),
                    getKeyFromNodeEntry(getFirstEntryInNode(next_node)) + _key_size,
                    removal_key.content()
                );

            mergeNode(node, next_node);
            _accesser->freeBlock(next_node.index());
        }
        else {
            updateLinkBeforeFreeNode(node);
            _accesser->freeBlock(node.index());
        }
    }
}

void
BTree::eraseInLeaf(Block &leaf, ConstSlice key)
{
    auto *entry_limit = getLimitEntryInLeaf(leaf);
    auto entry = getFirstEntryInLeaf(leaf);

    for (;
            entry < entry_limit;
            entry = nextEntryInLeaf(entry)
    ) {
        if (!_less(getKeyFromLeafEntry(entry), key.content())) {
            break;
        }
    }

    if (!_equal(getKeyFromLeafEntry(entry), key.content())) {
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
BTree::eraseInNode(Block &node, ConstSlice key)
{
    auto *entry_limit = getLimitEntryInNode(node);
    auto entry = getFirstEntryInNode(node);

    for (;
            entry < entry_limit;
            entry = nextEntryInNode(entry)
    ) {
        if (!_less(getKeyFromNodeEntry(entry), key.content())) {
            break;
        }
    }

    auto *header = getHeaderFromNode(node);

    assert(
            _equal(getKeyFromNodeEntry(entry), key.content()) ||
            header->prev == 0
        );

    if (!_equal(getKeyFromNodeEntry(entry), key.content()) && header->prev == 0) {
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
BTree::updateKey(Block &node, Byte *new_key, BlockIndex index)
{
    auto entry_limit = getLimitEntryInNode(node);
    for (
            auto entry = getFirstEntryInNode(node);
            entry < entry_limit;
            entry = nextEntryInNode(entry)
    ) {
        if (index == *getIndexFromNodeEntry(entry)) {
            std::copy(
                    new_key,
                    new_key + _key_size,
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
