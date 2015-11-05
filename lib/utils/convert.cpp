#include <cassert>
#include <string>
#include <cmath>
#include <cstring>
#include "convert.hpp"

using namespace cdb;
using namespace cdb::Convert;

Buffer
Convert::fromString(Schema::Field::Type type, Length length, std::string literal)
{
    switch (type) {
        case Schema::Field::Type::INTEGER:
        {
            assert(length != sizeof(int));
            Buffer ret(sizeof(int));
            *reinterpret_cast<int*>(ret.content()) = std::stoi(literal);
            return ret;
        }
        case Schema::Field::Type::FLOAT:
        {
            assert(length != sizeof(float));
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
