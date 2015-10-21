#include <gtest/gtest.h>
#include <string>

#include "third-party/pegtl/pegtl/analyze.hh"
#include "third-party/pegtl/pegtl.hh"
#include "lib/parser/grammar.hpp"

using namespace cdb::parser;

struct schema_root
    : pegtl::seq<
        schema_decl,
        pegtl::eof
    > { };

TEST(SchemaGrammarTest, Basic)
{
    static const char SCHEMA_TEXT[] =
        "id int unique,\n"
        "name char(255) UNIQUE auto_increment,\n"
        "gender char ( 1 ) ,\n"
        "primary key (id)";

    pegtl::analyze<schema_root>();

    std::string state;  // for no use
    EXPECT_NO_THROW({
        pegtl::parse<schema_root>(
                SCHEMA_TEXT,
                SCHEMA_TEXT + sizeof(SCHEMA_TEXT) - 1,
                "<schema-grammar-test>",
                state
            );
    });
}

TEST(SchemaGrammarTest, ErrorOnDecl)
{
    static const char SCHEMA_TEXT[] =
        "id int unique,\n"
        "lalalalala i am not a valid decl\n"
        "gender char ( 1 ) ,\n"
        "primary key (id)";

    auto run = [&] () {
        std::string state;  // for no use
        pegtl::parse<schema_root>(
                SCHEMA_TEXT,
                SCHEMA_TEXT + sizeof(SCHEMA_TEXT) - 1,
                "<schema-grammar-test>",
                state
            );
    };

    EXPECT_THROW(
        run(),
        pegtl::parse_error
    );
}

TEST(SchemaGrammarTest, ErrorOnType)
{
    static const char SCHEMA_TEXT[] =
        "id not-valid-type unique,\n"
        "gender char ( 1 ) ,\n"
        "primary key (id)";

    auto run = [&] () {
        std::string state;  // for no use
        pegtl::parse<schema_root>(
                SCHEMA_TEXT,
                SCHEMA_TEXT + sizeof(SCHEMA_TEXT) - 1,
                "<schema-grammar-test>",
                state
            );
    };

    EXPECT_THROW(
        run(),
        pegtl::parse_error
    );
}

TEST(SchemaGrammarTest, ErrorOnProperty)
{
    static const char SCHEMA_TEXT[] =
        "id int unique and-a-non-valid-property,\n"
        "gender char ( 1 ) ,\n"
        "primary key (id)";

    auto run = [&] () {
        std::string state;  // for no use
        pegtl::parse<schema_root>(
                SCHEMA_TEXT,
                SCHEMA_TEXT + sizeof(SCHEMA_TEXT) - 1,
                "<schema-grammar-test>",
                state
            );
    };

    EXPECT_THROW(
        run(),
        pegtl::parse_error
    );
}
