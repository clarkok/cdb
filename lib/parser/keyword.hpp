#ifndef _DB_PARSER_KEYWORD_H_
#define _DB_PARSER_KEYWORD_H_

#include "third-party/pegtl/pegtl.hh"

namespace cdb {

namespace parser {

#define KEY(key)    struct key_##key : pegtl_istring_t(#key) { }
    KEY(and);
    KEY(auto_increment);
    KEY(by);
    KEY(char);
    KEY(create);
    KEY(delete);
    KEY(float);
    KEY(insert);
    KEY(int);
    KEY(into);
    KEY(key);
    KEY(or);
    KEY(order);
    KEY(primary);
    KEY(select);
    KEY(table);
    KEY(text);
    KEY(unique);
    KEY(update);
    KEY(where);
#undef KEY

    template <typename K>
    struct key
        : pegtl::seq<K, pegtl::not_at<pegtl::identifier_other> > { };

    struct keyword 
        : key<
            pegtl::sor<
                key_and,
                key_auto_increment,
                key_by,
                key_char,
                key_create,
                key_delete,
                key_float,
                key_insert,
                key_int,
                key_into,
                key_key,
                key_or,
                key_order,
                key_primary,
                key_select,
                key_text,
                key_table,
                key_unique,
                key_update,
                key_where
            >
        > { };
}

}

#endif // _DB_PARSER_KEYWORD_H_
