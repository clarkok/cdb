#ifndef _DB_PARSER_GRAMMAR_H_
#define _DB_PARSER_GRAMMAR_H_

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
}

}

#endif // _DB_PARSER_GRAMMAR_H_
