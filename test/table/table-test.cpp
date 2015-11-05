#include <gtest/gtest.h>
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
    std::unique_ptr<Table> uut;

    TableTest()
            : driver(new BasicDriver(TEST_PATH)),
              allocator(new BitmapAllocator(driver.get(), 0)),
              accesser(new BasicAccesser(driver.get(), allocator.get()))
    {
        Schema *schema = Schema::Factory()
                .addIntegerField("id")
                .addCharField("name", 16)
                .addFloatField("gpa")
                .addIntegerField("gender")
                .setPrimary("id")
                .release();
    }
};


