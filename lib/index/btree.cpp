#include "btree.hpp"

using namespace cdb;

namespace cdb {
    class BTree::BTreeNode
    {
        Slice _content;
        Length _record_length;
        bool _is_leaf;
    public:
        BTreeNode(Slice content, Length record_length, bool is_leaf);
    };

    struct LeafMark
    {
        BlockIndex prev;
        BlockIndex next;
        Length length;
    };
}

BTree::BTreeNode::BTreeNode(Slice content, Length record_length, bool is_leaf)
    : _content(content), _record_length(record_length), _is_leaf(is_leaf)
{ }

BTree::BTree(
        DriverAccesser *accesser,
        Length record_length,
        Length record_count,
        BlockIndex root_index
    )
    : _accesser(accesser),
      _record_length(record_length),
      _record_count(record_count),
      _root(new BTreeNode(
              accesser->access(root_index),
              record_count < maximumRecordPerLeaf() ? record_length : SIZEOF_INDEX,
              record_count < maximumRecordPerLeaf()
        ))
{ }

Length
BTree::maximumRecordPerLeaf() const
{ return (Driver::BLOCK_SIZE - sizeof(LeafMark)) / _record_length; }

void
BTree::reset()
{
    // TODO
}
