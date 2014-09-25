#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include "clMgtProvList.hxx"
#include "MgtFactory.hxx"
#include "clMgtList.hxx"
#include <stdio.h>
#include <string>
#include <clLogApi.hxx>
#include "../client/MgtMsg.pb.hxx"
#include "boost/functional/hash.hpp"
#include <clMgtModule.hxx>
#include <clSafplusMsgServer.hxx>
#include <clMgtDatabase.hxx>

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
        string mine = this->str();
        string other = k2.str();
        return other < mine;
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
      std::string toXmlString() const
      {
        std::stringstream ss;
        ss << "[";
        ss << "key1=" << key1 << "," << "key2=" << key2 << "," << "key3=" << key3;
        ss << "]";
        return ss.str();
      }
      std::string str() const
      {
        std::stringstream ss;
        ss << key1 << key2 << key3 ;
        return ss.str();
      }
};

class TestObject : public MgtObject
  {
  public:
    TestObject(const char* objName) : MgtObject(objName)
      {
      }
    ~TestObject()
      {

      }
    void toString(std::stringstream& xmlString) {};
  };

class TestListObject: public MgtContainer
{
    MGT_REGISTER(TestListObject);
  public:
    MgtProv<std::string> obj_name;
    MgtProv<std::string>  obj_value;
    TestListObject(const char* objName): MgtContainer("mylist"),obj_name("obj_name"),obj_value("obj_value")
    {
      this->addChildObject(&obj_name,"obj_name");
      this->addChildObject(&obj_value,"obj_value");
    }
    ~TestListObject()
    {
    }
};
class TestListMultikeyObject: public MgtContainer
{
    MGT_REGISTER(TestListMultikeyObject);
  public:
    MgtProv<int> key1;
    MgtProv<int>  key2;
    MgtProv<std::string> key3;
    MgtProv<std::string> data;
    TestListMultikeyObject(const char* objName): MgtContainer("mylist"),key1("key1"),key2("key2"),key3("key3"),data("data")
    {
      this->addChildObject(&key1,"key1");
      this->addChildObject(&key2,"key2");
      this->addChildObject(&key3,"key3");
      this->addChildObject(&data,"data");
    }
    ~TestListMultikeyObject()
    {
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

  logDebug("MGT","TEST","Start test case string list");
  /**
   * Initialize
   */
  MgtList<std::string> stringList("mylist");
  stringList.setListKey("obj_name");
  std::string objKey[3] = {"hello","world","!!"};
  TestListObject testObject1("testobj1");
  testObject1.obj_value.value = "AAA";
  stringList.addChildObject(&testObject1,objKey[0]);

  TestListObject testObject2("testobj2");
  testObject2.obj_value.value = "BBB";
  stringList.addChildObject(&testObject2,objKey[1]);

  TestListObject testObject3("testobj3");
  testObject3.obj_value.value = "CCC";
  stringList.addChildObject(&testObject3,objKey[2]);
  /**
   * Checking size
   */
  if(stringList.getEntrySize() == 3)
  {
    logDebug("MGT","TEST","PASS: Entry size %d ", stringList.getEntrySize());
  }
  else
  {
    logDebug("MGT","TEST","FAIL: Entry size %d ", stringList.getEntrySize());
  }
  /**
   * Checking xpath
   */
    logDebug("MGT","TEST","XPATH of childs [%s]: %s ",objKey[0].c_str() ,testObject1.obj_name.getFullXpath().c_str());
    logDebug("MGT","TEST","XPATH of childs [%s]: %s ",objKey[1].c_str() ,testObject2.obj_name.getFullXpath().c_str());
    logDebug("MGT","TEST","XPATH of childs [%s]: %s ",objKey[2].c_str() ,testObject3.obj_name.getFullXpath().c_str());
  /**
   * Check operator[]
   */
  TestListObject *getObj = (TestListObject *)stringList[objKey[0]];
  if(testObject1.obj_name == getObj->obj_name)
    logDebug("MGT","TEST","PASS: operator[]: %s ",getObj->obj_name.value.c_str());
  else
    logDebug("MGT","TEST","FAIL: operator [] ");
  /**
   * Check set function
   */
  const char* xmlTest = "<mylist><obj_name>hello</obj_name><obj_value>111</obj_value></mylist>";
  SAFplus::Transaction t;
  if(stringList.set((void *)xmlTest,strlen(xmlTest),t))
  {
    logDebug("MGT","TEST","PASS: set operation OK. Now commit");
    t.commit();
    TestListObject *getObj = (TestListObject *)stringList[objKey[0]];
    logDebug("MGT","TEST","PASS: New value: %s ",getObj->obj_value.value.c_str());
  }
  else
  {
    logDebug("MGT","TEST","FAIL: set operation failed");
    t.abort();
  }
  logDebug("MGT","TEST","");
}
void testMgtClassList()
{
  logDebug("MGT","TEST","Start test case multiple key list");
  MgtList<multipleKey> stringList("mylist");
  stringList.setListKey("key1");
  stringList.setListKey("key2");
  stringList.setListKey("key3");

  TestListMultikeyObject testObject1("testobj1");
  testObject1.data.value = "testobj1val";

  TestListMultikeyObject testObject2("testobj2");
  testObject2.data.value = "testobj2val";

  TestListMultikeyObject testObject3("testobj3");
  testObject3.data.value = "testobj3val";

  multipleKey objKey1(2,2,"Java");
  multipleKey objKey2(2,4,"ASPX");
  multipleKey objKey3(2,6,"C++");
  multipleKey objKey4(2,4,"ASPX");
  stringList.addChildObject(&testObject1,objKey1);
  stringList.addChildObject(&testObject2,objKey2);
  stringList.addChildObject(&testObject3,objKey3);
  /**
   * Checking size
   */
  if(stringList.getEntrySize() == 3)
  {
    logDebug("MGT","TEST","PASS: Entry size %d ", stringList.getEntrySize());
  }
  else
  {
    logDebug("MGT","TEST","FAIL: Entry size %d ", stringList.getEntrySize());
  }
  /**
   * Checking xpath
   */
  logDebug("MGT","TEST","XPATH of childs [%s]: %s ",objKey1.str().c_str() ,stringList.getFullXpath(objKey1).c_str());
  logDebug("MGT","TEST","XPATH of childs [%s]: %s ",objKey2.str().c_str() ,stringList.getFullXpath(objKey2).c_str());
  logDebug("MGT","TEST","XPATH of childs [%s]: %s ",objKey3.str().c_str() ,stringList.getFullXpath(objKey3).c_str());
  /**
   * Check operator[]
   */
  TestListMultikeyObject *getObj = (TestListMultikeyObject *)stringList[objKey1];
  if(testObject1.key3.value == getObj->key3.value)
    logDebug("MGT","TEST","PASS: operator[]");
  else
    logDebug("MGT","TEST","FAIL: operator []");
  /**
   * Check set function
   */
  const char* xmlTest = "<mylist><key1>2</key1><key2>4</key2><key3>ASPXX</key3><data>1111</data></mylist>";
  SAFplus::Transaction t;
  if(stringList.set((void *)xmlTest,strlen(xmlTest),t))
  {
    logDebug("MGT","TEST","PASS: set operation OK. Now commit");
    t.commit();
    TestListMultikeyObject *getObj = (TestListMultikeyObject *)stringList[objKey2];
    logDebug("MGT","TEST","PASS: New value: %s ",getObj->data.value.c_str());
  }
  else
  {
    logDebug("MGT","TEST","FAIL: set operation failed");
    t.abort();
  }
  logDebug("MGT","TEST","");
}

void testMgtBind()
{
  MgtModule         mockModule("network");
  TestObject testobject("testObject");

  mockModule.loadModule();
  SAFplus::Handle handle=SAFplus::Handle::create();
  testobject.bind(handle, "network", "/ethernet/interfaces[name='eth0']");
  //testobject.bind(handle, "network", "1.3.6.1.4.1.99840.2.1.1");
}

void testDatabase()
{
  ClMgtDatabase *db = ClMgtDatabase::getInstance();
  ClRcT rc = db->initializeDB("test",99999,99999);
  if(CL_OK != rc)
  {
    logDebug("MGT","TEST","FAIL: Initialize db failed %x",rc);
    return;
  }
  logDebug("MGT","TEST","%s: Database initialized? %s",db->isInitialized()?"PASS":"FAIL",db->isInitialized()?"YES":"NO");
  MgtList<multipleKey> stringList("mylist");
  TestListMultikeyObject testObject1("testobj1");
  TestListMultikeyObject testObject2("testobj2");
  TestListMultikeyObject testObject3("testobj3");
  testObject1.data.value = "testobj1val";
  testObject1.key1.value = 123;
  testObject1.key2.value = 123;
  testObject1.key3.value = "key3_obj1";
  testObject2.data.value = "testobj2val";
  testObject2.key1.value = 456;
  testObject2.key2.value = 456;
  testObject2.key3.value = "key3_obj2";
  multipleKey objKey1(2,2,"Java");
  multipleKey objKey2(2,3,"444");
  stringList.addChildObject(&testObject1,objKey1);
  stringList.addChildObject(&testObject2,objKey2);
  stringList.write(objKey1,db);
  stringList.write(objKey2,db);
  stringList.removeChildObject(objKey1);
  stringList.addChildObject(&testObject3,objKey1);
  stringList.read(objKey1,db);
  logDebug("MGT","TEST","----Check data-----");
  ClBoolT isPass = CL_TRUE;
  if(testObject1.key1.value != testObject3.key1.value)
  {
    logDebug("MGT","TEST","FAIL: key1 not consistent [%x : %x]",testObject1.key1.value,testObject3.key1.value);
    isPass = CL_FALSE;
  }
  if(testObject1.key2.value != testObject3.key2.value)
  {
    logDebug("MGT","TEST","FAIL: key2 not consistent [%x : %x]",testObject1.key2.value,testObject3.key2.value);
    isPass = CL_FALSE;
  }
  if(testObject1.key3.value != testObject3.key3.value)
  {
    logDebug("MGT","TEST","FAIL: key3 not consistent [%s : %s]",testObject1.key3.value.c_str(),testObject3.key3.value.c_str());
    isPass = CL_FALSE;
  }
  if(testObject1.data.value != testObject3.data.value)
  {
    logDebug("MGT","TEST","FAIL: data not consistent [%s : %s]",testObject1.data.value.c_str(),testObject3.data.value.c_str());
    isPass = CL_FALSE;
  }
  if(isPass)
    logDebug("MGT","TEST","PASS: Read/Write database successfully");
  testObject3.key3.write(db);
  testObject2.key3.read(db);
  if(testObject3.key3.value == testObject2.key3.value)
    logDebug("MGT","TEST","PASS: Read/Write database successfully for prov");
  else
    logDebug("MGT","TEST","FAIL: Read/Write database FAILED for prov");
  db->finalizeDB();
}
ClBoolT gIsNodeRepresentative = CL_TRUE;
int main(int argc, char* argv[])
{
    ClRcT rc = CL_OK;

    //GAS: initialize expose by a explicit method
    SAFplus::ASP_NODEADDR = 0x2;

    logInitialize();
    logEchoToFd = 1; // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;

    utilsInitialize();

    // initialize SAFplus6 libraries
    if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK)
      {
      assert(0);
      }

    rc = clIocLibInitialize(NULL);
    assert(rc==CL_OK);

    safplusMsgServer.init(0, 25, 10);
    safplusMsgServer.Start();

    //-- Done initialize

    testTransaction01();

    testTransaction02();

    testTransactionExceptions();

    testGarbage();

    testMgtStringList();

    testMgtClassList();

    testMgtBind();

    testDatabase();

    safplusMsgServer.Stop();


    return 0;
}
