#include <clDbalBase.hxx>
#include <string>

int main()
{
  const char* s = "libclSQLiteDB.so";
  
  ClPluginHandle* plug = clLoadPlugin(CL_DBAL_PLUGIN_ID,CL_DBAL_PLUGIN_VER,s);
  if (plug)
  {
     DbalPlugin* p = dynamic_cast<DbalPlugin*> (plug->pluginApi);
     if (p)
     {
        //open(ClDBFileT dbFile, ClDBNameT dbName, ClDBFlagT dbFlag, ClUint32T maxKeySize, ClUint32T maxRecordSize
        ClRcT rc = p->open("/tmp/testDB1.db", "/tmp/testDB1.db", CL_DB_APPEND, 255, 5000);
        if (rc != CL_OK)
        {
          logError("DBAL","TEST", "open database fail");
          return 0;
        } 
     }
  }
  return 1;
}
