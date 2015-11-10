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

    namespace Comparator {
        typedef std::function<bool(const Byte *, const Byte *)> CmpFunc;

        CmpFunc getIntegerCompareFuncLT();
        CmpFunc getFloatCompareFuncLT();
        CmpFunc getCharCompareFuncLT();
        CmpFunc getCompareFuncByTypeLT(Schema::Field::Type type);

        CmpFunc getIntegerCompareFuncEQ();
        CmpFunc getFloatCompareFuncEQ();
        CmpFunc getCharCompareFuncEQ();
        CmpFunc getCompareFuncByTypeEQ(Schema::Field::Type type);

        CmpFunc getCombineCmpFuncLT(
                Schema::Field::Type typea,
                Length a_length,
                Schema::Field::Type typeb
        );
        CmpFunc getCombineCmpFuncEQ(
                Schema::Field::Type typea,
                Length a_length,
                Schema::Field::Type typeb
        );
    };
}

#endif //_DB_UTILS_COMPARATOR_H_
