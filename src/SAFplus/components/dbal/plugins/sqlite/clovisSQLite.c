 /*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
#include <string.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <sqlite3.h>
#if (SQLITE_VERSION_NUMBER < 3003013)
#error SQLITE version number must be >= 3003013
#endif
#include <clDbalApi.h>
#include <clDebugApi.h>
#include "clovisDbalInternal.h"
#include "clDbalInterface.h"
#include <clDbalCfg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ClDbalFunctionPtrsT *gDbalFunctionPtrs;

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

/*ClRcT clDbalInterface(ClDbalFunctionPtrsT *);*/
/*****************************************************************************/
static ClRcT cdbSQLiteDBCreate(ClDBNameT dbName, SQLiteDBHandle_t* pSQLiteHandle);
/*****************************************************************************/
static ClRcT  cdbSQLiteDBOpen(ClDBFileT dbFile, ClDBNameT  dbName, ClDBFlagT  dbFlag, ClUint32T  maxKeySize, ClUint32T maxRecordSize,
        ClDBHandleT* pDBHandle);
/*****************************************************************************/
static ClRcT cdbSQLiteDBClose(ClDBHandleT dbHandle);
/*****************************************************************************/
static ClRcT cdbSQLiteDBSync(ClDBHandleT dbHandle, ClUint32T flags);
/*****************************************************************************/
static ClRcT cdbSQLiteDBRecordAdd(ClDBHandleT  dbHandle, ClDBKeyT dbKey, ClUint32T  keySize, ClDBRecordT  dbRec, ClUint32T  recSize);
/*****************************************************************************/
static ClRcT cdbSQLiteDBRecordReplace(ClDBHandleT  dbHandle, ClDBKeyT  dbKey, ClUint32T  keySize, ClDBRecordT  dbRec, ClUint32T recSize);
/*****************************************************************************/
static ClRcT  cdbSQLiteDBRecordGet(ClDBHandleT  dbHandle, ClDBKeyT   dbKey, ClUint32T   keySize, ClDBRecordT*  pDBRec, ClUint32T*  pRecSize);
/*****************************************************************************/
static ClRcT cdbSQLiteDBRecordDelete(ClDBHandleT   dbHandle, ClDBKeyT    dbKey, ClUint32T   keySize);
/*****************************************************************************/
static ClRcT cdbSQLiteDBFirstRecordGet(ClDBHandleT dbHandle, ClDBKeyT* pDBKey, ClUint32T* pKeySize, ClDBRecordT* pDBRec, ClUint32T*  pRecSize);
/*****************************************************************************/
static ClRcT cdbSQLiteDBNextRecordGet(ClDBHandleT  dbHandle, ClDBKeyT  currentKey, ClUint32T  currentKeySize, ClDBKeyT*  pDBNextKey,                        ClUint32T*  pNextKeySize, ClDBRecordT*  pDBNextRec, ClUint32T*  pNextRecSize);
/*****************************************************************************/
static ClRcT  cdbSQLiteDBTxnOpen(ClDBFileT dbFile, ClDBNameT  dbName, ClDBFlagT dbFlag, ClUint32T  maxKeySize, ClUint32T  maxRecordSize, ClDBHandleT* pDBHandle);
/*****************************************************************************/
static ClRcT cdbSQLiteDBTransactionBegin(ClDBHandleT  dbHandle);
/*****************************************************************************/
static ClRcT cdbSQLiteDBTransactionCommit(ClDBHandleT  dbHandle);
/*****************************************************************************/
static ClRcT cdbSQLiteDBTransactionAbort(ClDBHandleT  dbHandle);
/*****************************************************************************/
static ClRcT cdbSQLiteDBRecordFree(ClDBHandleT  dbHandle, ClDBRecordT  dbRec );
/*****************************************************************************/
static ClRcT cdbSQLiteDBKeyFree(ClDBHandleT  dbHandle, ClDBKeyT     dbKey );
/*****************************************************************************/
static ClRcT cdbSQLiteDBInitialize(ClDBFileT    dbEnvFile);
/*****************************************************************************/

/* Earlier versions of sqlite 3 (3.3.6 for example) defined the 
   sqlite3_clear_bindings routine as experimental.  Some Linux distributions do
   not include the "experimental" APIs in the sqlite library.  Therefore,
   we have reproduced the routine here. */
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

ClRcT clDbalConfigInitialize(void* pDbalConfiguration)
{
    ClRcT errorCode = CL_OK;
    ClDbalConfigurationT* pConfig = NULL;

    CL_FUNC_ENTER();
    pConfig = (ClDbalConfigurationT *)pDbalConfiguration;

    NULL_CHECK(pConfig);

    errorCode = cdbSQLiteDBInitialize((ClDBFileT)pConfig->Database.sqliteConfig.enginePath);

    if(CL_OK != errorCode) {
        clLogError("DBA", "INI", "SQLite Initialization failed. rc [0x%x]", errorCode);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    CL_FUNC_EXIT();
    return(CL_OK);
}

static ClRcT cdbSQLiteDBInitialize(ClDBFileT dbEnvFile)
{
    /* 
     * No need for configuration as of now.
     * Defining these functions to do any configuration in the future.
     */

    return CL_OK;
}

static ClRcT cdbSQLiteDBCreate(ClDBNameT dbName, SQLiteDBHandle_t* pSQLiteHandle)
{
    sqlite3_stmt* stmt = NULL; 
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    rc = sqlite3_open(dbName, &(pSQLiteHandle->pDatabase));

    if(SQLITE_OK != rc) {
        clLogError("DBA", "DBO", "Failed to open the SQLite DB.");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
            sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        CL_FUNC_EXIT();
        return(rc);
    }
        
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, 
            "create table ObjectRepository(Key BLOB primary key, Data BLOB)", -1, &stmt, 0);

    if(SQLITE_OK != rc)
    {
        sqlite3_finalize(stmt); 
        clLogError("DBA", "DBO", "Failed to prepare statement to open a DB.");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        CL_FUNC_EXIT();
        return(rc);
    }

    rc = sqlite3_step(stmt);      
    
    if(SQLITE_DONE != rc)
    { 
        sqlite3_finalize(stmt); 
        clLogError("DBA", "DBO", "Failed to execute the SQL statement to create the db");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        CL_FUNC_EXIT();
        return(rc);               
    }

    rc = sqlite3_finalize(stmt); 

    if(SQLITE_OK != rc)
    {
        clLogError("DBA", "DBO", "Failed to finalize the SQL statement.");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        CL_FUNC_EXIT();
        return(rc);
    }
  
    CL_FUNC_EXIT();
    return rc;
}

static ClRcT  cdbSQLiteDBOpen(ClDBFileT    dbFile, ClDBNameT    dbName, ClDBFlagT    dbFlag, ClUint32T   maxKeySize, ClUint32T   maxRecordSize,
        ClDBHandleT* pDBHandle)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    sqlite3_stmt *stmt =  NULL;
    ClUint32T rc = 0;
    FILE* fp = NULL;
    ClBoolT enableSync = CL_FALSE;

    CL_FUNC_ENTER();

    NULL_CHECK(pDBHandle);
    NULL_CHECK(dbName);
    NULL_CHECK(dbFile);
    
    if(dbFlag >= CL_DB_MAX_FLAG) {
        errorCode = CL_RC(CL_CID_DBAL,CL_ERR_INVALID_PARAMETER);
        clLogError("DBA", "DBO", "SQLite DB Open failed: Invalid flag specified.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

	if((CL_DB_SYNC & dbFlag))
	{
		dbFlag &= ~(CL_DB_SYNC);
		enableSync = CL_TRUE;
	}

    /* Let the environment variable override the coded behaviour */
    if (getenv("ASP_DB_SYNC"))
    {
        if (clParseEnvBoolean("ASP_DB_SYNC") == CL_TRUE)
            enableSync = CL_TRUE;
        else
            enableSync = CL_FALSE;
    }

    pSQLiteHandle = (SQLiteDBHandle_t *)clHeapAllocate(sizeof(SQLiteDBHandle_t));

    if(NULL == pSQLiteHandle) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogError("DBA", "DBO", "SQLite DB Open failed: No Memory.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    clLogTrace("DBA", "DBO", "Opening the Database : [%s]", dbName);

    if (dbFlag == CL_DB_CREAT)
    {
        if((fp = fopen(dbName, "r")) != NULL)
        {
            fclose(fp);

            rc = remove(dbName);
            
            if (0 != rc)
            {
                clHeapFree(pSQLiteHandle);
                errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
                clLogError("DBA", "DBO", "SQLite DB remove failed.");
                CL_FUNC_EXIT();
                return(errorCode);    
            }
        }

        rc = cdbSQLiteDBCreate(dbName, pSQLiteHandle);
        if (rc != CL_OK)
        {
            sqlite3_close(pSQLiteHandle->pDatabase);
            clHeapFree(pSQLiteHandle);
            errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
            clLogError("DBA", "DBC", "SQLite DB Create failed. rc [0x%x]", rc);
            CL_FUNC_EXIT();
            return errorCode;
        }
    }
    else if(dbFlag == CL_DB_OPEN)
    {
		if ((fp = fopen(dbName, "r")) == NULL)
		{	
			clHeapFree(pSQLiteHandle);
			errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
			clLogError("DBA", "DBO", "Cannot open SQLite DB file [%s].",dbName);
			CL_FUNC_EXIT();
			return(errorCode);    
		}

		fclose(fp);

        rc = sqlite3_open(dbName, &(pSQLiteHandle->pDatabase));

        if(SQLITE_OK != rc) {
            clLogError("DBA", "DBO", "Failed to open the database");
            clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                    sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
            sqlite3_close(pSQLiteHandle->pDatabase);
            clHeapFree(pSQLiteHandle);
            errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
            CL_FUNC_EXIT();
            return(errorCode);
        }               
    }
    else if(dbFlag == CL_DB_APPEND)
    {
        if ((fp = fopen(dbName, "r")) == NULL)
        {
            /* Database doesn't exist. Create it.. */
            rc = cdbSQLiteDBCreate(dbName, pSQLiteHandle);
            if (rc != CL_OK)
            {
                sqlite3_close(pSQLiteHandle->pDatabase);
                clHeapFree(pSQLiteHandle);
                errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
                clLogError("DBA", "DBO", "SQLite DB Create failed. rc [0x%x]", rc);
                CL_FUNC_EXIT();
                return errorCode;
            }
        }
        else
        {
             fclose(fp);

             rc = sqlite3_open(dbName, &(pSQLiteHandle->pDatabase));

             if(SQLITE_OK != rc) {
                clLogError("DBA", "DBO", "Error in opening the DB in append mode");
                clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                        sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
                sqlite3_close(pSQLiteHandle->pDatabase);
                clHeapFree(pSQLiteHandle);
                errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
                CL_FUNC_EXIT();
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
        clLogError("DBA", "DBO", "Error in pragma prepare");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]",
                   sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		clHeapFree(pSQLiteHandle);
        errorCode= CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        CL_FUNC_EXIT();
        return errorCode;
    }

    rc = sqlite3_step(stmt);      
    
    if(SQLITE_DONE != rc)
    { 
        sqlite3_finalize(stmt); 
        clLogError("DBA", "DBO", "Failed to execute the pragma SQL statement");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        errorCode= CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
		sqlite3_close(pSQLiteHandle->pDatabase);
		clHeapFree(pSQLiteHandle);
        CL_FUNC_EXIT();
        return(errorCode);               
    }

    rc = sqlite3_finalize(stmt); 

    if(SQLITE_OK != rc)
    {
        clLogError("DBA", "DBO", "Failed to finalize the SQL statement.");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		clHeapFree(pSQLiteHandle);
        errorCode= CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        CL_FUNC_EXIT();
        return(errorCode);
    }
    
    /* Prepare the statements */
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "insert into ObjectRepository values(?, ?)", -1, &(pSQLiteHandle->stmt[0]), 0);

    if (rc != SQLITE_OK)
    {
        clLogError("DBA", "DBO", "Error in prepare statement in record add");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		clHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        CL_FUNC_EXIT();
        return(errorCode);       
    }

    /* INSERT or REPLACE */
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "replace into ObjectRepository(Key, Data) values(?, ?)", -1, 
		&(pSQLiteHandle->stmt[1]), 0);

    if (rc != SQLITE_OK)
    {
        clLogError("DBA", "DBO", "Failed to prepare the SQL statement.");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		clHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return(errorCode);       
    }

    /* DELETE */
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "delete from ObjectRepository where Key=?", -1, &(pSQLiteHandle->stmt[2]), 0);

    if (rc != SQLITE_OK)
    {
        clLogError("DBA", "DBO", "Failed to prepare the SQL statement.");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		clHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return(errorCode);       
    }

    /* RECORD GET */
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "select * from ObjectRepository where Key=?", -1, &(pSQLiteHandle->stmt[3]), 0);

    if (rc != SQLITE_OK)
    {
        clLogError("DBA", "DBO", "Failed to prepare the SQL statement.");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		clHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return(errorCode);       
    }

    /* FIRST RECORD GET */
    rc = sqlite3_prepare(pSQLiteHandle->pDatabase, "select * from ObjectRepository limit 0,1", -1, &(pSQLiteHandle->stmt[4]), 0);

    if(rc != SQLITE_OK)
    {
        clLogError("DBA", "DBO", "Failed to prepare the SQL statement.");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		clHeapFree(pSQLiteHandle);
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
        clLogError("DBA", "DBO", "Failed to prepare the SQL statement.");
        clLogError("DBA", "DBO", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
		sqlite3_close(pSQLiteHandle->pDatabase);
		clHeapFree(pSQLiteHandle);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return (errorCode);
    }

    *pDBHandle = (ClDBHandleT) pSQLiteHandle;

    CL_FUNC_EXIT();
    return(CL_OK);
}
/*****************************************************************************/

static ClRcT
cdbSQLiteDBClose(ClDBHandleT dbHandle)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;
    CL_FUNC_ENTER();

    pSQLiteHandle = (SQLiteDBHandle_t *)dbHandle;  

    /* Finalize the prepared statements */
    /* ADD */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[0]);
    
    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        CL_FUNC_EXIT(); 
        clLogError("DBA", "ADD", "Failed to finalize the SQL statment.");
        clLogError("DBA", "ADD", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }

    /* REPLACE */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[1]);
    
    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError("DBA", "REP", "Failed to finalize the SQL statement.");
        clLogError("DBA", "REP", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }

    /* DELETE */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[2]);
    
    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError("DBA", "DEL", "Failed to finalize the SQL statement.");
        clLogError("DBA", "DEL", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }
    
    /* RECORD GET */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[3]);
    
    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError("DBA", "GET", "Failed to finalize the SQL statement.");
        clLogError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }

    /* FIRST RECORD GET */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[4]);

    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError("DBA", "GET", "Failed to finalize the SQL statement.");
        clLogError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }

    /* NEXT RECORD GET */
    rc = sqlite3_finalize(pSQLiteHandle->stmt[5]);

    if (rc != SQLITE_OK)
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError("DBA", "GET", "Failed to finalize the SQL statement");
        clLogError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        return(errorCode);       
    }

    /* DB Close */
    rc = sqlite3_close(pSQLiteHandle->pDatabase);

    if(rc == SQLITE_BUSY) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError("DBA", "DBC", "Failed to close the SQLite DB.");
        clLogError("DBA", "DBC", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        CL_FUNC_EXIT();
        return(errorCode);
    }

    clHeapFree(pSQLiteHandle);
    
    CL_FUNC_EXIT();
    return (CL_OK);    
}

/****************************************************************************/
static ClRcT
cdbSQLiteDBSync(ClDBHandleT dbHandle,
                ClUint32T flags)
{
    /*Auto.DB sync would be through DB_SYNC open flag for DBAL*/
    return (CL_OK);
}
/****************************************************************************/

static ClRcT
cdbSQLiteDBRecordAdd(ClDBHandleT      dbHandle,
                       ClDBKeyT         dbKey,
                       ClUint32T       keySize,
                       ClDBRecordT      dbRec,
                       ClUint32T       recSize)
{   
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;

    CL_FUNC_ENTER();
    pSQLiteHandle = (SQLiteDBHandle_t *)dbHandle;

    NULL_CHECK(dbKey);
    NULL_CHECK(dbRec);

    clLogTrace("DBA", "ADD", "Adding a record into the database");

    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[0], 1, (const void *)dbKey, keySize, SQLITE_STATIC);
    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[0], 2, (const void *)dbRec, recSize, SQLITE_STATIC);

retry:

    rc = sqlite3_step(pSQLiteHandle->stmt[0]);

    if (rc != SQLITE_DONE)
    {
        if (rc == SQLITE_BUSY)
        {
            clLogInfo("DBA", "ADD", "Couldn't acquire lock to update the database. retrying..");
            goto retry;
        }

        /* Reset the stmt object to get the specific error code */
        rc = sqlite3_reset(pSQLiteHandle->stmt[0]);

        if (rc == SQLITE_CONSTRAINT)
        {
            /* Duplicate record is getting inserted */
            errorCode = CL_DBAL_RC(CL_ERR_DUPLICATE);
            clLogTrace("DBA", "ADD", "Failed to execute the SQL statement. Duplicate record is getting inserted.");
        }
        else
        {
            errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
            clLogError("DBA", "ADD", "Failed to execute the SQL statement. Unable to insert the record.");
            clLogError("DBA", "ADD", "SQLite Error : %s. errorCode [%d]", 
                   sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        }

        CL_FUNC_EXIT();
        goto finalize;       
    }
    
    rc = sqlite3_reset(pSQLiteHandle->stmt[0]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[0]);

finalize:    

    CL_FUNC_EXIT();
    return (errorCode);
}


static ClRcT
cdbSQLiteDBRecordReplace(ClDBHandleT      dbHandle,
                       ClDBKeyT         dbKey,
                       ClUint32T       keySize,
                       ClDBRecordT      dbRec,
                       ClUint32T       recSize)
{   
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;

    CL_FUNC_ENTER();

    pSQLiteHandle = (SQLiteDBHandle_t *)dbHandle;

    NULL_CHECK(dbKey);
    NULL_CHECK(dbRec);

    clLogTrace("DBA", "REP", "Replacing a record from the database");

    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[1], 1, (const void *)dbKey, keySize, SQLITE_STATIC);
    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[1], 2, (const void *)dbRec, recSize, SQLITE_STATIC);

retry:
    rc = sqlite3_step(pSQLiteHandle->stmt[1]);
	
    if (rc != SQLITE_DONE)
    {
        if (rc == SQLITE_BUSY)
        {
            clLogInfo("DBA", "REP", "Couldn't get lock to update the database. retrying..");
            goto retry;
        }

        /* Reset the stmt object to get the specific error code */
        rc = sqlite3_reset(pSQLiteHandle->stmt[1]);

        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError("DBA", "REP", "Failed to execute the SQL statement to replace a record. rc [%d]", rc);
        clLogError("DBA", "REP", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        goto finalize;
    }

    rc = sqlite3_reset(pSQLiteHandle->stmt[1]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[1]);
finalize:    

    CL_FUNC_EXIT();
    return (errorCode);
}


static ClRcT
cdbSQLiteDBRecordDelete(ClDBHandleT      dbHandle,
                    ClDBKeyT         dbKey,
                    ClUint32T       keySize)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;

    CL_FUNC_ENTER();
    pSQLiteHandle = (SQLiteDBHandle_t *)dbHandle;

    NULL_CHECK(dbKey);

    clLogTrace("DBA", "DEL", "Removing a record from the database");

    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[2], 1, (const void *)dbKey, keySize, SQLITE_STATIC);
    
retry:
    rc = sqlite3_step(pSQLiteHandle->stmt[2]);

    if (rc != SQLITE_DONE)
    {
        if (rc == SQLITE_BUSY)
        {
            clLogInfo("DBA", "DEL", "Couldn't get lock to update the database. retrying..");
            goto retry;
        }

        rc = sqlite3_reset(pSQLiteHandle->stmt[2]);

        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError("DBA", "DEL", "Failed to execute the SQL statement to delete a record.");
        clLogError("DBA", "DEL", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        goto finalize;
    }

    if (sqlite3_changes(pSQLiteHandle->pDatabase) == 0)
    {    
        errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
        clLogTrace("DBA", "DEL", "No record exists in the database");
        sqlite3_reset(pSQLiteHandle->stmt[2]);
        cl_clear_bindings(pSQLiteHandle->stmt[2]);
        goto finalize;
    }    

    rc = sqlite3_reset(pSQLiteHandle->stmt[2]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[2]);

finalize:    

    CL_FUNC_EXIT();
    return (errorCode);    
}

static ClRcT  
cdbSQLiteDBRecordGet(ClDBHandleT      dbHandle,
                 ClDBKeyT         dbKey,
                 ClUint32T       keySize,
                 ClDBRecordT*     pDBRec,
                 ClUint32T*      pRecSize)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;

    CL_FUNC_ENTER();
    
    NULL_CHECK(dbKey);
    NULL_CHECK(pDBRec);
    NULL_CHECK(pRecSize);

    pSQLiteHandle = (SQLiteDBHandle_t *)dbHandle;

    clLogTrace("DBA", "GET", "Retrieving a record from the database");

    rc = sqlite3_bind_blob(pSQLiteHandle->stmt[3], 1, (const void *) dbKey, keySize, SQLITE_STATIC);

retry:

    rc = sqlite3_step(pSQLiteHandle->stmt[3]);    

    if (rc != SQLITE_ROW)
    {
        if (rc == SQLITE_BUSY)
        {
            clLogTrace("DBA", "GET", "Couldn't get lock to update the database. retrying..");
            goto retry;
        }

        if (rc == SQLITE_DONE)
        {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            clLogTrace("DBA", "GET", "No record found in the database");
            rc = sqlite3_reset(pSQLiteHandle->stmt[3]);
            rc = cl_clear_bindings(pSQLiteHandle->stmt[3]);
        }
        else
        {
            rc = sqlite3_reset(pSQLiteHandle->stmt[3]);
            errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
            clLogError("DBA", "GET", "Failed to execute the SQL statement to retrieve a record.");
            clLogError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                    sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        }

        goto finalize;;       
    }

    *pRecSize = (ClUint32T) sqlite3_column_bytes(pSQLiteHandle->stmt[3], 1);
    
    *pDBRec = (ClDBRecordT) clHeapAllocate(*pRecSize);
    
    if(NULL == *pDBRec) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogError("DBA", "GET", "Failed to allocate memory.");
        CL_FUNC_EXIT();
        goto finalize;
    }

    memcpy(*pDBRec, sqlite3_column_blob(pSQLiteHandle->stmt[3], 1), *pRecSize);

    rc = sqlite3_reset(pSQLiteHandle->stmt[3]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[3]);

finalize:

    CL_FUNC_EXIT();
    return (errorCode);
}

static ClRcT
cdbSQLiteDBFirstRecordGet(ClDBHandleT      dbHandle,
                      ClDBKeyT*        pDBKey,
                      ClUint32T*      pKeySize,
                      ClDBRecordT*     pDBRec,
                      ClUint32T*      pRecSize)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;
 
    CL_FUNC_ENTER();
   
    NULL_CHECK(pDBKey);
    NULL_CHECK(pKeySize);

    NULL_CHECK(pDBRec);
    NULL_CHECK(pRecSize);

    pSQLiteHandle = (SQLiteDBHandle_t *)dbHandle;   
    
    clLogTrace("DBA", "GET", "Retrieving the first record from the database");

retry:    
    rc = sqlite3_step(pSQLiteHandle->stmt[4]);

    if((rc != SQLITE_ROW))
    { 
        if (rc == SQLITE_BUSY)
        {
            clLogInfo("DBA", "GET", "Couldn't get the lock to retrieve the value. retrying..");
            goto retry;
        }

        if (rc == SQLITE_DONE)
        {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            clLogTrace("DBA", "GET", "No records found in the database.");
            rc = sqlite3_reset(pSQLiteHandle->stmt[4]);
            rc = cl_clear_bindings(pSQLiteHandle->stmt[4]);
            goto finalize;
        }
             
        rc = sqlite3_reset(pSQLiteHandle->stmt[4]);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError("DBA", "GET", "Failed to retrieve the First Record.");
        clLogError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        goto finalize;       
    }

    *pKeySize = (ClUint32T) sqlite3_column_bytes(pSQLiteHandle->stmt[4], 0);
    *pRecSize = (ClUint32T) sqlite3_column_bytes(pSQLiteHandle->stmt[4], 1);

    *pDBKey = (ClDBKeyT) clHeapAllocate(*pKeySize);

    if(NULL == *pDBKey) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogError("DBA", "GET", "Failed to allocate memory");
        CL_FUNC_EXIT();
        goto finalize;
    }

    memcpy(*pDBKey, sqlite3_column_blob(pSQLiteHandle->stmt[4], 0), *pKeySize);
   
    *pDBRec = (ClDBRecordT) clHeapAllocate(*pRecSize);

    if(NULL == *pDBRec) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogError("DBA", "GET", "Failed to allocate memory");
        clHeapFree(*pDBKey);
        CL_FUNC_EXIT();
        goto finalize;
    }

    memcpy(*pDBRec, sqlite3_column_blob(pSQLiteHandle->stmt[4], 1), *pRecSize); 

    rc = sqlite3_reset(pSQLiteHandle->stmt[4]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[4]);

finalize:    
    
    CL_FUNC_EXIT();
    return (errorCode);
}

static ClRcT
cdbSQLiteDBNextRecordGet(ClDBHandleT      dbHandle,
                     ClDBKeyT         currentKey,
                     ClUint32T       currentKeySize,
                     ClDBKeyT*        pDBNextKey,
                     ClUint32T*      pNextKeySize,
                     ClDBRecordT*     pDBNextRec,
                     ClUint32T*      pNextRecSize)
{
    ClRcT errorCode = CL_OK;
    SQLiteDBHandle_t* pSQLiteHandle = NULL;
    ClUint32T rc = 0;

    CL_FUNC_ENTER();

    NULL_CHECK(currentKey);
    NULL_CHECK(pDBNextKey);
    NULL_CHECK(pNextKeySize);

    NULL_CHECK(pDBNextRec);
    NULL_CHECK(pNextRecSize);

    pSQLiteHandle = (SQLiteDBHandle_t *)dbHandle;

    clLogTrace("DBA", "GET", "Retrieving the next record from the database");

    sqlite3_bind_blob(pSQLiteHandle->stmt[5], 1, (const void *) currentKey, currentKeySize, SQLITE_STATIC);
    
retry1:    
    rc = sqlite3_step(pSQLiteHandle->stmt[5]);

    if (rc != SQLITE_ROW)
    { 
        if (rc == SQLITE_BUSY)
        {
            clLogTrace("DBA", "GET", "Couldn't get the lock the retrieve the value. retrying..");
            goto retry1;
        }

        if (rc == SQLITE_DONE)
        {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            clLogInfo("DBA", "GET", "Current record does not exist in the database.");
            rc = sqlite3_reset(pSQLiteHandle->stmt[5]);
            rc = cl_clear_bindings(pSQLiteHandle->stmt[5]);
            goto finalize;
        }

        rc = sqlite3_reset(pSQLiteHandle->stmt[5]);
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError("DBA", "GET", "Failed to retrieve the record with the key value specified.");
        clLogError("DBA", "GET", "SQLite Error : %s. errorCode [%d]", 
                sqlite3_errmsg(pSQLiteHandle->pDatabase), sqlite3_errcode(pSQLiteHandle->pDatabase));
        goto finalize;       
    }

    *pNextKeySize = (ClUint32T) sqlite3_column_bytes(pSQLiteHandle->stmt[5], 0);
    *pNextRecSize = (ClUint32T) sqlite3_column_bytes(pSQLiteHandle->stmt[5], 1);

    *pDBNextKey = (ClDBKeyT) clHeapAllocate(*pNextKeySize);

    if(NULL == *pDBNextKey) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogError("DBA", "GET", "Failed to allocate memory");
        CL_FUNC_EXIT();
        goto finalize;
    }

    memcpy(*pDBNextKey, sqlite3_column_blob(pSQLiteHandle->stmt[5], 0), *pNextKeySize);
   
    *pDBNextRec = (ClDBRecordT) clHeapAllocate(*pNextRecSize);

    if(NULL == *pDBNextRec) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogError("DBA", "GET", "Failed to allocate memory");
        clHeapFree(*pDBNextKey);
        CL_FUNC_EXIT();
        goto finalize;
    }

    memcpy(*pDBNextRec, sqlite3_column_blob(pSQLiteHandle->stmt[5], 1), *pNextRecSize); 

    rc = sqlite3_reset(pSQLiteHandle->stmt[5]);
    rc = cl_clear_bindings(pSQLiteHandle->stmt[5]);

finalize:

    CL_FUNC_EXIT();
    return (errorCode);
}

static ClRcT  
cdbSQLiteDBTxnOpen(ClDBFileT    dbFile,
               ClDBNameT    dbName, 
               ClDBFlagT    dbFlag,
               ClUint32T    maxKeySize,
               ClUint32T    maxRecordSize,
               ClDBHandleT* pDBHandle)
{
    ClRcT errorCode = CL_OK;

    CL_FUNC_ENTER();

    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    clLogError("DBA", "TXN", "SQLite Transaction is not supported");

    CL_FUNC_EXIT();
    return (errorCode);
}

static ClRcT
cdbSQLiteDBTransactionBegin(ClDBHandleT  dbHandle)
{
    ClRcT errorCode = CL_OK;

    CL_FUNC_ENTER();

    /* Transactions are not supported currently */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    clLogError("DBA", "TXN", "SQLite Transaction is not supported");
    CL_FUNC_EXIT();

    return (errorCode);
}

static ClRcT
cdbSQLiteDBTransactionCommit(ClDBHandleT  dbHandle)
{
    ClRcT errorCode = CL_OK;

    CL_FUNC_ENTER();

    /* Transactions are not supported currently */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    clLogError("DBA", "TXN", "SQLite Transaction is not supported");
    CL_FUNC_EXIT();

    return (errorCode);
}

static ClRcT
cdbSQLiteDBTransactionAbort(ClDBHandleT  dbHandle)
{
    ClRcT errorCode = CL_OK;

    CL_FUNC_ENTER();

    /* Transactions are not supported currently */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    clLogError("DBA", "TXN", "SQLite Transaction is not supported");
    CL_FUNC_EXIT();

    return (errorCode);
}

/*****************************************************************************/
static ClRcT
cdbSQLiteDBRecordFree(ClDBHandleT  dbHandle,
                        ClDBRecordT  dbRec)
{
    ClRcT errorCode = CL_OK;
    
    CL_FUNC_ENTER();
    NULL_CHECK(dbRec);
    clHeapFree(dbRec);
    CL_FUNC_EXIT();
    return (CL_OK);
}

/*****************************************************************************/
static ClRcT cdbSQLiteDBKeyFree(ClDBHandleT  dbHandle, ClDBKeyT     dbKey)
{
    ClRcT errorCode = CL_OK;

    CL_FUNC_ENTER();
    NULL_CHECK(dbKey);
    clHeapFree(dbKey);
    CL_FUNC_EXIT();
    return (CL_OK);
}

/****************************************************************************/
ClRcT clDbalEngineFinalize()
{
    return CL_OK;
}

/****************************************************************************/
ClRcT clDbalInterface(ClDbalFunctionPtrsT  *funcDbPtr)
{
    ClDbalConfigT* pDbalConfiguration = NULL;
    ClRcT          rc = CL_OK;
    int            sqLiteSoNum;
    
    clLogTrace("DBA", "INI", "SQLite version : %s", SQLITE_VERSION);
    sqLiteSoNum = sqlite3_libversion_number();
    
    if (sqLiteSoNum != SQLITE_VERSION_NUMBER)
    {        
        clLogWarning("DBA", "INI", "SQLite was compiled with version [%d], but dynamically loaded library is different: version [%d].  You may have 2 versions of sqlite installed in different directories (for example, /usr/lib, /usr/local/lib), or have different versions installed in the build machine vs. this machine.  This issue may cause runtime instability.", 
                  SQLITE_VERSION_NUMBER,sqLiteSoNum);
    }
    
    /* Check for the minimum sqlite version supported */
    if (!(sqLiteSoNum >= 3003013))
    {
        
        clLogError("DBA", "INI", "SQLite version [%d] found in the system is unsupported. "
                "Please install the SQLite version >= 3.3.13.", sqLiteSoNum);
        return CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
    }

    pDbalConfiguration = (ClDbalConfigT*)clHeapAllocate(sizeof(ClDbalConfigT));
    if ( NULL == pDbalConfiguration )
    {
        clLogError("DBA", "INI", "Failed to allocate memory.");
        return CL_DBAL_RC(CL_ERR_NO_MEMORY);
    }

    memset(pDbalConfiguration, '\0', sizeof(ClDbalConfigT));  

    strcpy((ClCharT*)pDbalConfiguration->Database.sqliteConfig.enginePath, CL_DBAL_SQLITE_DB_PATH);
    if ((rc = clDbalConfigInitialize((void *)pDbalConfiguration)) != CL_OK )
    {
        clLogError("DBA", "INI", "Failed to initialize DBAL Config. rc [0x%x]", rc);
        clHeapFree(pDbalConfiguration);
        return rc;
    }

    funcDbPtr->fpCdbOpen              = cdbSQLiteDBOpen;
    funcDbPtr->fpCdbClose             = cdbSQLiteDBClose;
    funcDbPtr->fpCdbSync              = cdbSQLiteDBSync;
    funcDbPtr->fpCdbRecordAdd         = cdbSQLiteDBRecordAdd;
    funcDbPtr->fpCdbRecordReplace     = cdbSQLiteDBRecordReplace;
    funcDbPtr->fpCdbRecordGet         = cdbSQLiteDBRecordGet;
    funcDbPtr->fpCdbRecordDelete      = cdbSQLiteDBRecordDelete;
    funcDbPtr->fpCdbFirstRecordGet    = cdbSQLiteDBFirstRecordGet;
    funcDbPtr->fpCdbNextRecordGet     = cdbSQLiteDBNextRecordGet;
    funcDbPtr->fpCdbTxnOpen           = cdbSQLiteDBTxnOpen;
    funcDbPtr->fpCdbTransactionBegin  = cdbSQLiteDBTransactionBegin;
    funcDbPtr->fpCdbTransactionCommit = cdbSQLiteDBTransactionCommit;
    funcDbPtr->fpCdbTransactionAbort  = cdbSQLiteDBTransactionAbort;
    funcDbPtr->fpCdbRecordFree        = cdbSQLiteDBRecordFree;
    funcDbPtr->fpCdbKeyFree           = cdbSQLiteDBKeyFree;
    funcDbPtr->fpCdbFinalize          = clDbalEngineFinalize;
    funcDbPtr->validDatabase          = DATABASE_ID;

    clHeapFree(pDbalConfiguration);

    return rc; 
}

#ifdef __cplusplus
 }
#endif

