#include <gtest/gtest.h>
#include <lib/utils/comparator.hpp>
#include <cmath>
#include "lib/driver/basic-driver.hpp"
#include "lib/driver/bitmap-allocator.hpp"
#include "lib/driver/basic-accesser.hpp"
#include "lib/table/table.hpp"

#include "../test-inc.hpp"

using namespace cdb;

static const char TEST_PATH[] = TMP_PATH_PREFIX "/table-test.tmp";
static const int SMALL_NUMBER = 10;
static const int LARGE_NUMBER = 10000;

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
        uut.reset(Table::Factory(accesser.get(), schema->copy(), root).release());
        uut->init();
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
                EXPECT_TRUE(0.00001 > std::fabs(gpa_float - 1.0));

                auto gender_col = schema->getColumnByName("gender");
                auto gender = Convert::toString(gender_col.getType(), gender_col.getValue(row));
                EXPECT_EQ("1", gender);
            }
    );
    EXPECT_EQ(3, count);

    builder->reset()
            .addRow()
            .addValue("3")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            .addRow()
            .addValue("4")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            .addRow()
            .addValue("5")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            ;
    uut->insert(builder->getSchema(), builder->getRows());

    count = 0;
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
                EXPECT_TRUE(0.00001 > std::fabs(gpa_float - 1.0));

                auto gender_col = schema->getColumnByName("gender");
                auto gender = Convert::toString(gender_col.getType(), gender_col.getValue(row));
                EXPECT_EQ("1", gender);
            }
    );
    EXPECT_EQ(6, count);
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
            .addValue("0")
            .addRow()
            .addValue("2")
            .addValue("lalala")
            .addValue("1.0")
            .addValue("1")
            ;

    uut->insert(builder->getSchema(), builder->getRows());

    std::unique_ptr<ConditionExpr> condition(
            new CompareExpr("id", CompareExpr::Operator::EQ, "0")
    );

    int count = 0;
    uut->select(
            uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "name"}),
            condition.get(),
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

    condition.reset(
            new CompareExpr("name", CompareExpr::Operator::EQ, "lalala")
    );
    count = 0;
    uut->select(
            uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "name"}),
            condition.get(),
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

    condition.reset(
            new AndExpr(
                    new CompareExpr("id", CompareExpr::Operator::LT, "2"),
                    new CompareExpr("gender", CompareExpr::Operator::EQ, "1")
            )
    );
    count = 0;
    uut->select(
            uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "name"}),
            condition.get(),
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

TEST_F(TableTest, index)
{
    std::unique_ptr<Table::RecordBuilder> builder(uut->getRecordBuilder(
            {
                    "id",
                    "name",
                    "gpa",
                    "gender",
            }
    ));

    for (int i = 0; i < SMALL_NUMBER; ++i) {
        builder->addRow()
                .addInteger(i)
                .addChar("name" + std::to_string(i))
                .addFloat(i)
                .addInteger(i & 1);
    }

    uut->insert(builder->getSchema(), builder->getRows());
    uut->createIndex("gpa");

    std::unique_ptr<ConditionExpr> condition(
            uut->optimizeCondition(
                    new AndExpr(
                            new CompareExpr("gpa", CompareExpr::Operator::LT, "8.1"),
                            new CompareExpr("gpa", CompareExpr::Operator::GE, "1.9")
                    )
            )
    );

    int count = 0;
    uut->select(
            uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "name", "gpa"}),
            condition.get(),
            [&](ConstSlice row)
            {
                auto id_col = schema->getColumnByName("id");
                auto id = Convert::toString(id_col.getType(), id_col.getValue(row));
                EXPECT_EQ(std::to_string(count + 2), id);

                auto name_col = schema->getColumnByName("name");
                auto name = Convert::toString(name_col.getType(), name_col.getValue(row));
                EXPECT_EQ("name" + std::to_string(count + 2), name);

                auto gpa_col = schema->getColumnByName("gpa");
                EXPECT_EQ(
                        static_cast<float>(count + 2),
                        *reinterpret_cast<const float*>(gpa_col.getValue(row).content())
                );

                count++;
            }
    );
    EXPECT_EQ(7, count);

    builder->reset();
    for (int i = SMALL_NUMBER; i < SMALL_NUMBER * 2; ++i) {
        builder->addRow()
                .addInteger(i)
                .addChar("name" + std::to_string(i))
                .addFloat(i)
                .addInteger(i & 1);
    }

    uut->insert(builder->getSchema(), builder->getRows());
    condition.reset(
            uut->optimizeCondition(
                    new AndExpr(
                            new CompareExpr("gpa", CompareExpr::Operator::LT, "2.9"),
                            new CompareExpr("gpa", CompareExpr::Operator::GE, "1.9")
                    )
            )
    );
    count = 0;
    uut->select(
            uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "name", "gpa"}),
            condition.get(),
            [&](ConstSlice row)
            {
                auto id_col = schema->getColumnByName("id");
                auto id = Convert::toString(id_col.getType(), id_col.getValue(row));
                EXPECT_EQ(std::to_string(count + 2), id);

                auto name_col = schema->getColumnByName("name");
                auto name = Convert::toString(name_col.getType(), name_col.getValue(row));
                EXPECT_EQ("name" + std::to_string(count + 2), name);

                auto gpa_col = schema->getColumnByName("gpa");
                EXPECT_EQ(
                        static_cast<float>(count + 2),
                        *reinterpret_cast<const float*>(gpa_col.getValue(row).content())
                );

                count++;
            }
    );
    EXPECT_EQ(1, count);
}

TEST_F(TableTest, LargeNumber)
{
    std::unique_ptr<Table::RecordBuilder> builder(uut->getRecordBuilder(
            {
                    "id",
                    "name",
                    "gpa",
                    "gender",
            }
    ));

    for (int i = 0; i < LARGE_NUMBER; ++i) {
        builder->addRow()
                .addInteger(i)
                .addChar("name" + std::to_string(i))
                .addFloat(i)
                .addInteger(i & 1);
    }

    auto row = builder->getRows();
    ASSERT_EQ(builder->cbegin()->content(), row.begin()->content());
    uut->insert(builder->getSchema(), row);

    std::unique_ptr<Schema> select_schema(uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "gpa", "gender"}));

    int count = 0;
    uut->select(
            select_schema.get(),
            nullptr,
            [&](ConstSlice row)
            {
                auto gpa_col = select_schema->getColumnByName("gpa");
                EXPECT_EQ(
                        static_cast<float>(count),
                        *reinterpret_cast<const float*>(gpa_col.getValue(row).content())
                );

                auto gender_col = select_schema->getColumnByName("gender");
                auto gender = Convert::toString(gender_col.getType(), gender_col.getValue(row));
                EXPECT_EQ(std::to_string(count & 1), gender);

                ++count;
            }
    );
    EXPECT_EQ(LARGE_NUMBER, count);
}

TEST_F(TableTest, LargeCompareToIndex)
{
    std::unique_ptr<Table::RecordBuilder> builder(uut->getRecordBuilder(
            {
                    "id",
                    "name",
                    "gpa",
                    "gender",
            }
    ));

    for (int i = 0; i < LARGE_NUMBER; ++i) {
        builder->addRow()
                .addInteger(i)
                .addChar("name" + std::to_string(i))
                .addFloat(i)
                .addInteger(i & 1);
    }

    auto row = builder->getRows();
    ASSERT_EQ(builder->cbegin()->content(), row.begin()->content());
    uut->insert(builder->getSchema(), row);

    std::unique_ptr<Schema> select_schema(uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "gpa", "gender"}));
    std::unique_ptr<ConditionExpr> condition(
            uut->optimizeCondition(
                    new AndExpr(
                            new CompareExpr("gpa", CompareExpr::Operator::LT, "8.1"),
                            new CompareExpr("gpa", CompareExpr::Operator::GE, "1.9")
                    )
            )
    );

    for (int i = 0; i < SMALL_NUMBER; ++i) {
        int count = 0;
        uut->select(
                select_schema.get(),
                condition.get(),
                [&](ConstSlice row)
                {
                    auto gpa_col = select_schema->getColumnByName("gpa");
                    EXPECT_EQ(
                            static_cast<float>(count + 2),
                            *reinterpret_cast<const float*>(gpa_col.getValue(row).content())
                    );

                    ++count;
                }
        );
        EXPECT_EQ(7, count);
    }
}

TEST_F(TableTest, LargeIndex)
{
    std::unique_ptr<Table::RecordBuilder> builder(uut->getRecordBuilder(
            {
                    "id",
                    "name",
                    "gpa",
                    "gender",
            }
    ));

    for (int i = 0; i < LARGE_NUMBER; ++i) {
        builder->addRow()
                .addInteger(i)
                .addChar("name" + std::to_string(i))
                .addFloat(i)
                .addInteger(i & 1);
    }

    auto row = builder->getRows();
    ASSERT_EQ(builder->cbegin()->content(), row.begin()->content());
    uut->insert(builder->getSchema(), row);

    uut->createIndex("gpa");

    std::unique_ptr<ConditionExpr> condition(
            uut->optimizeCondition(
                    new AndExpr(
                            new CompareExpr("gpa", CompareExpr::Operator::LT, "8.1"),
                            new CompareExpr("gpa", CompareExpr::Operator::GE, "1.9")
                    )
            )
    );
    std::unique_ptr<Schema> select_schema(uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "name", "gpa"}));

    for (int i = 0; i < SMALL_NUMBER; ++i) {
        int count = 0;
        uut->select(
                select_schema.get(),
                condition.get(),
                [&](ConstSlice row)
                {
                    auto gpa_col = select_schema->getColumnByName("gpa");
                    EXPECT_EQ(
                            static_cast<float>(2 + count),
                            *reinterpret_cast<const float*>(gpa_col.getValue(row).content())
                    );

                    ++count;
                }
        );
        EXPECT_EQ(7, count);
    }
}

TEST_F(TableTest, dropIndex)
{
    uut->createIndex("gpa");

    std::unique_ptr<Table::RecordBuilder> builder(uut->getRecordBuilder(
            {
                    "id",
                    "name",
                    "gpa",
                    "gender",
            }
    ));

    for (int i = 0; i < LARGE_NUMBER; ++i) {
        builder->addRow()
                .addInteger(i)
                .addChar("name" + std::to_string(i))
                .addFloat(i)
                .addInteger(i & 1);
    }

    auto row = builder->getRows();
    ASSERT_EQ(builder->cbegin()->content(), row.begin()->content());
    uut->insert(builder->getSchema(), row);

    std::unique_ptr<ConditionExpr> condition(
            uut->optimizeCondition(
                    new AndExpr(
                            new CompareExpr("gpa", CompareExpr::Operator::LT, "8.1"),
                            new CompareExpr("gpa", CompareExpr::Operator::GE, "1.9")
                    )
            )
    );
    std::unique_ptr<Schema> select_schema(uut->buildSchemaFromColumnNames(std::vector<std::string>{"id", "name", "gpa"}));

    for (int i = 0; i < SMALL_NUMBER; ++i) {
        int count = 0;
        uut->select(
                select_schema.get(),
                condition.get(),
                [&](ConstSlice row)
                {
                    auto gpa_col = select_schema->getColumnByName("gpa");
                    EXPECT_EQ(
                            static_cast<float>(2 + count),
                            *reinterpret_cast<const float*>(gpa_col.getValue(row).content())
                    );

                    ++count;
                }
        );
        EXPECT_EQ(7, count);
    }

    uut->dropIndex("gpa");

    for (int i = 0; i < SMALL_NUMBER; ++i) {
        int count = 0;
        uut->select(
                select_schema.get(),
                condition.get(),
                [&](ConstSlice row)
                {
                    auto gpa_col = select_schema->getColumnByName("gpa");
                    EXPECT_EQ(
                            static_cast<float>(2 + count),
                            *reinterpret_cast<const float*>(gpa_col.getValue(row).content())
                    );

                    ++count;
                }
        );
        EXPECT_EQ(7, count);
    }
}

TEST_F(TableTest, removeAll)
{
    uut->createIndex("gpa");

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
                EXPECT_TRUE(0.00001 > std::fabs(gpa_float - 1.0));

                auto gender_col = schema->getColumnByName("gender");
                auto gender = Convert::toString(gender_col.getType(), gender_col.getValue(row));
                EXPECT_EQ("1", gender);
            }
    );
    EXPECT_EQ(3, count);

    uut->erase(nullptr);

    count = 0;
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
                EXPECT_TRUE(0.00001 > std::fabs(gpa_float - 1.0));

                auto gender_col = schema->getColumnByName("gender");
                auto gender = Convert::toString(gender_col.getType(), gender_col.getValue(row));
                EXPECT_EQ("1", gender);
            }
    );
    EXPECT_EQ(0, count);

    uut->insert(builder->getSchema(), builder->getRows());

    count = 0;
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
                EXPECT_TRUE(0.00001 > std::fabs(gpa_float - 1.0));

                auto gender_col = schema->getColumnByName("gender");
                auto gender = Convert::toString(gender_col.getType(), gender_col.getValue(row));
                EXPECT_EQ("1", gender);
            }
    );
    EXPECT_EQ(3, count);
}

TEST_F(TableTest, removeWithCondition)
{
    uut->createIndex("gpa");

    std::unique_ptr<Table::RecordBuilder> builder(uut->getRecordBuilder(
            {
                    "id",
                    "name",
                    "gpa",
                    "gender",
            }
    ));

    for (int i = 0; i < LARGE_NUMBER; ++i) {
        builder->addRow()
                .addInteger(i)
                .addChar("name" + std::to_string(i))
                .addFloat(i)
                .addInteger(i & 1);
    }

    auto row = builder->getRows();
    ASSERT_EQ(builder->cbegin()->content(), row.begin()->content());
    uut->insert(builder->getSchema(), row);

    int count = 0;
    uut->select(
            nullptr,
            nullptr,
            [&](ConstSlice row)
            {
                auto id_col = schema->getPrimaryColumn();
                auto id = Convert::toString(id_col.getType(), id_col.getValue(row));
                EXPECT_EQ(std::to_string(count), id);

                auto name_col = schema->getColumnByName("name");
                auto name = Convert::toString(name_col.getType(), name_col.getValue(row));
                EXPECT_EQ("name" + std::to_string(count), name);

                auto gpa_col = schema->getColumnByName("gpa");
                EXPECT_EQ(
                        static_cast<float>(count),
                        *reinterpret_cast<const float*>(gpa_col.getValue(row).content())
                );

                auto gender_col = schema->getColumnByName("gender");
                auto gender = Convert::toString(gender_col.getType(), gender_col.getValue(row));
                EXPECT_EQ(std::to_string(count & 1), gender);

                ++count;
            }
    );
    EXPECT_EQ(LARGE_NUMBER, count);

    std::unique_ptr<ConditionExpr> condition(
            uut->optimizeCondition(
                new CompareExpr("gender", CompareExpr::Operator::EQ, "1")
            )
        );

    uut->erase(condition.get());

    count = 0;
    uut->select(
            nullptr,
            nullptr,
            [&](ConstSlice row)
            {
                auto id_col = schema->getPrimaryColumn();
                auto id = Convert::toString(id_col.getType(), id_col.getValue(row));
                EXPECT_EQ(std::to_string(count), id);

                auto name_col = schema->getColumnByName("name");
                auto name = Convert::toString(name_col.getType(), name_col.getValue(row));
                EXPECT_EQ("name" + std::to_string(count), name);

                auto gpa_col = schema->getColumnByName("gpa");
                EXPECT_EQ(
                        static_cast<float>(count),
                        *reinterpret_cast<const float*>(gpa_col.getValue(row).content())
                );

                auto gender_col = schema->getColumnByName("gender");
                auto gender = Convert::toString(gender_col.getType(), gender_col.getValue(row));
                EXPECT_EQ("0", gender);

                count += 2;
            }
    );
    EXPECT_EQ(LARGE_NUMBER, count);
}
