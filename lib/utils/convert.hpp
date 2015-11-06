#ifndef _DB_UTILS_CONVERT_H_
#define _DB_UTILS_CONVERT_H_

#include <exception>

#include "lib/table/schema.hpp"
#include "buffer.hpp"

namespace cdb {
    namespace Convert {
        struct ConvertTypeError : public std::exception
        {
            std::string literal;

            ConvertTypeError(std::string literal)
                    : literal(literal)
            { }

            virtual const char *
            what() const noexcept
            { return ("Cannot convert " + literal).c_str(); }
        };

        Buffer fromString(Schema::Field::Type type, Length length, std::string literal);
        void fromString(Schema::Field::Type type, Length length, std::string literal, Slice buff);
        std::string toString(Schema::Field::Type type, ConstSlice slice);

        Buffer next(Schema::Field::Type type, Length length, ConstSlice original);
        Buffer prev(Schema::Field::Type type, Length length, ConstSlice original);

        Buffer minLimit(Schema::Field::Type type, Length length);
        Buffer maxLimit(Schema::Field::Type type, Length length);
        void minLimit(Schema::Field::Type type, Length length, Slice buff);
        void maxLimit(Schema::Field::Type type, Length length, Slice buff);
    }
}

#endif //_DB_UTILS_CONVERT_H_
