#ifndef _DB_TABLE_TABLE_H_
#define _DB_TABLE_TABLE_H_

#include "schema.hpp"

namespace cdb {
    class Table
    {
    public:
        static const int MAX_TABLE_NAME_LENGTH = 32;

        static Schema *getSchemaForRootTable();
    };
}

#endif // _DB_TABLE_TABLE_H_
