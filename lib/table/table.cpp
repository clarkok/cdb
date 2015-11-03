#include "table.hpp"

using namespace cdb;

Schema *
Table::getSchemaForRootTable()
{
    Schema::Factory factory;

    factory.addIntegerField("id");
    factory.addCharField("name", MAX_TABLE_NAME_LENGTH);
    factory.addIntegerField("data");

    factory.addTextField("create_sql");
    return factory.release();
}
