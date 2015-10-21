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
            FieldType::INTEGER,
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
            FieldType::FLOAT,
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
            FieldType::CHAR,
            name
        });
}

void
SchemaFactory::addTextField(std::string name)
{
    _schema->_fields.emplace_back(Field{
            FieldType::TEXT,
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
                (iter->type == FieldType::CHAR 
                 ? iter->name.substr(0, iter->name.length() - 4) 
                 : iter->name)
        ) {
            _schema->_primary = iter - _schema->_fields.begin();
            return;
        }
    }
}

