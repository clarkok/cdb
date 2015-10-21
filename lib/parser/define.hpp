#ifndef _DB_PARSER_DEF_H_
#define _DB_PARSER_DEF_H_

#include "third-party/pegtl/pegtl.hh"

#include "keyword.hpp"

namespace cdb {

namespace parser {
    template<typename T>
    struct token
        : pegtl::seq<
            pegtl::not_at<pegtl::space>,
            T,
            pegtl::star<pegtl::space>
        > { };

    struct number
        : pegtl::plus<pegtl::digit> { };

    struct identifier
        : pegtl::seq<pegtl::not_at<keyword>, pegtl::identifier> { };

    struct comma
        : token<pegtl::one<','> > { };

    template<typename T>
    struct parenthesis
        : pegtl::seq<
            token<pegtl::one<'('> >,
            T,
            token<pegtl::one<')'> >
        > { };
}

}

#endif // _DB_PARSER_DEF_H_
