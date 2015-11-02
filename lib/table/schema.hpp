#ifndef _DB_TABLE_SCHEMA_H_
#define _DB_TABLE_SCHEMA_H_

#include <memory>
#include <string>
#include <vector>

#include "lib/utils/slice.hpp"

namespace cdb {
    enum class FieldTypeSpec
    {
        UNKNOWN,
        INTEGER,
        FLOAT,
        CHAR,
        TEXT
    };

    struct FieldType
    {
        FieldTypeSpec spec;
        unsigned int length : sizeof(FieldTypeSpec) * 8;

        FieldType(FieldTypeSpec spec, unsigned int length = 255)
            : spec(spec), length(length)
        { }
    };

    static_assert(sizeof(FieldTypeSpec) * 8 <= 32, "FieldTypeSpec too large, won't compile.");

    struct Field
    {
        FieldType type;
        std::string name;
    };

    class Column
    {
        Field *_fields;
        std::size_t _offset;
    public:
        Column(Field *field, std::size_t offset);

        template<typename T>
        T *toValue(Slice row)
        { return *reinterpret_cast<T*>(row.content() + _offset); }
    };

    class Schema
    {
        std::vector<Field> _fields;
        int _primary = -1;
        bool _primary_auto_increment;
        int _primary_autoinc_value;

        Schema() = default;
    public:
        static std::size_t getFieldSizeByType(FieldType type);
        std::size_t getRecordSize() const;

        Column getColumnByName(std::string name);

        friend class SchemaFactory;
    };

    class SchemaFactory
    {
        std::unique_ptr<Schema> _schema;
    public:
        SchemaFactory();
        std::unique_ptr<Schema> reset();

        void addIntegerField(std::string name);
        void addFloatField(std::string name);
        void addCharField(std::string name, int length);
        void addTextField(std::string name);
        void setPrimary(std::string name);
    };
}

#endif // _DB_TABLE_SCHEMA_H_
