#include "index-view.hpp"
#include "skip-view.hpp"

using namespace cdb;

View::Iterator
IndexView::begin()
{ return Iterator::make(this, new IteratorImpl(_tree->begin())); }

View::Iterator
IndexView::end()
{ return Iterator::make(this, new IteratorImpl(_tree->end())); }

View::Iterator
IndexView::lowerBound(const Byte *key)
{
    auto primary_col_length = _schema->getPrimaryColumn().getField()->length;
    return Iterator::make(
            this,
            new IteratorImpl(
                    _tree->lowerBound(
                            _tree->makeKey(
                                    key,
                                    primary_col_length
                            )
                    )
            )
    );
}

View::Iterator
IndexView::upperBound(const Byte *key)
{
    auto primary_col_length = _schema->getPrimaryColumn().getField()->length;
    return Iterator::make(
            this,
            new IteratorImpl(
                    _tree->upperBound(
                            _tree->makeKey(
                                    key,
                                    primary_col_length
                            )
                    )
            )
    );
}

ModifiableView *
IndexView::peek(Schema::Column col, const Byte *lower_bound, const Byte *upper_bound)
{
    auto primary_col = _schema->getPrimaryColumn();
    assert(primary_col.getType() == Schema::Field::Type::INTEGER);

    SkipTable *table = new SkipTable(
            0,
            SkipView::getIntegerCompareFunc()
    );

    auto cmp = SkipView::getCompareFuncForType(col.getType());
    _tree->forEach([&](const BTree::Iterator &iter) {
        auto *value = col.toValue<Byte>(iter.getValue());
        if (cmp(lower_bound, value) && cmp(value, upper_bound)) {
            table->insert(primary_col.getValue(iter.getValue()));
        }
    });

    return new SkipView(
            Schema::Factory()
                    .addIntegerField(primary_col.getField()->name)
                    .release(),
            table
    );
}

