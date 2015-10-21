#ifndef _DB_TABLE_SCHEMA_H_
#define _DB_TABLE_SCHEMA_H_

#include <memory>
#include <string>
#include <vector>

namespace cdb {
    enum class FieldType
    {
        UNKNOWN,
        INTEGER,
        FLOAT,
        CHAR,
        TEXT
    };

    struct Field
    {
        FieldType type;
        std::string name;
    };

    class Schema
    {
        std::vector<Field> _fields;
        int _primary = -1;

        Schema() = default;
    public:
        static constexpr std::size_t getFieldSizeByType(FieldType type);
        std::size_t getRecordSize() const;

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
