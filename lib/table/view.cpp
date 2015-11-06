#include <vector>

#include "lib/utils/comparator.hpp"
#include "view.hpp"
#include "skip-view.hpp"

using namespace cdb;

ModifiableView *
View::select(Schema *schema, Filter filter)
{
    SkipTable *table = new SkipTable(
            schema->getPrimaryColumn().offset,
            Comparator::getCompareFuncByTypeLT(schema->getPrimaryColumn().getType())
    );

    std::vector<Schema::Field::ID> map_table;
    for (auto &field : *schema) {
        auto col_in_this = _schema->getColumnByName(field.name);
        assert(col_in_this.getType() == field.type);
        map_table.push_back(col_in_this.field_id);
    }

    Buffer row(schema->getRecordSize());

    for (auto iter = begin(); iter != end(); iter.next()) {
        if (!filter(_schema.get(), iter.constSlice())) {
            continue;
        }

        for (unsigned int i = 0; i < map_table.size(); ++i) {
            auto original_col = _schema->getColumnById(map_table[i]);
            auto remote_col = schema->getColumnById(i);

            auto original_slice = original_col.getValue(iter.constSlice());
            std::copy(
                    original_slice.cbegin(),
                    original_slice.cend(),
                    remote_col.getValue(Slice(row)).begin()
            );
        }
        table->insert(row);
    }

    return new SkipView(schema->copy(), table);
}

ModifiableView *
View::selectRange(Schema *schema, Iterator b, Iterator e, Filter filter)
{
    assert(b._owner == this);
    assert(e._owner == this);

    SkipTable *table = new SkipTable(
            schema->getPrimaryColumn().offset,
            Comparator::getCompareFuncByTypeLT(schema->getPrimaryColumn().getType())
    );

    std::vector<Schema::Field::ID> map_table;
    for (auto &field : *schema) {
        auto col_in_this = _schema->getColumnByName(field.name);
        assert(col_in_this.getType() == field.type);
        map_table.push_back(col_in_this.field_id);
    }

    Buffer row(schema->getRecordSize());

    for (; b != e; b.next()) {
        if (!filter(_schema.get(), b.constSlice())) {
            continue;
        }

        for (unsigned int i = 0; i < map_table.size(); ++i) {
            auto original_col = _schema->getColumnById(map_table[i]);
            auto remote_col = schema->getColumnById(i);

            auto original_slice = original_col.getValue(b.constSlice());
            std::copy(
                    original_slice.cbegin(),
                    original_slice.cend(),
                    remote_col.getValue(Slice(row)).begin()
            );
        }
        table->insert(row);
    }

    return new SkipView(schema->copy(), table);
}

ModifiableView *
View::selectIndexed(Schema *schema, Iterator b, Iterator e, cdb::View::Filter filter)
{
    assert(b.getSchema()->getPrimaryColumn().getField()->type == _schema->getPrimaryColumn().getField()->type);

    SkipTable *table = new SkipTable(
            schema->getPrimaryColumn().offset,
            Comparator::getCompareFuncByTypeLT(schema->getPrimaryColumn().getType())
    );

    auto equal = Comparator::getCompareFuncByTypeEQ(_schema->getPrimaryColumn().getField()->type);

    auto key_col = _schema->getPrimaryColumn();
    auto index_key_col = b.getSchema()->getColumnByName(key_col.getField()->name);

    std::vector<Schema::Field::ID> map_table;
    for (auto &field : *schema) {
        auto col_in_this = _schema->getColumnByName(field.name);
        assert(col_in_this.getType() == field.type);
        map_table.push_back(col_in_this.field_id);
    }

    Buffer row(schema->getRecordSize());

    for (; b != e; b.next()) {
        auto key = index_key_col.getValue(b.constSlice());
        auto iter = lowerBound(key.cbegin());

        if (!equal(
                index_key_col.getValue(b.constSlice()).cbegin(),
                key_col.getValue(iter.constSlice()).cbegin()
        )) {
            continue;
        }

        if (!filter(_schema.get(), iter.constSlice())) {
            continue;
        }

        for (unsigned int i = 0; i < map_table.size(); ++i) {
            auto original_col = _schema->getColumnById(map_table[i]);
            auto remote_col = schema->getColumnById(i);

            auto original_slice = original_col.getValue(iter.constSlice());
            std::copy(
                    original_slice.cbegin(),
                    original_slice.cend(),
                    remote_col.getValue(Slice(row)).begin()
            );
        }
        table->insert(row);
    }

    return new SkipView(schema->copy(), table);
}
