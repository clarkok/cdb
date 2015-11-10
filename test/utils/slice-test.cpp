#include <gtest/gtest.h>
#include <cstring>

#include "lib/utils/slice.hpp"

using namespace cdb;

using cdb::Length;
using cdb::Byte;

static const Length TEST_LENGTH = 128;
static const char TEST_STRING[] = "Hello World";

TEST(SliceTest, Constructor)
{
    Buffer buffer(TEST_LENGTH);
    Slice uut(buffer);
    EXPECT_EQ(TEST_LENGTH, uut.length());

    std::strcpy(reinterpret_cast<char*>(buffer.content()), TEST_STRING);
    EXPECT_EQ(0, std::strcmp(reinterpret_cast<const char*>(uut.content()), TEST_STRING));
}

TEST(SliceTest, Iterator)
{
    Buffer buffer(TEST_LENGTH);
    Slice uut(buffer);

    EXPECT_EQ(buffer.begin(), uut.begin());
    EXPECT_EQ(buffer.end(), uut.end());
    EXPECT_EQ(buffer.cbegin(), uut.cbegin());
    EXPECT_EQ(buffer.cend(), uut.cend());
}

TEST(SliceTest, SubSlice)
{
    Buffer buffer(TEST_LENGTH);
    std::strcpy(reinterpret_cast<char*>(buffer.content()), TEST_STRING);

    Slice original(buffer);

    Slice sub_slice_with_only_index = original.subSlice(6);
    EXPECT_EQ(TEST_LENGTH - 6, sub_slice_with_only_index.length());
    EXPECT_EQ(TEST_STRING[6], sub_slice_with_only_index.content()[0]);

    Slice sub_slice_with_index_and_length = original.subSlice(6, 6);
    EXPECT_EQ(6, sub_slice_with_index_and_length.length());
    EXPECT_EQ(TEST_STRING[6], sub_slice_with_only_index.content()[0]);
}

TEST(ConstSliceTest, Constructor)
{
    Buffer buffer(TEST_LENGTH);
    std::strcpy(reinterpret_cast<char*>(buffer.content()), TEST_STRING);

    Slice slice(buffer);

    ConstSlice uut_by_buffer(buffer);
    EXPECT_EQ(TEST_LENGTH, uut_by_buffer.length());
    EXPECT_EQ(buffer.content(), uut_by_buffer.content());
    EXPECT_EQ(0, std::strcmp(reinterpret_cast<const char*>(uut_by_buffer.content()), TEST_STRING));

    ConstSlice uut_by_slice(slice);
    EXPECT_EQ(TEST_LENGTH, uut_by_slice.length());
    EXPECT_EQ(slice.content(), uut_by_slice.content());
    EXPECT_EQ(0, std::strcmp(reinterpret_cast<const char*>(uut_by_slice.content()), TEST_STRING));
}

TEST(ConstSliceTest, Iterator)
{
    Buffer buffer(TEST_LENGTH);
    ConstSlice uut(buffer);

    EXPECT_EQ(buffer.cbegin(), uut.cbegin());
    EXPECT_EQ(buffer.cend(), uut.cend());
}

TEST(ConstSliceTest, SubSlice)
{
    Buffer buffer(TEST_LENGTH);
    std::strcpy(reinterpret_cast<char*>(buffer.content()), TEST_STRING);

    ConstSlice original(buffer);

    ConstSlice sub_slice_with_only_index = original.subSlice(6);
    EXPECT_EQ(TEST_LENGTH - 6, sub_slice_with_only_index.length());
    EXPECT_EQ(TEST_STRING[6], sub_slice_with_only_index.content()[0]);

    ConstSlice sub_slice_with_index_and_length = original.subSlice(6, 6);
    EXPECT_EQ(6, sub_slice_with_index_and_length.length());
    EXPECT_EQ(TEST_STRING[6], sub_slice_with_only_index.content()[0]);
}
