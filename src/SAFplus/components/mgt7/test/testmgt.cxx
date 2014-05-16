#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include "clMgtProvList.hxx"

#include <stdio.h>

using namespace SAFplus;

/*
 * This test case make sure a ClMgtProv object work with the Transaction
 */
void testTransaction01()
{
    SAFplus::Transaction t;
    ClInt32T index;
    ClCharT failVal[] = "<failtest>Value</failtest>";
    ClCharT passVal[] = "<test>Value</test>";

    MgtProv<std::string> testVal("test");
    testVal.value = "Init";

    if (testVal.set(failVal, strlen(failVal), t))
    {
        t.commit();
    }
    else // expected to fail
    {
        t.abort(); 
    }

    if (testVal.value.compare("Init") != 0 )
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

    if (testVal.value.compare("Value") != 0 )
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

    MgtProvList<std::string> testVal("test");
    testVal.value.push_back("Init");

    if (testVal.set(failVal, strlen(failVal), t))
    {
        t.commit();
    }
    else
    {
        t.abort();
    }

    if ((testVal.value.size() != 1) || (testVal.value[0].compare("Init") != 0 ))
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

    if ((testVal.value.size() != 2) || (testVal.value[0].compare("Value1") != 0 ) || (testVal.value[1].compare("Value2") != 0 ))
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

    if ((testVal.value.size() != 2) || (testVal.value[0].compare("Value1") != 0 ) || (testVal.value[1].compare("Value2") != 0 ))
    {
        printf("FAIL: testTransaction02 - set value again.\n");
        return;
    }

    printf("PASS: testTransaction02 - ClMgtProvList & Transaction.\n");
}


void testTransactionExceptions()
{
    SAFplus::Transaction t;
    ClInt32T index;
    ClCharT failVal[] = "<failtest>Value</failtest>";
    ClCharT passVal1[] = "<test>Value1</test>";
    ClCharT passVal2[] = "<test>Value2</test>";

    MgtProvList<std::string> testVal("test");
    MgtProvList<std::string> testVal1("test");
    testVal.value.push_back("Init");

    // Test proper commit
    try
      {     
        testVal.xset(passVal1, strlen(passVal1), t);
        t.commit();
      }
    catch(TransactionException& e)
      {
        printf("FAIL: This case should have passed\n");
      }
    if ((testVal.value.size() != 1) || (testVal.value[0].compare("Value1") != 0 ))
    {
        printf("FAIL: transaction did not commit\n");
    }    
 
    try
      {     
        testVal.xset(failVal, strlen(failVal), t);
        printf("FAIL: This should have thrown an exception\n");
        t.commit();        
      }
    catch(TransactionException& e)
      {
        printf("TEST: throw ok\n");
      }

    try
      {     
        testVal.xset(passVal2, strlen(passVal2), t);
        testVal1.xset(passVal1, strlen(passVal1), t);
        t.commit();
      }
    catch(TransactionException& e)
      {
        printf("FAIL: This case should have passed\n");
      }
    if ((testVal.value.size() != 1) || (testVal.value[0].compare("Value2") != 0 ))
    {
        printf("FAIL: transaction did not commit\n");
    }    
    if ((testVal1.value.size() != 1) || (testVal1.value[0].compare("Value1") != 0 ))
    {
        printf("FAIL: transaction did not commit\n");
    }    

    
    try
      {     
        testVal.xset(passVal1, strlen(passVal1), t);
        testVal1.xset(failVal, strlen(failVal), t);
        t.commit();
      }
    catch(TransactionException& e)
      {
        printf("PASS: This case should have raised an exception\n");
      }
    if ((testVal.value.size() != 1) && (testVal.value[0].compare("Value1") == 0 ))
    {
        printf("FAIL: transaction should not have committed but value is set!\n");
    }    
}


#define strAndLen(x) (const void *) x,strlen((const char*)x)

void testGarbage()
{
    SAFplus::Transaction t;
    ClInt32T index;
    ClCharT failVal[] = "<failtest>Value</failtest>";
    ClCharT passVal[] = "<test>Value</test>";

    MgtProv<std::string> testVal("test");
    testVal.value = "Init";

    if (testVal.set(strAndLen("<badtag>VALUE</badtag>"), t))
    {
      printf("FAIL: badtag garbage accepted\n");
    }
    else
    {
       printf("PASS: badtag garbage rejected\n"); 
    }

    if (testVal.set(strAndLen("<test/>"), t))
    {
      printf("FAIL: html empty garbage accepted\n");
    }
    else
    {
       printf("PASS: html empty garbage rejected\n"); 
    }
    
    if (testVal.set(strAndLen("<tes"), t))
    {
      printf("FAIL: incomplete opener tag garbage accepted\n");
    }
    else
    {
       printf("PASS: incomplete opener tag  garbage rejected\n"); 
    }
    
    if (testVal.set(strAndLen("<test>value</tes>"), t))
    {
      printf("FAIL: bad open/close pair garbage accepted\n");
    }
    else
    {
       printf("PASS: bad open/close pair  garbage rejected\n"); 
    }

    if (testVal.set(strAndLen("<test>value</te"), t))
    {
      printf("FAIL: chopped string garbage accepted\n");
    }
    else
    {
       printf("PASS: chopped string  garbage rejected\n"); 
    }

    if (testVal.set(strAndLen("value"), t))
    {
      printf("FAIL: no tags  accepted\n");
    }
    else
    {
       printf("PASS: no tags rejected\n"); 
    }
    
    
}


int main(int argc, char* argv[])
{
    testTransaction01();

    testTransaction02();

    testTransactionExceptions();

    testGarbage();
    
    return 0;
}
