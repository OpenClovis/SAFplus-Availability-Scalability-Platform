#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include "clMgtProvList.hxx"

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
        printf("FAIL: testTransaction01 - abort.\n");
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
        printf("FAIL: testTransaction01 - set value.\n");
        return;
    }

    printf("PASS: testTransaction01 - ClMgtProv & Transaction.\n");
}

/*
 * This test case make sure a ClMgtProv object work with the Transaction
 */
void testTransaction02()
{
    SAFplus::Transaction t;
    ClInt32T index;
    ClCharT failVal[] = "<failtest>Value</failtest>";
    ClCharT passVal1[] = "<test>Value1</test>";
    ClCharT passVal2[] = "<test>Value2</test>";

    ClMgtProvList<std::string> testVal("test");
    testVal.Value.push_back("Init");

    if (testVal.set(failVal, strlen(failVal), t))
    {
        t.commit();
    }
    else
    {
        t.abort();
    }

    if ((testVal.Value.size() != 1) || (testVal.Value[0].compare("Init") != 0 ))
    {
        printf("FAIL: testTransaction02 - abort.\n");
        return;
    }


    if (testVal.set(passVal1, strlen(passVal1), t))
    {
        if (testVal.set(passVal2, strlen(passVal2), t))
        {
            t.commit();
        }
        else
        {
            t.abort();
        }
    }
    else
    {
        t.abort();
    }

    if ((testVal.Value.size() != 2) || (testVal.Value[0].compare("Value1") != 0 ) || (testVal.Value[1].compare("Value2") != 0 ))
    {
        printf("FAIL: testTransaction02 - set value.\n");
        return;
    }

    if (testVal.set(passVal1, strlen(passVal1), t))
    {
        if (testVal.set(passVal2, strlen(passVal2), t))
        {
            t.commit();
        }
        else
        {
            t.abort();
        }
    }
    else
    {
        t.abort();
    }

    if ((testVal.Value.size() != 2) || (testVal.Value[0].compare("Value1") != 0 ) || (testVal.Value[1].compare("Value2") != 0 ))
    {
        printf("FAIL: testTransaction02 - set value again.\n");
        return;
    }

    printf("PASS: testTransaction02 - ClMgtProvList & Transaction.\n");
}

int main(int argc, char* argv[])
{
    testTransaction01();

    testTransaction02();

    return 0;
}
