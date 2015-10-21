#include <gtest/gtest.h>
#include <string>
#include <iostream>

#include "third-party/pegtl/pegtl/analyze.hh"
#include "third-party/pegtl/pegtl.hh"
#include "lib/parser/parser.hpp"

using namespace cdb::parser;

static const char SCHEMA_TEXT[] =
    "id int unique,\n"
    "name char(255),\n"
    "gender char ( 1 ) ,\n"
    "primary key (id)";

template <typename Rule>
struct test_action
    : pegtl::nothing<Rule> { };

template <>
struct test_action<field_name>
{
    static void apply(const pegtl::input &in, std::string &)
    { std::cout << in.string() << std::endl; }
};

template <>
struct test_action<field_type>
{
    static void apply(const pegtl::input &in, std::string &)
    { std::cout << in.string() << std::endl; }
};

struct schema_root
    : pegtl::sor<
        schema_decl,
        pegtl::eof
    > { };

TEST(SchemaParserTest, Basic)
{
    pegtl::analyze<schema_root>();

    std::string state;  // for no use

    pegtl::parse<schema_root, test_action>(SCHEMA_TEXT, SCHEMA_TEXT + sizeof(SCHEMA_TEXT) - 1, SCHEMA_TEXT, state);
}
