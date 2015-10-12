#include <clDbalBase.hxx>
//#include <clPluginApi.hxx>

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
    ClRcT rc = p->open("/tmp/testDB1.db", "/tmp/testDB1.db", CL_DB_CREAT, 255, 5000);
    if (rc != CL_OK)
    {
      logError("DBAL","TEST", "open database fail");
      return 0;
    } 
    logNotice("DBAL","TEST", "open database OK");
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
  }
  else
  {
    logError("DBAL","TEST", "load plugin fail");
  }
  return 1;
}
