#include <cassert>
#include <vector>
#include <stack>

#include "btree-intl.hpp"

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

void
BTree::reset()
{
    clean();

    _root = _accesser->aquire(_accesser->allocateBlock());
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

void
BTree::clean()
{
    cleanNodeRecursive(_root);
    _root = _accesser->aquire(0);
}

BTree::Iterator
BTree::lowerBound(Key key)
{
    BlockStack path;
    keepTracingToLeaf(key, path);
    return findInLeaf(path.top(), key);
}

BTree::Iterator
BTree::upperBound(Key key)
{ 
    auto iter = lowerBound(key);
    if (_equal(iter.getKey(), getPointerOfKey(key))) {
        return nextIterator(lowerBound(key));
    }
    else {
        return iter;
    }
}

BTree::Iterator
BTree::insert(Key key)
{
    BlockStack path;
    keepTracingToLeaf(key, path);

    if (getHeaderFromNode(path.top())->entry_count >= maximumEntryPerLeaf()) {
        Iterator ret = end();

        Length split_offset = getHeaderFromNode(path.top())->entry_count / 2;
        Block new_node = splitLeaf(path.top(), split_offset);

        Key split_key = makeKey(
                getKeyFromLeafEntry(getFirstEntryInLeaf(new_node)),
                _key_size
            );

        if (maximumEntryPerLeaf() > 1 && 
             _less(getPointerOfKey(key), getPointerOfKey(split_key))
        ) {
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

            if (_less(getPointerOfKey(split_key), getKeyFromNodeEntry(getFirstEntryInNode(new_node)))) {
                insertInNode(path.top(), split_key, index_to_insert);
            }
            else {
                insertInNode(new_node, split_key, index_to_insert);
            }

            split_key = makeKey(
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

        return std::move(ret);
    }
    else {
        return insertInLeaf(path.top(), key);
    }
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
BTree::cleanNodeRecursive(Block &node)
{
    auto *header = getHeaderFromNode(node);
    if (header->node_is_leaf) {
        _accesser->freeBlock(node.index());
        return;
    }

    auto entry = getFirstEntryInNode(node);
    auto entry_limit = getLimitEntryInNode(node);

    for (; entry < entry_limit; entry = nextEntryInNode(entry)) {
        Block node = _accesser->aquire(*getIndexFromNodeEntry(entry));
        cleanNodeRecursive(node);
    }

    _accesser->freeBlock(node.index());
}

void
BTree::erase(Key key)
{
    Buffer removal_key_buffer(_key_size);
    Key removal_key(key);
    if (_key_size >= sizeof(Key::value)) {
        std::copy(
                key.pointer,
                key.pointer + _key_size,
                removal_key_buffer.content()
            );
        removal_key.pointer = removal_key_buffer.cbegin();
    }

    BlockStack path;
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

        updateKey(
                path.top(),
                makeKey(
                    getKeyFromLeafEntry(getFirstEntryInLeaf(leaf)),
                    _key_size
                ),
                leaf.index()
            );

        if (*parent_last_index == leaf.index()) {
            return;
        }

        Block next_leaf = _accesser->aquire(header->next);
        auto *next_header = getHeaderFromNode(next_leaf);

        if (next_header->entry_count + header->entry_count > maximumEntryPerLeaf()) {
            return;
        }

        if (_key_size >= sizeof(Key::value)) {
            std::copy(
                    getKeyFromLeafEntry(getFirstEntryInLeaf(next_leaf)),
                    getKeyFromLeafEntry(getFirstEntryInLeaf(next_leaf)) + _key_size,
                    removal_key_buffer.content()
                );
        }
        else {
            removal_key = makeKey(
                    getKeyFromLeafEntry(getFirstEntryInLeaf(next_leaf)),
                    _key_size
                );
        }

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

            updateKey(
                    parent,
                    makeKey(
                        getKeyFromNodeEntry(getFirstEntryInNode(node)),
                        _key_size
                    ), 
                    node.index()
                );

            if (*parent_last_index == node.index()) {
                return;
            }

            Block next_node = _accesser->aquire(header->next);
            auto *next_header = getHeaderFromNode(next_node);

            if (next_header->entry_count + header->entry_count > maximumEntryPerNode()) {
                return;
            }

            if (_key_size > sizeof(Key::value)) {
                std::copy(
                        getKeyFromNodeEntry(getFirstEntryInNode(next_node)),
                        getKeyFromNodeEntry(getFirstEntryInNode(next_node)) + _key_size,
                        removal_key_buffer.content()
                    );
            }
            else {
                removal_key = makeKey(
                        getKeyFromNodeEntry(getFirstEntryInNode(next_node)),
                        _key_size
                    );
            }

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
BTree::Iterator::next()
{ this->operator=(_owner->nextIterator(std::move(*this))); }

void
BTree::Iterator::prev()
{ this->operator=(_owner->prevIterator(std::move(*this))); }
