#include <gtest/gtest.h>
#include <memory>
#include <algorithm>
#include <iostream>
#include <set>

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

    void dump()
    {
        uut->__debug_output(
                std::cerr,
                [] (std::ostream &os, ConstSlice value) {
                    os << *reinterpret_cast<const int*>(value.content()) << "\t";
                }
            );
        std::cerr << std::endl;
    }

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

TEST_F(SkipTableTest, ReverseIterator)
{
    for (int i = SMALL_NUMBER; i; --i) {
        uut->insert(toConstSlice(i));
    }

    auto iter = uut->end().prev();
    for (int i = SMALL_NUMBER; i; i--) {
        EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));
        iter = iter.prev();
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

TEST_F(SkipTableTest, Erase)
{
    for (int j = 0; j < SMALL_NUMBER * SMALL_NUMBER; ++j) {
        for (int i = 0; i < SMALL_NUMBER; ++i) {
            uut->insert(toConstSlice(i));
        }

        auto iter = uut->begin();
        for (int i = 0; i < SMALL_NUMBER; ++i) {
            EXPECT_EQ(i, *reinterpret_cast<int*>(iter->content()));
            iter = uut->erase(iter);
        }
    }
}

TEST_F(SkipTableTest, LargeRandomTest) 
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

        auto lower_bound = uut->lowerBound(toConstSlice(i));
        EXPECT_EQ(i, *reinterpret_cast<int*>(lower_bound->content()));

        auto upper_bound = uut->upperBound(toConstSlice(i));
        if (i == LARGE_NUMBER - 1) {
            EXPECT_FALSE(upper_bound->content());
        }
        else {
            EXPECT_EQ(i + 1, *reinterpret_cast<int*>(upper_bound->content()));
        }
    }

    std::shuffle(v.begin(), v.end(), std::default_random_engine());
    for (auto &n : v) {
        auto iter = uut->lowerBound(toConstSlice(n));
        iter = uut->erase(iter);
        if (iter->content()) {
            EXPECT_TRUE(n < *reinterpret_cast<int*>(iter->content()));
        }
    }
}

TEST_F(SkipTableTest, TimeMeansureCompareToMultiSet)
{
    std::multiset<int> s;
    std::vector<int> v;
    for (int i = 0; i < LARGE_NUMBER; ++i) {
        v.push_back(i);
    }
    std::shuffle(v.begin(), v.end(), std::default_random_engine());

    for (auto &n : v) {
        s.insert(n);
    }

    auto iter = s.begin();
    for (int i = 0; i < LARGE_NUMBER; ++i) {
        EXPECT_EQ(i, *iter);
        iter++;

        auto lower_bound = s.lower_bound(i);
        EXPECT_EQ(i, *lower_bound);

        auto upper_bound = s.upper_bound(i);
        if (i == LARGE_NUMBER - 1) {
            EXPECT_FALSE(upper_bound != s.end());
        }
        else {
            EXPECT_EQ(i + 1, *upper_bound);
        }
    }

    std::shuffle(v.begin(), v.end(), std::default_random_engine());
    for (auto &n : v) {
        auto iter = s.lower_bound(n);
        iter = s.erase(iter);
        if (iter != s.end()) {
            EXPECT_TRUE(n < *iter);
        }
    }
}
