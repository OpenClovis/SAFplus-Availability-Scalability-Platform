#include <clDbalBase.hxx>

using namespace SAFplus;

int main()
{
  logEchoToFd = 1;
  SAFplus::logSeverity = SAFplus::LOG_SEV_MAX;
  safplusInitialize(SAFplus::LibDep::LOG|SAFplus::LibDep::UTILS);   
  clDbalInitialize();
  DbalPlugin* p = SAFplusI::defaultDbalPlugin;
  if (p)
  {     
    ClRcT rc = p->open("/tmp/testDB2.db", "/tmp/testDB2.db", CL_DB_CREAT, 255, 5000);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST", "open database fail");
      return 0;
    } 
    logNotice("DBAL","TEST", "open database OK");
#if 1
    const char* key = "KEY1";
    const char* val = "Value test of KEY1";
    rc = p->replaceRecord((ClDBKeyT)key, strlen(key)+1, (ClDBRecordT)val, strlen(val)+1);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","replacing record with key [%s] fail", key);
      return 0;
    }
    ClDBRecordT rec;
    ClUint32T recSz;
    rc = p->getRecord((ClDBKeyT)key, strlen(key)+1,&rec, &recSz);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","getting record with key [%s] fail", key);
      return 0;
    }
    logInfo("DBAL","TEST","Got value [%s]; size [%u]", (char*)rec, recSz);
    p->freeRecord(rec);
    rc = p->deleteRecord((ClDBKeyT)key, strlen(key)+1);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","deleting record with key [%s] fail", key);
      return 0;
    }
    rc = p->getRecord((ClDBKeyT)key, strlen(key)+1,&rec, &recSz);
    if (rc != CL_OK)
    {
      logInfo("DBAL","TEST","Record with key [%s] gotten fail because it was deleted before", key);
    }
    rc = p->openTransaction("/tmp/testDB2.db", "/tmp/testDB2.db", CL_DB_CREAT, 255, 5000);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","openTransaction fail [0x%x]", rc);
      return 0;
    }    
    char keyout[255];
    ClUint32T keySizeout;
    rc = p->getFirstRecord((ClDBKeyT*)&keyout, &keySizeout,&rec, &recSz);
    if (rc != CL_OK)
    {
      logInfo("DBAL","TEST","getFirstRecord failed due to the empty database");
    }
    else
    {
      logError("DBAL","TEST","getFirstRecord got unexpected result");
      return 0;
    } 
    rc = p->beginTransaction();
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","beginTransaction fail [0x%x]", rc);
      return 0;
    }
    logInfo("DBAL","TEST","beginTransaction ok");
    rc = p->replaceRecord((ClDBKeyT)key, strlen(key)+1, (ClDBRecordT)val, strlen(val)+1);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","replacing record with key [%s] fail", key);
      return 0;
    }
    rc = p->commitTransaction();
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","commitTransaction fail [0x%x]", rc);
      return 0;
    }
    logInfo("DBAL","TEST","commitTransaction ok");    
    //ClDBRecordT rec;
    //ClUint32T recSz;
    rc = p->getRecord((ClDBKeyT)key, strlen(key)+1,&rec, &recSz);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST","getting record with key [%s] fail", key);
      return 0;
    }
    logInfo("DBAL","TEST","Got value [%s]; size [%u]", (char*)rec, recSz);
    p->freeRecord(rec);
    
#endif
  }
  else
  {
    logError("DBAL","TEST", "load plugin fail");
  }
  return 1;
}
