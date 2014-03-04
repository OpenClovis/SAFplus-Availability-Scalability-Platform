#include "clTransaction.hxx"
#include "clMgtProv.hxx"

#include <stdio.h>

/*
 * This test case make sure a ClMgtProv object work with the Transaction
 */
void testTransaction01()
{
    SAFplus::Transaction t;
    ClInt32T index;
    ClCharT failVal[] = "<failtest>Value</failtest>";
    ClCharT passVal[] = "<test>Value</test>";

    ClMgtProv<std::string> testVal("test");
    testVal.Value = "Init";

    if (testVal.set(failVal, strlen(failVal), t))
    {
        t.commit();
    }
    else
    {
        t.abort();
    }

    if (testVal.Value.compare("Init") != 0 )
    {
        printf("FAIL: Test 01 - MGT with Transaction - abort.\n");
        return;
    }

    if (testVal.set(passVal, strlen(passVal), t))
    {
        t.commit();
    }
    else
    {
        t.abort();
    }

    if (testVal.Value.compare("Value") != 0 )
    {
        printf("FAIL: Test 02 - MGT with Transaction - check value.\n");
        return;
    }

    printf("PASS: ClTransaction & MGT object.\n");
}


int main(int argc, char* argv[])
{
    testTransaction01();

    return 0;
}
