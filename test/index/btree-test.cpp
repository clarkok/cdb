#include <gtest/gtest.h>
#include <memory>
#include <cstdio>

#include "lib/driver/bitmap-allocator.hpp"
#include "lib/driver/basic-driver.hpp"
#include "lib/driver/basic-accesser.hpp"
#include "lib/index/btree.hpp"

using namespace cdb;

static const char TEST_PATH[] = "/tmp/btree-test.tmp";
static const int TEST_NUMBER = 512;
static const int TEST_LARGE_NUMBER = 100000;

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
                    uut->getValueFromLeafEntry(entry)
                );
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
        EXPECT_EQ(((i + 16) / 16) * 16, *reinterpret_cast<const int*>(iter.getValue().content()));
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
}

TEST_F(BTreeTest, InternalTest)
{
    // lowerBoundInNode
    {
        Block node = uut->_accesser->aquire(uut->_accesser->allocateBlock());
        auto *header = uut->getHeaderFromNode(node);
        for (int i = 0; i < 16; ++i) {
            setKeyOfNodeEntry(
                    uut->getEntryInNodeByIndex(node, 0),
                    makeConstSlice(i)
                );
            setValueOfNodeEntry(
                    uut->getEntryInNodeByIndex(node, 0),
                    makeConstSlice(i)
                );
        }
    }
}
}
