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
                accesser->allocateBlock(),
                sizeof(int),
                sizeof(int)
            ));
        uut->reset();
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
        auto iter = uut->find(key);
        EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.getKey()));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter.getValue().content()));
    }
}

TEST_F(BTreeTest, InsertMultiple)
{
    for (int i = 1; i < TEST_NUMBER; i += 2) {
        for (int j = 0; j < i; ++j) {
            Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
            auto iter = uut->insert(key);
            *reinterpret_cast<int*>(iter.getValue().content()) = i;
        }
    }

    for (int i = 1; i < TEST_NUMBER; i += 2) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->find(key);
        EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.getKey()));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter.getValue().content()));
    }
}

TEST_F(BTreeTest, InsertInRevertOrder)
{
    for (int i = TEST_NUMBER - 1; i >= 0; --i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->insert(key);
        *reinterpret_cast<int*>(iter.getValue().content()) = i;
    }

    for (int i = 0; i < TEST_NUMBER; ++i) {
        Slice key(reinterpret_cast<Byte*>(&i), sizeof(i));
        auto iter = uut->find(key);
        EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.getKey()));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter.getValue().content()));
    }
}
