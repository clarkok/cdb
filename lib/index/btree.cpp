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
    };

    struct BTree::NodeMark
    {
        NodeHeader header;
        BlockIndex after;
    };

    struct BTree::LeafMark
    {
        NodeHeader header;
        BlockIndex prev;
        BlockIndex next;
    };
}

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
      _root_index(root_index),
      _root(accesser->aquire(root_index)),
      _key_size(key_size),
      _value_size(value_size)
{ }

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
    // TODO
}

BTree::Iterator
BTree::find(ConstSlice key)
{
    std::stack<Block> path;

    keepTracingToLeaf(key, path);

    return findInLeaf(path.top(), key);
}

void
BTree::keepTracingToLeaf(ConstSlice key, std::stack<Block> &path)
{
    assert(key.length() >= _key_size);
    path.push(_accesser->aquire(_root_index));

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
    auto *mark = getMarkFromNode(node);
    auto *entry_limit = node.begin() + mark->header.entry_count * nodeEntrySize();

    auto *entry = getFirstEntryInNode(node);

    /* Node Entries
     *
     * | key | index |
     *
     * all keys in block by `index` less than `key`
     */

    for (; entry < entry_limit; entry = nextEntryInNode(entry)) {
        if (_less(key.content(), getKeyFromNodeEntry(entry))) {
            return getIndexFromNodeEntry(entry);
        }
    }
    return mark->after;
}

BTree::NodeMark *
BTree::getMarkFromNode(Slice node)
{ return reinterpret_cast<NodeMark*>(node.content()); }

Byte *
BTree::getFirstEntryInNode(Slice node)
{ return node.content() + sizeof(NodeMark); }

Byte *
BTree::nextEntryInNode(Byte *entry)
{ return entry + nodeEntrySize(); }

Byte *
BTree::getKeyFromNodeEntry(Byte *entry)
{ return entry; }

BlockIndex
BTree::getIndexFromNodeEntry(Byte *entry)
{ return *reinterpret_cast<BlockIndex*>(entry + _key_size); }

BTree::Iterator
BTree::findInLeaf(Block &leaf, ConstSlice key)
{
    auto *mark = getMarkFromLeaf(leaf);
    auto *entry_limit = leaf.begin() + mark->header.entry_count * leafEntrySize();

    auto *entry = getFirstEntryInLeaf(leaf);

    for (; entry < entry_limit; entry = nextEntryInLeaf(entry)) {
        if (_equal(getKeyFromLeafEntry(entry), key.content())) {
            return Iterator(this, leaf, entry - leaf.begin());
        }
    }

    // TODO throw exception
    throw 1;
}

BTree::LeafMark *
BTree::getMarkFromLeaf(Slice leaf)
{ return reinterpret_cast<LeafMark*>(leaf.content()); }

Byte *
BTree::getFirstEntryInLeaf(Slice leaf)
{ return leaf.content() + sizeof(LeafMark); }

Byte *
BTree::nextEntryInLeaf(Byte *entry)
{ return entry + leafEntrySize(); }

Byte *
BTree::getKeyFromLeafEntry(Byte *entry)
{ return entry; }

Slice
BTree::getValueFromLeafEntry(Byte *entry)
{ return Slice(entry + _key_size, _value_size); }

BTree::Iterator
BTree::insert(ConstSlice key, ConstSlice value)
{
    assert(key.length() >= _key_size);
}

Slice
BTree::splitLeaf(Slice leaf, BlockIndex &new_index, ConstSlice key, ConstSlice value)
{
}
