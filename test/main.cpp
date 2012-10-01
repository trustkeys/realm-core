#include <cstring>
#include <iostream>

#include <UnitTest++.h>

#include <tightdb.hpp>

#if defined(_MSC_VER) && defined(_DEBUG)
//    #include <vld.h>
#endif

using namespace std;

TIGHTDB_TABLE_1(OneIntTable,
                first,  Int)

TIGHTDB_TABLE_2(TupleTableType,
                first,  Int,
                second, String)

int main(int argc, char const *const argv[])
{
    bool const no_error_exit_staus = 2 <= argc && strcmp(argv[1], "--no-error-exit-staus") == 0;

#ifdef TIGHTDB_DEBUG
    cout << "Running Debug unit tests\n\n";
#else
    cout << "Running Release unit tests\n\n";
#endif

    const int res = UnitTest::RunAllTests();

#ifdef _MSC_VER
    getchar(); // wait for key
#endif

    return no_error_exit_staus ? 0 : res;
}