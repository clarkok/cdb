#include <cassert>

#include "schema.hpp"

using namespace cdb;

SchemaFactory::SchemaFactory()
{ reset(); }

std::unique_ptr<Schema>
SchemaFactory::reset()
{
    auto ret = std::move(_schema);
    _schema.reset(new Schema());
    return ret;
}

void
SchemaFactory::addIntegerField(std::string name)
{
    _schema->_fields.emplace_back(Field{
            FieldTypeSpec::INTEGER,
            name
        });

    if (_schema->_primary < 0) {
        _schema->_primary = _schema->_fields.size() - 1;
    }
}

void
SchemaFactory::addFloatField(std::string name)
{
    _schema->_fields.emplace_back(Field{
            FieldTypeSpec::FLOAT,
            name
        });
}

void
SchemaFactory::addCharField(std::string name, int length)
{
    name.resize(name.length() + sizeof(length));
    std::copy(
            reinterpret_cast<const char*>(&length),
            reinterpret_cast<const char*>(&length) + sizeof(length),
            name.end() - sizeof(length)
        );
    _schema->_fields.emplace_back(Field{
            FieldTypeSpec::CHAR,
            name
        });
}

void
SchemaFactory::addTextField(std::string name)
{
    _schema->_fields.emplace_back(Field{
            FieldTypeSpec::TEXT,
            name
        });
}

void
SchemaFactory::setPrimary(std::string name)
{
    for (
            auto iter = _schema->_fields.begin();
            iter != _schema->_fields.end();
            ++iter
    ) {
        if (name == 
                (iter->type.spec == FieldTypeSpec::CHAR 
                 ? iter->name.substr(0, iter->name.length() - 4) 
                 : iter->name)
        ) {
            _schema->_primary = iter - _schema->_fields.begin();
            return;
        }
    }
}

std::size_t 
Schema::getFieldSizeByType(FieldType type) 
{
    switch (type.spec) {
        case FieldTypeSpec::INTEGER:
            return 4;
        case FieldTypeSpec::FLOAT:
            return 4;
        case FieldTypeSpec::CHAR:
            return type.length;
        case FieldTypeSpec::TEXT:
            return 4;       // TODO
        default:
            return 0;
    }
}

std::size_t
Schema::getRecordSize() const
{
    std::size_t ret = 0;
    for (auto f : _fields) {
        ret += getFieldSizeByType(f.type);
    }
    return ret;
}

Column
Schema::getColumnByName(std::string name)
{
    std::size_t offset = 0;
    for (auto f : _fields) {
        if (f.name == name) {
            return Column(&f, offset);
        }
        offset += getFieldSizeByType(f.type);
    }
    throw 1;    // TODO throw column not found exception
}
