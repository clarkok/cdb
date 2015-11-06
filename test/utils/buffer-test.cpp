#include <gtest/gtest.h>
#include <cstring>

#include "lib/utils/buffer.hpp"

using namespace cdb;

using cdb::Length;
using cdb::Byte;

static const Length TEST_LENGTH = 128;
static const char TEST_STRING[] = "Hello World";

TEST(BufferTest, Constructor)
{
    Buffer uut_by_length(TEST_LENGTH);
    EXPECT_EQ(TEST_LENGTH, uut_by_length.length());

    std::strcpy(reinterpret_cast<char*>(uut_by_length.content()), TEST_STRING);
    EXPECT_EQ(0, std::strcmp(reinterpret_cast<const char*>(uut_by_length.content()), TEST_STRING));

    Buffer uut_by_other_Buffer(uut_by_length);
    EXPECT_EQ(TEST_LENGTH, uut_by_other_Buffer.length());
    EXPECT_EQ(0, std::strcmp(reinterpret_cast<const char*>(uut_by_other_Buffer.content()), TEST_STRING));

    Buffer uut_by_begin_end(uut_by_length.cbegin(), uut_by_length.cend());
    EXPECT_EQ(TEST_LENGTH, uut_by_begin_end.length());
    EXPECT_EQ(0, std::strcmp(reinterpret_cast<const char*>(uut_by_begin_end.content()), TEST_STRING));
}

TEST(BufferTest, Iterator)
{
    Buffer uut(
            reinterpret_cast<const Byte*>(TEST_STRING),
            reinterpret_cast<const Byte*>(TEST_STRING + std::strlen(TEST_STRING) + 1)
        );

    EXPECT_EQ(std::strlen(TEST_STRING) + 1, uut.cend() - uut.cbegin());
    EXPECT_EQ(std::strlen(TEST_STRING) + 1, uut.end() - uut.begin());
    EXPECT_EQ(0, std::strcmp(TEST_STRING, reinterpret_cast<const char*>(uut.cbegin())));
    EXPECT_EQ(0, std::strcmp(TEST_STRING, reinterpret_cast<const char*>(uut.begin())));
}

TEST(BufferTest, OwnAPieceOfMemory)
{
    std::unique_ptr<Buffer> uut_ptr1(new Buffer(TEST_LENGTH));
    std::strcpy(reinterpret_cast<char*>(uut_ptr1->content()), TEST_STRING);

    std::unique_ptr<Buffer> uut_ptr2(new Buffer(*uut_ptr1));

    EXPECT_EQ(0, std::strcmp(reinterpret_cast<const char*>(uut_ptr1->content()), TEST_STRING));
    uut_ptr1.reset();
    EXPECT_EQ(0, std::strcmp(reinterpret_cast<const char*>(uut_ptr2->content()), TEST_STRING));
}

TEST(BufferTest, CopyOnWrite)
{
    Buffer uut1(TEST_LENGTH);
    std::strcpy(reinterpret_cast<char*>(uut1.content()), TEST_STRING);

    Buffer uut2(uut1);
    EXPECT_EQ(uut1.cbegin(), uut2.cbegin());
    EXPECT_EQ(uut1.cend(), uut2.cend());

    uut2.content()[0] = 'V';
    EXPECT_EQ(0, std::strcmp(reinterpret_cast<const char*>(uut1.content()), TEST_STRING));
    EXPECT_EQ('V', uut2.content()[0]);
    EXPECT_EQ('e', uut2.content()[1]);
    EXPECT_EQ('l', uut2.content()[2]);
}

TEST(BufferTest, Copy)
{
    Buffer uut(TEST_LENGTH);
    
    for (int i = 0; i < TEST_LENGTH; ++i) {
        Buffer uut1(uut);
        *reinterpret_cast<int*>(uut1.content()) = i;
        uut = uut1;
        EXPECT_EQ(i, *reinterpret_cast<const int*>(uut.content()));
    }
}

TEST(BufferTest, Assign)
{
    Buffer uut(TEST_LENGTH);
    Buffer uut1(TEST_LENGTH);
    
    for (int i = 0; i < TEST_LENGTH; ++i) {
        uut1 = uut;
        if (i) {
            EXPECT_EQ(i - 1, *reinterpret_cast<int *>(uut1.content()));
        }
        *reinterpret_cast<int *>(uut1.content()) = i;
        uut = uut1;
        EXPECT_EQ(i, *reinterpret_cast<int *>(uut.content()));
    }
}
