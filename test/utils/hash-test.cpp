#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <ctime>

#include "lib/utils/hash.hpp"

using namespace cdb;

static const int CONFLICT_NUMBER = 1000000;
static const char TEST_STRING[] = "1234567";

static void 
random_string(char *str, int length)
{
    static const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "01234567890"
                                   "!@#$%^&*()_+=-`\\|<>,./?;:";

    while (length--) {
        *str++ = ALPHABET[std::rand() % (sizeof(ALPHABET) / sizeof(ALPHABET[0]))];
    }
    *str = '\0';
}

TEST(HashTest, Basic)
{
    EXPECT_NE(0, FNVHasher::hash(reinterpret_cast<const Byte*>(TEST_STRING), std::strlen(TEST_STRING)));
}

TEST(HashTest, Conflict)
{
    char TMP_STRING[sizeof(TEST_STRING) / sizeof(TEST_STRING[0])];
    auto test_length = std::strlen(TEST_STRING);

    HashResult original = FNVHasher::hash(
            reinterpret_cast<const Byte*>(TEST_STRING), test_length);

    std::srand(std::time(NULL));

    for (int i = 0; i < CONFLICT_NUMBER; ++i) {
        random_string(TMP_STRING, test_length);
        HashResult random_result = FNVHasher::hash(
                reinterpret_cast<const Byte*>(TMP_STRING), test_length);

        EXPECT_NE(random_result, original)
            << "test_string: " << TMP_STRING;
    }
}
