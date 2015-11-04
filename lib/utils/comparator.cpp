#include <cstring>
#include "comparator.hpp"

using namespace cdb;

Comparator::CmpFunc
Comparator::getIntegerCompareFuncLT()
{
    static CmpFunc func =
            [](const Byte *a, const Byte *b)
            {
                return *reinterpret_cast<const int*>(a) <
                       *reinterpret_cast<const int*>(b);
            };
    return func;
}

Comparator::CmpFunc
Comparator::getFloatCompareFuncLT()
{
    static CmpFunc func =
            [](const Byte *a, const Byte *b)
            {
                return *reinterpret_cast<const float*>(a) <
                       *reinterpret_cast<const float*>(b);
            };
    return func;
}

Comparator::CmpFunc
Comparator::getCharCompareFuncLT()
{
    static CmpFunc func =
            [](const Byte *a, const Byte *b)
            {
                return std::strcmp(
                        reinterpret_cast<const char *>(a),
                        reinterpret_cast<const char *>(b)
                ) < 0;
            };
    return func;
}

Comparator::CmpFunc
Comparator::getCompareFuncByTypeLT(Schema::Field::Type type)
{
    switch (type) {
        case Schema::Field::Type::INTEGER:
            return getIntegerCompareFuncLT();
        case Schema::Field::Type::FLOAT:
            return getFloatCompareFuncLT();
        case Schema::Field::Type::CHAR:
            return getCharCompareFuncLT();
        default:
            throw ComparatorUnknownTypeException();
    }
}

Comparator::CmpFunc
Comparator::getIntegerCompareFuncEQ()
{
    static CmpFunc func =
            [](const Byte *a, const Byte *b)
            {
                return *reinterpret_cast<const int*>(a) ==
                       *reinterpret_cast<const int*>(b);
            };
    return func;
}

Comparator::CmpFunc
Comparator::getFloatCompareFuncEQ()
{
    static CmpFunc func =
            [](const Byte *a, const Byte *b)
            {
                return *reinterpret_cast<const float*>(a) ==
                       *reinterpret_cast<const float*>(b);
            };
    return func;
}

Comparator::CmpFunc
Comparator::getCharCompareFuncEQ()
{
    static CmpFunc func =
            [](const Byte *a, const Byte *b)
            {
                return std::strcmp(
                        reinterpret_cast<const char *>(a),
                        reinterpret_cast<const char *>(b)
                ) == 0;
            };
    return func;
}

Comparator::CmpFunc
Comparator::getCompareFuncByTypeEQ(Schema::Field::Type type)
{
    switch (type) {
        case Schema::Field::Type::INTEGER:
            return getIntegerCompareFuncEQ();
        case Schema::Field::Type::FLOAT:
            return getFloatCompareFuncEQ();
        case Schema::Field::Type::CHAR:
            return getCharCompareFuncEQ();
        default:
            throw ComparatorUnknownTypeException();
    }
}
