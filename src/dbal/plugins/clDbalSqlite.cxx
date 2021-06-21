#include <clDbalBase.hxx>
#include <string.h>
#include <clCommon6.h>
#include <clCommonErrors6.h>
#include <sqlite3.h>
#if (SQLITE_VERSION_NUMBER < 3003013)
#error SQLITE version number must be >= 3003013
#endif
#include <clCommon.hxx>
#include <clLogIpi.hxx>
//#include <clDbg.hxx>
//#include "../../clovisDbalInternal.h"
//#include "../../clDbalInterface.h"
//#include <clDbalCfg.h>

namespace SAFplus {

typedef struct SQLiteDBHandle_t {
    sqlite3* pDatabase;

    /* statement objects
     *
     * stmt[0] - RecordAdd
     * stmt[1] - RecordReplace
     * stmt[2] - RecordDelete
     * stmt[3] - RecordGet
     * stmt[4] - FirstRecordGet
     * stmt[5] - NextRecordGet
     */
    sqlite3_stmt* stmt[7];

  }SQLiteDBHandle_t;

class SqlitePlugin: public DbalPlugin
{
public:
  //SqlitePlugin() {}
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
  virtual ~SqlitePlugin();

protected:
  ClRcT cdbSQLiteDBCreate(SQLiteDBHandle_t* handle, ClDBNameT dbName);
  virtual ClRcT close();
};

//static SqlitePlugin api;

static int cl_clear_bindings(sqlite3_stmt *pStmt)
{
  int i;
  int rc = SQLITE_OK;
  for(i=1; rc==SQLITE_OK && i<=sqlite3_bind_parameter_count(pStmt); i++)
  {
    rc = sqlite3_bind_null(pStmt, i);
  }
  return rc;
}

SqlitePlugin::~SqlitePlugin()
{ 
  close(); 
}

ClRcT SqlitePlugin::open(ClDBFileT dbFile, ClDBNameT dbName, ClDBFlagT dbFlag, ClUint32T maxKeySize, ClUint32T maxRecordSize)
{
    if (pDBHandle) close();

    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    sqlite3_stmt *stmt =  NULL;
    ClUint32T rc = 0;
    FILE* fp = NULL;
    ClBoolT enableSync = CL_FALSE;

    //NULL_CHECK(pDBHandle);
    NULL_CHECK(dbName);
    NULL_CHECK(dbFile);
    
    if(dbFlag >= CL_DB_MAX_FLAG) {
        errorCode = CL_RC(CL_CID_DBAL,CL_ERR_INVALID_PARAMETER);
        logError("DBA", "DBO", "SQLite DB Open failed: Invalid flag specified.");
        
        return(errorCode);
    }

	if((CL_DB_SYNC & dbFlag))
	{
		dbFlag &= ~(CL_DB_SYNC);
		enableSync = CL_TRUE;
	}

    /* <cfg name="SAFPLUS_DB_SYNC">Should the database sync disks upon write?  Yes, reduces performance but decreases lost data upon failure.  This environment variable will override the coded behaviour.</cfg> */
    enableSync = SAFplus::parseEnvBoolean("SAFPLUS_DB_SYNC",enableSync);
  

    pSQLiteHandle = (SQLiteDBHandle_t *)SAFplusHeapAlloc(sizeof(SQLiteDBHandle_t));

    if(NULL == pSQLiteHandle) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logError("DBA", "DBO", "SQLite DB Open failed: No Memory.");
        
        return(errorCode);
    }

    //pDBHandle = (ClDBHandleT) pSQLiteHandle;

    logTrace("DBA", "DBO", "Opening the Database : [%s]", dbName);

    if (dbFlag == CL_DB_CREAT)
    {
        if((fp = fopen(dbName, "r")) != NULL)
        {
            fclose(fp);

            rc = remove(dbName);
            
            if (0 != rc)
            {
                SAFplusHeapFree(pSQLiteHandle);
                errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
                logError("DBA", "DBO", "SQLite DB remove failed.");
                
                return(errorCode);    
            }
        }

        rc = cdbSQLiteDBCreate(pSQLiteHandle, dbName);
        if (rc != CL_OK)
        {
            sqlite3_close(pSQLiteHandle->pDatabase);
            SAFplusHeapFree(pSQLiteHandle);
            errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
            logError("DBA", "DBC", "SQLite DB Create failed. rc [0x%x]", rc);
            
            return errorCode;
        }
    }
    else if(dbFlag == CL_DB_OPEN)
    {
		if ((fp = fopen(dbName, "r")) == NULL)
		{	
			SAFplusHeapFree(pSQLiteHandle);
			errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
			logError("DBA", "DBO", "Cannot open SQLite DB file [%s].",dbName);
			
			return(errorCode);    
		}

		fclose(fp);

        rc = sqlite3_open(dbName, &(pSQLiteHandle->pDatabase));

        if(SQLITE_OK != rc) {
            logError("DBA", "DBO", "Failed to open the database");
            logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                    sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
            sqlite3_close(pSQLiteHandle->pDatabase);
            SAFplusHeapFree(pSQLiteHandle);
            errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
            
            return(errorCode);
        }               
    }
    else if(dbFlag == CL_DB_APPEND)
    {
        if ((fp = fopen(dbName, "r")) == NULL)
        {
            /* Database doesn't exist. Create it.. */
            rc = cdbSQLiteDBCreate(pSQLiteHandle, dbName);
            if (rc != CL_OK)
            {
                sqlite3_close(pSQLiteHandle->pDatabase);
                SAFplusHeapFree(pSQLiteHandle);
                errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
                logError("DBA", "DBO", "SQLite DB Create failed. rc [0x%x]", rc);
                
                return errorCode;
            }
        }
        else
        {
             fclose(fp);

             rc = sqlite3_open(dbName, &(pSQLiteHandle->pDatabase));

             if(SQLITE_OK != rc) {
                logError("DBA", "DBO", "Error in opening the DB in append mode");
                logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                        sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
                sqlite3_close(pSQLiteHandle->pDatabase);
                SAFplusHeapFree(pSQLiteHandle);
                errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
                
                return(errorCode);
            }           
        } 
    }
    
    if(enableSync == CL_TRUE)
    {
        rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "pragma synchronous = full", -1, &stmt, 0);
    }
    else
    {
        rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "pragma synchronous = off", -1, &stmt, 0);
    }

    if(rc != SQLITE_OK)
    {
        logError("DBA", "DBO", "Error in pragma prepare");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]",
                   sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		SAFplusHeapFree(pSQLiteHandle);
        errorCode= CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        
        return errorCode;
    }

    rc = sqlite3_step(stmt);      
    
    if(SQLITE_DONE != rc)
    { 
        sqlite3_finalize(stmt); 
        logError("DBA", "DBO", "Failed to execute the pragma SQL statement");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        errorCode= CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
		sqlite3_close(pSQLiteHandle->pDatabase);
		SAFplusHeapFree(pSQLiteHandle);
        
        return(errorCode);               
    }

    rc = sqlite3_finalize(stmt); 

    if(SQLITE_OK != rc)
    {
        logError("DBA", "DBO", "Failed to finalize the SQL statement.");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		SAFplusHeapFree(pSQLiteHandle);
        errorCode= CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        
        return(errorCode);
    }
    
    /* Prepare the statements */
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "insert into ObjectRepository values(?, ?)", -1, &(pSQLiteHandle->stmt[0]), 0);

    if (rc != SQLITE_OK)
    {
        logError("DBA", "DBO", "Error in prepare statement in record add");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		SAFplusHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        
        return(errorCode);       
    }

    /* INSERT or REPLACE */
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "replace into ObjectRepository(Key, Data) values(?, ?)", -1, 
		&(pSQLiteHandle->stmt[1]), 0);

    if (rc != SQLITE_OK)
    {
        logError("DBA", "DBO", "Failed to prepare the SQL statement.");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		SAFplusHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return(errorCode);       
    }

    /* DELETE */
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "delete from ObjectRepository where Key=?", -1, &(pSQLiteHandle->stmt[2]), 0);

    if (rc != SQLITE_OK)
    {
        logError("DBA", "DBO", "Failed to prepare the SQL statement.");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		SAFplusHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return(errorCode);       
    }

    /* RECORD GET */
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "select * from ObjectRepository where Key=?", -1, &(pSQLiteHandle->stmt[3]), 0);

    if (rc != SQLITE_OK)
    {
        logError("DBA", "DBO", "Failed to prepare the SQL statement.");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		SAFplusHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return(errorCode);       
    }

    /* FIRST RECORD GET */
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "select * from ObjectRepository limit 0,1", -1, &(pSQLiteHandle->stmt[4]), 0);

    if(rc != SQLITE_OK)
    {
        logError("DBA", "DBO", "Failed to prepare the SQL statement.");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		SAFplusHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return (errorCode);       
    }
 
    /* NEXT RECORD GET */
    rc = sqlite3_prepare(
			pSQLiteHandle->pDatabase, 
			"select * from ObjectRepository where rowid = (select min(rowid) from ObjectRepository "
			"where rowid > (select rowid from ObjectRepository where Key=?))", 
			-1, &(pSQLiteHandle->stmt[5]), 0);
    if (rc != SQLITE_OK)
    {
        logError("DBA", "DBO", "Failed to prepare the SQL statement.");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		SAFplusHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return (errorCode);
    }

    pDBHandle = (ClDBHandleT) pSQLiteHandle;

    
    return(CL_OK);
}

ClRcT SqlitePlugin::cdbSQLiteDBCreate(SQLiteDBHandle_t* handle, ClDBNameT dbName)
{
    sqlite3_stmt* stmt = NULL; 
    ClRcT rc = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = handle;
    ClRcT errorCode = CL_OK;
    NULL_CHECK(pSQLiteHandle);

    rc = sqlite3_open(dbName, &(pSQLiteHandle->pDatabase));

    if(SQLITE_OK != rc) {
      logError("DBA", "DBO", "Failed to open the SQLite DB [%s] SQLite Error [%s] errorCode [%d]", 
               dbName, sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        
        return(rc);
    }
        
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, 
            "create table ObjectRepository(Key BLOB primary key, Data BLOB)", -1, &stmt, 0);

    if(SQLITE_OK != rc)
    {
        sqlite3_finalize(stmt); 
        logError("DBA", "DBO", "Failed to prepare statement to open a DB.");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        
        return(rc);
    }

    rc = sqlite3_step(stmt);      
    
    if(SQLITE_DONE != rc)
    { 
        sqlite3_finalize(stmt); 
        logError("DBA", "DBO", "Failed to execute the SQL statement to create the db");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        
        return(rc);               
    }

    rc = sqlite3_finalize(stmt); 

    if(SQLITE_OK != rc)
    {
        logError("DBA", "DBO", "Failed to finalize the SQL statement.");
        logError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        
        return(rc);
    }
  
    
    return rc;
}

ClRcT SqlitePlugin::close()
{
    ClRcT errorCode = CL_OK;

    if (!pDBHandle) return errorCode;

    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;
    

    pSQLiteHandle = (SQLiteDBHandle_t *)pDBHandle;  

    /* Finalize the prepared statements */
    /* ADD */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[0]);
    
    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
         
        logError("DBA", "ADD", "Failed to finalize the SQL statment.");
        logError("DBA", "ADD", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }

    /* REPLACE */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[1]);
    
    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logError("DBA", "REP", "Failed to finalize the SQL statement.");
        logError("DBA", "REP", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }

    /* DELETE */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[2]);
    
    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logError("DBA", "DEL", "Failed to finalize the SQL statement.");
        logError("DBA", "DEL", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }
    
    /* RECORD GET */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[3]);
    
    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logError("DBA", "GET", "Failed to finalize the SQL statement.");
        logError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }

    /* FIRST RECORD GET */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[4]);

    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logError("DBA", "GET", "Failed to finalize the SQL statement.");
        logError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }

    /* NEXT RECORD GET */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[5]);

    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logError("DBA", "GET", "Failed to finalize the SQL statement");
        logError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }

    /* DB Close */
    rc = sqlite3_close(pSQLiteHandle->pDatabase);

    if(rc == SQLITE_BUSY) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logError("DBA", "DBC", "Failed to close the SQLite DB.");
        logError("DBA", "DBC", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        
        return(errorCode);
    }

    SAFplusHeapFree(pSQLiteHandle);
    pDBHandle=NULL;
    
    logInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nSqlite closed.");
    return (CL_OK);  
}

ClRcT SqlitePlugin::insertRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;

    
    pSQLiteHandle = (SQLiteDBHandle_t *)pDBHandle;

    NULL_CHECK(pSQLiteHandle);
    NULL_CHECK(dbKey);
    NULL_CHECK(dbRec);

    logTrace("DBA", "ADD", "Adding a record into the database");

    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[0], 1, (const void *)dbKey, keySize, SQLITE_STATIC);
    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[0], 2, (const void *)dbRec, recSize, SQLITE_STATIC);

retry:

    rc = sqlite3_step(pSQLiteHandle->stmt[0]);

    if (rc != SQLITE_DONE)
    {
        if (rc == SQLITE_BUSY)
        {
            logInfo("DBA", "ADD", "Couldn't acquire lock to update the database. retrying..");
            goto retry;
        }

        /* Reset the stmt object to get the specific error code */
        rc = sqlite3_reset(pSQLiteHandle->stmt[0]);

        if (rc == SQLITE_CONSTRAINT)
        {
            /* Duplicate record is getting inserted */
            errorCode = CL_DBAL_RC(CL_ERR_DUPLICATE);
            logTrace("DBA", "ADD", "Failed to execute the SQL statement. Duplicate record is getting inserted.");
        }
        else
        {
            errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
            logError("DBA", "ADD", "Failed to execute the SQL statement. Unable to insert the record.");
            logError("DBA", "ADD", "SQLite Error : %s. errorCode [%d]", 
                   sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        }

        
        goto finalize;       
    }
    
    rc = sqlite3_reset(pSQLiteHandle->stmt[0]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[0]);

finalize:    

    
    return (errorCode);
}

ClRcT SqlitePlugin::replaceRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;

    

    pSQLiteHandle = (SQLiteDBHandle_t *)pDBHandle;

    NULL_CHECK(pSQLiteHandle);
    NULL_CHECK(dbKey);
    NULL_CHECK(dbRec);

    logTrace("DBA", "REP", "Replacing a record from the database");

    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[1], 1, (const void *)dbKey, keySize, SQLITE_STATIC);
    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[1], 2, (const void *)dbRec, recSize, SQLITE_STATIC);

retry:
    rc = sqlite3_step(pSQLiteHandle->stmt[1]);
	
    if (rc != SQLITE_DONE)
    {
        if (rc == SQLITE_BUSY)
        {
            logInfo("DBA", "REP", "Couldn't get lock to update the database. retrying..");
            goto retry;
        }

        /* Reset the stmt object to get the specific error code */
        rc = sqlite3_reset(pSQLiteHandle->stmt[1]);

        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logError("DBA", "REP", "Failed to execute the SQL statement to replace a record. rc [%d]", rc);
        logError("DBA", "REP", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        goto finalize;
    }

    rc = sqlite3_reset(pSQLiteHandle->stmt[1]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[1]);
finalize:    

    
    return (errorCode);
}

ClRcT SqlitePlugin::getRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;

    
    
    NULL_CHECK(dbKey);
    NULL_CHECK(pDBRec);
    NULL_CHECK(pRecSize);

    pSQLiteHandle = (SQLiteDBHandle_t *)pDBHandle;
    NULL_CHECK(pSQLiteHandle);
    //TODO: happens too often, even for trace, but first verify efficient use
    logTrace("DBA", "GET", "Retrieving a record from the database");

    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[3], 1, (const void *) dbKey, keySize, SQLITE_STATIC);

retry:

    rc = sqlite3_step(pSQLiteHandle->stmt[3]);    

    if (rc != SQLITE_ROW)
    {
        if (rc == SQLITE_BUSY)
        {
            logTrace("DBA", "GET", "Couldn't get lock to update the database. retrying..");
            goto retry;
        }

        if (rc == SQLITE_DONE)
        {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            logTrace("DBA", "GET", "No record found in the database");
            rc = sqlite3_reset(pSQLiteHandle->stmt[3]);
            rc = cl_clear_bindings(pSQLiteHandle->stmt[3]);
        }
        else
        {
            rc = sqlite3_reset(pSQLiteHandle->stmt[3]);
            errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
            logError("DBA", "GET", "Failed to execute the SQL statement to retrieve a record.");
            logError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                    sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        }

        goto finalize;
    }

    *pRecSize = (ClUint32T) sqlite3_column_bytes(pSQLiteHandle->stmt[3], 1);
    
    *pDBRec = (ClDBRecordT) SAFplusHeapAlloc(*pRecSize);
    
    if(NULL == *pDBRec) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logError("DBA", "GET", "Failed to allocate memory.");
        
        goto finalize;
    }

    memcpy(*pDBRec, sqlite3_column_blob(pSQLiteHandle->stmt[3], 1), *pRecSize);

    rc = sqlite3_reset(pSQLiteHandle->stmt[3]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[3]);

finalize:

    
    return (errorCode);
}

ClRcT SqlitePlugin::getFirstRecord(ClDBKeyT* pDBKey, ClUint32T* pKeySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;
 
    
   
    NULL_CHECK(pDBKey);
    NULL_CHECK(pKeySize);

    NULL_CHECK(pDBRec);
    NULL_CHECK(pRecSize);

    pSQLiteHandle = (SQLiteDBHandle_t *)pDBHandle;   
    NULL_CHECK(pSQLiteHandle);
    logTrace("DBA", "GET", "Retrieving the first record from the database");

retry:    
    rc = sqlite3_step(pSQLiteHandle->stmt[4]);

    if((rc != SQLITE_ROW))
    { 
        if (rc == SQLITE_BUSY)
        {
            logInfo("DBA", "GET", "Couldn't get the lock to retrieve the value. retrying..");
            goto retry;
        }

        if (rc == SQLITE_DONE)
        {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            logTrace("DBA", "GET", "No records found in the database.");
            rc = sqlite3_reset(pSQLiteHandle->stmt[4]);
            rc = cl_clear_bindings(pSQLiteHandle->stmt[4]);
            goto finalize;
        }
             
        rc = sqlite3_reset(pSQLiteHandle->stmt[4]);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logError("DBA", "GET", "Failed to retrieve the First Record.");
        logError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        goto finalize;       
    }

    *pKeySize = (ClUint32T) sqlite3_column_bytes(pSQLiteHandle->stmt[4], 0);
    *pRecSize = (ClUint32T) sqlite3_column_bytes(pSQLiteHandle->stmt[4], 1);

    *pDBKey = (ClDBKeyT) SAFplusHeapAlloc(*pKeySize);

    if(NULL == *pDBKey) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logError("DBA", "GET", "Failed to allocate memory");
        
        goto finalize;
    }

    memcpy(*pDBKey, sqlite3_column_blob(pSQLiteHandle->stmt[4], 0), *pKeySize);
   
    *pDBRec = (ClDBRecordT) SAFplusHeapAlloc(*pRecSize);

    if(NULL == *pDBRec) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logError("DBA", "GET", "Failed to allocate memory");
        SAFplusHeapFree(*pDBKey);
        
        goto finalize;
    }

    memcpy(*pDBRec, sqlite3_column_blob(pSQLiteHandle->stmt[4], 1), *pRecSize); 

    rc = sqlite3_reset(pSQLiteHandle->stmt[4]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[4]);

finalize:    
    
    
    return (errorCode);
}

ClRcT SqlitePlugin::getNextRecord(ClDBKeyT currentKey, ClUint32T currentKeySize, ClDBKeyT* pDBNextKey, ClUint32T* pNextKeySize, ClDBRecordT* pDBNextRec, ClUint32T* pNextRecSize)
{
   ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;

    

    NULL_CHECK(currentKey);
    NULL_CHECK(pDBNextKey);
    NULL_CHECK(pNextKeySize);

    NULL_CHECK(pDBNextRec);
    NULL_CHECK(pNextRecSize);

    pSQLiteHandle = (SQLiteDBHandle_t *)pDBHandle;
    NULL_CHECK(pSQLiteHandle);
    logTrace("DBA", "GET", "Retrieving the next record from the database");

    sqlite3_bind_blob(pSQLiteHandle->stmt[5], 1, (const void *) currentKey, currentKeySize, SQLITE_STATIC);
    
retry1:    
    rc = sqlite3_step(pSQLiteHandle->stmt[5]);

    if (rc != SQLITE_ROW)
    { 
        if (rc == SQLITE_BUSY)
        {
            logTrace("DBA", "GET", "Couldn't get the lock the retrieve the value. retrying..");
            goto retry1;
        }

        if (rc == SQLITE_DONE)
        {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            logInfo("DBA", "GET", "Current record does not exist in the database.");
            rc = sqlite3_reset(pSQLiteHandle->stmt[5]);
            rc = cl_clear_bindings(pSQLiteHandle->stmt[5]);
            goto finalize;
        }

        rc = sqlite3_reset(pSQLiteHandle->stmt[5]);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logError("DBA", "GET", "Failed to retrieve the record with the key value specified.");
        logError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        goto finalize;       
    }

    *pNextKeySize = (ClUint32T) sqlite3_column_bytes(pSQLiteHandle->stmt[5], 0);
    *pNextRecSize = (ClUint32T) sqlite3_column_bytes(pSQLiteHandle->stmt[5], 1);

    *pDBNextKey = (ClDBKeyT) SAFplusHeapAlloc(*pNextKeySize);

    if(NULL == *pDBNextKey) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logError("DBA", "GET", "Failed to allocate memory");
        
        goto finalize;
    }

    memcpy(*pDBNextKey, sqlite3_column_blob(pSQLiteHandle->stmt[5], 0), *pNextKeySize);
   
    *pDBNextRec = (ClDBRecordT) SAFplusHeapAlloc(*pNextRecSize);

    if(NULL == *pDBNextRec) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logError("DBA", "GET", "Failed to allocate memory");
        SAFplusHeapFree(*pDBNextKey);
        
        goto finalize;
    }

    memcpy(*pDBNextRec, sqlite3_column_blob(pSQLiteHandle->stmt[5], 1), *pNextRecSize); 

    rc = sqlite3_reset(pSQLiteHandle->stmt[5]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[5]);

finalize:

    
    return (errorCode);
}

ClRcT SqlitePlugin::deleteRecord(ClDBKeyHandleT dbKey, ClUint32T keySize)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;

    
    pSQLiteHandle = (SQLiteDBHandle_t *)pDBHandle;
    NULL_CHECK(pSQLiteHandle);
    NULL_CHECK(dbKey);

    logTrace("DBA", "DEL", "Removing a record from the database");

    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[2], 1, (const void *)dbKey, keySize, SQLITE_STATIC);
    
retry:
    rc = sqlite3_step(pSQLiteHandle->stmt[2]);

    if (rc != SQLITE_DONE)
    {
        if (rc == SQLITE_BUSY)
        {
            logInfo("DBA", "DEL", "Couldn't get lock to update the database. retrying..");
            goto retry;
        }

        rc = sqlite3_reset(pSQLiteHandle->stmt[2]);

        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logError("DBA", "DEL", "Failed to execute the SQL statement to delete a record.");
        logError("DBA", "DEL", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        goto finalize;
    }

    if (sqlite3_changes(pSQLiteHandle->pDatabase) == 0)
    {    
        errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
        logTrace("DBA", "DEL", "No record exists in the database");
        sqlite3_reset(pSQLiteHandle->stmt[2]);
        cl_clear_bindings(pSQLiteHandle->stmt[2]);
        goto finalize;
    }    

    rc = sqlite3_reset(pSQLiteHandle->stmt[2]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[2]);

finalize:    

    
    return (errorCode);
}

ClRcT SqlitePlugin::syncDbal(ClUint32T flags)
{
   /*Auto.DB sync would be through DB_SYNC open flag for DBAL*/
    return (CL_OK);
}

ClRcT SqlitePlugin::openTransaction(ClDBFileT dbFile, ClDBNameT  dbName, ClDBFlagT dbFlag, ClUint32T  maxKeySize, ClUint32T  maxRecordSize)
{
    ClRcT errorCode = CL_OK;

    

    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    logError("DBA", "TXN", "SQLite Transaction is not supported");

    
    return (errorCode);
}

ClRcT SqlitePlugin::beginTransaction()
{
    ClRcT errorCode = CL_OK;

    

    /* Transactions are not supported currently */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    logError("DBA", "TXN", "SQLite Transaction is not supported");
    

    return (errorCode);
}

ClRcT SqlitePlugin::commitTransaction()
{
   ClRcT errorCode = CL_OK;

    

    /* Transactions are not supported currently */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    logError("DBA", "TXN", "SQLite Transaction is not supported");
    

    return (errorCode);
}

ClRcT SqlitePlugin::abortTransaction()
{
    ClRcT errorCode = CL_OK;

    

    /* Transactions are not supported currently */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    logError("DBA", "TXN", "SQLite Transaction is not supported");
    

    return (errorCode);
}

ClRcT SqlitePlugin::freeRecord(ClDBRecordT dbRec)
{
    ClRcT errorCode = CL_OK;

    NULL_CHECK(dbRec);
    SAFplusHeapFree(dbRec);
    return (CL_OK);
}

ClRcT SqlitePlugin::freeKey(ClDBRecordT dbKey)
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
  SAFplus::SqlitePlugin* api = new SAFplus::SqlitePlugin();
  api->pluginId         = SAFplus::CL_DBAL_PLUGIN_ID;
  api->pluginVersion    = SAFplus::CL_DBAL_PLUGIN_VER;
  api->type = "Sqlite";

  // return it
  //return (SAFplus::ClPlugin*) &SAFplus::api;
  return (SAFplus::ClPlugin*) api;
}
