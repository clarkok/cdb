#ifndef _DB_TABLE_SCHEMA_H_
#define _DB_TABLE_SCHEMA_H_

#include <memory>
#include <string>
#include <vector>

#include "lib/utils/slice.hpp"

namespace cdb {
    /**
     * class to representing a Schema for database table
     */
    class Schema
    {
    public:
        /**
         * A field for schema, just represent type, not data
         */
        struct Field
        {
            /** type for field ID */
            typedef int ID;

            /** type for field type */
            enum class Type
            {
                INTEGER,
                FLOAT,
                CHAR,
                TEXT
            };

            Type type;
            std::size_t length;
            std::string name;
            ID id;
            int autoinc_value = 0;

            Field(Type type, std::size_t length, std::string name, ID id, int autoinc_value)
                    : type(type), length(length), name(name), id(id), autoinc_value(autoinc_value)
            { }

            /**
             * increment the auto increment value and return
             *
             * @return the new auto increment value
             */
            inline int
            autoIncrement()
            { return ++autoinc_value; }
        };

        /**
         * A column for schema, represent a field in a row
         */
        struct Column
        {
            Schema *owner;
            Field::ID field_id;
            std::size_t offset;

            inline Field *
            getField() const
            { return &owner->_fields[field_id]; }

            inline Field::Type
            getType() const
            { return getField()->type; }

            inline Slice
            getValue(Slice row)
            {
                return row.subSlice(
                        static_cast<Length>(offset),
                        static_cast<Length>(getFieldSize(getField()))
                );
            }

            /**
             * Fetch the data of this column in a row
             */
            template<typename T>
            T *toValue(Slice row)
            { return reinterpret_cast<T*>(row.content() + offset); }
        };

    private:
        std::vector<Field> _fields;
        Field::ID _primary_field = std::numeric_limits<Field::ID>::max();

        Schema() = default;

    public:
        /**
         * get the size of the given field
         *
         * @param field to calculate size of
         * @return the field size
         */
        static std::size_t getFieldSize(const Field *field);

        /**
         * get the size of a record of this schema
         *
         * @return the record size
         */
        std::size_t getRecordSize() const;

        /**
         * get a Column by name
         *
         * @param name of the column to search
         * @return the column of the given name
         */
        Column getColumnByName(std::string name);

        /**
         * get a Column by id
         *
         * @param id of the column to fetch
         * @return the column of the given id
         */
        Column getColumnById(Field::ID id);

        /**
         * get the primary column
         *
         * @return the primary key's column
         */
        Column getPrimaryColumn();

        inline decltype(_fields.cbegin())
        begin() const
        { return _fields.cbegin(); }

        inline decltype(_fields.cend())
        end() const
        { return _fields.cend(); }

        class Factory
        {
            std::unique_ptr<Schema> _schema;

            void addField(Field::Type type, std::string name, std::size_t length);
        public:
            Factory();
            Schema *reset();
            Schema *release();

            Factory &addCharField(std::string name, std::size_t length);
            Factory &addFloatField(std::string name);
            Factory &addIntegerField(std::string name);
            Factory &addTextField(std::string name);
            Factory &setPrimary(std::string name);
        };
    };
}

#endif // _DB_TABLE_SCHEMA_H_
