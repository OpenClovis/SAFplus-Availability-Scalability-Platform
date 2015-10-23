#include <clDbalBase.hxx>
#include <clCkptApi.hxx>
#include <string.h>
#include <clCommon6.h>
#include <clCommonErrors6.h>
#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clDbg.hxx>
#include <boost/interprocess/shared_memory_object.hpp>


namespace SAFplus {

class CkptPlugin: public DbalPlugin
{
public:
  //CkptPlugin() {}
  virtual ClRcT open(ClDBFileT dbFile, ClDBNameT dbName, ClDBFlagT dbFlag, ClUint32T maxKeySize, ClUint32T maxRecordSize);  
  virtual ClRcT insertRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize);
  virtual ClRcT replaceRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize);
  virtual ClRcT getRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize);
  virtual ClRcT getFirstRecord(ClDBKeyT* pDBKey, ClUint32T* pKeySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize);
  virtual ClRcT getNextRecord(ClDBKeyT currentKey, ClUint32T currentKeySize, ClDBKeyT* pDBNextKey, ClUint32T* pNextKeySize, ClDBRecordT* pDBNextRec, ClUint32T* pNextRecSize);
  virtual ClRcT deleteRecord(ClDBKeyHandleT dbKey, ClUint32T keySize);
  virtual ClRcT syncDbal(ClUint32T flags);
  virtual ClRcT openTransaction(ClDBFileT dbFile, ClDBNameT  dbName, ClDBFlagT dbFlag, ClUint32T  maxKeySize, ClUint32T  maxRecordSize);
  virtual ClRcT beginTransaction();
  virtual ClRcT commitTransaction();
  virtual ClRcT abortTransaction();
  virtual ClRcT freeRecord(ClDBRecordT dbRec);
  virtual ClRcT freeKey(ClDBRecordT dbKey);
  virtual ~CkptPlugin();

protected:
  virtual ClRcT close();
};

//static CkptPlugin api;

CkptPlugin::~CkptPlugin()
{ 
  close(); 
}

ClRcT CkptPlugin::open(ClDBFileT dbFile, ClDBNameT dbName, ClDBFlagT dbFlag, ClUint32T maxKeySize, ClUint32T maxRecordSize)
{
    if (pDBHandle) close();

    ClRcT errorCode = CL_OK;
    
    ClUint32T rc = 0;
    FILE* fp = NULL;
    
    NULL_CHECK(dbName);
    NULL_CHECK(dbFile);
    
    if(dbFlag >= CL_DB_MAX_FLAG) 
    {
        errorCode = CL_RC(CL_CID_DBAL,CL_ERR_INVALID_PARAMETER);
        logError("DBA", "DBO", "Checkpoint DB Open failed: Invalid flag specified.");
        
        return(errorCode);
    }
/*
	if((CL_DB_SYNC & dbFlag))
	{
		dbFlag &= ~(CL_DB_SYNC);
		enableSync = CL_TRUE;
	}
*/
    /* Let the environment variable override the coded behaviour */
/*
    if (getenv("ASP_DB_SYNC"))
    {
        if (SAFplus::clParseEnvBoolean("ASP_DB_SYNC"))
            enableSync = CL_TRUE;
        else
            enableSync = CL_FALSE;
    }
*/
    Handle ckptHdl; // TODO: convert dbName to checkpoint handle???
    Checkpoint* p = NULL;

    /*if(NULL == p) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logError("DBA", "DBO", "Checkpoint DB Open failed: No Memory.");
        
        return(errorCode);
    }*/


    //logTrace("DBA", "DBO", "Opening the Database : [%s]", dbName);

    if (dbFlag == CL_DB_CREAT)
    {
        // remove the checkpoint from shared mem
        boost::interprocess::shared_memory_object::remove(dbName);
        p = new Checkpoint(ckptHdl, Checkpoint::SHARED | Checkpoint::LOCAL);
    }    
    else if(dbFlag == CL_DB_OPEN)
    {
        try
        {
            p = new Checkpoint(ckptHdl, Checkpoint::EXISTING|Checkpoint::SHARED|Checkpoint::LOCAL);
        }
        catch (boost::interprocess::interprocess_exception& e)
        {
            //TODO: need to check error code???
	    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
	    logError("DBA", "DBO", "Cannot open checkpoint file [%s].",dbName);
	    return(errorCode);    
	}
     }                       
     else if(dbFlag == CL_DB_APPEND)
     {
        p = new Checkpoint(ckptHdl, Checkpoint::SHARED | Checkpoint::LOCAL);        
     }
    
     if (!p)
     {        
        logError("DBA", "DBO", "Checkpoint DB Open failed: No Memory.");
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        return errorCode;  
     }

     pDBHandle = (ClDBHandleT) p;

     return(CL_OK);
}

ClRcT CkptPlugin::close()
{
    ClRcT errorCode = CL_OK;

    if (!pDBHandle) return errorCode;
   
    delete (Checkpoint*)pDBHandle;
    pDBHandle=NULL;
    
    return (CL_OK);  
}

ClRcT CkptPlugin::insertRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize)
{
    ClRcT errorCode = CL_OK;
    Checkpoint* p = NULL;
    ClUint32T rc = 0;

    
    p = (Checkpoint*)pDBHandle;

    NULL_CHECK(dbKey);
    NULL_CHECK(dbRec);

    logTrace("DBA", "ADD", "Adding a record into the database");    
    char data[sizeof(Buffer)-1+keySize]; // -1 because inside Buffer the data field is already length one.
    Buffer* key = new(data) Buffer(keySize);
    memcpy(key->data, dbKey, keySize);
    const Buffer& rec = p->read(*key);
    if (&rec)
    {
        /* Duplicate record is getting inserted */
        errorCode = CL_DBAL_RC(CL_ERR_DUPLICATE);
        logTrace("DBA", "ADD", "Failed to execute the SQL statement. Duplicate record is getting inserted.");
        return errorCode;
    }
    char vmem[sizeof(Buffer)-1+recSize];
    Buffer* writtenRec = new(vmem) Buffer(recSize);
    memcpy(writtenRec->data, dbRec, recSize);
    key->setNullT(true);
    writtenRec->setNullT(true);
    p->write(*key,*writtenRec);
    
    return (errorCode);
}

ClRcT CkptPlugin::replaceRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize)
{
    ClRcT errorCode = CL_OK;
    Checkpoint* p = NULL;
    ClUint32T rc = 0;

    
    p = (Checkpoint*)pDBHandle;

    NULL_CHECK(dbKey);
    NULL_CHECK(dbRec);

    logTrace("DBA", "ADD", "Replacing a record in the database");    
    char data[sizeof(Buffer)-1+keySize]; // -1 because inside Buffer the data field is already length one.
    Buffer* key = new(data) Buffer(keySize);
    memcpy(key->data, dbKey, keySize);
    
    char vmem[sizeof(Buffer)-1+recSize];
    Buffer* writtenRec = new(vmem) Buffer(recSize);
    memcpy(writtenRec->data, dbRec, recSize);
    key->setNullT(true);
    writtenRec->setNullT(true);
    p->write(*key,*writtenRec);     

    
    return (errorCode);
}

ClRcT CkptPlugin::getRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize)
{
    ClRcT errorCode = CL_OK;
    Checkpoint* p = NULL;
    ClUint32T rc = 0;
    
    NULL_CHECK(dbKey);
    NULL_CHECK(pDBRec);
    NULL_CHECK(pRecSize);

    p = (Checkpoint*)pDBHandle;
    logTrace("DBA", "GET", "Retrieving a record from the database");
    char data[sizeof(Buffer)-1+keySize]; // -1 because inside Buffer the data field is already length one.
    Buffer* key = new(data) Buffer(keySize);
    memcpy(key->data, dbKey, keySize);
    const Buffer& rec = p->read(*key);
    if (&rec==NULL)
    {
       errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
       logTrace("DBA", "GET", "No record found in the database");
       return errorCode;
    }
    *pRecSize = rec.len();
    *pDBRec = (ClDBRecordT)SAFplusHeapAlloc(*pRecSize);
    if (*pDBRec == NULL)
    {        
        logError("DBA", "DBO", "Checkpoint DB Open failed: No Memory.");
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        return errorCode;  
    }
    memcpy(*pDBRec, rec.data, *pRecSize);
    
    return (errorCode);
}

ClRcT CkptPlugin::getFirstRecord(ClDBKeyT* pDBKey, ClUint32T* pKeySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize)
{
    ClRcT errorCode = CL_OK;
    Checkpoint* p = NULL;
    ClUint32T rc = 0;    
   
    NULL_CHECK(pDBKey);
    NULL_CHECK(pKeySize);

    NULL_CHECK(pDBRec);
    NULL_CHECK(pRecSize);

    p = (Checkpoint*)pDBHandle;
    
    logTrace("DBA", "GET", "Retrieving the first record from the database");
    
    Checkpoint::Iterator i=p->begin();
    if (i!=p->end())
    {
        Checkpoint::KeyValuePair& item = *i;
        *pKeySize = (*item.first).len();
        *pDBKey = (ClDBKeyT)SAFplusHeapAlloc(*pKeySize);
        if (*pDBKey == NULL)
        {        
            logError("DBA", "DBO", "Checkpoint getFirstRecord failed: No Memory.");
            errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
            return errorCode;  
        }
        memcpy(*pDBKey, (*item.first).data, *pKeySize);
        *pRecSize = (*item.second).len();
        *pDBRec = (ClDBRecordT)SAFplusHeapAlloc(*pRecSize);
        if (*pDBRec == NULL)
        {        
            logError("DBA", "DBO", "Checkpoint getFirstRecord failed: No Memory.");
            errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
            return errorCode;  
        }
        memcpy(*pDBRec, (*item.second).data, *pRecSize);
        return CL_OK;
    }

    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace("DBA", "GET", "No records found in the database.");
    return (errorCode);
}

ClRcT CkptPlugin::getNextRecord(ClDBKeyT currentKey, ClUint32T currentKeySize, ClDBKeyT* pDBNextKey, ClUint32T* pNextKeySize, ClDBRecordT* pDBNextRec, ClUint32T* pNextRecSize)
{
    ClRcT errorCode = CL_OK;
    Checkpoint* p = NULL;
    ClUint32T rc = 0;       

    NULL_CHECK(currentKey);
    NULL_CHECK(pDBNextKey);
    NULL_CHECK(pNextKeySize);

    NULL_CHECK(pDBNextRec);
    NULL_CHECK(pNextRecSize);    
    
    return (errorCode);
}

ClRcT CkptPlugin::deleteRecord(ClDBKeyHandleT dbKey, ClUint32T keySize)
{
    ClRcT errorCode = CL_OK;
    Checkpoint* p = NULL;
    ClUint32T rc = 0;  
       
    NULL_CHECK(dbKey);

    logTrace("DBA", "DEL", "Removing a record from the database");      

    
    return (errorCode);
}

ClRcT CkptPlugin::syncDbal(ClUint32T flags)
{
   /*Auto.DB sync would be through DB_SYNC open flag for DBAL*/
    return (CL_OK);
}

ClRcT CkptPlugin::openTransaction(ClDBFileT dbFile, ClDBNameT  dbName, ClDBFlagT dbFlag, ClUint32T  maxKeySize, ClUint32T  maxRecordSize)
{
    ClRcT errorCode = CL_OK;
    Checkpoint* p = NULL;
    ClUint32T rc = 0;

    

    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    logError("DBA", "TXN", "Checkpoint Transaction is not supported");

    
    return (errorCode);
}

ClRcT CkptPlugin::beginTransaction()
{
    ClRcT errorCode = CL_OK;
    Checkpoint* p = NULL;
    ClUint32T rc = 0;
    

    /* Transactions are not supported currently */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    logError("DBA", "TXN", "Checkpoint Transaction is not supported");
    

    return (errorCode);
}

ClRcT CkptPlugin::commitTransaction()
{
    ClRcT errorCode = CL_OK;
    Checkpoint* p = NULL;
    ClUint32T rc = 0;
    

    /* Transactions are not supported currently */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    logError("DBA", "TXN", "Checkpoint Transaction is not supported");
    

    return (errorCode);
}

ClRcT CkptPlugin::abortTransaction()
{
    ClRcT errorCode = CL_OK;
    Checkpoint* p = NULL;
    ClUint32T rc = 0;

    

    /* Transactions are not supported currently */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    logError("DBA", "TXN", "Checkpoint Transaction is not supported");
    

    return (errorCode);
}

ClRcT CkptPlugin::freeRecord(ClDBRecordT dbRec)
{
    ClRcT errorCode = CL_OK;

    NULL_CHECK(dbRec);
    SAFplusHeapFree(dbRec);
    return (CL_OK);
}

ClRcT CkptPlugin::freeKey(ClDBRecordT dbKey)
{
    ClRcT errorCode = CL_OK;

    NULL_CHECK(dbKey);
    SAFplusHeapFree(dbKey);
    return (CL_OK);
}

};

extern "C" SAFplus::ClPlugin* clPluginInitialize(uint_t preferredPluginVersion)
{
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::CkptPlugin* api = new SAFplus::CkptPlugin();
  api->pluginId         = SAFplus::CL_DBAL_PLUGIN_ID;
  api->pluginVersion    = SAFplus::CL_DBAL_PLUGIN_VER;
  api->type = "Ckpt";

  // return it
  //return (SAFplus::ClPlugin*) &SAFplus::api;
  return (SAFplus::ClPlugin*) api;
}
