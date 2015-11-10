#include <gtest/gtest.h>
#include <cstdio>

#include "lib/database/database.hpp"
#include "../test-inc.hpp"

using namespace cdb;

static const char TEST_PATH[] = TMP_PATH_PREFIX "/database-test.tmp";

class DatabaseTest : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    { std::remove(TEST_PATH); }
};

TEST_F(DatabaseTest, init)
{
    std::unique_ptr<Database> uut(Database::Factory(TEST_PATH));
    uut->init();
}

TEST_F(DatabaseTest, createTable)
{
    std::unique_ptr<Database> uut(Database::Factory(TEST_PATH));
    uut->init();

    Table *table = uut->createTable(
            "test_table",
            Schema::Factory()
                .addIntegerField("id")
                .addCharField("name", 16)
                .addFloatField("gpa")
                .addIntegerField("gender")
                .release()
        );

    std::unique_ptr<Table::RecordBuilder> builder(
            table->getRecordBuilder({
                "id",
                "name",
                "gpa",
                "gender"
            })
        );

    builder->addRow()
            .addInteger(0)
            .addChar("lalala")
            .addFloat(0)
            .addInteger(0)
            .addRow()
            .addInteger(1)
            .addChar("lalala")
            .addFloat(1)
            .addInteger(1)
            .addRow()
            .addInteger(2)
            .addChar("lalala")
            .addFloat(2)
            .addInteger(2)
            ;

    table->insert(builder->getSchema(), builder->getRows());

    std::unique_ptr<Schema> schema(builder->getSchema()->copy());

    int count = 0;
    table->select(
            nullptr,
            nullptr,
            [&](ConstSlice row)
            {
                auto id_col = schema->getColumnById(0);
                auto id = Convert::toString(id_col.getType(), id_col.getValue(row));
                EXPECT_EQ(std::to_string(count), id);
                ++count;
            }
        );
    EXPECT_EQ(3, count);

    table->createIndex("gpa", "gpaidx");
}

TEST_F(DatabaseTest, OpenAgain)
{
    std::unique_ptr<Database> uut(Database::Factory(TEST_PATH));

    Table *table = uut->getTableByName("test_table");
    std::unique_ptr<Schema> schema(table->getSchema()->copy());

    int count = 0;
    table->select(
            nullptr,
            nullptr,
            [&](ConstSlice row)
            {
                auto id_col = schema->getColumnById(0);
                auto id = Convert::toString(id_col.getType(), id_col.getValue(row));
                EXPECT_EQ(std::to_string(count), id);
                ++count;
            }
        );
    EXPECT_EQ(3, count);

    auto table_name = uut->indexFor("gpaidx");
    EXPECT_EQ("test_table", table_name);
    table = uut->getTableByName(table_name);
    table->dropIndex("gpaidx");

    uut.reset();
    uut.reset(Database::Factory(TEST_PATH));

    {
            auto *table = uut->getTableByName("test_table");

            int count = 0;
            table->select(
                    nullptr,
                    nullptr,
                    [&](ConstSlice row)
                    {
                        auto id_col = schema->getColumnById(0);
                        auto id = Convert::toString(id_col.getType(), id_col.getValue(row));
                        EXPECT_EQ(std::to_string(count), id);
                        ++count;
                    }
                );
            EXPECT_EQ(3, count);
    }
}
