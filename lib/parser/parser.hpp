#ifndef _DB_PARSER_PARSER_H_
#define _DB_PARSER_PARSER_H_

#include <exception>
#include "lib/database/database.hpp"

namespace cdb {
    struct ParserQuitingException : public std::exception
    { };

    class Parser
    {
        Database *_db;
    public:
        Parser(Database *db)
            : _db(db)
        { }

        void exec(std::string);
    };
}

#endif // _DB_PARSER_PARSER_H_
