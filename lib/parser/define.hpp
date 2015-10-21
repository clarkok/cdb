#ifndef _DB_PARSER_DEF_H_
#define _DB_PARSER_DEF_H_

#include "third-party/pegtl/pegtl.hh"

namespace cdb {

namespace parser {
    struct spacing
        : pegtl::plus<pegtl::space> { };

    struct number
        : pegtl::plus<pegtl::digit> { };

    template<typename T>
    struct parenthesis
        : pegtl::seq<
            pegtl::one<'('>,
            pegtl::opt<spacing>,
            T,
            pegtl::opt<spacing>,
            pegtl::one<')'>
        > { };
}

}

#endif // _DB_PARSER_DEF_H_
