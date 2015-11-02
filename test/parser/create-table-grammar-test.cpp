#include <gtest/gtest.h>
#include <string>

#include "lib/parser/grammar.hpp"
#include "third-party/pegtl/pegtl/analyze.hh"
#include "third-party/pegtl/pegtl.hh"

using namespace cdb::parser;

struct create_table_root
    : pegtl::seq<
        create_table_stmt,
        pegtl::eof
    > { };

TEST(CreateTableTest, Basic)
{
    static const char TEST_SQL[] = 
        "create table test_table (\n"
        "   id int unique auto_increment,\n"
        "   first_name char(255),\n"
        "   last_name char(255),\n"
        "   primary key (id)"
        ")\n";

    pegtl::analyze<create_table_root>();

    std::string state; // for no use;
    EXPECT_NO_THROW({
        pegtl::parse<create_table_root>(
                TEST_SQL,
                TEST_SQL + sizeof(TEST_SQL) - 1,
                "<create-table-test>",
                state
            );
    });
}
