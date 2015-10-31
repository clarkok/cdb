#include <gtest/gtest.h>
#include <memory>
#include <algorithm>
#include <iostream>

#include "../test-inc.hpp"
#include "lib/index/skip-table.hpp"

using namespace cdb;

static const int SMALL_NUMBER = 10;
static const int LARGE_NUMBER = 10000;  // tested up to 1,000,000. make it small to less the test time

class SkipTableTest : public ::testing::Test
{
protected:
    std::unique_ptr<SkipTable> uut;

    SkipTableTest()
        : uut(new SkipTable(
                    [](ConstSlice a, ConstSlice b) -> bool
                    {
                        return 
                            *reinterpret_cast<const int*>(a.content()) <
                            *reinterpret_cast<const int*>(b.content());
                    }
                ))
    { }

    template <typename T>
    ConstSlice toConstSlice(T &value)
    { return ConstSlice(reinterpret_cast<const Byte*>(&value), sizeof(T)); }
};

TEST_F(SkipTableTest, Insert)
{
    for (int i = 0; i < SMALL_NUMBER; ++i) {
        auto iter = uut->insert(toConstSlice(i));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));
    }
}

TEST_F(SkipTableTest, InsertReverse)
{
    for (int i = SMALL_NUMBER; i; --i) {
        auto iter = uut->insert(toConstSlice(i));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));
    }
}

TEST_F(SkipTableTest, Iterator)
{
    for (int i = 0; i < SMALL_NUMBER; ++i) {
        uut->insert(toConstSlice(i));
    }

    auto iter = uut->begin();
    for (int i = 0; i < SMALL_NUMBER; ++i) {
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));
        iter = iter.next();
    }
}

TEST_F(SkipTableTest, IteratorOnReverseInsertion)
{
    for (int i = SMALL_NUMBER; i; --i) {
        uut->insert(toConstSlice(i));
    }

    auto iter = uut->begin();
    for (int i = 1; i <= SMALL_NUMBER; ++i) {
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));
        iter = iter.next();
    }
}

TEST_F(SkipTableTest, Lookup)
{
    for (int i = 0; i < SMALL_NUMBER; ++i) {
        uut->insert(toConstSlice(i));
    }

    for (int i = 0; i < SMALL_NUMBER; ++i) {
        auto iter = uut->lowerBound(toConstSlice(i));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));

        iter = uut->upperBound(toConstSlice(i));
        if (i == SMALL_NUMBER - 1) {
            EXPECT_EQ(nullptr, iter->content());
        }
        else {
            ASSERT_TRUE(iter->content());
            EXPECT_EQ(i + 1, *reinterpret_cast<int*>(iter->content()));
        }
    }
}

TEST_F(SkipTableTest, RandomOrder)
{
    std::vector<int> v;
    for (int i = 0; i < SMALL_NUMBER; ++i) {
        v.push_back(i);
    }
    std::shuffle(v.begin(), v.end(), std::default_random_engine());

    for (auto &n : v) {
        uut->insert(toConstSlice(n));
    }

    auto iter = uut->begin();
    for (int i = 0; i < SMALL_NUMBER; ++i) {
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));
        iter = iter.next();
    }

    for (int i = 0; i < SMALL_NUMBER; ++i) {
        auto iter = uut->lowerBound(toConstSlice(i));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));

        iter = uut->upperBound(toConstSlice(i));
        if (i == 9) {
            EXPECT_EQ(nullptr, iter->content());
        }
        else {
            ASSERT_TRUE(iter->content());
            EXPECT_EQ(i + 1, *reinterpret_cast<int*>(iter->content()));
        }
    }
}

TEST_F(SkipTableTest, InsertSameValue)
{
    for (int i = 0; i < SMALL_NUMBER; ++i) {
        for (int j = 0; j < i; ++j) {
            uut->insert(toConstSlice(i));
        }
    }

    for (int i = 0; i < SMALL_NUMBER; ++i) {
        auto begin = uut->lowerBound(toConstSlice(i));
        auto end = uut->upperBound(toConstSlice(i));

        int count = 0;
        while (begin != end) {
            EXPECT_EQ(i, *reinterpret_cast<int*>(begin->content()));
            count ++;
            begin = begin.next();
        }

        EXPECT_EQ(i, count);
    }
}

TEST_F(SkipTableTest, LargeNumber)
{
    std::vector<int> v;
    for (int i = 0; i < LARGE_NUMBER; ++i) {
        v.push_back(i);
    }
    std::shuffle(v.begin(), v.end(), std::default_random_engine());

    for (auto &n : v) {
        uut->insert(toConstSlice(n));
    }

    auto iter = uut->begin();
    for (int i = 0; i < LARGE_NUMBER; ++i) {
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));
        iter = iter.next();
    }

    for (int i = 0; i < LARGE_NUMBER; ++i) {
        auto iter = uut->lowerBound(toConstSlice(i));
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));

        iter = uut->upperBound(toConstSlice(i));
        if (i == LARGE_NUMBER - 1) {
            EXPECT_EQ(nullptr, iter->content());
        }
        else {
            ASSERT_TRUE(iter->content());
            EXPECT_EQ(i + 1, *reinterpret_cast<int*>(iter->content()));
        }
    }
}
