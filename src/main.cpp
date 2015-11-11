#include <iostream>
#include <cstring>
#include <ctime>

#include "lib/parser/parser.hpp"

using namespace cdb;

static const char PROMPT[] = "DB> ";
static const char LINE_BREAKER[] = "\\   ";

int
main(int, char**)
{
    std::string sql;
    std::string line;
    cdb::Parser parser(getGlobalDatabase());

    try {
        while (std::cin) {

            do {
                std::cout << (sql.length() ? LINE_BREAKER : PROMPT);
                std::getline(std::cin, line);
                sql += line + "\n";
            } while (std::cin && line.find(";") == std::string::npos);

            try {
                auto t = std::clock();
                parser.exec(sql);
                t = std::clock() - t;
                std::cout << (static_cast<float>(t)) / CLOCKS_PER_SEC << "s." << std::endl << std::endl;
            }
            catch (const std::exception &e)
            {
                if (dynamic_cast<const ParserQuitingException*>(&e)) {
                    throw ParserQuitingException();
                }
                std::cerr << e.what() << std::endl << std::endl;
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
