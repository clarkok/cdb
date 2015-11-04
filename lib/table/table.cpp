#include "table.hpp"
#include "lib/condition/column-name-visitor.hpp"
#include "lib/index/btree.hpp"
#include "lib/utils/comparator.hpp"
#include "index-view.hpp"

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

Table::Table(DriverAccesser *accesser, Schema *schema, BlockIndex root)
        : _accesser(accesser), _schema(schema), _root(root)
{ }

void
Table::select(Schema *schema, ConditionExpr *condition, Accesser accesser)
{
    std::unique_ptr<Schema> internal_schema;

    if (!schema) {
        internal_schema.reset(_schema->copy());
    }
    else {
        std::set<std::string> columns_set = getColumnNames(condition);
        mergeColumnNamesInSchema(schema, columns_set);
        internal_schema.reset(buildSchemaFromColumnNames(columns_set));
    }

    auto primary_col = _schema->getPrimaryColumn();

    BTree tree(
            _accesser,
            Comparator::getCompareFuncByTypeLT(primary_col.getType()),
            Comparator::getCompareFuncByTypeEQ(primary_col.getType()),
            _root,
            primary_col.getField()->length,
            _schema->getRecordSize()
    );

    if (!condition) {
        std::unique_ptr<View> view(IndexView(_schema->copy(), &tree).select(schema));
        for (auto iter = view->begin(); iter != view->end(); iter.next()) {
            accesser(iter.slice());
        }
        return;
    }

    std::unique_ptr<View> basic_view(IndexView(_schema->copy(), &tree).select(internal_schema.release()));

}

Schema *
Table::buildSchemaFromColumnNames(std::set<std::string> columns_set)
{
    auto primary_col = _schema->getPrimaryColumn();
    columns_set.insert(primary_col.getField()->name);
    Schema::Factory builder;
    for (auto &name : columns_set) {
        auto original_column = _schema->getColumnByName(name);

        switch (original_column.getType()) {
            case Schema::Field::Type::INTEGER:
                builder.addIntegerField(name);
                break;
            case Schema::Field::Type::FLOAT:
                builder.addFloatField(name);
                break;
            case Schema::Field::Type::CHAR:
                builder.addCharField(name, original_column.getField()->length);
                break;
            default:
                throw TableFieldNotSupportedException();
        }
    }
    return builder.release();
}

std::set<std::string>
Table::getColumnNames(ConditionExpr *expr)
{
    if (!expr) {
        return std::set<std::string>();
    }
    ColumnNameVisitor v;
    expr->accept(&v);
    return v.get();
}

std::set<std::string>
Table::mergeColumnNamesInSchema(Schema *schema, std::set<std::string> &set)
{
    for (auto &field : *schema) {
        set.insert(field.name);
    }
    return set;
}