#ifndef _DB_TABLE_TABLE_H_
#define _DB_TABLE_TABLE_H_

#include <exception>
#include <functional>
#include <memory>
#include <vector>
#include <set>
#include <lib/utils/convert.hpp>

#include "schema.hpp"
#include "lib/condition/condition.hpp"
#include "lib/driver/driver-accesser.hpp"
#include "view.hpp"
#include "index-view.hpp"

namespace cdb {
    struct TableTypeNotSupportedException : public std::exception
    {
        virtual const char *
        what() const noexcept
        { return "Type not support currently"; }
    };

    struct TableIndexExistsException : public std::exception
    {
        std::string field;

        TableIndexExistsException(std::string field)
                : field(field)
        { }

        virtual const char *
        what() const noexcept
        { return ("Index exists on field `" + field + '`').c_str(); }
    };

    struct TableIndexNotFoundException : public std::exception
    {
        std::string field;

        TableIndexNotFoundException(std::string field)
                : field(field)
        { }

        virtual const char *
        what() const noexcept
        { return ("Index not found on field `" + field + '`').c_str(); }
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

        class OptimizeVisitor;
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
        Schema *buildSchemaFromColumnNames(std::set<std::string> columns_set);
        BlockIndex findIndex(std::string column_name);
        Schema *buildSchemaForIndex(std::string column_name);
        View::Filter buildFilter(ConditionExpr *condition);
        inline Length calculateThreshold() const;
        inline Length calculateRecordPerBlock() const;

        IndexView *buildDataView();
        BTree *buildDataBTree();
        BTree *buildIndexBTree(BlockIndex root, Schema *index_schema);

    public:
        static const int MAX_TABLE_NAME_LENGTH = 32;

        typedef std::function<void(ConstSlice)> Accesser;
        typedef std::function<void(Slice)> Operator;

        static Schema *getSchemaForRootTable();

        Schema *buildSchemaFromColumnNames(std::vector<std::string> column_names);

        inline Length
        getCount() const
        { return _count; }

        ConditionExpr *optimizeCondition(ConditionExpr *);

        /**
         * Select on this table
         *
         * @param schema null if select all fields
         * @param condition null if select all rows
         * @param accesser call on each row
         */
        void select(Schema *schema, ConditionExpr *condition, Accesser accesser);

        /**
         * Create an index on this table
         *
         * @param column_name the name of column to create index on
         * @return root block of the new Index
         */
        BlockIndex createIndex(std::string column_name);

        /**
         * Drop an index on this table
         *
         * @param column_name the name of column to drop index on
         */
        void dropIndex(std::string column_name);

        /**
         * Insert records into this table
         *
         * @param schema the schema in rows
         * @param rows data
         * @param row_count
         */
        void insert(Schema *schema, const std::vector<ConstSlice> &rows);

        class RecordBuilder;
        RecordBuilder *getRecordBuilder(std::vector<std::string> fields);

        class Factory
        {
            std::unique_ptr<Table> _table;
        public:
            Factory(
                    DriverAccesser *accesser,
                    Schema *schema,
                    BlockIndex root
            )
                    : _table(new Table(accesser, schema, root))
            { }

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

        class RecordBuilder
        {
            std::unique_ptr<Schema> _schema;
            std::vector<Buffer> _buffs;
            Length _column_index;

            RecordBuilder(Schema *schema)
                    : _schema(schema), _column_index(0)
            { }

            friend class Table;
        public:
            inline decltype(_buffs.begin())
            begin()
            { return _buffs.begin(); }

            inline decltype(_buffs.end())
            end()
            { return _buffs.end(); }

            inline RecordBuilder &
            reset()
            {
                _buffs.clear();
                _column_index = 0;

                return *this;
            }

            inline std::vector<ConstSlice>
            getRows() const
            {
                std::vector<ConstSlice> ret;
                for (auto &buffer : _buffs) {
                    ret.emplace_back(buffer);
                }
                return ret;
            }

            inline Schema *
            getSchema() const
            { return _schema.get(); }

            inline RecordBuilder &
            addRow()
            {
                _buffs.emplace_back(_schema->getRecordSize());
                _column_index = 0;

                return *this;
            }

            inline Schema::Field::Type
            currentType() const
            { return _schema->getColumnById(_column_index).getType(); }

            inline RecordBuilder &
            addInteger(std::string literal)
            {
                auto column = _schema->getColumnById(_column_index);
                auto type = column.getType();
                assert(type == Schema::Field::Type::INTEGER || type == Schema::Field::Type::FLOAT);
                Convert::fromString(
                        type,
                        column.getField()->length,
                        literal,
                        column.getValue(Slice(_buffs.back()))
                );

                ++_column_index;
                return *this;
            }

            inline RecordBuilder &
            addInteger(int value)
            {
                auto column = _schema->getColumnById(_column_index);
                auto type = column.getType();
                assert(type == Schema::Field::Type::INTEGER || type == Schema::Field::Type::FLOAT);

                if (type == Schema::Field::Type::INTEGER) {
                    *reinterpret_cast<int *>(column.getValue(Slice(_buffs.back())).content()) = value;
                }
                else {
                    *reinterpret_cast<float *>(column.getValue(Slice(_buffs.back())).content()) = value;
                }

                ++_column_index;
                return *this;
            }

            inline RecordBuilder &
            addFloat(std::string literal)
            {
                auto column = _schema->getColumnById(_column_index);
                auto type = column.getType();
                assert(type == Schema::Field::Type::FLOAT);
                Convert::fromString(
                        type,
                        column.getField()->length,
                        literal,
                        column.getValue(Slice(_buffs.back()))
                );

                ++_column_index;
                return *this;
            }

            inline RecordBuilder &
            addFloat(float value)
            {
                auto column = _schema->getColumnById(_column_index);
                auto type = column.getType();
                assert(type == Schema::Field::Type::FLOAT);

                *reinterpret_cast<float *>(column.getValue(Slice(_buffs.back())).content()) = value;

                ++_column_index;
                return *this;
            }

            inline RecordBuilder &
            addChar(std::string literal)
            {
                auto column = _schema->getColumnById(_column_index);
                auto type = column.getType();
                assert(type == Schema::Field::Type::CHAR);
                Convert::fromString(
                        type,
                        column.getField()->length,
                        literal,
                        column.getValue(Slice(_buffs.back()))
                );

                ++_column_index;
                return *this;
            }

            inline RecordBuilder &
            addValue(std::string literal)
            {
                switch (currentType()) {
                    case Schema::Field::Type::INTEGER:
                        return addInteger(literal);
                    case Schema::Field::Type::FLOAT:
                        return addFloat(literal);
                    case Schema::Field::Type::CHAR:
                        return addChar(literal);
                    default:
                        throw TableTypeNotSupportedException();
                }
            }
        };
    };
}

#endif // _DB_TABLE_TABLE_H_
