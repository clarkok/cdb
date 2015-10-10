#ifndef _DB_INDEX_BTREE_H_
#define _DB_INDEX_BTREE_H_

#include <iterator>

#include "lib/driver/driver-accesser.hpp"

namespace cdb {

    class BTree
    {
    public:
        struct iterator : public std::iterator<std::bidirectional_iterator_tag, Slice>
        { };

        struct const_iterator : public std::iterator<std::bidirectional_iterator_tag, ConstSlice>
        { };

    private:
        class BTreeNode;

        static const auto SIZEOF_INDEX = sizeof(BlockIndex);

        DriverAccesser *_accesser;
        Length _record_length;
        Length _record_count;
        std::unique_ptr<BTreeNode> _root;

        Length maximumRecordPerLeaf() const;

    public:
        BTree(
                DriverAccesser *accesser,
                Length record_length,
                Length record_count,
                BlockIndex root_index
            );
        ~BTree();

        void reset();
    };

}

#endif // _DB_INDEX_BTREE_H_
