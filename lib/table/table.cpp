#include "table.hpp"
#include "lib/condition/column-name-visitor.hpp"
#include "lib/index/btree.hpp"
#include "lib/utils/comparator.hpp"
#include "lib/utils/convert.hpp"
#include "index-view.hpp"

using namespace cdb;

class Table::IndexVisitor : public ConditionVisitor
{
    Table *_owner;
    std::unique_ptr<ModifiableView> _index_view;
    Schema *_primary_schema;
    Length _threshold = std::numeric_limits<Length>::max();
public:
    IndexVisitor(Table *owner, Schema *primary_schema, Length threshold)
            : _owner(owner), _primary_schema(primary_schema), _threshold(threshold)
    { }

    ModifiableView *release()
    { return _index_view.release(); }

    virtual ~IndexVisitor() = default;

    virtual void visit(AndExpr *expr)
    {
        std::unique_ptr<ModifiableView> res;
        expr->lh->accept(this);
        if (_index_view) {
            res.reset(_index_view.release());
        }
        expr->rh->accept(this);
        if (_index_view && res) {
            _index_view->intersect(res->begin(), res->end());
        }
        else if (res) {
            std::swap(res, _index_view);
        }
    }

    virtual void visit(OrExpr *expr)
    {
        expr->lh->accept(this);
        if (!_index_view) {
            return;
        }
        std::unique_ptr<ModifiableView> lhs(_index_view.release());
        expr->rh->accept(this);
        if (!_index_view) {
            return;
        }
        _index_view->join(lhs->begin(), lhs->end());
        if (_index_view->count() > _threshold) {
            _index_view.reset();
        }
    }

    virtual void visit(CompareExpr *expr)
    {
        auto index_root = _owner->findIndex(expr->column_name);
        if (!index_root) {
            _index_view = nullptr;
            return;
        }

        auto type = _owner->_schema->getColumnByName(expr->column_name).getType();

        Schema *index_schema = _owner->buildSchemaForIndex(expr->column_name);
        auto primary_type = index_schema->getPrimaryColumn().getType();
        auto primary_length = index_schema->getPrimaryColumn().getField()->length;

        BTree *tree = new BTree(
                _owner->_accesser,
                Comparator::getCompareFuncByTypeLT(type),
                Comparator::getCompareFuncByTypeEQ(type),
                index_root,
                primary_length,
                index_schema->getRecordSize()
        );

        std::unique_ptr<View> index_view(new IndexView(index_schema, tree));
        auto key = Convert::fromString(primary_type, primary_length, expr->literal);

        switch (expr->op) {
            case CompareExpr::Operator::EQ:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->lowerBound(key.content()),
                        index_view->upperBound(key.content())
                ));
                break;
            }
            case CompareExpr::Operator::NE:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->begin(),
                        index_view->lowerBound(key.content())
                ));
                if (_index_view->count() > _threshold) {
                    _index_view.reset();
                    return;
                }
                std::unique_ptr<ModifiableView> upper(index_view->selectRange(
                        _primary_schema,
                        index_view->upperBound(key.content()),
                        index_view->end()
                ));
                _index_view->join(upper->begin(), upper->end());
                break;
            }
            case CompareExpr::Operator::GT:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->upperBound(key.content()),
                        index_view->end()
                ));
                break;
            }
            case CompareExpr::Operator::GE:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->lowerBound(key.content()),
                        index_view->end()
                ));
                break;
            }
            case CompareExpr::Operator::LT:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->begin(),
                        index_view->lowerBound(key.content())
                ));
                break;
            }
            case CompareExpr::Operator::LE:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->begin(),
                        index_view->upperBound(key.content())
                ));
                break;
            }
        }
        if (_index_view->count() > _threshold) {
            _index_view.reset();
        }
        return;
    }
};

class Table::FilterVisitor : public ConditionVisitor
{
    const Schema *_schema;
    ConstSlice _data;
    bool _result = true;
public:
    FilterVisitor(const Schema *schema, ConstSlice data)
            : _schema(schema), _data(data)
    { }
    virtual ~FilterVisitor() = default;

    inline bool
    result() const
    { return _result; }

    virtual void visit(AndExpr *expr)
    {
        expr->lh->accept(this);
        if (!_result) {
            return;
        }
        expr->rh->accept(this);
    }

    virtual void visit(OrExpr *expr)
    {
        expr->lh->accept(this);
        if (_result) {
            return;
        }
        expr->rh->accept(this);
    }

    virtual void visit(CompareExpr *expr)
    {
        auto col = _schema->getColumnByName(expr->column_name);
        auto value = Convert::fromString(col.getType(), col.getField()->length, expr->literal);
        auto data = col.getValue(_data);

        switch (expr->op) {
            case CompareExpr::Operator::EQ:
            {
                _result = Comparator::getCompareFuncByTypeEQ(col.getType())(
                        data.content(),
                        value.content()
                );
                break;
            }
            case CompareExpr::Operator::NE:
            {
                _result = !Comparator::getCompareFuncByTypeEQ(col.getType())(
                        data.content(),
                        value.content()
                );
                break;
            }
            case CompareExpr::Operator::GT:
            {
                _result = Comparator::getCompareFuncByTypeLT(col.getType())(
                        value.content(),
                        data.content()
                );
                break;
            }
            case CompareExpr::Operator::GE:
            {
                _result = !Comparator::getCompareFuncByTypeLT(col.getType())(
                        data.content(),
                        value.content()
                );
                break;
            }
            case CompareExpr::Operator::LT:
            {
                _result = Comparator::getCompareFuncByTypeLT(col.getType())(
                        data.content(),
                        value.content()
                );
                break;
            }
            case CompareExpr::Operator::LE:
            {
                _result = !Comparator::getCompareFuncByTypeLT(col.getType())(
                        value.content(),
                        data.content()
                );
                break;
            } } }
};

Schema *
Table::buildSchemaForIndex(std::string column_name) {
    auto column = _schema->getColumnByName(column_name);

    Schema::Factory builder;
    switch (column.getType()) {
        case Schema::Field::Type::INTEGER:
            builder.addIntegerField(column.getField()->name);
            break;
        case Schema::Field::Type::FLOAT:
            builder.addFloatField(column.getField()->name);
            break;
        case Schema::Field::Type::CHAR:
            builder.addCharField(column.getField()->name, column.getField()->length);
            break;
        default:
            throw TableFieldNotSupportedException();
    }

    auto primary_col = _schema->getPrimaryColumn();
    switch (primary_col.getType()) {
        case Schema::Field::Type::INTEGER:
            builder.addIntegerField(primary_col.getField()->name);
            break;
        case Schema::Field::Type::FLOAT:
            builder.addFloatField(primary_col.getField()->name);
            break;
        case Schema::Field::Type::CHAR:
            builder.addCharField(primary_col.getField()->name, primary_col.getField()->length);
            break;
        default:
            throw TableFieldNotSupportedException();
    }

    return builder.release();
}

BlockIndex
Table::findIndex(std::string column_name)
{
    for (auto &index : _indices) {
        if (index.column_name == column_name) {
            return index.root;
        }
    }
    return 0;
}

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

    if (!condition) {
        std::unique_ptr<View> view(IndexView(
                _schema->copy(),
                new BTree(
                        _accesser,
                        Comparator::getCompareFuncByTypeLT(primary_col.getType()),
                        Comparator::getCompareFuncByTypeEQ(primary_col.getType()),
                        _root,
                        primary_col.getField()->length,
                        _schema->getRecordSize()
                )
        ).select(schema));
        for (auto iter = view->begin(); iter != view->end(); iter.next()) {
            accesser(iter.slice());
        }
        return;
    }

    std::unique_ptr<Schema> primary_schema(buildSchemaFromColumnNames({primary_col.getField()->name}));
    IndexVisitor v(this, primary_schema.get(), calculateThreshold());
    condition->accept(&v);
    std::unique_ptr<ModifiableView> indexed_view(v.release());

    IndexView data_view(
            _schema->copy(),
            new BTree(
                    _accesser,
                    Comparator::getCompareFuncByTypeLT(primary_col.getType()),
                    Comparator::getCompareFuncByTypeEQ(primary_col.getType()),
                    _root,
                    primary_col.getField()->length,
                    _schema->getRecordSize()
            )
    );

    std::unique_ptr<View> view;
    if (indexed_view) {
        view.reset(
                data_view.selectIndexed(
                        schema,
                        indexed_view->begin(),
                        indexed_view->end(),
                        buildFilter(condition)
                )
        );
    }
    else {
        view.reset(
                data_view.select(
                        schema,
                        buildFilter(condition)
                )
        );
    }
    for (auto iter = view->begin(); iter != view->end(); iter.next()) {
        accesser(iter.slice());
    }
}

View::Filter
Table::buildFilter(ConditionExpr *condition)
{
    return [=] (const Schema *schema, ConstSlice slice) -> bool
    {
        FilterVisitor v(schema, slice);
        condition->accept(&v);
        return v.result();
    };
}

Length
Table::calculateThreshold() const
{ return _count / (Driver::BLOCK_SIZE / _schema->getRecordSize()); }

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