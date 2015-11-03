#include <cassert>
#include <lib/driver/driver.hpp>

#include "schema.hpp"

using namespace cdb;

std::size_t
Schema::getFieldSize(const Field *field)
{ return field->length; }

std::size_t
Schema::getRecordSize() const
{
    std::size_t ret = 0;
    for (auto &field : _fields) {
        ret += getFieldSize(&field);
    }
    return ret;
}

Schema::Column
Schema::getColumnByName(std::string name)
{
    std::size_t offset = 0;
    for (auto &field : _fields) {
        if (field.name == name) {
            return Column{this, field.id, offset};
        }
        else {
            offset += getFieldSize(&field);
        }
    }
    return Column{this, 0, 0};
}

Schema::Column
Schema::getColumnById(Field::ID id)
{
    std::size_t offset = 0;
    auto iter = _fields.begin();
    while (id--) {
        offset += getFieldSize(&*iter);
    }
    return Column{this, iter->id, offset};
}

Schema::Column
Schema::getPrimaryColumn()
{ return getColumnById(_primary_field); }

Schema::Factory::Factory()
{ reset(); }

Schema *
Schema::Factory::reset()
{
    auto ret = _schema.release();
    _schema.reset(new Schema());
    return ret;
}

Schema *
Schema::Factory::release(){
    return _schema.release();
}

void
Schema::Factory::addCharField(std::string name, std::size_t length)
{ addField(Field::Type::CHAR, name, length); }

void
Schema::Factory::addFloatField(std::string name)
{ addField(Field::Type::FLOAT, name, sizeof(float)); }

void
Schema::Factory::addIntegerField(std::string name)
{
    addField(Field::Type::INTEGER, name, sizeof(int));
    if (_schema->_primary_field == std::numeric_limits<Field::ID>::max()) {
        _schema->_primary_field = _schema->_fields.back().id;
    }
}

void
Schema::Factory::addTextField(std::string name)
{ addField(Field::Type::TEXT, name, sizeof(int)); }

void
Schema::Factory::setPrimary(std::string name)
{ _schema->_primary_field = _schema->getColumnByName(name).field_id; }

void
Schema::Factory::addField(Field::Type type, std::string name, std::size_t length)
{
    _schema->_fields.emplace_back(
            type,
            length,
            name,
            static_cast<Field::ID>(_schema->_fields.size()),
            0
    );
}
