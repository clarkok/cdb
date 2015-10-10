#include <gtest/gtest.h>
#include <memory>
#include <cstdio>

#include "lib/driver/basic-driver.hpp"
#include "lib/driver/bitmap-allocator.hpp"

using namespace cdb;

static const char TEST_PATH[] = "/tmp/bitmap-allocator-test.tmp";

TEST(BitmapAllocatorTest, Reseting)
{
    std::unique_ptr<Driver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BlockAllocator> uut(new BitmapAllocator(drv.get(), 1));

    uut->reset();

    uut.reset();
    drv.reset();
}

TEST(BitmapAllocatorTest, AllocatingSingle)
{
    std::unique_ptr<Driver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BlockAllocator> uut(new BitmapAllocator(drv.get(), 1));

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

    for (int i = 2; i < 63; ++i) {
        uut->freeBlock(i);
    }
}

TEST(BitmapAllocatorTest, AllocatingMultple)
{
    std::unique_ptr<Driver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BlockAllocator> uut(new BitmapAllocator(drv.get(), 1));

    EXPECT_EQ(2, uut->allocateBlocks(2));
    EXPECT_EQ(4, uut->allocateBlocks(2));

    uut->freeBlocks(2, 2);
    uut->freeBlocks(4, 2);

    EXPECT_EQ(2, uut->allocateBlocks(2));

    for (int i = 1; i < 16; ++i) {
        EXPECT_EQ(4 * i, uut->allocateBlocks(4));
    }

    for (int i = 1; i < 16; ++i) {
        uut->freeBlocks(4 * i, 4);
    }

    uut->freeBlocks(2, 2);
}

TEST(BitmapAllocatorTest, AllocatingOverOneSection)
{
    std::unique_ptr<Driver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BlockAllocator> uut(new BitmapAllocator(drv.get(), 1));

    for (int i = 2; i < 8191; ++i) {
        EXPECT_EQ(i < 8160 ? i : i + 1, uut->allocateBlock());
    }

    EXPECT_EQ(8192, uut->allocateBlock());
}

TEST(BitmapAllocatorTest, CanBeOpennedAgain)
{
    std::unique_ptr<Driver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BlockAllocator> uut(new BitmapAllocator(drv.get(), 1));

    EXPECT_EQ(8193, uut->allocateBlock());

    uut->reset();
}

TEST(BitmapAllocatorTest, CanAllocatingWithHinting)
{
    std::unique_ptr<Driver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BlockAllocator> uut(new BitmapAllocator(drv.get(), 1));

    for (int i = 64; i < 96; ++i) {
        EXPECT_EQ(i, uut->allocateBlock(i));
    }
    EXPECT_EQ(96, uut->allocateBlock(96));
    for (int i = 64; i < 96; ++i) {
        uut->freeBlock(i);
    }
    uut->freeBlock(96);

    for (int i = 8161; i < 8192; ++i) {
        EXPECT_EQ(i, uut->allocateBlock(i));
    }
    EXPECT_EQ(8128, uut->allocateBlock(8161));
    for (int i = 8161; i < 8192; ++i) {
        uut->freeBlock(i);
    }
    uut->freeBlock(8128);
}

TEST(BitmapAllocatorTest, HintAtNotExist)
{
    std::unique_ptr<Driver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BlockAllocator> uut(new BitmapAllocator(drv.get(), 1));

    for (int i = 32768; i < 32768 + 8160; ++i) {
        EXPECT_EQ(i, uut->allocateBlock(32768));
    }

    for (int i = 32768 + 8161; i < 32768 + 8192; ++i) {
        EXPECT_EQ(i, uut->allocateBlock(32768));
    }
}

TEST(BitmapAllocatorTest, FindBeforeHint)
{
    std::unique_ptr<Driver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BlockAllocator> uut(new BitmapAllocator(drv.get(), 1));

    EXPECT_EQ(24576, uut->allocateBlock(32768));
    uut->freeBlock(24576);

    for (int i = 32768; i < 32768 + 8160; ++i) {
        uut->freeBlock(i);
    }

    for (int i = 32768 + 8161; i < 32768 + 8192; ++i) {
        uut->freeBlock(i);
    }
}

TEST(BitmapAllocatorTest, AnotherStartAt)
{
    std::unique_ptr<Driver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BlockAllocator> uut(new BitmapAllocator(drv.get(), 0));
    uut->reset();

    EXPECT_EQ(1, uut->allocateBlock());

    uut->freeBlock(1);
}

TEST(BitmapAllocatorTest, TearDown)
{ std::remove(TEST_PATH); }
