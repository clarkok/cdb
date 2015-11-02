#ifndef _DB_PARSER_GRAMMAR_H_
#define _DB_PARSER_GRAMMAR_H_

#ifdef __CYGWIN__
#include <sstream>
#include <string>
namespace std
{
    template <typename T>
    std::string
    to_string(T value)
    {
        std::stringstream sout;
        sout << value;
        return sout.str();
    }
}
#endif

#include "third-party/pegtl/pegtl.hh"

#include "error.hpp"
#include "define.hpp"
#include "keyword.hpp"

namespace cdb {

namespace parser {

    struct field_name
        : token<identifier> { };

    struct field_type_int
        : token<key<key_int> > { };

    struct field_type_float
        : token<key<key_float> > { };

    struct field_type_char
        : pegtl::seq<
            token<key<key_char> >,
            parenthesis<token<number> >
        > { };

    struct field_type_text
        : token<key<key_text> > { };

    struct field_type
        : pegtl::sor<
            field_type_int,
            field_type_float,
            field_type_char,
            field_type_text,
            pegtl::raise<UnknownTypeError>
        > { };

    struct field_property
        : pegtl::if_must<
            pegtl::not_at<comma>,
            pegtl::sor<
                token<key<key_unique> >,
                token<key<key_auto_increment> >,
                pegtl::raise<UnknownFieldPropertyError>
            >
        > { };

    struct field_decl
        : pegtl::seq<
            field_name,
            field_type,
            pegtl::star<field_property>
        > { };

    struct primary_decl
        : pegtl::seq<
            token<key<key_primary> >,
            token<key<key_key> >,
            parenthesis<field_name>
        > { };

    struct schema_decl_item
        : pegtl::sor<
            field_decl,
            primary_decl,
            pegtl::raise<UnknownSchemaDeclError>
        > { };

    struct schema_decl
        : pegtl::seq<
            schema_decl_item,
            pegtl::star<
                comma,
                schema_decl_item
            >
        > { };

    struct table_name
        : token<identifier> { };

    struct create_table_stmt
        : pegtl::seq<
            token<key<key_create> >,
            token<key<key_table> >,
            table_name,
            parenthesis<schema_decl>
        > { };

    struct value_number
        : pegtl::seq<
            pegtl::opt<pegtl::one<'+', '-'> >,
            pegtl::plus<pegtl::digit>,
            pegtl::opt<
                dot,
                pegtl::star<pegtl::digit>
            >,
            pegtl::opt<
                exp,
                pegtl::plus<pegtl::digit>
            >
        > { };

    struct single : pegtl::one< 'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '0', '\n' > {};
    struct spaces : pegtl::seq< pegtl::one< 'z' >, pegtl::star< pegtl::space > > {};
    struct hexbyte : pegtl::if_must< pegtl::one< 'x' >, pegtl::xdigit, pegtl::xdigit > {};
    struct decbyte : pegtl::if_must< pegtl::digit, pegtl::rep_opt< 2, pegtl::digit > > {};
    struct unichar : pegtl::if_must< pegtl::one< 'u' >, pegtl::one< '{' >, pegtl::plus< pegtl::xdigit >, pegtl::one< '}' > > {};
    struct escaped : pegtl::if_must< pegtl::one< '\\' >, pegtl::sor< hexbyte, decbyte, unichar, single, spaces > > {};
    struct regular : pegtl::not_one< '\r', '\n' > {};
    struct character : pegtl::sor< escaped, regular > {};

    struct value_string
        : pegtl::if_must<
            pegtl::if_must<pegtl::one<'\''> >,
            pegtl::until<pegtl::one<'\''>, character >
        > { };

    struct value
        : pegtl::sor<
            value_number,
            value_string
        > { };

    struct value_row
        : list<value> { };

    struct field_list
        : list<field_name> { };
}

}

#endif // _DB_PARSER_GRAMMAR_H_
