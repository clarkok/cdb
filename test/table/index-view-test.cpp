#include <gtest/gtest.h>
#include <lib/index/skip-table.hpp>
#include <lib/table/skip-view.hpp>

#include "lib/driver/basic-driver.hpp"
#include "lib/driver/cached-accesser.hpp"
#include "lib/driver/bitmap-allocator.hpp"
#include "../test-inc.hpp"
#include "lib/table/index-view.hpp"

using namespace cdb;

static const char TEST_PATH[] = TMP_PATH_PREFIX "index-view-test.tmp";
static const int TEST_NUMBER = 10;

class IndexViewTest : public ::testing::Test
{
protected:
    struct IndexTestStruct
    {
        int id;
        int padding;
        int value;
    };

    static void TearDownTestCase()
    { std::remove(TEST_PATH); }

    std::unique_ptr<Driver> drv;
    std::unique_ptr<BlockAllocator> allocator;
    std::unique_ptr<CachedAccesser> accesser;
    std::unique_ptr<IndexView> uut;

    IndexViewTest()
            : drv(new BasicDriver(TEST_PATH)),
              allocator(new BitmapAllocator(drv.get(), 0)),
              accesser(new CachedAccesser(drv.get(), allocator.get()))
    {
        allocator->reset();
        BTree *btree = new BTree(
                accesser.get(),
                [](const Byte *a, const Byte *b) -> bool
                {
                    return
                            *reinterpret_cast<const int*>(a) <
                            *reinterpret_cast<const int*>(b);
                },
                [](const Byte *a, const Byte *b) -> bool
                {
                    return
                            *reinterpret_cast<const int*>(a) ==
                            *reinterpret_cast<const int*>(b);
                },
                accesser->allocateBlock(),
                sizeof(int),
                sizeof(IndexTestStruct)
        );
        btree->reset();

        IndexTestStruct buff;

        for (int i = 0; i < TEST_NUMBER; ++i) {
            buff = {i, 0, i * 2};
            auto iter = btree->insert(btree->makeKey(&i));
            *reinterpret_cast<IndexTestStruct*>(iter.getValue().content()) = buff;
        }

        uut.reset(new IndexView(
                Schema::Factory()
                        .addIntegerField("id")
                        .addIntegerField("padding")
                        .addIntegerField("value")
                        .release(),
                btree
        ));
    }
};

TEST_F(IndexViewTest, Iterators)
{
    auto primary_col = uut->getSchema()->getPrimaryColumn();
    auto value_col = uut->getSchema()->getColumnByName("value");
    auto iter = uut->begin();
    for (int i = 0; i < TEST_NUMBER; ++i) {
        EXPECT_EQ(i, *reinterpret_cast<const int*>(primary_col.getValue(iter.slice()).content()));
        EXPECT_EQ(i * 2, *reinterpret_cast<const int*>(value_col.getValue(iter.slice()).content()));
        iter.next();
    }
}

TEST_F(IndexViewTest, Peek)
{
    auto value_col = uut->getSchema()->getColumnByName("value");

    int lower_bound = TEST_NUMBER / 2;
    int upper_bound = TEST_NUMBER + TEST_NUMBER / 2;

    SkipView *ret = dynamic_cast<SkipView*>(
            uut->peek(
                    value_col,
                    reinterpret_cast<const Byte *>(&lower_bound),
                    reinterpret_cast<const Byte *>(&upper_bound)
            )
    );

    auto iter = ret->begin();
    for (int i = ((lower_bound + 1) >> 1); i < ((upper_bound + 1) >> 1); ++i) {
        EXPECT_EQ(i, *reinterpret_cast<const int*>(iter.slice().content()));
        iter.next();
    }
    EXPECT_TRUE(iter == ret->end());

    delete ret;
}
