#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <random>

#include "../test-inc.hpp"
#include "lib/driver/bitmap-allocator.hpp"
#include "lib/driver/basic-driver.hpp"
#include "lib/driver/basic-accesser.hpp"
#include "lib/index/btree.hpp"

#include "lib/index/btree-intl.hpp"

using namespace cdb;

static const char DUMP_PATH[] = TMP_PATH_PREFIX "btree-dump.txt";
static const char TEST_PATH[] = TMP_PATH_PREFIX "btree-test.tmp";
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

TEST_F(BTreeTest, MakeKey)
{
    int key_small_than_Key = TEST_NUMBER;
    ASSERT_TRUE(sizeof(key_small_than_Key) <= sizeof(BTree::Key::value));
    auto key = uut->makeKey(&key_small_than_Key);
    EXPECT_EQ(key_small_than_Key, *reinterpret_cast<const int*>(&key.value));

    char test[] = {1, 2, 3, 4};
    key = uut->makeKey(test, sizeof(test) / sizeof(test[0]));
    EXPECT_EQ(0x04030201, *reinterpret_cast<const int*>(&key.value));

    uut->_key_size = sizeof(BTree::Key::value) * 2;
    key = uut->makeKey(&key_small_than_Key);
    EXPECT_EQ(&key_small_than_Key, reinterpret_cast<const int*>(key.pointer));

    key = uut->makeKey(test, sizeof(test) / sizeof(test[0]));
    EXPECT_EQ(test, reinterpret_cast<const char*>(key.pointer));
}

TEST_F(BTreeTest, GetPointerOfKey)
{
    int key_small_than_Key = TEST_NUMBER;
    ASSERT_TRUE(sizeof(key_small_than_Key) <= sizeof(BTree::Key::value));
    auto key = uut->makeKey(&key_small_than_Key);
    EXPECT_EQ(reinterpret_cast<const int*>(&key.value), reinterpret_cast<const int*>(uut->getPointerOfKey(key)));

    uut->_key_size = sizeof(BTree::Key::value) * 2;
    key = uut->makeKey(&key_small_than_Key);
    EXPECT_EQ(&key_small_than_Key, reinterpret_cast<const int*>(uut->getPointerOfKey(key)));
}

TEST_F(BTreeTest, EntrySize)
{
    EXPECT_EQ(uut->_key_size + 4, uut->nodeEntrySize());
    EXPECT_EQ(uut->_key_size + uut->_value_size, uut->leafEntrySize());
}

TEST_F(BTreeTest, MaximumEntry)
{
    EXPECT_EQ((Driver::BLOCK_SIZE - sizeof(BTree::NodeMark)) / uut->nodeEntrySize(), uut->maximumEntryPerNode());
    EXPECT_EQ((Driver::BLOCK_SIZE - sizeof(BTree::LeafMark)) / uut->leafEntrySize(), uut->maximumEntryPerLeaf());
}

// ----

TEST_F(BTreeTest, Insert)
{
    for (int i = 0; i < TEST_NUMBER; ++i) {
        auto iter = uut->insert(uut->makeKey(&i));
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_NUMBER; ++i) {
        auto iter = uut->lowerBound(uut->makeKey(&i));
        EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.getKey()));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter.getValue().content()));
    }
}

TEST_F(BTreeTest, InsertInReverseOrder)
{
    for (int i = TEST_NUMBER - 1; i >= 0; --i) {
        auto iter = uut->insert(uut->makeKey(&i));
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_NUMBER; ++i) {
        auto iter = uut->lowerBound(uut->makeKey(&i));
        EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.getKey()));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter.getValue().content()));
    }
}

TEST_F(BTreeTest, LowerBoundButNotFound)
{
    for (int i = 0; i <= TEST_NUMBER; ++i) {
        if (i % 16 == 0) {
            auto iter = uut->insert(uut->makeKey(&i));
            *reinterpret_cast<int*>(iter.getValue().content()) = i;
        }
    }

    for (int i = 0; i <= TEST_NUMBER; ++i) {
        auto iter = uut->lowerBound(uut->makeKey(&i));

        EXPECT_EQ(((i + 15) / 16) * 16, *reinterpret_cast<const int*>(iter.getKey()));
        EXPECT_EQ(((i + 15) / 16) * 16, *reinterpret_cast<const int*>(iter.getValue().content()));
    }
}

TEST_F(BTreeTest, UpperBoundButNotFound)
{
    for (int i = 0; i <= TEST_NUMBER; ++i) {
        if (i % 16 == 0) {
            auto iter = uut->insert(uut->makeKey(&i));
            *reinterpret_cast<int*>(iter.getValue().content()) = i;
        }
    }

    for (int i = 0; i < TEST_NUMBER; ++i) {
        auto iter = uut->upperBound(uut->makeKey(&i));

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
        auto iter = uut->insert(uut->makeKey(&i));
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_NUMBER; i += 16) {
        int count = 0;
        int i_upper = i + 16;

        uut->forEach(
            uut->lowerBound(uut->makeKey(&i)),
            uut->lowerBound(uut->makeKey(&i_upper)),
            [&count](const BTree::Iterator &) -> void
            { ++count; }
        );

        EXPECT_EQ(16, count) << "i = " << i;
    }
}

TEST_F(BTreeTest, ForEachReverse)
{
    for (int i = 0; i <= TEST_NUMBER; ++i) {
        auto iter = uut->insert(uut->makeKey(&i));
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_NUMBER; i += 16) {
        int count = 0;
        int i_upper = i + 16;

        uut->forEachReverse(
            uut->lowerBound(uut->makeKey(&i)),
            uut->lowerBound(uut->makeKey(&i_upper)),
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
        auto iter = uut->insert(uut->makeKey(&i));
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

TEST_F(BTreeTest, EraseTest)
{
    for (int i = 0; i < TEST_NUMBER; ++i) {
        *reinterpret_cast<int*>(uut->insert(uut->makeKey(&i)).getValue().content()) = i;
    }

    int count = 0;
    uut->forEach(
        [&count](const BTree::Iterator &) -> void
        { ++count; }
    );
    EXPECT_EQ(TEST_NUMBER, count);

    for (int i = 0; i < TEST_NUMBER; ++i) {
        uut->erase(uut->makeKey(&i));
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
        *reinterpret_cast<int*>(uut->insert(uut->makeKey(&i)).getValue().content()) = i;
    }

    int count = 0;
    uut->forEach(
        [&count](const BTree::Iterator &) -> void
        { ++count; }
    );
    EXPECT_EQ(TEST_NUMBER, count);

    for (int i = TEST_NUMBER - 1; i >= 0; --i) {
        uut->erase(uut->makeKey(&i));
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
        auto iter = uut->insert(uut->makeKey(&i));
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_LARGE_NUMBER; ++i) {
        uut->erase(uut->makeKey(&i));
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
        *reinterpret_cast<int*>(uut->insert(uut->makeKey(&list[i])).getValue().content()) = i;

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
        uut->erase(uut->makeKey(&list[i]));

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

TEST_F(BTreeTest, Iterator)
{
    for (int i = 0; i <= TEST_NUMBER; ++i) {
        auto iter = uut->insert(uut->makeKey(&i));
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_NUMBER; i += 16) {
        int count = 0;
        int i_upper = i + 16;

        auto lower_iter = uut->lowerBound(uut->makeKey(&i));
        auto upper_iter = uut->lowerBound(uut->makeKey(&i_upper));

        while (lower_iter != upper_iter) {
            ++count;
            lower_iter.next();
        }
        EXPECT_EQ(16, count) << "i = " << i;

        lower_iter = uut->lowerBound(uut->makeKey(&i));
        count = 0;
        while (lower_iter != upper_iter) {
            ++count;
            upper_iter.prev();
        }
        EXPECT_EQ(16, count) << "i = " << i;
    }
}

TEST_F(BTreeTest, OtherKeyValueSize)
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
            sizeof(28)
    ));
    uut->reset();

    for (int i = 0; i < TEST_LARGE_NUMBER; ++i) {
        auto iter = uut->insert(uut->makeKey(&i));
        EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.getKey()));
    }
}

}
