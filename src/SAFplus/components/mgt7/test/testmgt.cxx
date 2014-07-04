#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include "clMgtProvList.hxx"
#include "clMgtList.hxx"
#include <stdio.h>
#include <string>
#include <clLogApi.hxx>
#include "boost/functional/hash.hpp"
using namespace std;
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
        logDebug("MGT","TEST","Building key for object...");
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
        logDebug("MGT","TEST","FAIL: testTransaction01 - abort.");
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
        logDebug("MGT","TEST","FAIL: testTransaction01 - set value.");
        return;
    }

    logDebug("MGT","TEST","PASS: testTransaction01 - ClMgtProv & Transaction.");
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
        logDebug("MGT","TEST","FAIL: testTransaction02 - abort.");
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
        logDebug("MGT","TEST","FAIL: testTransaction02 - set value.");
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
        logDebug("MGT","TEST","FAIL: testTransaction02 - set value again.");
        return;
    }

    logDebug("MGT","TEST","PASS: testTransaction02 - ClMgtProvList & Transaction.");
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
        logDebug("MGT","TEST","FAIL: This case should have passed");
      }
    if ((testVal.value.size() != 1) || (testVal.value[0].compare("Value1") != 0 ))
    {
        logDebug("MGT","TEST","FAIL: transaction did not commit");
    }    
 
    try
      {     
        testVal.xset(failVal, strlen(failVal), t);
        logDebug("MGT","TEST","FAIL: This should have thrown an exception");
        t.commit();        
      }
    catch(TransactionException& e)
      {
        logDebug("MGT","TEST","TEST: throw ok");
      }

    try
      {     
        testVal.xset(passVal2, strlen(passVal2), t);
        testVal1.xset(passVal1, strlen(passVal1), t);
        t.commit();
      }
    catch(TransactionException& e)
      {
        logDebug("MGT","TEST","FAIL: This case should have passed");
      }
    if ((testVal.value.size() != 1) || (testVal.value[0].compare("Value2") != 0 ))
    {
        logDebug("MGT","TEST","FAIL: transaction did not commit");
    }    
    if ((testVal1.value.size() != 1) || (testVal1.value[0].compare("Value1") != 0 ))
    {
        logDebug("MGT","TEST","FAIL: transaction did not commit");
    }    

    
    try
      {     
        testVal.xset(passVal1, strlen(passVal1), t);
        testVal1.xset(failVal, strlen(failVal), t);
        t.commit();
      }
    catch(TransactionException& e)
      {
        logDebug("MGT","TEST","PASS: This case should have raised an exception");
      }
    if ((testVal.value.size() != 1) && (testVal.value[0].compare("Value1") == 0 ))
    {
        logDebug("MGT","TEST","FAIL: transaction should not have committed but value is set!");
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
      logDebug("MGT","TEST","FAIL: badtag garbage accepted");
    }
    else
    {
       logDebug("MGT","TEST","PASS: badtag garbage rejected");
    }

    if (testVal.set(strAndLen("<test/>"), t))
    {
      logDebug("MGT","TEST","FAIL: html empty garbage accepted");
    }
    else
    {
       logDebug("MGT","TEST","PASS: html empty garbage rejected");
    }
    
    if (testVal.set(strAndLen("<tes"), t))
    {
      logDebug("MGT","TEST","FAIL: incomplete opener tag garbage accepted");
    }
    else
    {
       logDebug("MGT","TEST","PASS: incomplete opener tag  garbage rejected");
    }
    
    if (testVal.set(strAndLen("<test>value</tes>"), t))
    {
      logDebug("MGT","TEST","FAIL: bad open/close pair garbage accepted");
    }
    else
    {
       logDebug("MGT","TEST","PASS: bad open/close pair  garbage rejected");
    }

    if (testVal.set(strAndLen("<test>value</te"), t))
    {
      logDebug("MGT","TEST","FAIL: chopped string garbage accepted");
    }
    else
    {
       logDebug("MGT","TEST","PASS: chopped string  garbage rejected");
    }

    if (testVal.set(strAndLen("value"), t))
    {
      logDebug("MGT","TEST","FAIL: no tags  accepted");
    }
    else
    {
       logDebug("MGT","TEST","PASS: no tags rejected");
    }
    
    
}

void testMgtStringList()
{
  const char* xmlTest = "<mylist><testobj1 name=\"hello\" dummy=\"testval\"><name>testobj1_newname</name></testobj1></mylist>";
  logDebug("MGT","TEST","Start test case string list");
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
    logDebug("MGT","TEST","PASS: Entry size %d ", stringList.getEntrySize());
  }
  else
  {
    logDebug("MGT","TEST","FAIL: Entry size %d ", stringList.getEntrySize());
  }

  logDebug("MGT","TEST","PASS: X-PATH %s ",stringList.getFullXpath(objKey2).c_str());

  logDebug("MGT","TEST","PASS: DUMPING ");
  stringList.dbgDumpChildren();

  MgtObject::Iterator iter = stringList.begin();
  while(iter != stringList.end())
  {
    logDebug("MGT","TEST","PASS: %s ",iter->second->name.c_str());
    iter++;
  }

  MgtProv<std::string> *getObj = (MgtProv<std::string> *)stringList[objKey4];
  if(testObject1.name == getObj->name)
    logDebug("MGT","TEST","PASS: operator[]: %s ",getObj->name.c_str());
  else
    logDebug("MGT","TEST","FAIL: operator [] ");

  SAFplus::Transaction t;
  stringList.set((void *)xmlTest,strlen(xmlTest),t);
  logDebug("MGT","TEST","%s ",getObj->name.c_str());
  logDebug("MGT","TEST","");
}
void testMgtClassList()
{
  const char* xmlTest = "<mylist><testobj1 key1=\"2\" key2=\"2\" key3=\"Java\"><name>testobj1_newname</name></testobj1></mylist>";
  logDebug("MGT","TEST","Start test case multiple key list");
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

  string xml="<root><interface key1=\"value1\" key2=\"value\"><name>eth0</name></interface><interface key1=\"value2\" key2=\"value\"><name>eth1</name></interface></root>";

  if(stringList.getEntrySize() == 3)
  {
    logDebug("MGT","TEST","PASS: Entry size %d ", stringList.getEntrySize());
  }
  else
  {
    logDebug("MGT","TEST","FAIL: Entry size %d ", stringList.getEntrySize());
  }

  logDebug("MGT","TEST","PASS: X-PATH %s ",stringList.getFullXpath(objKey1).c_str());

  logDebug("MGT","TEST","PASS: DUMPING ");
  stringList.dbgDumpChildren();

  MgtObject::Iterator iter = stringList.begin();
  while(iter != stringList.end())
  {
    logDebug("MGT","TEST","PASS: %s ",iter->second->name.c_str());
    iter++;
  }

  MgtProv<std::string> *getObj = (MgtProv<std::string> *)stringList[objKey4];
  if(getObj == NULL)
  {
    logDebug("MGT","TEST","FAIL: operator[] Can't find object ");
    return;
  }
  if(testObject2.name.compare(getObj->name) == 0)
    logDebug("MGT","TEST","PASS: operator[]: %s ",getObj->name.c_str());
  else
    logDebug("MGT","TEST","FAIL: operator[] %s ",getObj->name.c_str());
  SAFplus::Transaction t;
  stringList.set((void *)xmlTest,strlen(xmlTest),t);
  logDebug("MGT","TEST","%s",getObj->name.c_str());
}

int main(int argc, char* argv[])
{
    //GAS: initialize expose by a explicit method
    SAFplus::ASP_NODEADDR = 0x1;

    logInitialize();
    logEchoToFd = 1; // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;

    utilsInitialize();

    ClRcT rc;
    // initialize SAFplus6 libraries
    if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc =
        clBufferInitialize(NULL)) != CL_OK)
      {
        assert(0);
      }

    //-- Done initialize

    testTransaction01();

    testTransaction02();

    testTransactionExceptions();

    testGarbage();

    testMgtStringList();

    testMgtClassList();

    return 0;
}
