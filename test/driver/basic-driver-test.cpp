#include <gtest/gtest.h>
#include <cstring>

#include "lib/driver/basic-driver.hpp"

using namespace cdb;

static const char TEST_PATH[] = "/tmp/basic-driver-test.tmp";
static const char TEST_STRING[] = "Hello world";
static const int MULTIPLE_TIME = 10;

TEST(BasicDriverTest, WriteSingle)
{
    BasicDriver uut(TEST_PATH);

    Buffer buffer(Driver::BLOCK_SIZE);
    std::strcpy(reinterpret_cast<char*>(buffer.content()), TEST_STRING);

    uut.writeBlock(0, buffer);
}

TEST(BasicDriverTest, ReadSingle)
{
    BasicDriver uut(TEST_PATH);

    Buffer buffer(Driver::BLOCK_SIZE);

    uut.readBlock(0, buffer);

    EXPECT_EQ(0, std::strcmp(reinterpret_cast<const char*>(buffer.content()), TEST_STRING));
}

TEST(BasicDriverTest, WriteMultple)
{
    BasicDriver uut(TEST_PATH);

    Buffer buffer(Driver::BLOCK_SIZE * MULTIPLE_TIME);

    for (int i = 0; i < MULTIPLE_TIME; ++i) {
        std::strcpy(reinterpret_cast<char*>(buffer.content() + i * Driver::BLOCK_SIZE), TEST_STRING);
    }

    uut.writeBlocks(0, MULTIPLE_TIME, buffer);
}

TEST(BasicDriverTest, ReadMultple)
{
    BasicDriver uut(TEST_PATH);

    Buffer buffer(Driver::BLOCK_SIZE * MULTIPLE_TIME);

    uut.readBlocks(0, MULTIPLE_TIME, buffer);

    for (int i = 0; i < MULTIPLE_TIME; ++i) {
        EXPECT_EQ(0, 
                std::strcmp(reinterpret_cast<char*>(buffer.content() + i * Driver::BLOCK_SIZE),
                    TEST_STRING));
    }
}
