#include <iostream>
#include "lib/utils/comparator.hpp"
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

ModifiableView *
SkipView::peek(Schema::Column col, const Byte *lower_bound, const Byte *upper_bound)
{
    auto primary_col = _schema->getPrimaryColumn();
    assert(primary_col.getType() == Schema::Field::Type::INTEGER);

    SkipTable *table = new SkipTable(
            0,
            Comparator::getIntegerCompareFuncLT()
    );

    auto cmp = Comparator::getCompareFuncByTypeLT(col.getType());
    auto limit = _table->end();
    for (auto iter = _table->begin(); iter != limit; iter = iter.next()) {
        auto *value = col.toValue<Byte>(*iter);
        if (cmp(lower_bound, value) && cmp(value, upper_bound)) {
            table->insert(primary_col.getValue(*iter));
        }
    }

    return new SkipView(
            Schema::Factory()
                    .addIntegerField(primary_col.getField()->name)
                    .release(),
            table
    );
}

ModifiableView *
SkipView::intersect(Iterator b, Iterator e)
{
    auto primary_col = _schema->getColumnById(0);
    auto other_col = b.getSchema()->getColumnById(0);

    assert(primary_col.getType() == Schema::Field::Type::INTEGER);
    assert(primary_col.getType() == other_col.getType());

    auto cmp = Comparator::getCompareFuncByTypeLT(primary_col.getType());

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

        if (iter == _table->end()) {
            break;
        }

        while (
            b != e &&
            cmp(
                    other_col.toValue<Byte>(b.slice()),
                    primary_col.toValue<Byte>(*iter)
            )) {
            b.next();
        }

        if (iter != _table->end() && b != e) {
            iter = iter.next();
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

ModifiableView *
SkipView::join(Iterator b, Iterator e)
{
    auto primary_col = _schema->getColumnById(0);
    auto other_col = b.getSchema()->getColumnById(0);

    assert(_schema->getRecordSize() == b.getSchema()->getRecordSize());
    assert(primary_col.getType() == Schema::Field::Type::INTEGER);
    assert(primary_col.getType() == other_col.getType());

    auto cmp = Comparator::getCompareFuncByTypeLT(primary_col.getType());

    while (b != e) {
        auto key = other_col.toValue<Byte>(b.slice());
        auto iter = _table->lowerBound(key);
        if (
                iter == _table->end() ||
                cmp(
                        key,
                        primary_col.toValue<Byte>(*iter)
                )) {
            _table->insert(b.slice());
        }
        b.next();
    }

    return this;
}

