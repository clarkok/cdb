#include "table.hpp"

using namespace cdb;

std::unique_ptr<Schema>
Table::getSchemaForRootTable()
{
    SchemaFactory factory;

    factory.addIntegerField("id");
    factory.addCharField("name", MAX_TABLE_NAME_LENGTH);
    factory.addIntegerField("data");

    factory.addTextField("create_sql");
    return factory.reset();
}
