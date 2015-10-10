#include <gtest/gtest.h>
#include <memory>
#include <cstring>
#include <cstdio>

#include "lib/driver/bitmap-allocator.hpp"
#include "lib/driver/basic-accesser.hpp"
#include "lib/driver/basic-driver.hpp"

using namespace cdb;

static const char TEST_PATH[] = "/tmp/basic-allocator-test.tmp";
static const char TEST_STRING[] = "Hello World!";

TEST(BasicAccesserTest, Access)
{
    std::unique_ptr<BasicDriver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BitmapAllocator> allocator(new BitmapAllocator(drv.get(), 0));
    allocator->reset();

    std::unique_ptr<BasicAccesser> uut(new BasicAccesser(drv.get(), allocator.get()));

    auto index = uut->allocateBlock();
    auto slice = uut->access(index);

    std::strcpy(reinterpret_cast<char*>(slice.content()), TEST_STRING);

    uut.reset();
    allocator.reset();
    drv.reset();

    // ----
    drv.reset(new BasicDriver(TEST_PATH));
    allocator.reset(new BitmapAllocator(drv.get(), 0));
    uut.reset(new BasicAccesser(drv.get(), allocator.get()));

    slice = uut->access(index);

    EXPECT_EQ(0, std::strcmp(reinterpret_cast<char*>(slice.content()), TEST_STRING));
    uut->freeBlock(index);
}

TEST(BasicAccesserTest, MultiAccess)
{
    std::unique_ptr<BasicDriver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BitmapAllocator> allocator(new BitmapAllocator(drv.get(), 0));
    std::unique_ptr<BasicAccesser> uut(new BasicAccesser(drv.get(), allocator.get()));

    auto index = uut->allocateBlock();
    auto slice1 = uut->access(index);
    auto slice2 = uut->access(index);

    std::strcpy(reinterpret_cast<char*>(slice1.content()), TEST_STRING);
    EXPECT_EQ(0, std::strcmp(reinterpret_cast<char*>(slice2.content()), TEST_STRING));

    uut->release(index);
    EXPECT_EQ(0, std::strcmp(reinterpret_cast<char*>(slice1.content()), TEST_STRING));

    uut->release(index);
    uut->freeBlock(index);
}

TEST(BasicAccesserTest, TearDown)
{ std::remove(TEST_PATH); }
