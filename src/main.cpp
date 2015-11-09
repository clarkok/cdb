#include <iostream>
#include <cstring>

#include "lib/parser/parser.hpp"

using namespace cdb;

int
main(int, char**)
{
    std::string sql;
    std::string line;
    cdb::Parser parser(getGlobalDatabase());

    try {
        while (std::cin) {
            do {
                std::cin >> line;
                sql += line + " ";
            } while (std::cin && line.find(";") == std::string::npos);
            parser.exec(sql);
            sql = "";
        }
    }
    catch (ParserQuitingException) {
    }

    std::cout << "Bye." << std::endl;
    return 0;
}
