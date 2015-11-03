#include <gtest/gtest.h>
#include <memory>
#include <cstring>
#include <cstdio>

#include "../test-inc.hpp"

#include "lib/driver/bitmap-allocator.hpp"
#include "lib/driver/basic-accesser.hpp"
#include "lib/driver/basic-driver.hpp"

using namespace cdb;

static const char TEST_PATH[] = TMP_PATH_PREFIX "basic-allocator-test.tmp";
static const char TEST_STRING[] = "Hello World!";

class BasicAccesserTest : public ::testing::Test
{
protected:
    static void TearDownTestCase()
    { std::remove(TEST_PATH); }
};

TEST_F(BasicAccesserTest, Access)
{
    std::unique_ptr<BasicDriver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BitmapAllocator> allocator(new BitmapAllocator(drv.get(), 0));
    allocator->reset();

    std::unique_ptr<BasicAccesser> uut(new BasicAccesser(drv.get(), allocator.get()));
    auto index = uut->allocateBlock();

    {
        auto block = uut->aquire(index);
        std::strcpy(reinterpret_cast<char*>(block.content()), TEST_STRING);
    }

    uut.reset();
    allocator.reset();
    drv.reset();

    // ----
    drv.reset(new BasicDriver(TEST_PATH));
    allocator.reset(new BitmapAllocator(drv.get(), 0));
    uut.reset(new BasicAccesser(drv.get(), allocator.get()));

    {
        auto block = uut->aquire(index);
        EXPECT_EQ(0, std::strcmp(reinterpret_cast<char*>(block.content()), TEST_STRING));
    }

    uut->freeBlock(index);
}

TEST_F(BasicAccesserTest, MultiAccess)
{
    std::unique_ptr<BasicDriver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BitmapAllocator> allocator(new BitmapAllocator(drv.get(), 0));
    std::unique_ptr<BasicAccesser> uut(new BasicAccesser(drv.get(), allocator.get()));

    auto index = uut->allocateBlock();
    auto block1 = uut->aquire(index);

    std::strcpy(reinterpret_cast<char*>(block1.content()), TEST_STRING);

    {
        auto block2 = uut->aquire(index);
        EXPECT_EQ(0, std::strcmp(reinterpret_cast<char*>(block2.content()), TEST_STRING));
    }

    EXPECT_EQ(0, std::strcmp(reinterpret_cast<char*>(block1.content()), TEST_STRING));

    uut->freeBlock(index);
}

TEST_F(BasicAccesserTest, Free)
{
    std::unique_ptr<BasicDriver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BitmapAllocator> allocator(new BitmapAllocator(drv.get(), 0));
    std::unique_ptr<BasicAccesser> uut(new BasicAccesser(drv.get(), allocator.get()));

    auto block = uut->aquire(uut->allocateBlock());
    uut->freeBlock(block.index());

    block = uut->aquire(uut->allocateBlock());
}

TEST_F(BasicAccesserTest, BlockSelfAssignment)
{
    std::unique_ptr<BasicDriver> drv(new BasicDriver(TEST_PATH));
    std::unique_ptr<BitmapAllocator> allocator(new BitmapAllocator(drv.get(), 0));
    std::unique_ptr<BasicAccesser> uut(new BasicAccesser(drv.get(), allocator.get()));

    auto block = uut->aquire(uut->allocateBlock());

    std::strcpy(reinterpret_cast<char*>(block.content()), TEST_STRING);
    block = block;
    EXPECT_EQ(0, std::strcmp(TEST_STRING, reinterpret_cast<char*>(block.content())));
    *block.content() = 'V';
    EXPECT_EQ('V', *block.content());
}
