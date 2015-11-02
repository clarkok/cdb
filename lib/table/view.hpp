#ifndef _DB_TABLE_VIEW_H_
#define _DB_TABLE_VIEW_H_

#include <memory>

#include "schema.hpp"

namespace cdb {
    class View
    {
    public:
        typedef std::shared_ptr<View> ViewPtr;

        struct Iterator
        {
        };

    protected:
        std::weak_ptr<View> _this;

        std::unique_ptr<Schema> _schema;

        View(std::unique_ptr<Schema> schema);
        static ViewPtr create(std::unique_ptr<Schema> schema);
    public:
        virtual ~View() = default;

    };
}

#endif // _DB_TABLE_VIEW_H_
