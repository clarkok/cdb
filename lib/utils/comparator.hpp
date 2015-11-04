//
// Created by c on 11/4/15.
//

#ifndef _DB_UTILS_COMPARATOR_H_
#define _DB_UTILS_COMPARATOR_H_

#include <exception>
#include <functional>
#include "buffer.hpp" // Byte

#include "lib/table/schema.hpp"

namespace cdb {
    struct ComparatorUnknownTypeException : public std::exception
    {
        virtual const char *
        what() const _GLIBCXX_NOEXCEPT
        { return "Unknown type when get Comparator"; }
    };

    class Comparator
    {
    public:
        typedef std::function<bool(const Byte *, const Byte *)> CmpFunc;

        static CmpFunc getIntegerCompareFuncLT();
        static CmpFunc getFloatCompareFuncLT();
        static CmpFunc getCharCompareFuncLT();
        static CmpFunc getCompareFuncByTypeLT(Schema::Field::Type type);

        static CmpFunc getIntegerCompareFuncEQ();
        static CmpFunc getFloatCompareFuncEQ();
        static CmpFunc getCharCompareFuncEQ();
        static CmpFunc getCompareFuncByTypeEQ(Schema::Field::Type type);
    };
}

#endif //_DB_UTILS_COMPARATOR_H_
