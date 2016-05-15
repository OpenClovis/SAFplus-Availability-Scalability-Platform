#include <clDbalBase.hxx>
#include <clTestApi.hxx>

using namespace SAFplus;

/* please provide database name including the path to it and plugin file name (the plugin shared lib name).
   To run sqlite test, e.g., ./testDBPlugin /tmp/test.db libclSQLiteDB.so , if no plugin file is provided,
   it will be initialized with environment variable
*/

void testBasic(DbalPlugin* p)
{
  ClRcT rc;
  const char* key = "KEY1";
  const char* val = "Value test of KEY1";
  rc = p->replaceRecord((ClDBKeyT)key, strlen(key)+1, (ClDBRecordT)val, strlen(val)+1);
  if (rc != CL_OK)
    {
      logError("DBAL","TEST","replacing record with key [%s] fail", key);
      clTestFailed(("replacing record with key [%s] fail", key));
    }
  clTest(("replacing record with key [%s]", key), rc==CL_OK, (" "));
  ClDBRecordT rec;
  ClUint32T recSz;
  rc = p->getRecord((ClDBKeyT)key, strlen(key)+1,&rec, &recSz);
  if (rc != CL_OK)
    {
      logError("DBAL","TEST","getting record with key [%s] fail", key);
      clTestFailed(("getting record with key [%s] fail", key));
    }
  else
    {
      logInfo("DBAL","TEST","Got value [%s]; size [%u]", (char*)rec, recSz);
      clTest(("getting record with key [%s]", key), !strcmp((char*)rec, val), (" "));
      p->freeRecord(rec);
    }

  rc = p->deleteRecord((ClDBKeyT)key, strlen(key)+1);
  clTest(("deleting record with key [%s]", key), rc==CL_OK, (" "));

  rc = p->getRecord((ClDBKeyT)key, strlen(key)+1,&rec, &recSz);
  clTest(("Record with key [%s] should fail because it was deleted before", key), rc!=CL_OK,(" "));
}

void testRandomData(DbalPlugin* p, int limit,int loops)
{
  ClRcT rc;

  // Create a bunch of records
  for (int i=0;i<loops;i++)
    {
      srand(i);
      int size = rand() % limit;
      unsigned char* buf = new unsigned char[size];
      assert(buf);
      
      for (int j=0;j<size;j++)
        {
          buf[j] = rand() & 255;
        }

      rc = p->replaceRecord((unsigned char *)&i, sizeof(int), buf, size);
      clTest(("create record [%d]", i), rc==CL_OK, ("record size [%d] records added [%d]",size,i));

      delete buf;
    }

  // Now verify them
  for (int i=0;i<loops;i++)
    {
      srand(i);
      int size = rand() % limit;

      unsigned char* rec;
      ClUint32T recSz;
      rc = p->getRecord((unsigned char*)&i, sizeof(int),&rec, &recSz);
      clTest(("read record [%d]",i), rc==CL_OK, ("record key [%d], size [%d]",i,size));
      if (rc == CL_OK)
        {
        clTest(("val record [%d] size", i), size == recSz, ("record size [%d], expected size [%d]",recSz,size));
      
        for (int j=0;j<size;j++)
          {
            unsigned char c = rand() & 255;
            clTest((" "), rec[j] == c, ("data mismatch [%d] expected [%d]",rec[j],c)); 
          }
        p->freeRecord(rec);
        }
    }

  //rc = p->deleteRecord((ClDBKeyT)key, strlen(key)+1);
  //clTest(("deleting record with key [%s]", key), rc==CL_OK, (" "));
}


void testRandomKey(DbalPlugin* p, int keylimit,int loops)
{
  ClRcT rc;

  // Create a bunch of records
  for (int i=0;i<loops;i++)
    {
      srand(i);
      int size = rand() % 4096;
      int keysize = rand() % keylimit;
      unsigned char* buf = new unsigned char[size];
      unsigned char* keybuf = new unsigned char[keysize];
      assert(buf);
      assert(keybuf);
      
      for (int j=0;j<keysize;j++)
        {
          keybuf[j] = rand() & 255;
        }
      for (int j=0;j<size;j++)
        {
          buf[j] = rand() & 255;
        }

      rc = p->replaceRecord((unsigned char *)keybuf, keysize, buf, size);
      clTest(("create record [%d]", i), rc==CL_OK, ("key size [%d] record size [%d] records added [%d]",keysize, size,i));

      delete buf;
      delete keybuf;
    }

  // Now verify them
  for (int i=0;i<loops;i++)
    {
      srand(i);
      int size = rand() % 4096;
      int keysize = rand() % keylimit;

      unsigned char* keybuf = new unsigned char[keysize];
      assert(keybuf);

      // recreate the key
      for (int j=0;j<keysize;j++) keybuf[j] = rand() & 255;

      unsigned char* rec;
      ClUint32T recSz;
      rc = p->getRecord((unsigned char*)keybuf, keysize,&rec, &recSz);
      clTest(("read record [%d]",i), rc==CL_OK, ("record key size [%d], size [%d]",keysize,size));
      if (rc == CL_OK)
        {
        clTest(("val record [%d] size", i), size == recSz, ("record size [%d], expected size [%d]",recSz,size));
      
        for (int j=0;j<size;j++)
          {
            unsigned char c = rand() & 255;
            clTest((" "), rec[j] == c, ("data mismatch [%d] expected [%d]",rec[j],c)); 
          }
        p->freeRecord(rec);
        }
      delete keybuf;
    }

  //rc = p->deleteRecord((ClDBKeyT)key, strlen(key)+1);
  //clTest(("deleting record with key [%s]", key), rc==CL_OK, (" "));
}


void testPersistentWrite(DbalPlugin* p, int keylimit,int loops)
{
  ClRcT rc;

  // Create a bunch of records
  for (int i=0;i<loops;i++)
    {
      srand(i);
      int size = rand() % 4096;
      int keysize = rand() % keylimit;
      unsigned char* buf = new unsigned char[size];
      unsigned char* keybuf = new unsigned char[keysize];
      assert(buf);
      assert(keybuf);
      
      for (int j=0;j<keysize;j++)
        {
          keybuf[j] = rand() & 255;
        }
      for (int j=0;j<size;j++)
        {
          buf[j] = rand() & 255;
        }

      rc = p->replaceRecord((unsigned char *)keybuf, keysize, buf, size);
      //rc = p->replaceRecord((unsigned char *)&i, sizeof(int), buf, size);
      clTest(("create record [%d] key size [%d] record size [%d]", i,keysize,size), rc==CL_OK, ("key size [%d] record size [%d] records added [%d]",keysize, size,i));

      delete buf;
      delete keybuf;
    }
  p->syncDbal(0);
}

void testPersistentRead(DbalPlugin* p, int keylimit,int loops)
{
  ClRcT rc;
  // Now verify them
  for (int i=0;i<loops;i++)
    {
      srand(i);
      int size = rand() % 4096;
      int keysize = rand() % keylimit;

      unsigned char* keybuf = new unsigned char[keysize];
      assert(keybuf);

      // recreate the key
      for (int j=0;j<keysize;j++) keybuf[j] = rand() & 255;

      unsigned char* rec;
      ClUint32T recSz;
      rc = p->getRecord((unsigned char*)keybuf, keysize,&rec, &recSz);
      //rc = p->getRecord((unsigned char*)&i, sizeof(int),&rec, &recSz);
      clTest(("read record [%d]",i), rc==CL_OK, ("record key size [%d], size [%d], error [0x%x]",keysize,size,rc));
      if (rc == CL_OK)
        {
        clTest(("val record [%d] size", i), size == recSz, ("record size [%d], expected size [%d]",recSz,size));
      
        for (int j=0;j<size;j++)
          {
            unsigned char c = rand() & 255;
            clTest((" "), rec[j] == c, ("data mismatch [%d] expected [%d]",rec[j],c)); 
          }
        p->freeRecord(rec);
        }
      delete keybuf;
    }
}



void testBasicTxn()
{
#if 0 // transaction test is temporarily deferred later
    rc = p->openTransaction("/tmp/testDB2.db", "/tmp/testDB2.db", CL_DB_CREAT, 255, 5000);
    if (rc != CL_OK && !strcmp(p->type, "Berkeley"))
    {
      logError("DBAL","TEST","openTransaction fail [0x%x]", rc);
      clTestFailed(("openTransaction fail [0x%x]", rc));
      goto retfree;
    }    
    clTest(("openTransaction "), (rc!=CL_OK && strcmp(p->type, "Berkeley")), (" "));
    char keyout[255];
    ClUint32T keySizeout;
    rc = p->getFirstRecord((ClDBKeyT*)&keyout, &keySizeout,&rec, &recSz);
    clTest(("getFirstRecord failed due to the empty database"), rc!=CL_OK, (" "));
    if (rc != CL_OK)
    {
      logInfo("DBAL","TEST","getFirstRecord failed due to the empty database");
    }    
    else
    {
      logError("DBAL","TEST","getFirstRecord got unexpected result");
      goto retfree;
    } 
    rc = p->beginTransaction();
    if (rc != CL_OK && !strcmp(p->type, "Berkeley"))
    {
      logError("DBAL","TEST","beginTransaction fail [0x%x]", rc);
      clTestFailed(("beginTransaction fail [0x%x]", rc));
      goto retfree;
    }
    logInfo("DBAL","TEST","beginTransaction ok");
    clTest(("beginTransaction "), (rc!=CL_OK && strcmp(p->type, "Berkeley")), (" "));
    rc = p->replaceRecord((ClDBKeyT)key, strlen(key)+1, (ClDBRecordT)val, strlen(val)+1);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","replacing record with key [%s] fail", key);
      clTestFailed(("replacing record with key [%s] fail", key));
      goto retfree;
    }
    clTest(("replacing record with key [%s]", key), rc==CL_OK, (" "));
    rc = p->commitTransaction();
    if (rc != CL_OK && !strcmp(p->type, "Berkeley"))
    {
      logError("DBAL","TEST","commitTransaction fail [0x%x]", rc);
      clTestFailed(("commitTransaction fail [0x%x]", rc));
      goto retfree;
    }
    logInfo("DBAL","TEST","commitTransaction ok");    
    clTest(("commitTransaction "), (rc!=CL_OK && strcmp(p->type, "Berkeley")), (" "));
    //ClDBRecordT rec;
    //ClUint32T recSz;
    rc = p->getRecord((ClDBKeyT)key, strlen(key)+1,&rec, &recSz);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","getting record with key [%s] fail", key);
      clTestFailed(("getting record with key [%s] fail", key));
      goto retfree;
    }
    logInfo("DBAL","TEST","Got value [%s]; size [%u]", (char*)rec, recSz);
    clTest(("getting record with key [%s] ", key), !strcmp((char*)rec, val), (" "));
    p->freeRecord(rec);
#endif
}

char dbType[4];

int main(int argc, char* argv[])
{  
  logEchoToFd = 1;
  //SAFplus::testVerbosity = SAFplus::TEST_PRINT_ALL;
  safplusInitialize(SAFplus::LibDep::LOG|SAFplus::LibDep::UTILS);   
  SAFplus::logSeverity = SAFplus::LOG_SEV_INFO;  // must be set after initialize if you want to override a env var
  if (argc<2) 
    {
      logNotice("DBAL","TEST", "Please provide database name including the path to it and dbal plugin file name (the plugin shared lib name)");
      return 0;
    }  
  DbalPlugin* p = NULL;

  clTestGroupInitialize(("DBL-BAS-FN.TG-001: Basic c++ dbal plugin test"));

  if (argc>=3)
    {
      p = clDbalObjCreate(argv[2]);
      memcpy(dbType,&argv[2][5],3); // Grab a portion of the lib name to uniquify the test case.  Lib name is "libclXXXXX.so", so grab XXX 
    }
  else
    {
      p = clDbalObjCreate();  
      strcpy(dbType,"DEF"); 
    }
  clTestGroupMalfunction(("Unable to load plugin"), p == NULL, { clTestGroupFinalize(); return 0; } );

  const char* dbname = argv[1]; 
  ClRcT rc = p->open(dbname, dbname, CL_DB_APPEND, 255, 5000);
  if (rc != CL_OK)
    {
      logError("DBAL","TEST", "open database fail");
      clTestFailed(("open database [%s] fail", dbname));
      delete p;
      clTestGroupFinalize();
      return 0;
    } 
  clTest(("opening database [%s] ok", dbname), rc == CL_OK, ("rc is %x", rc));

  if (argc>=4)  // If there is a special command to run, then run it, not the standard tests.
    {
      if (strcmp(argv[3],"write") == 0)
        {
          clTestCase(("DBL-%s-PER.TC-001: persistent write",dbType), testPersistentWrite(p,1024,1000));
          delete p;
          clTestGroupFinalize();
          return 1;
          
        }
      if (strcmp(argv[3],"read") == 0)
        {
          clTestCase(("DBL-%s-PER.TC-002: persistent read",dbType), testPersistentRead(p,1024,1000));
          delete p;
          clTestGroupFinalize();
          return 1;

        }
    }

  // logNotice("DBAL","TEST", "open database OK");

  clTestCase(("DBL-%s-BAS.TC-001: basic read/write/delete",dbType), testBasic(p));
  clTestCase(("DBL-%s-DTA.TC-002: random data size",dbType), testRandomData(p,4096,200));
  clTestCase(("DBL-%s-KEY.TC-003: random key and data size",dbType), testRandomKey(p,1024,200));


  delete p;
  clTestGroupFinalize();
  return 1;
}
