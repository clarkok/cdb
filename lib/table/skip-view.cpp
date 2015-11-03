//
// Created by c on 11/3/15.
//

#include "skip-view.hpp"

using namespace cdb;

View::Iterator
SkipView::begin()
{ return Iterator::make(this, new SkipView::IteratorImpl(_table->begin())); }

View::Iterator
SkipView::end()
{ return Iterator::make(this, new SkipView::IteratorImpl(_table->end())); }

View::Iterator
SkipView::lowerBound(const Byte *value)
{ return Iterator::make(this, new SkipView::IteratorImpl(_table->lowerBound(value))); }

View::Iterator
SkipView::upperBound(const Byte *value)
{ return Iterator::make(this, new SkipView::IteratorImpl(_table->upperBound(value))); }

View *
SkipView::peek(Schema::Column col, const Byte *lower_bound, const Byte *upper_bound)
{
    auto primary_col = _schema->getPrimaryColumn();
    assert(primary_col.getType() == Schema::Field::Type::INTEGER);

    SkipTable *table = new SkipTable(
            0,
            getIntegerCompareFunc()
    );

    auto cmp = getCompareFuncForType(col.getType());
    auto limit = _table->end();
    for (auto iter = _table->begin(); iter != limit; iter.next()) {
        auto *value = col.toValue<Byte>(*iter);
        if (cmp(lower_bound, value) && cmp(value, upper_bound)) {
            table->insert(primary_col.getValue(*iter));
        }
    }

    Schema::Factory schema_builder;
    schema_builder.addIntegerField(primary_col.getField()->name);

    return new SkipView(schema_builder.release(), table);
}

View *
SkipView::intersect(Iterator b, Iterator e)
{
    auto primary_col = _schema->getColumnById(0);
    auto other_col = b.getSchema()->getColumnById(0);

    assert(primary_col.getType() == Schema::Field::Type::INTEGER);
    assert(primary_col.getType() == other_col.getType());

    auto cmp = getCompareFuncForType(primary_col.getType());

    auto iter = _table->begin();
    while (true) {
        while (
                iter != _table->end() &&
                cmp(
                        primary_col.toValue<Byte>(*iter),
                        other_col.toValue<Byte>(b.slice())
                )) {
            iter = _table->erase(iter);
        }
        while (
            b != e &&
            cmp(
                    primary_col.toValue<Byte>(*iter),
                    other_col.toValue<Byte>(b.slice())
            )) {
            b.next();
        }
        if (iter != _table->end() && b != e) {
            iter.next();
            b.next();
        }
        else if (b != e) {
            while (iter != _table->end()) {
                iter = _table->erase(iter);
            }
        }
        else {
            break;
        }
    }
    return this;
}

View *
SkipView::join(Iterator b, Iterator e)
{
    auto primary_col = _schema->getColumnById(0);
    auto other_col = b.getSchema()->getColumnById(0);

    assert(_schema->getRecordSize() == b.getSchema()->getRecordSize());
    assert(primary_col.getType() == Schema::Field::Type::INTEGER);
    assert(primary_col.getType() == other_col.getType());

    auto cmp = getCompareFuncForType(primary_col.getType());

    while (b != e) {
        auto key = other_col.toValue<Byte>(b.slice());
        auto iter = _table->lowerBound(key);
        if (cmp(
                key,
                primary_col.toValue<Byte>(*iter)
        )) {
            _table->insert(b.slice());
        }
        b.next();
    }

    return this;
}

SkipTable::Comparator
SkipView::getCompareFuncForType(Schema::Field::Type type)
{
    switch (type) {
        case Schema::Field::Type::INTEGER:
            return getIntegerCompareFunc();
        case Schema::Field::Type::FLOAT:
            return getFloatCompareFunc();
        case Schema::Field::Type::CHAR:
            return getCharCompareFunc();
        default:
            assert(false);
    }
}

SkipTable::Comparator
SkipView::getIntegerCompareFunc()
{
    static auto func =
            [](SkipTable::Key a, SkipTable::Key b)
            {
                return *reinterpret_cast<const int*>(a) <
                       *reinterpret_cast<const int*>(b);
            };
    return func;
}

SkipTable::Comparator
SkipView::getFloatCompareFunc()
{
    static auto func =
            [](SkipTable::Key a, SkipTable::Key b)
            {
                return *reinterpret_cast<const float*>(a) <
                       *reinterpret_cast<const float*>(b);
            };
    return func;
}

SkipTable::Comparator
SkipView::getCharCompareFunc()
{
    static auto func =
            [](SkipTable::Key a, SkipTable::Key b)
            {
                return *reinterpret_cast<const float*>(a) <
                       *reinterpret_cast<const float*>(b);
            };
    return func;
}
