#include <cstring>
#include <map>
#include <string>
#include <iostream>

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
    Length root_count;
};

const char Database::MAGIC[8] = "--CDB--";

Database *
Database::Factory(std::string path)
{
    std::unique_ptr<Database> db(new Database());

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
        init();
    }

    std::unique_ptr<Schema> root_schema(Table::getSchemaForRootTable());
    auto root_index = header->root_index;
    _root_table.reset(
            Table::Factory(
                _accesser.get(),
                "__root__",
                root_schema->copy(),
                root_index,
                header->root_count
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
                auto name = Convert::toString(name_col.getType(), name_col.getValue(row));
                auto data = *reinterpret_cast<const int*>(data_col.getValue(row).content());
                auto count = *reinterpret_cast<const int*>(count_col.getValue(row).content());
                auto index_for = Convert::toString(index_for_col.getType(), index_for_col.getValue(row));
                auto create_sql = create_sql_col.getValue(row);

                std::cout << "\'" << name << "\'" << std::endl;

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
                    auto iter = factory_map.find(index_for);
                    iter->second.addIndex(column_name, data, name);
                }
            }
        );

    for (auto &pair : factory_map) {
        _tables.emplace_back(pair.second.release());
    }
}

void
Database::close()
{
    _root_table->reset();
    std::unique_ptr<Schema> root_schema(Table::getSchemaForRootTable());

    auto id_col = root_schema->getColumnByName("id");
    auto name_col = root_schema->getColumnByName("name");
    auto data_col = root_schema->getColumnByName("data");
    auto count_col = root_schema->getColumnByName("count");
    auto index_for_col = root_schema->getColumnByName("index_for");
    auto create_sql_col = root_schema->getColumnByName("create_sql");

    std::unique_ptr<Table::RecordBuilder> builder(_root_table->getRecordBuilder(
                {
                    "id",
                    "name",
                    "data",
                    "count",
                    "index_for",
                    "create_sql"
                }
            ));
    Buffer insert_in_root(root_schema->getRecordSize());
    int id = 0;
    for (auto &table : _tables) {
        *reinterpret_cast<int*>(id_col.getValue(insert_in_root).content()) = ++id;
        Convert::fromString(
                name_col.getType(),
                name_col.getField()->length,
                table->getName(),
                name_col.getValue(insert_in_root)
            );
        *reinterpret_cast<int*>(data_col.getValue(insert_in_root).content()) = 
            static_cast<int>(table->getRoot());
        *reinterpret_cast<int*>(count_col.getValue(insert_in_root).content()) =
            static_cast<int>(table->getCount());
        Convert::fromString(
                index_for_col.getType(),
                index_for_col.getField()->length,
                "",
                index_for_col.getValue(insert_in_root)
            );
        table->getSchema()->serialize(create_sql_col.getValue(insert_in_root));

        builder->addRow(insert_in_root);

        for (auto &index : *table) {
            *reinterpret_cast<int*>(id_col.getValue(insert_in_root).content()) = ++id;
            Convert::fromString(
                    name_col.getType(),
                    name_col.getField()->length,
                    index.name,
                    name_col.getValue(insert_in_root)
                );
            *reinterpret_cast<int*>(data_col.getValue(insert_in_root).content()) = 
                static_cast<int>(index.root);
            *reinterpret_cast<int*>(count_col.getValue(insert_in_root).content()) =
                static_cast<int>(table->getCount());
            Convert::fromString(
                    index_for_col.getType(),
                    index_for_col.getField()->length,
                    table->getName(),
                    index_for_col.getValue(insert_in_root)
                );
            Convert::fromString(
                    create_sql_col.getType(),
                    create_sql_col.getField()->length,
                    index.column_name,
                    create_sql_col.getValue(insert_in_root)
                );
            builder->addRow(insert_in_root);
        }
    }

    _root_table->insert(builder->getSchema(), builder->getRows());

    auto header_block = _accesser->aquire(0);
    auto *header = reinterpret_cast<DBHeader*>(header_block.content());
    header->root_index = _root_table->getRoot();
    header->root_count = _root_table->getCount();
}

void
Database::init()
{
    _root_table.reset();
    _tables.clear();

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
    header->root_index = _root_table->getRoot();
    header->root_count = _root_table->getCount();
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

    _tables.back()->init();

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

std::string
Database::indexFor(std::string name)
{
    std::unique_ptr<Schema> schema(_root_table->buildSchemaFromColumnNames(
                    std::vector<std::string>{"id", "index_for"}));

    std::unique_ptr<ConditionExpr> condition(
                new CompareExpr(
                    "name",
                    CompareExpr::Operator::EQ,
                    name
                )
            );

    std::string result;

    _root_table->select(
            schema.get(),
            condition.get(),
            [&](const ConstSlice &row)
            {
                auto index_col = schema->getColumnByName("index_for");
                result = Convert::toString(
                        index_col.getType(), 
                        index_col.getValue(row)
                        );
            }
        );

    if (!result.length()) {
        throw DatabaseIndexNotFoundException(name);
    }

    return result;
}

Database *
cdb::getGlobalDatabase()
{
    static std::unique_ptr<Database> db(Database::Factory("/tmp/db"));

    return db.get();
}
