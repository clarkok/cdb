#ifndef _DB_TABLE_DATA_H_
#define _DB_TABLE_DATA_H_

#include "lib/driver/buffer.hpp"

namespace cdb {
    class Data
    {
    protected:
        BufferView _content;
    public:
        Data(const BufferView &content);
        virtual ~Data() = default;
        BufferView getView() const;
        void setView(const BufferView &content);
    };

    class IntegerData : public Data
    {
    public:
        typedef std::int32_t Integer;
    };
}

#endif // _DB_TABLE_DATA_H_
