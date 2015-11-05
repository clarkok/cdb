#ifndef _DB_TABLE_TABLE_H_
#define _DB_TABLE_TABLE_H_

#include <exception>
#include <functional>
#include <memory>
#include <vector>
#include <set>

#include "schema.hpp"
#include "lib/condition/condition.hpp"
#include "lib/driver/driver-accesser.hpp"
#include "view.hpp"

namespace cdb {
    struct TableFieldNotSupportedException : public std::exception
    {
        virtual const char *
        what() const _GLIBCXX_NOEXCEPT
        { return "Type not support currently"; }
    };

    class Table
    {
        struct Index
        {
            std::string column_name;
            BlockIndex root;

            Index(std::string column_name, BlockIndex root)
                    : column_name(column_name), root(root)
            { }
        };

        class IndexVisitor;
        class FilterVisitor;

        DriverAccesser *_accesser;
        std::unique_ptr<Schema> _schema;
        BlockIndex _root;
        std::vector<Index> _indices;
        Length _count;

        Table(DriverAccesser *accesser, Schema *schema, BlockIndex root);

        static std::set<std::string> getColumnNames(ConditionExpr *expr);
        static std::set<std::string> mergeColumnNamesInSchema(Schema *schema, std::set<std::string> &set);
        Schema *buildSchemaFromColumnNames(std::set<std::string>(columns_set));
        BlockIndex findIndex(std::string column_name);
        Schema *buildSchemaForIndex(std::string column_name);
        inline Length calculateThreshold() const;
        View::Filter buildFilter(ConditionExpr *condition);

    public:
        static const int MAX_TABLE_NAME_LENGTH = 32;

        typedef std::function<void(ConstSlice)> Accesser;
        typedef std::function<void(Slice)> Operator;

        static Schema *getSchemaForRootTable();

        /**
         * @param schema null if select all fields
         * @param condition null if select all rows
         * @param accesser call on each row
         */
        void select(Schema *schema, ConditionExpr *condition, Accesser accesser);

        class Factory
        {
            std::unique_ptr<Table> _table;
        public:
            Factory() = default;
            ~Factory() = default;

            Table *release()
            { return _table.release(); }

            Factory &
            addIndex(std::string column_name, BlockIndex root)
            {
                _table->_indices.emplace_back(column_name, root);
                return *this;
            }
        };
    };
}

#endif // _DB_TABLE_TABLE_H_
