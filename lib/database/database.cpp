#include <cstring>
#include <map>
#include <string>

#include "lib/utils/convert.hpp"
#include "lib/driver/basic-driver.hpp"
#include "lib/driver/bitmap-allocator.hpp"
#include "lib/driver/basic-accesser.hpp"
#include "database.hpp"

using namespace cdb;

struct Database::DBHeader
{
    char magic[8];
    BlockIndex root_index;
};

Database *
Database::Factory(std::string path)
{
    std::unique_ptr<Database> db;

    db->_driver.reset(new BasicDriver(path.c_str()));
    db->_allocator.reset(new BitmapAllocator(db->_driver.get(), 1));
    db->_accesser.reset(new BasicAccesser(db->_driver.get(), db->_allocator.get()));

    db->open();

    return db.release();
}

void
Database::open()
{
    auto header_block = _accesser->aquire(0);
    auto header = reinterpret_cast<const DBHeader*>(header_block.content());
    if (std::strcmp(MAGIC, reinterpret_cast<const char*>(header->magic))) {
        throw DatabaseInvalidException();
    }

    std::unique_ptr<Schema> root_schema(Table::getSchemaForRootTable());
    auto root_index = header->root_index;
    _root_table.reset(
            Table::Factory(
                _accesser.get(),
                "__root__",
                root_schema->copy(),
                root_index
            ).release()
        );

    auto name_col = root_schema->getColumnByName("name");
    auto data_col = root_schema->getColumnByName("data");
    auto count_col = root_schema->getColumnByName("count");
    auto index_for_col = root_schema->getColumnByName("index_for");
    auto create_sql_col = root_schema->getColumnByName("create_sql");

    std::map<std::string, Table::Factory> factory_map;

    _root_table->select(
            nullptr,
            nullptr,
            [&](ConstSlice row)
            {
                auto name = Convert::toString(name_col.getType(), index_for_col.getValue(row));
                auto data = *reinterpret_cast<const int*>(data_col.getValue(row).content());
                auto count = *reinterpret_cast<const int*>(count_col.getValue(row).content());
                auto index_for = Convert::toString(index_for_col.getType(), index_for_col.getValue(row));
                auto create_sql = create_sql_col.getValue(row);

                if (index_for == "") {
                    // this is a table
                    factory_map.emplace(name, Table::Factory(
                                _accesser.get(),
                                name,
                                Schema::Factory::parse(create_sql),
                                static_cast<BlockIndex>(data),
                                static_cast<Length>(count)
                            ));
                }
                else {
                    // this is an index
                    auto column_name = Convert::toString(create_sql_col.getType(), create_sql);
                    auto iter = factory_map.find(name);
                    iter->second.addIndex(column_name, data, name);
                }
            }
        );

    for (auto &pair : factory_map) {
        _tables.emplace_back(pair.second.release());
    }
}

void
Database::init()
{
    _allocator->reset();
    auto header_block = _accesser->aquire(0);
    auto *header = reinterpret_cast<DBHeader*>(header_block.content());
    std::copy(
            std::begin(MAGIC),
            std::end(MAGIC),
            std::begin(header->magic)
        );
    header->root_index = _accesser->allocateBlock();

    _root_table.reset(
            Table::Factory(
                _accesser.get(),
                "__root__",
                Table::getSchemaForRootTable(),
                header->root_index
            ).release()
        );

    _root_table->init();
}

void
Database::reset()
{ init(); }

Table *
Database::getTableByName(std::string name)
{
    for (auto iter = _tables.begin(); iter != _tables.end(); ++iter) {
        if ((*iter)->getName() == name) {
            return iter->get();
        }
    }
    throw DatabaseTableNotFoundException(name);
}

Table *
Database::createTable(std::string name, Schema *schema)
{
    _tables.emplace_back(Table::Factory(
                _accesser.get(),
                name,
                schema->copy(),
                _accesser->allocateBlock()
            ).release());

    return _tables.back().get();
}

void
Database::dropTable(std::string name)
{
    getTableByName(name)->drop();
    for (auto iter = _tables.begin(); iter != _tables.end(); ++iter) {
        if ((*iter)->getName() == name) {
            _tables.erase(iter);
            return;
        }
    }
}
