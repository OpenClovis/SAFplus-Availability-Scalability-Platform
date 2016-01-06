#include <clDbalBase.hxx>
#include <clTestApi.hxx>

using namespace SAFplus;

/* please provide database name including the path to it and plugin file name (the plugin shared lib name).
   To run sqlite test, e.g., ./testDBPlugin /tmp/berkeleyDB.db libclSQLiteDB.so , if no plugin file is provided,
   it will be initialized with environment variable
*/
int main(int argc, char* argv[])
{  
  logEchoToFd = 1;
  SAFplus::logSeverity = SAFplus::LOG_SEV_MAX;
  safplusInitialize(SAFplus::LibDep::LOG|SAFplus::LibDep::UTILS);   
  if (argc<2) 
  {
    logNotice("DBAL","TEST", "Please provide database name including the path to it and dbal plugin file name (the plugin shared lib name)");
    return 0;
  }  
  DbalPlugin* p = NULL;
  if (argc>=3)
  {
    p = clDbalObjCreate(argv[2]);
  }
  else
  {
    p = clDbalObjCreate();   
  }
  if (p)
  {     
    clTestGroupInitialize(("Basic c++ dbal plugin test"));
    const char* dbname = argv[1]; 
    ClRcT rc = p->open(dbname, dbname, CL_DB_CREAT, 255, 5000);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST", "open database fail");
      clTestFailed(("open database [%s] fail", dbname));
      goto retfree;
    } 
    clTest(("opening database [%s] ok", dbname), rc == CL_OK, (" "));
    logNotice("DBAL","TEST", "open database OK");
#if 1
    const char* key = "KEY1";
    const char* val = "Value test of KEY1";
    rc = p->replaceRecord((ClDBKeyT)key, strlen(key)+1, (ClDBRecordT)val, strlen(val)+1);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","replacing record with key [%s] fail", key);
      clTestFailed(("replacing record with key [%s] fail", key));
      goto retfree;
    }
    clTest(("replacing record with key [%s]", key), rc==CL_OK, (" "));
    ClDBRecordT rec;
    ClUint32T recSz;
    rc = p->getRecord((ClDBKeyT)key, strlen(key)+1,&rec, &recSz);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","getting record with key [%s] fail", key);
      clTestFailed(("getting record with key [%s] fail", key));
      goto retfree;
    }
    logInfo("DBAL","TEST","Got value [%s]; size [%u]", (char*)rec, recSz);
    clTest(("getting record with key [%s]", key), !strcmp((char*)rec, val), (" "));
    p->freeRecord(rec);
    rc = p->deleteRecord((ClDBKeyT)key, strlen(key)+1);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","deleting record with key [%s] fail", key);
      clTestFailed(("deleting record with key [%s] fail", key));
      goto retfree;
    }
    clTest(("deleting record with key [%s]", key), rc==CL_OK, (" "));
    rc = p->getRecord((ClDBKeyT)key, strlen(key)+1,&rec, &recSz);
    if (rc != CL_OK)
    {
      logInfo("DBAL","TEST","Record with key [%s] gotten fail because it was deleted before", key);      
    }
    clTest(("Record with key [%s] gotten fail because it was deleted before", key), rc!=CL_OK,(" "));
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
    delete p;
    
#endif
  }
  else
  {
    logError("DBAL","TEST", "load plugin fail");
  }
  clTestGroupFinalize();
  return 1;

retfree:

  delete p;
  clTestGroupFinalize();
  return 0;
}
