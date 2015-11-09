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

bool
Schema::hasColumn(std::string name) const
{
    for (auto &field : _fields) {
        if (field.name == name) {
            return true;
        }
    }
    return false;
}

Schema::Column
Schema::getColumnByName(std::string name) const
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
    throw SchemaColumnNotFoundException(name);
}

Schema::Column
Schema::getColumnById(Field::ID id) const
{
    assert(static_cast<decltype(_fields.size())>(id) < _fields.size());
    std::size_t offset = 0;
    auto iter = _fields.begin();
    while (id--) {
        offset += getFieldSize(&*iter);
        iter++;
    }
    return Column{this, iter->id, offset};
}

Schema::Column
Schema::getPrimaryColumn() const
{ return getColumnById(_primary_field); }

Schema *
Schema::copy() const
{
    Schema *ret = new Schema();
    ret->_fields = _fields;
    ret->_primary_field = _primary_field;
    return ret;
}

void
Schema::serialize(Slice slice) const
{
    auto iter = slice.begin();
    auto id = getPrimaryColumn().getField()->id;
    *reinterpret_cast<int*>(iter.start()) = id;
    iter += sizeof(int);
    for (auto &field : _fields) {
        std::copy(
                field.name.cbegin(),
                field.name.cend(),
                iter
            );
        iter += field.name.length();
        *iter++ = '\0';
        *iter++ = static_cast<Byte>(field.type);
        *reinterpret_cast<std::size_t*>(iter.start()) = field.length;
        iter += sizeof(field.length);
        *reinterpret_cast<int*>(iter.start()) = field.autoinc_value;
        iter += sizeof(field.autoinc_value);
    }
    *iter = '\0';
}

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

Schema::Factory &
Schema::Factory::addCharField(std::string name, std::size_t length)
{
    addField(Field::Type::CHAR, name, length);
    return *this;
}

Schema::Factory &
Schema::Factory::addFloatField(std::string name)
{
    addField(Field::Type::FLOAT, name, sizeof(float));
    return *this;
}

Schema::Factory &
Schema::Factory::addIntegerField(std::string name)
{
    addField(Field::Type::INTEGER, name, sizeof(int));
    if (_schema->_primary_field == std::numeric_limits<Field::ID>::max()) {
        _schema->_primary_field = _schema->_fields.back().id;
    }
    return *this;
}

Schema::Factory &
Schema::Factory::addTextField(std::string name)
{
    addField(Field::Type::TEXT, name, sizeof(int));
    return *this;
}

Schema::Factory &
Schema::Factory::setPrimary(std::string name)
{
    _schema->_primary_field = _schema->getColumnByName(name).field_id;
    return *this;
}

Schema::Factory &
Schema::Factory::setAutoincValue(int value)
{
    auto primary_col = _schema->getPrimaryColumn();
    primary_col.getField()->autoinc_value = value;
    return *this;
}

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

Schema *
Schema::Factory::parse(ConstSlice slice)
{
    auto iter = slice.cbegin();
    Schema::Factory builder;

    std::string primary_name = "";

    int primary_id = *reinterpret_cast<const int*>(iter.start());
    iter += sizeof(int);

    while (*iter) {
        std::string name(reinterpret_cast<const char*>(iter.start()));
        iter += name.length() + 1;
        auto type = static_cast<Field::Type>(*iter++);
        auto length = *reinterpret_cast<const std::size_t*>(iter.start());
        iter += sizeof(length);
        auto autoinc_value = *reinterpret_cast<const int*>(iter.start());
        iter += sizeof(autoinc_value);

        builder.addField(type, name, length);
        // TODO set auto inc value
        
        if (0 == primary_id --) {
            primary_name = name;
        }
    }

    builder.setPrimary(primary_name);

    return builder.release();
}


