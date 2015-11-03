#include <gtest/gtest.h>

#include "lib/table/skip-view.hpp"

using namespace cdb;

static const int TEST_NUMBER = 10;

struct ViewTestStruct
{
    int id;
    int padding;
    int value;
};

TEST(ViewTest, Select)
{
    SkipTable *table = new SkipTable(
            0,
            SkipView::getIntegerCompareFunc()
    );

    for (int i = 0; i < TEST_NUMBER; ++i) {
        ViewTestStruct value = {i, i + 1, i * 2};
        table->insert(ConstSlice(reinterpret_cast<const Byte *>(&value), sizeof(value)));
    }
    Schema *schema = Schema::Factory()
            .addIntegerField("id")
            .addIntegerField("padding")
            .addIntegerField("value")
            .release();

    SkipView uut(
            schema,     // evil, don't do it elsewhere
            table
    );

    Schema *schema_to_select = Schema::Factory()
            .addIntegerField("id")
            .addIntegerField("value")
            .release();

    auto id_col = schema_to_select->getColumnByName("id");
    auto value_col = schema_to_select->getColumnByName("value");

    auto *ret = uut.select(schema_to_select);

    auto iter = ret->begin();
    EXPECT_EQ(2 * sizeof(int), iter.slice().length());
    for (int i = 0; i < TEST_NUMBER; ++i) {
        EXPECT_EQ(i, *reinterpret_cast<const int*>(id_col.getValue(iter.slice()).content()));
        EXPECT_EQ(i * 2, *reinterpret_cast<const int*>(value_col.getValue(iter.slice()).content()));
        iter.next();
    }
    EXPECT_TRUE(iter == ret->end());

    delete ret;
}
