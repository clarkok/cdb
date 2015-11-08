#ifndef _DB_DATABASE_DATABASE_H_
#define _DB_DATABASE_DATABASE_H_

#include <exception>
#include <list>
#include "lib/driver/driver.hpp"
#include "lib/driver/block-allocator.hpp"
#include "lib/driver/driver-accesser.hpp"
#include "lib/table/table.hpp"

namespace cdb {
    struct DatabaseInvalidException : public std::exception
    {
        const char *what() const noexcept
        { return "Database is invalid."; }
    };

    struct DatabaseTableNotFoundException : public std::exception
    {
        std::string name;

        DatabaseTableNotFoundException(std::string name)
            : name(name)
        { }

        const char *what() const noexcept
        { return ("Table " + name + " not found").c_str(); }
    };

    class Database
    {
        std::unique_ptr<Driver> _driver;
        std::unique_ptr<BlockAllocator> _allocator;
        std::unique_ptr<DriverAccesser> _accesser;

        std::list<std::unique_ptr<Table> > _tables;
        std::unique_ptr<Table> _root_table;

        struct DBHeader;

        Database() = default;
        Database(const Database &) = delete;
        Database & operator = (const Database &) = delete;

        void open();
        void close();
    public:
        static constexpr const char MAGIC[8] = "--CDB--";

        ~Database();

        void init();
        void reset();

        Table *getTableByName(std::string name);
        Table *createTable(std::string name, Schema *schema);
        void dropTable(std::string name);

        static Database *Factory(std::string path);
    };
}

#endif // _DB_DATABASE_DATABASE_H_
