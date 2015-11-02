#ifndef _DB_TEST_INC_H_
#define _DB_TEST_INC_H_

#ifdef _MSC_VER
    #define TMP_PATH_PREFIX "./"
#elif defined __APPLE__
    #define TMP_PATH_PREFIX "./"
#else
    #define TMP_PATH_PREFIX "/tmp/"
#endif

#endif // _DB_TEST_INC_H_
