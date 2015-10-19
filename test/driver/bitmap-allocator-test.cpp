#include <gtest/gtest.h>
#include <memory>
#include <cstdio>

#include "../test-inc.hpp"
#include "lib/driver/basic-driver.hpp"
#include "lib/driver/bitmap-allocator.hpp"

using namespace cdb;

static const char TEST_PATH[] = TMP_PATH_PREFIX "bitmap-allocator-test.tmp";

class BitmapAllocatorTest : public ::testing::Test
{
protected:
    static void TearDownTestCase()
    { std::remove(TEST_PATH); }

    std::unique_ptr<Driver> drv;
    std::unique_ptr<BlockAllocator> uut;

    BitmapAllocatorTest()
        : drv(new BasicDriver(TEST_PATH)), uut(new BitmapAllocator(drv.get(), 1))
    { uut->reset(); }
};

TEST_F(BitmapAllocatorTest, AllocatingSingle)
{
    EXPECT_EQ(2, uut->allocateBlock());
    uut->freeBlock(2);
    EXPECT_EQ(2, uut->allocateBlock());
    uut->freeBlock(2);

    for (int i = 2; i < 63; ++i) {
        EXPECT_EQ(i, uut->allocateBlock());
    }

    for (int i = 2; i < 63; ++i) {
        uut->freeBlock(i);
    }

    for (int i = 2; i < 63; ++i) {
        EXPECT_EQ(i, uut->allocateBlock());
    }
}

TEST_F(BitmapAllocatorTest, AllocatingMultple)
{
    EXPECT_EQ(2, uut->allocateBlocks(2));
    EXPECT_EQ(4, uut->allocateBlocks(2));

    uut->freeBlocks(2, 2);
    uut->freeBlocks(4, 2);

    EXPECT_EQ(2, uut->allocateBlocks(2));

    for (int i = 1; i < 16; ++i) {
        EXPECT_EQ(4 * i, uut->allocateBlocks(4));
    }
}

TEST_F(BitmapAllocatorTest, AllocatingOverOneSection)
{
    for (int i = 2; i < 8191; ++i) {
        EXPECT_EQ(i < 8160 ? i : i + 1, uut->allocateBlock());
    }

    EXPECT_EQ(8192, uut->allocateBlock());
}

TEST_F(BitmapAllocatorTest, CanBeOpennedAgain)
{
    for (int i = 2; i < 8191; ++i) {
        EXPECT_EQ(i < 8160 ? i : i + 1, uut->allocateBlock());
    }

    EXPECT_EQ(8192, uut->allocateBlock());

    uut.reset();
    drv.reset();

    // ----
    
    drv.reset(new BasicDriver(TEST_PATH));
    uut.reset(new BitmapAllocator(drv.get(), 1));

    EXPECT_EQ(8193, uut->allocateBlock());
}

TEST_F(BitmapAllocatorTest, CanAllocatingWithHinting)
{
    for (int i = 64; i < 96; ++i) {
        EXPECT_EQ(i, uut->allocateBlock(i));
    }
    EXPECT_EQ(96, uut->allocateBlock(96));

    for (int i = 8161; i < 8192; ++i) {
        EXPECT_EQ(i, uut->allocateBlock(i));
    }
    EXPECT_EQ(8128, uut->allocateBlock(8161));
}

TEST_F(BitmapAllocatorTest, HintAtNotExist)
{
    for (int i = 32768; i < 32768 + 8160; ++i) {
        EXPECT_EQ(i, uut->allocateBlock(32768));
    }

    for (int i = 32768 + 8161; i < 32768 + 8192; ++i) {
        EXPECT_EQ(i, uut->allocateBlock(32768));
    }
}

TEST_F(BitmapAllocatorTest, FindBeforeHint)
{
    for (int i = 32768; i < 32768 + 8160; ++i) {
        EXPECT_EQ(i, uut->allocateBlock(32768));
    }

    for (int i = 32768 + 8161; i < 32768 + 8192; ++i) {
        EXPECT_EQ(i, uut->allocateBlock(32768));
    }

    EXPECT_EQ(24576, uut->allocateBlock(32768));
}

TEST_F(BitmapAllocatorTest, AnotherStartAt)
{
    uut.reset(new BitmapAllocator(drv.get(), 0));

    uut->reset();

    EXPECT_EQ(1, uut->allocateBlock());
}

