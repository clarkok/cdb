#ifndef _DB_PARSER_ERROR_H_
#define _DB_PARSER_ERROR_H_

namespace cdb {

namespace parser {
    struct ParseError
    { };

    struct UnknownTypeError : ParseError
    { };

    struct UnknownSchemaDeclError : ParseError
    { };

    struct UnknownFieldPropertyError : ParseError
    { };
}

}

#endif // _DB_PARSER_ERROR_H_
