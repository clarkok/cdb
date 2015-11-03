#include <gtest/gtest.h>

#include "lib/table/skip-view.hpp"

using namespace cdb;

static const int TEST_NUMBER = 10;

class SkipViewTest : public ::testing::Test
{
protected:
    std::unique_ptr<SkipView> uut;

    SkipViewTest()
    {
        SkipTable *table = new SkipTable(
                0,
                SkipView::getIntegerCompareFunc()
        );

        for (int i = 0; i < TEST_NUMBER; ++i) {
            table->insert(toConstSlice(i));
        }

        uut.reset(new SkipView(
                Schema::Factory().addIntegerField("test").release(),
                table
        ));
    }

    template <typename T>
    SkipTable::Key toKey(T &value)
    { return reinterpret_cast<SkipTable::Key>(&value); }

    template <typename T>
    ConstSlice toConstSlice(T &value)
    { return ConstSlice(reinterpret_cast<SkipTable::Key>(&value), static_cast<Length>(sizeof(T))); }

    int toInt(SkipView::Iterator &iter)
    { return *reinterpret_cast<const int*>(iter.slice().content()); }
};

TEST_F(SkipViewTest, Iterators)
{
    auto iter = uut->begin();
    for (int i = 0; i < TEST_NUMBER; ++i) {
        EXPECT_EQ(i, toInt(iter));
        iter.next();
    }
    EXPECT_TRUE(iter == uut->end());
}

TEST_F(SkipViewTest, Intersect)
{
    SkipTable *table = new SkipTable(
            0,
            SkipView::getIntegerCompareFunc()
    );

    for (int i = -TEST_NUMBER; i < TEST_NUMBER + 5; i += 2) {
        table->insert(toConstSlice(i));
    }

    SkipView another(
            Schema::Factory().addIntegerField("test").release(),
            table
    );

    uut->intersect(another.begin(), another.end());

    auto iter = uut->begin();
    for (int i = 0; i < TEST_NUMBER; i += 2) {
        EXPECT_EQ(i, toInt(iter));
        iter.next();
    }
    EXPECT_TRUE(iter == uut->end());
}

TEST_F(SkipViewTest, Join)
{
    SkipTable *table = new SkipTable(
            0,
            SkipView::getIntegerCompareFunc()
    );

    for (int i = -TEST_NUMBER; i < 0; ++i) {
        table->insert(toConstSlice(i));
    }

    for (int i = 0; i < TEST_NUMBER; i += 2) {
        table->insert(toConstSlice(i));
    }

    for (int i = TEST_NUMBER; i < TEST_NUMBER + 5; ++i) {
        table->insert(toConstSlice(i));
    }

    SkipView another(
            Schema::Factory().addIntegerField("test").release(),
            table
    );

    uut->join(another.begin(), another.end());

    auto iter = uut->begin();
    for (int i = -TEST_NUMBER; i < TEST_NUMBER + 5; ++i) {
        EXPECT_EQ(i, toInt(iter));
        iter.next();
    }
    EXPECT_TRUE(iter == uut->end());
}

struct SkipViewTestStruct
{
    int id;
    int padding;
    int value;
};

TEST_F(SkipViewTest, peek)
{
    SkipTable *table = new SkipTable(
            0,
            SkipView::getIntegerCompareFunc()
    );

    for (int i = 0; i < TEST_NUMBER; ++i) {
        SkipViewTestStruct value = {i, i + 1, i * 2};
        table->insert(toConstSlice(value));
    }
    Schema *schema = Schema::Factory()
            .addIntegerField("id")
            .addIntegerField("padding")
            .addIntegerField("value")
            .release();

    Schema::Column value_col = schema->getColumnByName("value");

    SkipView uut(
            schema,     // evil, don't do it elsewhere
            table
    );

    int lower_bound = TEST_NUMBER / 2;
    int upper_bound = TEST_NUMBER + TEST_NUMBER / 2;

    SkipView *ret = dynamic_cast<SkipView*>(uut.peek(
            value_col,
            toKey(lower_bound),
            toKey(upper_bound)
    ));

    auto iter = ret->begin();
    for (int i = ((lower_bound + 1) >> 1); i < ((upper_bound + 1) >> 1); ++i) {
        EXPECT_EQ(i, toInt(iter));
        iter.next();
    }
    EXPECT_TRUE(iter == ret->end());

    delete ret;
}