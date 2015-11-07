#include "table.hpp"
#include "lib/condition/column-name-visitor.hpp"
#include "lib/index/btree.hpp"
#include "lib/utils/comparator.hpp"
#include "lib/utils/convert.hpp"
#include "index-view.hpp"
#include "optimize-visitor.hpp"

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

        Schema *index_schema = _owner->buildSchemaForIndex(expr->column_name);
        auto index_col = index_schema->getPrimaryColumn();
        auto index_type = index_col.getType();
        auto index_length = index_col.getField()->length;

        auto primary_col = index_schema->getColumnById(1);
        auto primary_type = primary_col.getType();
        auto primary_length = primary_col.getField()->length;

        std::unique_ptr<View> index_view(new IndexView(
                index_schema,
                _owner->buildIndexBTree(index_root, index_schema)
        ));

        Buffer lower_key(index_schema->getRecordSize());
        Convert::fromString(
                index_type,
                index_length,
                expr->literal,
                index_col.getValue(Slice(lower_key))
        );
        Convert::minLimit(
                primary_type,
                primary_length,
                primary_col.getValue(Slice(lower_key))
        );

        Buffer upper_key(index_schema->getRecordSize());
        Convert::fromString(
                index_type,
                index_length,
                expr->literal,
                index_col.getValue(Slice(upper_key))
        );
        Convert::maxLimit(
                primary_type,
                primary_length,
                primary_col.getValue(Slice(upper_key))
        );

        switch (expr->op) {
            case CompareExpr::Operator::EQ:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->lowerBound(lower_key.content()),
                        index_view->upperBound(upper_key.content())
                ));
                break;
            }
            case CompareExpr::Operator::NE:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->begin(),
                        index_view->lowerBound(lower_key.content())
                ));
                if (_index_view->count() > _threshold) {
                    _index_view.reset();
                    return;
                }
                std::unique_ptr<ModifiableView> upper(index_view->selectRange(
                        _primary_schema,
                        index_view->upperBound(upper_key.content()),
                        index_view->end()
                ));
                _index_view->join(upper->begin(), upper->end());
                break;
            }
            case CompareExpr::Operator::GT:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->upperBound(upper_key.content()),
                        index_view->end()
                ));
                break;
            }
            case CompareExpr::Operator::GE:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->lowerBound(lower_key.content()),
                        index_view->end()
                ));
                break;
            }
            case CompareExpr::Operator::LT:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->begin(),
                        index_view->lowerBound(lower_key.content())
                ));
                break;
            }
            case CompareExpr::Operator::LE:
            {
                _index_view.reset(index_view->selectRange(
                        _primary_schema,
                        index_view->begin(),
                        index_view->upperBound(upper_key.content())
                ));
                break;
            }
        }
        if (_index_view->count() > _threshold) {
            _index_view.reset();
        }
    }

    virtual void visit(RangeExpr *expr)
    {
        auto index_root = _owner->findIndex(expr->column_name);
        if (!index_root) {
            _index_view = nullptr;
            return ;
        }

        Schema *index_schema = _owner->buildSchemaForIndex(expr->column_name);
        auto index_col = index_schema->getPrimaryColumn();
        auto index_type = index_col.getType();
        auto index_length = index_col.getField()->length;

        auto primary_col = index_schema->getColumnById(1);
        auto primary_type = primary_col.getType();
        auto primary_length = primary_col.getField()->length;

        std::unique_ptr<View> index_view(new IndexView(
                index_schema,
                _owner->buildIndexBTree(index_root, index_schema)
        ));

        Buffer lower_key(index_schema->getRecordSize());
        Convert::fromString(
                index_type,
                index_length,
                expr->lower_value,
                index_col.getValue(Slice(lower_key))
        );
        Convert::minLimit(
                primary_type,
                primary_length,
                primary_col.getValue(Slice(lower_key))
        );

        Buffer upper_key(index_schema->getRecordSize());
        Convert::fromString(
                index_type,
                index_length,
                expr->upper_value,
                index_col.getValue(Slice(upper_key))
        );
        Convert::maxLimit(
                primary_type,
                primary_length,
                primary_col.getValue(Slice(upper_key))
        );

        _index_view.reset(index_view->selectRange(
                _primary_schema,
                index_view->lowerBound(lower_key.content()),
                index_view->upperBound(upper_key.content())
        ));

        if (_index_view->count() > _threshold) {
            _index_view.reset();
        }
    }

    virtual void visit(FalseExpr *)
    { assert(false); }
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
            }
        }
    }

    virtual void visit(RangeExpr *expr)
    {
        auto col = _schema->getColumnByName(expr->column_name);
        auto lower_value = Convert::fromString(col.getType(), col.getField()->length, expr->lower_value);
        auto upper_value = Convert::fromString(col.getType(), col.getField()->length, expr->upper_value);
        auto less = Comparator::getCompareFuncByTypeLT(col.getType());
        auto data = col.getValue(_data);

        _result =
                !less(data.content(), lower_value.content()) &&
                 less(data.content(), upper_value.content());
    }

    virtual void visit(FalseExpr *)
    { _result = false; }
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
            throw TableTypeNotSupportedException();
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
            throw TableTypeNotSupportedException();
    }

    builder.setPrimary(column_name);

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
    factory.addIntegerField("count");

    factory.addTextField("create_sql");
    return factory.release();
}

Table::Table(DriverAccesser *accesser, Schema *schema, BlockIndex root)
        : _accesser(accesser), _schema(schema), _root(root), _count(0)
{ }

void
Table::select(Schema *schema, ConditionExpr *condition, Accesser accesser)
{
    std::unique_ptr<Schema> internal_schema;

    if (!schema) {
        internal_schema.reset(_schema->copy());
        schema = internal_schema.get();
    }
    else {
        std::set<std::string> columns_set = getColumnNames(condition);
        mergeColumnNamesInSchema(schema, columns_set);
        internal_schema.reset(buildSchemaFromColumnNames(columns_set));
    }

    auto primary_col = _schema->getPrimaryColumn();

    if (!condition) {
        std::unique_ptr<View> view(buildDataView());
        view.reset(view->select(schema));
        for (auto iter = view->begin(); iter != view->end(); iter.next()) {
            accesser(iter.constSlice());
        }
        return;
    }

    if (dynamic_cast<FalseExpr*>(condition)) {
        return;
    }

    std::unique_ptr<Schema> primary_schema(buildSchemaFromColumnNames(
            std::vector<std::string>{primary_col.getField()->name})
    );
    IndexVisitor v(this, primary_schema.get(), calculateThreshold());
    condition->accept(&v);
    std::unique_ptr<ModifiableView> indexed_view(v.release());

    std::unique_ptr<IndexView> data_view(buildDataView());

    std::unique_ptr<View> view;
    if (indexed_view) {
        view.reset(
                data_view->selectIndexed(
                        schema,
                        indexed_view->begin(),
                        indexed_view->end(),
                        buildFilter(condition)
                )
        );
    }
    else {
        view.reset(
                data_view->select(
                        schema,
                        buildFilter(condition)
                )
        );
    }
    for (auto iter = view->begin(); iter != view->end(); iter.next()) {
        accesser(iter.constSlice());
    }
}

View::Filter
Table::buildFilter(ConditionExpr *condition)
{
    // TODO
    return [=] (const Schema *schema, ConstSlice slice) -> bool
    {
        FilterVisitor v(schema, slice);
        condition->accept(&v);
        return v.result();
    };
}

Length
Table::calculateRecordPerBlock() const
{ return (Driver::BLOCK_SIZE / _schema->getRecordSize()); }

Length
Table::calculateThreshold() const
{ return _count / calculateRecordPerBlock(); }

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
                throw TableTypeNotSupportedException();
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

BlockIndex
Table::createIndex(std::string column_name)
{
    std::unique_ptr<Schema> index_schema(buildSchemaForIndex(column_name));
    BlockIndex index_root = _accesser->allocateBlock();

    std::unique_ptr<View> view(buildDataView());
    view.reset(view->select(index_schema.get()));

    std::unique_ptr<BTree> index_tree(buildIndexBTree(index_root, index_schema->copy()));
    index_tree->reset();

    for (auto iter = view->begin(); iter != view->end(); iter.next()) {
        auto slice = iter.constSlice();
        index_tree->insert(index_tree->makeKey(slice.content(), slice.length()));
    }

    _indices.emplace_back(column_name, index_root);
    return index_root;
}

void
Table::dropIndex(std::string column_name)
{
    auto index_root = findIndex(column_name);
    std::unique_ptr<BTree> index_tree(
            buildIndexBTree(
                    index_root,
                    buildSchemaForIndex(column_name)
            )
    );

    index_tree->clean();
    index_tree.reset();
    _accesser->freeBlock(index_root);
}

IndexView *
Table::buildDataView()
{
    return new IndexView(
            _schema->copy(),
            buildDataBTree()
    );
}

BTree *
Table::buildDataBTree()
{
    auto primary_col = _schema->getPrimaryColumn();
    return new BTree(
            _accesser,
            Comparator::getCompareFuncByTypeLT(primary_col.getType()),
            Comparator::getCompareFuncByTypeEQ(primary_col.getType()),
            _root,
            primary_col.getField()->length,
            _schema->getRecordSize()
    );
}

BTree *
Table::buildIndexBTree(BlockIndex root, Schema *index_schema)
{
    auto index_col = index_schema->getPrimaryColumn();
    auto index_type = index_col.getType();
    auto index_length = index_col.getField()->length;

    auto primary_col = _schema->getPrimaryColumn();
    auto primary_type = primary_col.getType();

    return new BTree(
            _accesser,
            Comparator::getCombineCmpFuncLT(index_type, index_length, primary_type),
            Comparator::getCombineCmpFuncEQ(index_type, index_length, primary_type),
            root,
            index_schema->getRecordSize(),
            0
    );
}

Schema *
Table::buildSchemaFromColumnNames(std::vector<std::string> column_names)
{
    Schema::Factory builder;
    for (auto &column : column_names) {
        auto col = _schema->getColumnByName(column);

        switch (col.getType()) {
            case Schema::Field::Type::INTEGER:
                builder.addIntegerField(column);
                break;
            case Schema::Field::Type::FLOAT:
                builder.addFloatField(column);
                break;
            case Schema::Field::Type::CHAR:
                builder.addCharField(column, col.getField()->length);
                break;
            default:
                throw TableTypeNotSupportedException();
        }
    }
    return builder.release();
}

void
Table::insert(Schema *schema, const std::vector<ConstSlice> &rows)
{
    std::vector<Schema::Field::ID> map_table;
    for (auto &field : *schema) {
        auto col_in_this = _schema->getColumnByName(field.name);
        assert(col_in_this.getType() == field.type);
        map_table.push_back(col_in_this.field_id);
    }

    auto primary_col = _schema->getPrimaryColumn();
    auto primary_length = primary_col.getField()->length;
    bool use_auto_increment = primary_col.getField()->isAutoIncreased() &&
            !schema->hasColumn(_schema->getPrimaryColumn().getField()->name);

    decltype(primary_col) remote_primary;

    assert(!use_auto_increment);
    if (!use_auto_increment) {
        remote_primary = schema->getPrimaryColumn();
    }

    std::unique_ptr<BTree> data_tree(buildDataBTree());
    std::vector<std::unique_ptr<BTree> > index_trees;

    for (auto &index : _indices) {
        std::unique_ptr<Schema> index_schema(buildSchemaForIndex(index.column_name));
        index_trees.emplace_back(buildIndexBTree(
                index.root,
                index_schema.get()
        ));
    }

    Buffer key_buff(primary_col.getField()->length);
    for (auto &row : rows) {
        assert(row.length() == schema->getRecordSize());
        if (use_auto_increment) {
            int autoinc_value = primary_col.getField()->autoIncrement();
            std::copy(
                    reinterpret_cast<const Byte *>(&autoinc_value),
                    reinterpret_cast<const Byte *>(&autoinc_value) + sizeof(autoinc_value),
                    key_buff.begin()
            );
        }
        else {
            auto key_field = remote_primary.getValue(row);
            std::copy(
                    key_field.cbegin(),
                    key_field.cend(),
                    key_buff.begin()
            );
        }
        auto iter = data_tree->insert(data_tree->makeKey(
                key_buff.content(),
                key_buff.length())
        );
        for (unsigned int i = 0; i < map_table.size(); ++i) {
            auto remote_col = _schema->getColumnById(map_table[i]);
            auto original_col = schema->getColumnById(i);

            auto original_slice = original_col.getValue(row);
            auto remote_slice = remote_col.getValue(iter.getValue());
            assert(remote_slice.length() >= original_slice.length());
            std::copy(
                    original_slice.cbegin(),
                    original_slice.cend(),
                    remote_slice.begin()
            );
        }

        for (unsigned int i = 0; i < index_trees.size(); ++i) {
            auto &column_name = _indices[i].column_name;
            auto index_col = _schema->getColumnByName(column_name);
            auto index_length = index_col.getField()->length;
            Buffer index_buf(index_length + primary_length);
            auto index_value = index_col.getValue(iter.getValue());
            std::copy(
                    index_value.cbegin(),
                    index_value.cend(),
                    index_buf.begin()
            );
            std::copy(
                    key_buff.cbegin(),
                    key_buff.cend(),
                    index_buf.begin() + index_length
            );

            auto &index_tree = index_trees[i];
            index_tree->insert(index_tree->makeKey(
                    index_buf.content(),
                    index_buf.length())
            );
        }
    }

    _count += rows.size();
}

Table::RecordBuilder *
Table::getRecordBuilder(std::vector<std::string> fields)
{ return new RecordBuilder(buildSchemaFromColumnNames(fields)); }

ConditionExpr *
Table::optimizeCondition(ConditionExpr *expr)
{
    OptimizeVisitor v(this);
    expr->accept(&v);
    return v.get();
}

void
Table::reset()
{
    std::unique_ptr<BTree> data_btree(buildDataBTree());
    data_btree->reset();

    for (auto &index : _indices) {
        std::unique_ptr<Schema> schema(buildSchemaForIndex(index.column_name));
        std::unique_ptr<BTree> index_btree(
                buildIndexBTree(
                    index.root,
                    schema.get()
                    )
                );
        index_btree->reset();
    }
}
