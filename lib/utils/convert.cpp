#include <cassert>
#include <string>
#include <cmath>
#include <cstring>
#include "convert.hpp"

#include <iostream>

using namespace cdb;
using namespace cdb::Convert;

Buffer
Convert::fromString(Schema::Field::Type type, Length length, std::string literal)
{
    switch (type) {
        case Schema::Field::Type::INTEGER:
        {
            assert(length == sizeof(int));
            Buffer ret(sizeof(int));
            *reinterpret_cast<int*>(ret.content()) = std::stoi(literal);
            return ret;
        }
        case Schema::Field::Type::FLOAT:
        {
            assert(length == sizeof(float));
            Buffer ret(sizeof(float));
            *reinterpret_cast<float*>(ret.content()) = std::stof(literal);
            return ret;
        }
        case Schema::Field::Type::CHAR:
        {
            if (literal.length() >= length) {
                throw ConvertTypeError(literal);
            }
            Buffer ret(length);
            std::copy(
                    literal.cbegin(),
                    literal.cend(),
                    ret.begin()
            );
            std::fill(
                    ret.begin() + literal.length(),
                    ret.end(),
                    0
            );
            return ret;
        }
        case Schema::Field::Type::TEXT:
        {
            throw ConvertTypeError(literal);
        }
    }
}

void
Convert::fromString(Schema::Field::Type type, Length length, std::string literal, Slice buff)
{
    assert(buff.length() >= length);
    switch (type) {
        case Schema::Field::Type::INTEGER:
        {
            assert(length == sizeof(int));
            *reinterpret_cast<int*>(buff.content()) = std::stoi(literal);
            break;
        }
        case Schema::Field::Type::FLOAT:
        {
            assert(length == sizeof(float));
            *reinterpret_cast<float*>(buff.content()) = std::stof(literal);
            break;
        }
        case Schema::Field::Type::CHAR:
        {
            if (literal.length() >= length) {
                throw ConvertTypeError(literal);
            }
            std::copy(
                    literal.cbegin(),
                    literal.cend(),
                    buff.begin()
            );
            std::fill(
                    buff.begin() + literal.length(),
                    buff.end(),
                    0
            );
            break;
        }
        case Schema::Field::Type::TEXT:
        {
            throw ConvertTypeError(literal);
        }
    }
}

std::string
Convert::toString(Schema::Field::Type type, ConstSlice slice)
{
    switch (type) {
        case Schema::Field::Type::INTEGER:
        {
            assert(slice.length() >= sizeof(int));
            return std::to_string(*reinterpret_cast<const int*>(slice.content()));
        }
        case Schema::Field::Type::FLOAT:
        {
            assert(slice.length() >= sizeof(float));
            return std::to_string(*reinterpret_cast<const float*>(slice.content()));
        }
        case Schema::Field::Type::CHAR:
        {
            return std::string(reinterpret_cast<const char*>(slice.content()));
        }
        case Schema::Field::Type::TEXT:
        {
            throw ConvertTypeError("TEXT");
        }
    }
}

Buffer
Convert::next(Schema::Field::Type type, Length length, ConstSlice original)
{
    switch (type) {
        case Schema::Field::Type::INTEGER:
        {
            assert(original.length() >= sizeof(int));
            Buffer ret(length);
            *reinterpret_cast<int *>(ret.content()) = *reinterpret_cast<const int*>(original.content()) + 1;
            return ret;
        }
        case Schema::Field::Type::FLOAT:
        {
            assert(original.length() >= sizeof(float));
            Buffer ret(length);
            float orig = *reinterpret_cast<const float*>(original.content());
            *reinterpret_cast<float *>(ret.content()) =
                    std::nextafterf(orig, orig + 1.0f); // TODO buggy
            return ret;
        }
        case Schema::Field::Type::CHAR:
        {
            Buffer ret(original.cbegin(), original.cend());
            long slen = std::strlen(reinterpret_cast<const char*>(original.content()));
            auto orig_len = slen;
            while (slen >= 0 && ret.content()[--slen] == std::numeric_limits<Byte>::max()) { }
            if (slen >= 0) {
                ++ret.content()[slen];
                return ret;
            }
            if (orig_len + 1 == length) {
                throw ConvertTypeError(std::string(reinterpret_cast<const char*>(original.content())));
            }
            ret.content()[orig_len] = 1;
            ret.content()[orig_len + 1] = 0;
            return ret;
        }
        case Schema::Field::Type::TEXT:
        {
            throw ConvertTypeError("TEXT");
        }
    }
}

Buffer
Convert::prev(Schema::Field::Type type, Length length, ConstSlice original)
{
    switch (type) {
        case Schema::Field::Type::INTEGER:
        {
            assert(original.length() >= sizeof(int));
            Buffer ret(length);
            *reinterpret_cast<int *>(ret.content()) = *reinterpret_cast<const int*>(original.content()) - 1;
            return ret;
        }
        case Schema::Field::Type::FLOAT:
        {
            assert(original.length() >= sizeof(float));
            Buffer ret(length);
            float orig = *reinterpret_cast<const float*>(original.content());
            *reinterpret_cast<float *>(ret.content()) =
                    std::nextafterf(orig, orig - 1.0f); // TODO buggy
            return ret;
        }
        case Schema::Field::Type::CHAR:
        {
            Buffer ret(original.cbegin(), original.cend());
            auto slen = std::strlen(reinterpret_cast<const char*>(original.content()));
            if (!slen) {
                throw ConvertTypeError(std::string(reinterpret_cast<const char*>(original.content())));
            }
            --ret.content()[slen - 1];
            return ret;
        }
        case Schema::Field::Type::TEXT:
        {
            throw ConvertTypeError("TEXT");
        }
    }
}

void
Convert::minLimit(Schema::Field::Type type, Length length, Slice buff)
{
    switch (type) {
        case Schema::Field::Type::INTEGER:
            assert(length == sizeof(int));
            *reinterpret_cast<int *>(buff.content()) = std::numeric_limits<int>::min();
            break;
        case Schema::Field::Type::FLOAT:
            assert(length == sizeof(float));
            *reinterpret_cast<float *>(buff.content()) = std::numeric_limits<float>::min();
            break;
        case Schema::Field::Type::CHAR:
            *reinterpret_cast<char *>(buff.content()) = '\0';   // only first char in enough
            break;
        default:
            throw ConvertTypeError("TEXT");
    }
}

void
Convert::maxLimit(Schema::Field::Type type, Length length, Slice buff)
{
    switch (type) {
        case Schema::Field::Type::INTEGER:
            assert(length == sizeof(int));
            *reinterpret_cast<int *>(buff.content()) = std::numeric_limits<int>::max();
            break;
        case Schema::Field::Type::FLOAT:
            assert(length == sizeof(float));
            *reinterpret_cast<float *>(buff.content()) = std::numeric_limits<float>::max();
            break;
        case Schema::Field::Type::CHAR:
            assert(length == buff.length());
            std::fill(
                    buff.begin(),
                    buff.end(),
                    std::numeric_limits<char>::max()
            );
            *reinterpret_cast<char *>(buff.content() + length - 1) = '\0';
            break;
        default:
            throw ConvertTypeError("TEXT");
    }
}

Buffer
Convert::minLimit(Schema::Field::Type type, Length length)
{
    Buffer ret(length);
    minLimit(type, length, ret);
    return ret;
}

Buffer
Convert::maxLimit(Schema::Field::Type type, Length length)
{
    Buffer ret(length);
    maxLimit(type, length, ret);
    return ret;
}
