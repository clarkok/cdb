#include <gtest/gtest.h>
#include <lib/utils/comparator.hpp>
#include "lib/driver/basic-driver.hpp"
#include "lib/driver/bitmap-allocator.hpp"
#include "lib/driver/basic-accesser.hpp"
#include "lib/table/table.hpp"

#include "../test-inc.hpp"

using namespace cdb;

static const char TEST_PATH[] = TMP_PATH_PREFIX "/table-test.tmp";

class TableTest : public ::testing::Test
{
protected:
    std::unique_ptr<Driver> driver;
    std::unique_ptr<BlockAllocator> allocator;
    std::unique_ptr<DriverAccesser> accesser;
    std::unique_ptr<Schema> schema;
    std::unique_ptr<Table> uut;

    TableTest()
            : driver(new BasicDriver(TEST_PATH)),
              allocator(new BitmapAllocator(driver.get(), 0)),
              accesser(new BasicAccesser(driver.get(), allocator.get()))
    {
        allocator->reset();
        schema.reset(Schema::Factory()
                .addIntegerField("id")
                .addCharField("name", 16)
                .addFloatField("gpa")
                .addIntegerField("gender")
                .setPrimary("id")
                .release());

        auto root = allocator->allocateBlock();

        std::unique_ptr<BTree> tree(
                new BTree(
                        accesser.get(),
                        Comparator::getIntegerCompareFuncLT(),
                        Comparator::getIntegerCompareFuncEQ(),
                        root,
                        sizeof(int),
                        schema->getRecordSize()
                )
        );
        tree->reset();
        tree.reset();

        uut.reset(Table::Factory(accesser.get(), schema->copy(), root).release());
    }
};

TEST_F(TableTest, insert)
{
    std::unique_ptr<Table::RecordBuilder> builder(uut->getRecordBuilder(
            {
                    "id",
                    "name",
                    "gpa",
                    "gender",
            }
    ));

    builder->addRow()
            .addValue("0")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("0")
        ;

    uut->insert(builder->getSchema(), builder->getRows());

    builder->reset()
            .addRow()
            .addValue("1")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            .addRow()
            .addValue("2")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
        ;

    uut->insert(builder->getSchema(), builder->getRows());
}

TEST_F(TableTest, select)
{
    std::unique_ptr<Table::RecordBuilder> builder(uut->getRecordBuilder(
            {
                    "id",
                    "name",
                    "gpa",
                    "gender",
            }
    ));

    builder->addRow()
            .addValue("0")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            .addRow()
            .addValue("1")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            .addRow()
            .addValue("2")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            ;

    uut->insert(builder->getSchema(), builder->getRows());

    int count = 0;

    uut->select(
            nullptr,
            nullptr,
            [&](ConstSlice row)
            {
                auto id_col = schema->getPrimaryColumn();
                auto id = Convert::toString(id_col.getType(), id_col.getValue(row));
                EXPECT_EQ(std::to_string(count ++), id);

                auto name_col = schema->getColumnByName("name");
                auto name = Convert::toString(name_col.getType(), name_col.getValue(row));
                EXPECT_EQ("lalala", name);

                auto gpa_col = schema->getColumnByName("gpa");
                auto gpa = Convert::toString(gpa_col.getType(), gpa_col.getValue(row));
                auto gpa_float = std::stof(gpa);
                EXPECT_TRUE(0.00001 > std::abs(gpa_float - 1.0));

                auto gender_col = schema->getColumnByName("gender");
                auto gender = Convert::toString(gender_col.getType(), gender_col.getValue(row));
                EXPECT_EQ("1", gender);
            }
    );
    EXPECT_EQ(3, count);
}

TEST_F(TableTest, selectWithSchema)
{
    std::unique_ptr<Table::RecordBuilder> builder(uut->getRecordBuilder(
            {
                    "id",
                    "name",
                    "gpa",
                    "gender",
            }
    ));

    builder->addRow()
            .addValue("0")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            .addRow()
            .addValue("1")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            .addRow()
            .addValue("2")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            ;

    uut->insert(builder->getSchema(), builder->getRows());

    int count = 0;

    uut->select(
            uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "name"}),
            nullptr,
            [&](ConstSlice row)
            {
                auto id_col = schema->getPrimaryColumn();
                auto id = Convert::toString(id_col.getType(), id_col.getValue(row));
                EXPECT_EQ(std::to_string(count ++), id);

                auto name_col = schema->getColumnByName("name");
                auto name = Convert::toString(name_col.getType(), name_col.getValue(row));
                EXPECT_EQ("lalala", name);
            }
    );
    EXPECT_EQ(3, count);
}

TEST_F(TableTest, selectWithCondition)
{
    std::unique_ptr<Table::RecordBuilder> builder(uut->getRecordBuilder(
            {
                    "id",
                    "name",
                    "gpa",
                    "gender",
            }
    ));

    builder->addRow()
            .addValue("0")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            .addRow()
            .addValue("1")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            .addRow()
            .addValue("2")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            ;

    uut->insert(builder->getSchema(), builder->getRows());

    int count = 0;

    uut->select(
            uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "name"}),
            new CompareExpr("id", CompareExpr::Operator::EQ, "0"),
            [&](ConstSlice row)
            {
                auto id_col = schema->getPrimaryColumn();
                auto id = Convert::toString(id_col.getType(), id_col.getValue(row));
                EXPECT_EQ(std::to_string(count ++), id);

                auto name_col = schema->getColumnByName("name");
                auto name = Convert::toString(name_col.getType(), name_col.getValue(row));
                EXPECT_EQ("lalala", name);
            }
    );
    EXPECT_EQ(1, count);
}
