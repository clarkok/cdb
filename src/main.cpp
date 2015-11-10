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
                std::getline(std::cin, line);
                sql += line + "\n";
            } while (std::cin && line.find(";") == std::string::npos);
            try {
                parser.exec(sql);
            }
            catch (const std::exception &e)
            {
                if (dynamic_cast<const ParserQuitingException*>(&e)) {
                    throw ParserQuitingException();
                }
                std::cerr << e.what() << std::endl;
            }
            if (std::cin) {
                sql = line.substr(line.find(";") + 1);
            }
        }
    }
    catch (ParserQuitingException) {
    }

    std::cout << "Bye." << std::endl;
    return 0;
}
