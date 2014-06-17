#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include "clMgtProvList.hxx"
#include "clMgtList.hxx"
#include <stdio.h>
#include "boost/functional/hash.hpp"
using namespace SAFplus;


class multipleKey
{
    public:
      int key1;
      int key2;
      std::string key3;
      multipleKey(int k1, int k2, std::string k3):key1(k1),key2(k2),key3(k3)
      {
      }
      multipleKey()
      {
      }
      bool operator<(const multipleKey k2) const
      {
        if(key3 < k2.key3)
          return true;
        else
          return false;
      }
      void build(std::map<std::string,std::string> keyList)
      {
        std::map<std::string,std::string>::iterator iter;
        iter = keyList.find("key1");
        if(iter != keyList.end())
        {
          key1 = atoi(iter->second.c_str());
        }
        iter = keyList.find("key2");
        if(iter != keyList.end())
        {
          key2 = atoi(iter->second.c_str());
        }
        iter = keyList.find("key3");
        if(iter != keyList.end())
        {
          key3 = iter->second;
        }
        printf("HERE \n");
      }
      std::string str()
      {
        std::stringstream ss;
        ss << key1 << key2 << key3 ;
        return ss.str();
      }
};
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

void testMgtStringList()
{
  const char* xmlTest = "<root><name>hello</name></root>";
  printf("Start test case string list\n");
  MgtList<std::string> stringList("mylist");
  stringList.setListKey("name");
  MgtProv<std::string> testObject1("testobj1");
  MgtProv<std::string> testObject2("testobj2");
  MgtProv<std::string> testObject3("testobj3");
  std::string objKey1("hello");
  std::string objKey2("world");
  std::string objKey3("python");
  std::string objKey4("hello");
  stringList.addChildObject(&testObject1,objKey1);
  stringList.addChildObject(&testObject2,objKey2);
  stringList.addChildObject(&testObject3,objKey3);
  if(stringList.getEntrySize() == 3)
  {
    printf("PASS: Entry size %d \n", stringList.getEntrySize());
  }
  else
  {
    printf("FAIL: Entry size %d \n", stringList.getEntrySize());
  }

  printf("PASS: X-PATH %s \n",stringList.getFullXpath().c_str());

  printf("PASS: DUMPING \n");
  stringList.dbgDumpChildren();

  MgtObject::Iterator iter = stringList.begin();
  while(iter != stringList.end())
  {
    printf("PASS: %s \n",iter->second->name.c_str());
    iter++;
  }

  MgtProv<std::string> *getObj = (MgtProv<std::string> *)stringList[objKey4];
  if(testObject1.name == getObj->name)
    printf("PASS: operator[]: %s \n",getObj->name.c_str());
  else
    printf("FAIL: operator [] \n");

  SAFplus::Transaction t;
  stringList.set((void *)xmlTest,strlen(xmlTest),t);
  printf("%s \n",getObj->name.c_str());
  printf("\n\n\n");
}
void testMgtClassList()
{
  const char* xmlTest = "<root><multi><name>hello</name><key1>2</key1><key2>5</key2><key3>ASPX</key3></multi></root>";
  printf("Start test case multiple key list\n");
  MgtList<multipleKey> stringList("mylist");
  stringList.setListKey("key1");
  stringList.setListKey("key2");
  stringList.setListKey("key3");
  MgtProv<std::string> testObject1("testobj1");
  MgtProv<std::string> testObject2("testobj2");
  MgtProv<std::string> testObject3("testobj3");
  multipleKey objKey1(2,2,"Java");
  multipleKey objKey2(2,4,"ASPX");
  multipleKey objKey3(2,6,"C++");
  multipleKey objKey4(2,4,"ASPX");
  stringList.addChildObject(&testObject1,objKey1);
  stringList.addChildObject(&testObject2,objKey2);
  stringList.addChildObject(&testObject3,objKey3);
  if(stringList.getEntrySize() == 3)
  {
    printf("PASS: Entry size %d \n", stringList.getEntrySize());
  }
  else
  {
    printf("FAIL: Entry size %d \n", stringList.getEntrySize());
  }

  printf("PASS: X-PATH %s \n",stringList.getFullXpath().c_str());

  printf("PASS: DUMPING \n");
  stringList.dbgDumpChildren();

  MgtObject::Iterator iter = stringList.begin();
  while(iter != stringList.end())
  {
    printf("PASS: %s \n",iter->second->name.c_str());
    iter++;
  }

  MgtProv<std::string> *getObj = (MgtProv<std::string> *)stringList[objKey4];
  if(getObj == NULL)
  {
    printf("FAIL: operator[] Can't find object \n");
    return;
  }
  if(testObject2.name.compare(getObj->name) == 0)
    printf("PASS: operator[]: %s \n",getObj->name.c_str());
  else
    printf("FAIL: operator[] %s \n",getObj->name.c_str());
  SAFplus::Transaction t;
  stringList.set((void *)xmlTest,strlen(xmlTest),t);
  printf("%s \n",getObj->name.c_str());
  printf("\n\n\n");
}
int main(int argc, char* argv[])
{
    testTransaction01();

    testTransaction02();

    testTransactionExceptions();

    testGarbage();

    testMgtStringList();

    testMgtClassList();

    return 0;
}
