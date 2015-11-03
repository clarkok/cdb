#include <vector>

#include "view.hpp"
#include "skip-view.hpp"

using namespace cdb;

View *
View::select(Schema *schema)
{
    SkipTable *table = new SkipTable(
            schema->getPrimaryColumn().offset,
            SkipView::getCompareFuncForType(schema->getPrimaryColumn().getType())
    );

    std::vector<Schema::Field::ID> map_table;
    for (auto &field : *schema) {
        auto col_in_this = _schema->getColumnByName(field.name);
        map_table.push_back(col_in_this.field_id);
        assert(col_in_this.getType() == field.type);
    }

    Buffer row(schema->getRecordSize());

    for (auto iter = begin(); iter != end(); iter.next()) {
        for (int i = 0; i < map_table.size(); ++i) {
            auto original_col = _schema->getColumnById(map_table[i]);
            auto remote_col = schema->getColumnById(i);

            auto original_slice = original_col.getValue(iter.slice());
            std::copy(
                    original_slice.cbegin(),
                    original_slice.cend(),
                    remote_col.getValue(row).begin()
            );
        }
        table->insert(row);
    }

    return new SkipView(schema, table);
}
