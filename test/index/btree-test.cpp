#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>

#include "lib/driver/bitmap-allocator.hpp"
#include "lib/driver/basic-driver.hpp"
#include "lib/driver/basic-accesser.hpp"
#include "lib/index/btree.hpp"

using namespace cdb;

static const char DUMP_PATH[] = "/tmp/btree-dump.txt";
static const char TEST_PATH[] = "/tmp/btree-test.tmp";
static const int TEST_NUMBER = 512;
static const int TEST_LARGE_NUMBER = 10000;    // tested up to 1000000, make it small to reduce test time

namespace cdb {
    class BTreeTest : public ::testing::Test
    {
    protected:
        static void TearDownTestCase()
        { std::remove(TEST_PATH); }

        std::unique_ptr<Driver> drv;
        std::unique_ptr<BlockAllocator> allocator;
        std::unique_ptr<BasicAccesser> accesser;
        std::unique_ptr<BTree> uut;

        BTreeTest()
            : drv(new BasicDriver(TEST_PATH)),
              allocator(new BitmapAllocator(drv.get(), 0)),
              accesser(new BasicAccesser(drv.get(), allocator.get()))
        {
            allocator->reset();
            uut.reset(new BTree(
                    accesser.get(),
                    [](const Byte *a, const Byte *b) -> bool 
                    {
                        return 
                            *reinterpret_cast<const int*>(a) <
                            *reinterpret_cast<const int*>(b);
                    },
                    [](const Byte *a, const Byte *b) -> bool
                    {
                        return 
                            *reinterpret_cast<const int*>(a) ==
                            *reinterpret_cast<const int*>(b);
                    },
                    accesser->allocateBlock(),
                    sizeof(int),
                    sizeof(int)
                ));
            uut->reset();
        }

        template <typename T>
        ConstSlice makeConstSlice(T &obj)
        { return ConstSlice(reinterpret_cast<const Byte*>(&obj), sizeof(T)); }

        void setKeyOfNodeEntry(Byte* entry, ConstSlice key)
        {
            std::copy(
                    key.cbegin(),
                    key.cend(),
                    uut->getKeyFromNodeEntry(entry)
                );
        }

        void setValueOfNodeEntry(Byte* entry, ConstSlice value)
        {
            std::copy(
                    value.cbegin(),
                    value.cend(),
                    reinterpret_cast<Byte*>(uut->getIndexFromNodeEntry(entry))
                );
        }

        void setKeyOfLeafEntry(Byte* entry, ConstSlice key)
        {
            std::copy(
                    key.cbegin(),
                    key.cend(),
                    uut->getKeyFromLeafEntry(entry)
                );
        }

        void setValueOfLeafEntry(Byte* entry, ConstSlice value)
        {
            std::copy(
                    value.cbegin(),
                    value.cend(),
                    uut->getValueFromLeafEntry(entry).begin()
                );
        }

        void treeDump(std::ostream &os)
        {
            os << "first_leaf: " << uut->_first_leaf << std::endl;
            os << "last_leaf: " << uut->_last_leaf << std::endl;
            treeDump(uut->_root, os);
        }

        void treeDump(Block node, std::ostream &os)
        {
            os << node.index() << std::endl;

            // dump head
            auto *header = uut->getHeaderFromNode(node);
            os << "    node_is_leaf: " << header->node_is_leaf << std::endl;
            os << "    node_length: " << header->node_length << std::endl;
            os << "    entry_count: " << header->entry_count << std::endl;
            os << "    prev: " << header->prev << std::endl;
            os << "    next: " << header->next << std::endl;

            if (header->node_is_leaf) {
                os << std::endl;
                for (int i = 0; i < header->entry_count; ++i) {
                    auto *entry = uut->getEntryInLeafByIndex(node, i);
                    os << "    " << *reinterpret_cast<int*>(uut->getKeyFromLeafEntry(entry))
                        << " : " << *reinterpret_cast<int*>(uut->getValueFromLeafEntry(entry).content())
                        << std::endl;
                }
                os << std::endl;
            }
            else {
                os << "    before: " << uut->getMarkFromNode(node)->before << std::endl;
                os << std::endl;
                for (int i = 0; i < header->entry_count; ++i) {
                    auto *entry = uut->getEntryInNodeByIndex(node, i);
                    os << "    " << *reinterpret_cast<int*>(uut->getKeyFromNodeEntry(entry))
                        << ": " << *reinterpret_cast<int*>(uut->getIndexFromNodeEntry(entry))
                        << std::endl;
                }
                os << std::endl;

                if (uut->getMarkFromNode(node)->before) {
                    treeDump(uut->_accesser->aquire(uut->getMarkFromNode(node)->before), os);
                }

                for (int i = 0; i < header->entry_count; ++i) {
                    auto *entry = uut->getEntryInNodeByIndex(node, i);
                    treeDump(uut->_accesser->aquire(*uut->getIndexFromNodeEntry(entry)), os);
                }
            }
        }
    };

TEST_F(BTreeTest, Insert)
{
    for (int i = 0; i < TEST_NUMBER; ++i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->insert(key);
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_NUMBER; ++i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->lowerBound(key);
        EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.getKey()));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter.getValue().content()));
    }
}

TEST_F(BTreeTest, InsertInReverseOrder)
{
    for (int i = TEST_NUMBER - 1; i >= 0; --i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->insert(key);
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_NUMBER; ++i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->lowerBound(key);
        EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.getKey()));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter.getValue().content()));
    }
}

TEST_F(BTreeTest, LowerBoundButNotFound)
{
    for (int i = 0; i <= TEST_NUMBER; ++i) {
        if (i % 16 == 0) {
            Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
            auto iter = uut->insert(key);
            *reinterpret_cast<int*>(iter.getValue().content()) = i;
        }
    }

    for (int i = 0; i <= TEST_NUMBER; ++i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->lowerBound(key);

        EXPECT_EQ(((i + 15) / 16) * 16, *reinterpret_cast<const int*>(iter.getKey()));
        EXPECT_EQ(((i + 15) / 16) * 16, *reinterpret_cast<const int*>(iter.getValue().content()));
    }
}

TEST_F(BTreeTest, UpperBoundButNotFound)
{
    for (int i = 0; i <= TEST_NUMBER; ++i) {
        if (i % 16 == 0) {
            Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
            auto iter = uut->insert(key);
            *reinterpret_cast<int*>(iter.getValue().content()) = i;
        }
    }

    for (int i = 0; i < TEST_NUMBER; ++i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->upperBound(key);

        EXPECT_EQ(((i + 16) / 16) * 16, *reinterpret_cast<const int*>(iter.getKey()));
        EXPECT_EQ(
                ((i + 16) / 16) * 16,
                *reinterpret_cast<const int*>(iter.getValue().content())
            ) << "i = " << i;
    }
}

TEST_F(BTreeTest, ForEach)
{
    for (int i = 0; i <= TEST_NUMBER; ++i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->insert(key);
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_NUMBER; i += 16) {
        int count = 0;
        int i_upper = i + 16;
        Slice key_lower(reinterpret_cast<Byte*>(&i), sizeof(i));
        Slice key_upper(reinterpret_cast<Byte*>(&i_upper), sizeof(i_upper));

        uut->forEach(
            uut->lowerBound(key_lower),
            uut->lowerBound(key_upper),
            [&count](const BTree::Iterator &) -> void
            { ++count; }
        );

        EXPECT_EQ(16, count) << "i = " << i;
    }
}

TEST_F(BTreeTest, ForEachReverse)
{
    for (int i = 0; i <= TEST_NUMBER; ++i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->insert(key);
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_NUMBER; i += 16) {
        int count = 0;
        int i_upper = i + 16;
        Slice key_lower(reinterpret_cast<Byte*>(&i), sizeof(i));
        Slice key_upper(reinterpret_cast<Byte*>(&i_upper), sizeof(i_upper));

        uut->forEachReverse(
            uut->lowerBound(key_lower),
            uut->lowerBound(key_upper),
            [&count](const BTree::Iterator &) -> void
            { ++count; }
        );

        EXPECT_EQ(16, count) << "i = " << i;
    }
}

TEST_F(BTreeTest, EmptyIterator)
{
    int count = 0;
    uut->forEach(
        [&count](const BTree::Iterator &) -> void
        { ++count; }
    );
    EXPECT_EQ(0, count);
    uut->forEachReverse(
        [&count](const BTree::Iterator &) -> void
        { ++count; }
    );
    EXPECT_EQ(0, count);
}

TEST_F(BTreeTest, LargeAmountData)
{
    for (int i = 0; i < TEST_LARGE_NUMBER; ++i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->insert(key);
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    int count = 0;
    uut->forEach(
        [&count](const BTree::Iterator &i) -> void
        { 
            EXPECT_EQ(count, *reinterpret_cast<int*>(i.getValue().content()))
                << *reinterpret_cast<const int*>(i.getKey());
            ++count;
        }
    );
    EXPECT_EQ(TEST_LARGE_NUMBER, count);

    count = 0;
    uut->forEachReverse(
        [&count](const BTree::Iterator &i) -> void
        { 
            ++count;
            EXPECT_EQ(
                    TEST_LARGE_NUMBER - count,
                    *reinterpret_cast<int*>(i.getValue().content())
                )
                << *reinterpret_cast<const int*>(i.getKey());
        }
    );
    EXPECT_EQ(TEST_LARGE_NUMBER, count);
}

TEST_F(BTreeTest, InternalTest)
{
    // findInNode
    {
        // normal test
        Block node = uut->_accesser->aquire(uut->_accesser->allocateBlock());
        auto *mark = uut->getMarkFromNode(node);
        *mark = {
            {
                false,      // node_is_leaf
                1,          // node_length
                16,         // entry_count
                0,          // prev
                0           // next
            },
            0xFFFFFFFF      // before
        };
        for (int i = 0; i < 16; ++i) {
            setKeyOfNodeEntry(
                    uut->getEntryInNodeByIndex(node, i),
                    makeConstSlice(i)
                );
            setValueOfNodeEntry(
                    uut->getEntryInNodeByIndex(node, i),
                    makeConstSlice(i)
                );
        }
        for (int i = 0; i < 16; ++i) {
            EXPECT_EQ(i, uut->findInNode(node, makeConstSlice(i)));
        }

        // full size test
        int full_size = uut->maximumEntryPerNode();
        *mark = {
            {
                false,          // node_is_leaf
                1,              // node_length
                static_cast<unsigned int>(full_size),      // entry_count
                0,              // prev
                0               // next
            },
            0xFFFFFFFF          // before
        };

        for (int i = 16; i < 16 + full_size; ++i) {
            setKeyOfNodeEntry(
                    uut->getEntryInNodeByIndex(node, i - 16),
                    makeConstSlice(i)
                );
            setValueOfNodeEntry(
                    uut->getEntryInNodeByIndex(node, i - 16),
                    makeConstSlice(i)
                );
        }
        for (int i = 16; i < 16 + full_size; ++i) {
            EXPECT_EQ(i, uut->findInNode(node, makeConstSlice(i)));
        }
        for (int i = 0; i < 16; ++i) {
            EXPECT_EQ(0xFFFFFFFF, uut->findInNode(node, makeConstSlice(i)));
        }

        // enpty size test
        *mark = {
            {
                false,          // node_is_leaf
                1,              // node_length
                0,              // entry_count
                0,              // prev
                0               // next
            },
            0xFFFFFFFF          // before
        };

        for (int i = 0; i < 1000; ++i) {
            EXPECT_EQ(0xFFFFFFFF, uut->findInNode(node, makeConstSlice(i)));
        }

        // lower bound not found test
        *mark = {
            {
                false,          // node_is_leaf
                1,              // node_length
                17,             // entry_count
                0,              // prev
                0               // next
            },
            0xFFFFFFFF          // before
        };
        for (int i = 0; i <= 16; ++i) {
            int t = i * 16;
            setKeyOfNodeEntry(
                    uut->getEntryInNodeByIndex(node, i),
                    makeConstSlice(t)
                );
            setValueOfNodeEntry(
                    uut->getEntryInNodeByIndex(node, i),
                    makeConstSlice(t)
                );
        }
        for (int i = 0; i < 16 * 16; ++i) {
            EXPECT_EQ(
                    (i / 16) * 16,
                    uut->findInNode(node, makeConstSlice(i))
                ) << "i = " << i;
        }
    }

    // findInLeaf
    {
        // normal test
        Block leaf = uut->_accesser->aquire(uut->_accesser->allocateBlock());
        auto *header = uut->getHeaderFromNode(leaf);
        *header = {
            true,       // node_is_leaf
            1,          // node_length
            16,         // entry_count
            0,          // prev
            0,          // next
        };

        for (int i = 0; i < 16; ++i) {
            setKeyOfLeafEntry(
                    uut->getEntryInLeafByIndex(leaf, i),
                    makeConstSlice(i)
                );
            setValueOfLeafEntry(
                    uut->getEntryInLeafByIndex(leaf, i),
                    makeConstSlice(i)
                );
        }
        for (int i = 0; i < 16; ++i) {
            auto iter = uut->findInLeaf(leaf, makeConstSlice(i));
            EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.getKey()));
            EXPECT_EQ(i, *reinterpret_cast<int*>(iter.getValue().content()));
        }

        // full test
        Block next_leaf = uut->_accesser->aquire(uut->_accesser->allocateBlock());
        auto *next_header = uut->getHeaderFromNode(next_leaf);

        uut->_first_leaf = leaf.index();
        uut->_last_leaf = next_leaf.index();

        *next_header = {
            true,
            1,
            1,
            0,
            0
        };
        int full_size = uut->maximumEntryPerLeaf();
        *header = {
            true,       // node_is_leaf
            1,          // node_length
            static_cast<unsigned int>(full_size),   // entry_count
            0,          // prev
            next_leaf.index()       // next
        };

        for (int i = 16; i < full_size + 16; ++i) {
            setKeyOfLeafEntry(
                    uut->getEntryInLeafByIndex(leaf, i - 16),
                    makeConstSlice(i)
                );
            setValueOfLeafEntry(
                    uut->getEntryInLeafByIndex(leaf, i - 16),
                    makeConstSlice(i)
                );
        }

        int key = full_size + 32;
        setKeyOfLeafEntry(
                uut->getEntryInLeafByIndex(next_leaf, 0),
                makeConstSlice(key)
            );
        setValueOfLeafEntry(
                uut->getEntryInLeafByIndex(next_leaf, 0),
                makeConstSlice(key)
            );

        for (int i = 16; i < full_size + 16; ++i) {
            auto iter = uut->findInLeaf(leaf, makeConstSlice(i));
            EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.getKey()));
            EXPECT_EQ(i, *reinterpret_cast<int*>(iter.getValue().content()));
        }

        for (int i = full_size + 16; i < full_size + 32; ++i) {
            auto iter = uut->findInLeaf(leaf, makeConstSlice(i));
            EXPECT_EQ(full_size + 32, *reinterpret_cast<const int*>(iter.getKey()));
            EXPECT_EQ(full_size + 32, *reinterpret_cast<int*>(iter.getValue().content()));
        }
    }

    // splitLeaf
    {
        // root leaf split test
        {
            uut->_root = uut->_accesser->aquire(uut->_accesser->allocateBlock());
            Block &leaf = uut->_root;
            auto *header = uut->getHeaderFromNode(leaf);
            int full_size = uut->maximumEntryPerLeaf();

            *header = {
                true,       // node_is_leaf
                1,          // node_length
                static_cast<unsigned int>(full_size),   // entry_count
                0,
                0
            };

            for (int i = 0; i < full_size; ++i) {
                setKeyOfLeafEntry(
                        uut->getEntryInLeafByIndex(leaf, i),
                        makeConstSlice(i)
                    );
                setValueOfLeafEntry(
                        uut->getEntryInLeafByIndex(leaf, i),
                        makeConstSlice(i)
                    );
            }

            auto split_offset = header->entry_count / 2;
            Block new_leaf = uut->splitLeaf(leaf, split_offset);
            auto *new_header = uut->getHeaderFromNode(new_leaf);

            EXPECT_TRUE(header->node_is_leaf);
            EXPECT_EQ(1, header->node_length);
            EXPECT_EQ(split_offset, header->entry_count);
            EXPECT_EQ(0, header->prev);
            EXPECT_EQ(new_leaf.index(), header->next);

            EXPECT_TRUE(new_header->node_is_leaf);
            EXPECT_EQ(1, new_header->node_length);
            EXPECT_EQ(full_size - split_offset, new_header->entry_count);
            EXPECT_EQ(leaf.index(), new_header->prev);
            EXPECT_EQ(0, new_header->next);

            for (int i = 0; i < split_offset; ++i) {
                auto *entry = uut->getEntryInLeafByIndex(leaf, i);
                EXPECT_EQ(i, *reinterpret_cast<int*>(uut->getKeyFromLeafEntry(entry)));
                EXPECT_EQ(i, *reinterpret_cast<int*>(uut->getValueFromLeafEntry(entry).content()));
            }

            for (int i = split_offset; i < full_size; ++i) {
                auto *entry = uut->getEntryInLeafByIndex(new_leaf, i - split_offset);
                EXPECT_EQ(i, *reinterpret_cast<int*>(uut->getKeyFromLeafEntry(entry)));
                EXPECT_EQ(i, *reinterpret_cast<int*>(uut->getValueFromLeafEntry(entry).content()));
            }
        }

        // leaf with both links test
        {
            int full_size = uut->maximumEntryPerLeaf();

            Block leaf = uut->_accesser->aquire(uut->_accesser->allocateBlock());
            Block prev_leaf = uut->_accesser->aquire(uut->_accesser->allocateBlock());
            Block next_leaf = uut->_accesser->aquire(uut->_accesser->allocateBlock());

            auto *header = uut->getHeaderFromNode(leaf);
            *header = {
                true,
                1,
                static_cast<unsigned int>(full_size),
                prev_leaf.index(),
                next_leaf.index()
            };

            auto *prev_header = uut->getHeaderFromNode(prev_leaf);
            prev_header->next = leaf.index();

            auto *next_header = uut->getHeaderFromNode(next_leaf);
            next_header->prev = leaf.index();

            for (int i = 0; i < full_size; ++i) {
                setKeyOfLeafEntry(
                        uut->getEntryInLeafByIndex(leaf, i),
                        makeConstSlice(i)
                    );
                setValueOfLeafEntry(
                        uut->getEntryInLeafByIndex(leaf, i),
                        makeConstSlice(i)
                    );
            }

            auto split_offset = header->entry_count / 2;
            Block new_leaf = uut->splitLeaf(leaf, split_offset);
            auto *new_header = uut->getHeaderFromNode(new_leaf);

            EXPECT_TRUE(header->node_is_leaf);
            EXPECT_EQ(1, header->node_length);
            EXPECT_EQ(split_offset, header->entry_count);
            EXPECT_EQ(prev_leaf.index(), header->prev);
            EXPECT_EQ(new_leaf.index(), header->next);

            EXPECT_TRUE(new_header->node_is_leaf);
            EXPECT_EQ(1, new_header->node_length);
            EXPECT_EQ(full_size - split_offset, new_header->entry_count);
            EXPECT_EQ(leaf.index(), new_header->prev);
            EXPECT_EQ(next_leaf.index(), new_header->next);

            EXPECT_EQ(prev_header->next, leaf.index());
            EXPECT_EQ(next_header->prev, new_leaf.index());

            for (int i = 0; i < split_offset; ++i) {
                auto *entry = uut->getEntryInLeafByIndex(leaf, i);
                EXPECT_EQ(i, *reinterpret_cast<int*>(uut->getKeyFromLeafEntry(entry)));
                EXPECT_EQ(i, *reinterpret_cast<int*>(uut->getValueFromLeafEntry(entry).content()));
            }

            for (int i = split_offset; i < full_size; ++i) {
                auto *entry = uut->getEntryInLeafByIndex(new_leaf, i - split_offset);
                EXPECT_EQ(i, *reinterpret_cast<int*>(uut->getKeyFromLeafEntry(entry)));
                EXPECT_EQ(i, *reinterpret_cast<int*>(uut->getValueFromLeafEntry(entry).content()));
            }
        }
    }
}

TEST_F(BTreeTest, EraseTest)
{
    for (int i = 0; i < TEST_NUMBER; ++i) {
        *reinterpret_cast<int*>(uut->insert(makeConstSlice(i)).getValue().content()) = i;
    }

    int count = 0;
    uut->forEach(
        [&count](const BTree::Iterator &) -> void
        { ++count; }
    );
    EXPECT_EQ(TEST_NUMBER, count);

    for (int i = 0; i < TEST_NUMBER; ++i) {
        uut->erase(makeConstSlice(i));
        int count = 0;
        uut->forEach(
            [&count, i](const BTree::Iterator &iter) -> void
            {
                ++count;
                EXPECT_EQ(i + count, *reinterpret_cast<int*>(iter.getValue().content()));
            }
        );
        EXPECT_EQ(TEST_NUMBER - i - 1, count);
        count = 0;
        uut->forEachReverse(
            [&count, i](const BTree::Iterator &iter) -> void
            {
                ++count;
                EXPECT_EQ(
                    TEST_NUMBER - count,
                    *reinterpret_cast<int*>(iter.getValue().content())
                );
            }
        );
        EXPECT_EQ(TEST_NUMBER - i - 1, count);
    }
}

TEST_F(BTreeTest, EraseInReverseOrder)
{
    for (int i = 0; i < TEST_NUMBER; ++i) {
        *reinterpret_cast<int*>(uut->insert(makeConstSlice(i)).getValue().content()) = i;
    }

    int count = 0;
    uut->forEach(
        [&count](const BTree::Iterator &) -> void
        { ++count; }
    );
    EXPECT_EQ(TEST_NUMBER, count);

    for (int i = TEST_NUMBER - 1; i >= 0; --i) {
        uut->erase(makeConstSlice(i));
        int count = 0;
        uut->forEach(
            [&count](const BTree::Iterator &iter) -> void
            {
                EXPECT_EQ(count, *reinterpret_cast<int*>(iter.getValue().content()));
                ++count;
            }
        );
        EXPECT_EQ(i, count);
        count = 0;
        uut->forEachReverse(
            [&count, i](const BTree::Iterator &iter) -> void
            {
                ++count;
                EXPECT_EQ(
                    i - count,
                    *reinterpret_cast<int*>(iter.getValue().content())
                );
            }
        );
        EXPECT_EQ(i, count);
    }
}

TEST_F(BTreeTest, EraseTestLargeAmount)
{
    for (int i = 0; i < TEST_LARGE_NUMBER; ++i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->insert(key);
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    std::ofstream fout(DUMP_PATH + std::string("-original"));
    treeDump(fout);

    for (int i = 0; i < TEST_LARGE_NUMBER; ++i) {
        uut->erase(makeConstSlice(i));
    }

    int count = 0;
    uut->forEach([&count](const BTree::Iterator &) { ++count; });
    EXPECT_EQ(0, count);
}

TEST_F(BTreeTest, RandomTest)
{
    std::vector<int> list(TEST_NUMBER);
    for (int i = 0; i < TEST_NUMBER; ++i) {
        list[i] = i;
    }
    std::shuffle(list.begin(), list.end(), std::default_random_engine(0));

    for (int i = 0; i < TEST_NUMBER; ++i) {
        *reinterpret_cast<int*>(uut->insert(makeConstSlice(list[i])).getValue().content()) = i;

        int count = 0;
        int last = -1;
        uut->forEach(
            [&count, &last, i](const BTree::Iterator &iter) -> void
            { 
                ++count;
                EXPECT_TRUE(last < *reinterpret_cast<const int*>(iter.getKey()))
                    << last << " <> " << *reinterpret_cast<const int*>(iter.getKey())
                    << " when i = " << i;
                last = *reinterpret_cast<const int*>(iter.getKey());
            }
        );
        EXPECT_EQ(i + 1, count);
    }

    for (int i = TEST_NUMBER - 1; i >= 0; --i) {
        uut->erase(makeConstSlice(list[i]));

        int count = 0;
        int last = -1;
        uut->forEach(
            [&count, &last, i](const BTree::Iterator &iter) -> void
            { 
                ++count;
                EXPECT_TRUE(last < *reinterpret_cast<const int*>(iter.getKey()))
                    << last << " <> " << *reinterpret_cast<const int*>(iter.getKey())
                    << " when i = " << i;
                last = *reinterpret_cast<const int*>(iter.getKey());
            }
        );
        EXPECT_EQ(i, count);
    }
}

}
