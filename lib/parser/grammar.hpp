#ifndef _DB_PARSER_GRAMMAR_H_
#define _DB_PARSER_GRAMMAR_H_

#include "third-party/pegtl/pegtl.hh"

#include "error.hpp"
#include "define.hpp"
#include "keyword.hpp"

namespace cdb {

namespace parser {

    struct field_name
        : pegtl::seq<pegtl::not_at<keyword>, pegtl::identifier> { };

    struct field_type_int
        : key<key_int> { };

    struct field_type_float
        : key<key_float> { };

    struct field_type_char
        : pegtl::seq<
            key<key_char>,
            pegtl::opt<spacing>,
            parenthesis<number>
        > { };

    struct field_type_text
        : key<key_text> { };

    struct field_type
        : pegtl::sor<
            field_type_int,
            field_type_float,
            field_type_char,
            field_type_text,
            pegtl::raise<UnknownTypeError>
        > { };

    struct field_decl
        : pegtl::seq<
            field_name,
            spacing,
            field_type,
            pegtl::opt<
                spacing,
                key<key_unique>
            >
        > { };

    struct primary_decl
        : pegtl::seq<
            key<key_primary>,
            spacing,
            key<key_key>,
            pegtl::opt<spacing>,
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
                pegtl::opt<spacing>,
                pegtl::one<','>,
                pegtl::opt<spacing>,
                schema_decl_item
            >
        > { };
}

}

#endif // _DB_PARSER_GRAMMAR_H_
