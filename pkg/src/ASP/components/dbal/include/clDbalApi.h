/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : dbal
 * File        : clDbalApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains essential definitions for the Database Abstraction     
 * Layer.                                                                    
 *****************************************************************************/


/********************************************************************************/
/******************************** DBAL  APIs ************************************/
/********************************************************************************/
/*                                                                              */
/*    clDbalOpen		                                                        */
/*    clDbalClose		                                                        */
/*    clDbalRecordInsert	                                                    */
/*    clDbalRecordReplace	                                                    */
/*    clDbalRecordGet	                                                        */
/*    clDbalRecordDelete	                                                    */
/*    clDbalFirstRecordGet	                                                    */
/*    clDbalNextRecordGet	                                                    */
/*    clDbalTransactionBegin	                                                */
/*    clDbalTransactionCommit                                                   */
/*    clDbalTransactionAbort	                                                */
/*    clDbalRecordFree                                                          */
/*    clDbalKeyFree                                                             */
/*                                                                              */
/********************************************************************************/

/**
 *  \file
 *  \brief Header file of DBAL related Definitions and APIs
 *  \ingroup dbal_apis
 */

/**
 *  \addtogroup dbal_apis
 *  \{
 */

#ifndef _CL_DBAL_API_H_
#define _CL_DBAL_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clDbalErrors.h"    

/*******************************************************************************************/

/**
 *  Definition of Database type.
 */
typedef enum ClDBType
{
/**
 * Hash table.
 */
 	CL_DB_TYPE_HASH = 0,		
/**
 * B-tree.
 */
	CL_DB_TYPE_BTREE,		

    CL_DB_MAX_TYPE

}ClDBTypeT;

typedef struct ClDbalBerkeleyConfigurationT {

/**
 * Path where the database files are to be stored.
 */ 
ClUint8T* engineEnvironmentPath;   

}ClDbalBerkeleyConfigurationT;

typedef struct ClDbalSQLiteConfiguration {

    /* Path where the database files has to be stored. */
    ClUint8T* enginePath;
} ClDbalSQLiteConfigurationT;

typedef struct ClDbalConfigurationT {
    ClUint32T engineType;
    union database {
        ClDbalBerkeleyConfigurationT berkeleyConfig;
        ClDbalSQLiteConfigurationT sqliteConfig;
    }Database;
}ClDbalConfigurationT;

/**
 * Engine Handle.                
 */       
typedef ClPtrT   ClDBEngineT; 


/*******************************************************************************************/

/**
 *  Definition of database open flag type. Database can be opened in following
 *  three modes : Create, Open, Append. 
 */

typedef ClUint8T ClDBFlagT;

/**
 * This DB flag is used for creating a DB. If the DB already exists then it 
 * is removed and a new DB is be created.
 */
#define CL_DB_CREAT     0X1

/**
 * This DB flag is used for opening an existing DB. If the DB doesn't exist already
 * then an error is returned.
 */
#define CL_DB_OPEN      0x2 

/**
 * This DB flag is used for opening a DB in APPEND mode. Data inserted to the DB 
 * would be appended to the existing DB (if there is any).
 */
#define CL_DB_APPEND    0x4

/**
 *  One of the three DB open modes (::CL_DB_CREAT, ::CL_DB_OPEN, ::CL_DB_APPEND) 
 *  may be OR-ed with DB sync flag.
 *  When specified, the underlying DB engine will automatically synchronize 
 *  all database operations to the disk.
 *  Note : Currently this flag is supported only with GDBM and SQLite and it has no effect 
 *  in case of other DB types. Please use clDbalSync() with other DBs to synchronize 
 *  DB operations to the disk.
 */
#define CL_DB_SYNC      0x8
   
#define CL_DB_MAX_FLAG  0x10

/**
 * Type of the name of database.
 */   
typedef const char*  ClDBNameT;        

/**
 * Type of the DB File name.
 */
typedef const char*  ClDBFileT;  

/**
 * Database Handle. The handle uniquely identifies the created/opened
 * database instance.
 */
typedef ClPtrT      ClDBHandleT;      

/**
 * Type of DB Record Handle (handle to the record which is to be inserted in DB 
 * or to be fetched from the DB).
 */
typedef ClUint8T*   ClDBRecordHandleT; 

/**
 * Deprecated DB Record Handle type. ClDBRecordHandleT should be used in place of
 * ClDBRecordT.             
 */
typedef ClDBRecordHandleT ClDBRecordT;
    
/**
 * Type of DB Key Handle (handle to the key of the record which is 
 * to be inserted in DB or to be fetched from the DB).
 */
typedef ClUint8T*   ClDBKeyHandleT;       

/**
 * Deprecated DB Key Handle type. ClDBKeyHandleT should be used in place of
 * ClDBKeyT.             
 */
typedef ClDBKeyHandleT ClDBKeyT;
 

/****************************************************************************
 * Database Maintenance APIs
 * These APIs are responsible for housekeeping of the repository.  
 * Their functionalities include creating, opening, and closing a database and
 * performing insert, get, delete, replace, etc operations on the databse.
 ***************************************************************************/


/*****************************************************************************/

/** 
 ***************************************
 *  \brief Opens a database instance.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param  dbFile This parameter specifies the file name along with its path,
 *  where the database is to be created. If this parameter is NULL and the 
 *  database is Berkeley Database, then an in-memory database is created.
 *  This parameter is ignored if the database is other than Berkeley Database.
 *
 *  \param  dbName This parameter specifies the database name along with its
 *  path, where the database is to be created. The records are stored in the 
 *  database of this name. This parameter is used in case of both Berkeley
 *  and GDBM Database.
 *
 *  \param dbFlag This flag accepts following three values: 
 *  \arg If \e CL_DB_CREAT is specified and the database already exists, 
 *  then the existing database is deleted and a fresh one is created. 
 *  If the database does not exist, then a new one is created. 
 *  \arg If \e CL_DB_OPEN is specified then existing database is opened.
 *  If the database does not exist, then an error is returned.
 *  \arg If \e CL_DB_APPEND is specified and the database already exists, 
 *  then the existing database is opened in append mode.If the database does
 *  not exist, then a fresh one is created.
 *
 *  \param  maxKeySize Maximum size of the key (in bytes) to be stored in the database.
 *  This parameter is ignored in case of Berkeley and GDBM databases.
 *
 *  \param maxRecordSize Maximum size of the record (in bytes) to be stored in the database.
 *  This parameter is ignored in case of Berkeley and GDBM databases.
 *
 *  \param pDBHandle (out)Pointer to variable of type \e ClDBHandleT in which
 *  the newly created database handle is returned. This handle should be used
 *  for all the subsequent operations on this database instance.
 *   
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid parameters. 
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.  
 *  \retval CL_DBAL_ERR_DB_ERROR If the underlying database fails.  
 *
 *  \par Description:
 *  This API opens a database instance on the specified Database. Once this
 *  API is executed successfully the DB instance is ready for use. This API 
 *  must be executed before performing any DB operation (insert, delete,
 *  replace, close, etc).
 *  DB handle returned by this API should be used for further DB
 *  operations on this database instance.
 *  
 *  \sa clDbalClose()
 *
 */
ClRcT  
clDbalOpen(CL_IN  ClDBFileT     dbFile,
           CL_IN  ClDBNameT     dbName, 
           CL_IN  ClDBFlagT     dbFlag,
           CL_IN  ClUint32T     maxKeySize,
           CL_IN  ClUint32T     maxRecordSize,
           CL_OUT ClDBHandleT*  pDBHandle);

/*****************************************************************************/

/**
 ****************************************** 
 *  \brief Closes a database instance.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle of the Database instance to be closed. 
 *  This is the handle that was returned to the user when 
 *  clDbalOpen() call was successfully executed.
 *  All Db operations if performed using this DB handle after successful
 *  clDbalClose() are invalid and returned with an error.
 *
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle. 
 * 
 *  \par Description:
 *  This function closes the association, represented by the dbHandle parameter, 
 *  between the invoking process and the DB instance. In case of Berkeley Database,
 *  if the database being closed is an in-memory database, then all data is lost
 *  once the database is closed. 
 *  The process must have invoked clDbalOpen() before it invokes this function. 
 *  A process must invoke this function once for each handle acquired by invoking  
 *  clDbalOpen(). If the clDbalClose() function returns successfully, the
 *  clDbalClose() function releases all resources acquired when clDbalOpen()
 *  was called.
 *
 *  \sa clDbalOpen()
 *
 */
ClRcT
clDbalClose(CL_IN ClDBHandleT dbHandle);
/*****************************************************************************/


/**
 ****************************************** 
 *  \brief Flushes (synchronizes) the DB modifications stored in the in-memory cache to disk.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle of the Database instance to be synchronized. 
 *  This is the handle that was returned to the user when 
 *  clDbalOpen() call was successfully executed.
 *
 *  \param flags The flags parameter is currently unused, and must be set to 0.
 *
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle. 
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid parameters (flags). 
 *  \retval CL_DBAL_ERR_DB_ERROR If the underlying database fails.  
 * 
 *  \par Description:
 *  This function is used to flush the DB modifications stored in the in-memory 
 *  cache to disk for the DB represented by the dbHandle parameter. 
 *  When you perform a database modification (insert, delete, replace etc), 
 *  your modification may be made in the in-memory cache. 
 *  This means that your data modifications are not necessarily 
 *  flushed to disk, and so your data may not appear in the database after an 
 *  application restart (if you have not called clDbalSync() after DB modifications).
 *  Note that as a normal part of closing a database (using clDbalClose()), its cache is written to disk. 
 *  However, in the event of an application or system failure, there is no guarantee that 
 *  your databases will close cleanly. In this event, it is possible for you to lose data. 
 *  Therefore, if you want your data to be durable across system failures,
 *  i.e. you want some guarantee that your database modifications are persistent, 
 *  then you should periodically call clDbalSync().  Syncs cause any dirty entries in the
 *  in-memory cache and the operating system's file cache to be written to disk. 
 *  Note that clDbalSync() is expensive and you should use it sparingly.
 *
 *  \sa clDbalClose()
 *
 */
ClRcT
clDbalSync(CL_IN ClDBHandleT dbHandle, ClUint32T flags);
/*****************************************************************************/



/**
 * Record Operation APIs
 */

/**
 ***************************************** 
 *  \brief Adds a record into a database instance.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *  ClDbal
 *
 *  \param dbHandle Handle to the Database into which the record is being added. 
 *  This is the handle that was returned to the user when 
 *  clDbalOpen() call was successfully executed.
 *  \param dbKey Record's key handle. In case of GDBM the key must be a string.
 *  \param keySize Size of the key.
 *  \param dbRec Record handle. In case of GDBM, the record must be a string.
 *  \param recSize Size of the record.
 *   
 *  \retval CL_OK  The API executed successfully. 
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.  
 *  \retval CL_ERR_DUPLICATE If an already existing key is added.  
 *  \retval CL_DBAL_ERR_DB_ERROR If the underlying database fails.  
 *
 *  \par Description:
 *  This API adds a record into a Database known by its handle. If the specified
 *  key already exists in the database, then an error is returned.
 *
 *  \sa clDbalOpen(), 
 *      clDbalClose(), 
 *      clDbalRecordGet(),
 *      clDbalRecordReplace(), 
 *      clDbalRecordDelete()
 *
 */
ClRcT
clDbalRecordInsert(CL_IN  ClDBHandleT       dbHandle,
                   CL_IN  ClDBKeyHandleT    dbKey,
                   CL_IN  ClUint32T         keySize,
                   CL_IN  ClDBRecordHandleT dbRec,
                   CL_IN  ClUint32T         recSize);
/*****************************************************************************/

/**
 ***************************************** 
 *  \brief Replaces a record in a database instance.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle to the Database into which the record is being added.
 *  This is the handle that was returned to the user when 
 *  clDbalOpen() call was successfully executed.
 *  \param dbKey Record's key handle.
 *  \param keySize Size of the key.
 *  \param dbRec Record handle.
 *  \param recSize Size of the record.
 *
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.  
 *  \retval CL_DBAL_ERR_DB_ERROR If the underlying database fails.  
 *
 *  \par Description: 
 *  This API replaces a record in a Database known by its handle. If the specified
 *  key already exists in the database, then the record associated with it is
 *  replaced with the current record. If the key does not exist, then the record
 *  is added.
 *
 *  \sa clDbalOpen(), clDbalClose(), clDbalRecordGet(),
 *      clDbalRecordInsert(), clDbalRecordDelete()
 *
 */
ClRcT
clDbalRecordReplace(CL_IN ClDBHandleT       dbHandle,
                    CL_IN ClDBKeyHandleT    dbKey,
                    CL_IN ClUint32T         keySize,
                    CL_IN ClDBRecordHandleT dbRec,
                    CL_IN ClUint32T         recSize);
/*****************************************************************************/

/**
 ***************************************** 
 * \brief Retrieves a record from a database instance.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *  
 *  \param dbHandle Handle to the Database from which the record is being retrieved.
 *  This is the handle that was returned to the user when 
 *  clDbalOpen() call was successfully executed.
 *  \param dbKey Key handle, whose associated record is being retrieved.
 *  \param keySize Size of the key.
 *  \param pDBRec (out) Pointer to record handle, in which the record is being returned.
 *  Memory allocation is done by DBAL but you must free this memory using
 *  clDbalRecordFree() API.
 *  \param pRecSize (out) Pointer to variable in which size of the record is being returned.
 *
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.  
 *  \retval CL_DBAL_ERR_DB_ERROR If the underlying database fails.  
 *  \retval CL_ERR_NOT_EXIST If the key does not exist.  
 *
 *  \par Description:
 *  This API retrieves a record from the Database using the database handle and
 *  the record's key. If no record is found on the key of the specified record, an error
 *  is returned.  
 *
 *  \sa clDbalOpen(), clDbalClose(), clDbalRecordReplace(),
 *      clDbalRecordInsert(), clDbalRecordDelete()
 *
 */
ClRcT  
clDbalRecordGet(CL_IN  ClDBHandleT          dbHandle,
                CL_IN  ClDBKeyHandleT       dbKey,
                CL_IN  ClUint32T            keySize,
                CL_OUT ClDBRecordHandleT*   pDBRec,
                CL_OUT ClUint32T*           pRecSize);
/*****************************************************************************/

/**
 ***************************************** 
 *  \brief Deletes a record from a database instance.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle to the Database from which the record is being deleted.
 *  \param dbKey Key handle of the record.
 *  \param keySize Size of the key.
 * 
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_HANDLE on passing an invalid handle.  
 *  \retval CL_DBAL_ERR_DB_ERROR If the underlying database fails.  
 *  \retval CL_ERR_NOT_EXIST If the key does not exist.  
 *
 *  \par Description:
 *  This API is used to delete a record from a Database using its handle and record's 
 *  key.If the record is not found, the API returns an error.
 *
 *  \sa clDbalOpen(), clDbalClose(), clDbalRecordReplace(),
 *      clDbalRecordInsert(), clDbalRecordGet()
 *
 */
ClRcT
clDbalRecordDelete(CL_IN ClDBHandleT        dbHandle,
                   CL_IN ClDBKeyHandleT     dbKey,
                   CL_IN ClUint32T          keySize);
/*****************************************************************************/

/**
 ***************************************** 
 *  \brief Returns the first key and associated record from a database
 *  instance.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle to the Database from where the object is being deleted.
 *  This is the handle that was returned to the user when 
 *  clDbalOpen() call was successfully executed.
 *
 *  \param pDBKey (out) Pointer to key handle, in which the first record's key is returned.
 *  Memory allocation is done by DBAL but you must free this memory using
 *  clDbalKeyFree() API.
 *
 *  \param pKeySize (out) Pointer to variable in which size of the key is being returned.
 *
 *  \param pDBRec (out) Pointer to record handle in which the first record is returned.
 *  Memory allocation is done by DBAL but you must free this memory using
 *  clDbalRecordFree() API.
 *
 *  \param pRecSize (out) Pointer to variable in which size of the record is being returned.
 *
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.  
 *  \retval CL_DBAL_ERR_DB_ERROR If the underlying database fails.  
 *  \retval CL_ERR_NOT_EXIST If the first record does not exist.  
 *
 *  \par Description:
 *  This API is used to return the first key and associated record from a database.
 *  If no records are found in the specified database, an error is returned.
 *
 *  \sa clDbalOpen(), clDbalClose(), clDbalRecordReplace(),
 *      clDbalRecordInsert(), clDbalRecordGet(), clDbalRecordDelete(),
 *      clDbalNextRecordGet()  
 *
 */
ClRcT
clDbalFirstRecordGet(CL_IN  ClDBHandleT         dbHandle,
                     CL_OUT ClDBKeyHandleT*     pDBKey,
                     CL_OUT ClUint32T*          pKeySize,
                     CL_OUT ClDBRecordHandleT*  pDBRec,
                     CL_OUT ClUint32T*          pRecSize);
/*****************************************************************************/

/**
 ***************************************** 
 *  \brief Returns the next key and associated record from a database instance.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle to the Database from which the next record is being retrieved.
 *  This is the handle that was returned to the user when 
 *  clDbalOpen() call was successfully executed.
 *  \param currentKey Current record's key handle.
 *  \param currentKeySize Size of the current key.
 *
 *  \param pDBNextKey (out) Pointer to key handle, in which the next record's key is being returned.
 *  Memory allocation is done by DBAL but you must free this memory using
 *  clDbalKeyFree() API.
 *
 *  \param pNextKeySize (out) Pointer to variable in which size of the key is being returned.
 *
 *  \param pDBNextRec (out) Pointer to record handle, in which handle of the next record is returned.
 *  Memory allocation is done by DBAL but you must free this memory using
 *  clDbalRecordFree() API.
 *
 *  \param pNextRecSize (out) Pointer to variable in which size of the record is being returned.
 *                    
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.  
 *  \retval CL_DBAL_ERR_DB_ERROR If the underlying database fails.  
 *  \retval CL_ERR_NOT_EXIST If the key does not exist.
 *
 *  \par Description:
 *  This API is used to retrieve the next key and associated record from a Database using the current
 *  record's key. If no record is available, an error is returned.
 *
 *  \sa clDbalOpen(), clDbalClose(), clDbalRecordReplace(),
 *      clDbalRecordInsert(), clDbalRecordGet(), clDbalRecordDelete() 
 *
 */
ClRcT
clDbalNextRecordGet(CL_IN  ClDBHandleT          dbHandle,
                    CL_IN  ClDBKeyHandleT       currentKey,
                    CL_IN  ClUint32T            currentKeySize,
                    CL_OUT ClDBKeyHandleT*      pDBNextKey,
                    CL_OUT ClUint32T*           pNextKeySize,
                    CL_OUT ClDBRecordHandleT*   pDBNextRec,
                    CL_OUT ClUint32T*           pNextRecSize);
/*****************************************************************************/


/**
 * Transaction Related APIs
 */

/** 
 ***************************************
 *  \brief Opens a database instance with transaction support.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param  dbFile This parameter specifies the file name along with its path,
 *  where the database is to be created. If this parameter is NULL and the 
 *  database is Berkeley Database, then an in-memory database is created.
 *  This parameter is ignored if the database is other than Berkeley Database.
 *
 *  \param  dbName This parameter specifies the database name along with its
 *  path, where the database is to be created. The records are stored in the 
 *  database of this name. This parameter is used in case of both Berkeley
 *  and GDBM Database.
 *
 *  \param dbFlag This flag accepts following three values: 
 *  \arg If \e CL_DB_CREAT is specified and the database already exists, 
 *  then the existing database is deleted and a fresh one is created. 
 *  If the database does not exist, then a new one is created. 
 *  \arg If \e CL_DB_OPEN is specified then existing database is opened.
 *  If the database does not exist, then an error is returned.
 *  \arg If \e CL_DB_APPEND is specified and the database already exists, 
 *  then the existing database is opened in append mode.If the database does
 *  not exist, then a fresh one is created.
 *
 *  \param  maxKeySize Maximum size of the key (in bytes) to be stored in the database.
 *  This parameter is ignored in case of Berkeley and GDBM databases.
 *
 *  \param maxRecordSize Maximum size of the record (in bytes) to be stored in the database.
 *  This parameter is ignored in case of Berkeley and GDBM databases.
 *
 *  \param pDBHandle (out)Pointer to variable of type \e ClDBHandleT in which
 *  the newly created database handle is returned. This handle should be used
 *  for all the subsequent operations on this database instance.
 *   
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid parameters. 
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.  
 *  \retval CL_DBAL_ERR_DB_ERROR If the underlying database fails.  
 *  \retval CL_ERR_NOT_SUPPORTED If the underlying database does not support transactions. 
 *
 *  \par Description:
 *  This API opens a database instance on the specified Database and is
 *  transaction protected. Once this API is executed successfully the DB 
 *  instance is ready for use in the transaction environment. This API 
 *  must be executed before calling clDbalTransactionBegin() on the DB
 *  instance. DB handle returned by this API should be used for further
 *  operations on this database instance.
 *  
 *  \sa clDbalClose()
 *
 */
ClRcT  
clDbalTxnOpen(CL_IN  ClDBFileT     dbFile,
              CL_IN  ClDBNameT     dbName, 
              CL_IN  ClDBFlagT     dbFlag,
              CL_IN  ClUint32T     maxKeySize,
              CL_IN  ClUint32T     maxRecordSize,
              CL_OUT ClDBHandleT*  pDBHandle);

/**
 ***************************************** 
 *  \brief Begins the transaction on a database instance.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle to the Database in which the transaction is to be started.
 *  This is the handle that was returned to the user when 
 *  clDbalTxnOpen() call was successfully executed.
 *
 *  \note
 *  Transactions may only span threads if they do so serially, that is, each transaction 
 *  must be active in only a single thread of control at a given point of time. 
 *   
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.  
 *  \retval CL_ERR_NOT_SUPPORTED If the underlying database does not support transactions. 
 *
 *  \par Description:
 *   This API marks the beginning of a transaction. This API must be called before calling
 *   further DBAL transaction related APIs. This API returns \e transactionId as part of 
 *   the \e dbHandle, which will be passed for further operations.
 *
 *  \sa clDbalTransactionCommit(), clDbalTransactionAbort() 
 *
 */
ClRcT
clDbalTransactionBegin(CL_IN ClDBHandleT  dbHandle);
/*****************************************************************************/

/**
 ***************************************** 
 *  \brief Commits the transaction on a database instance.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle to the Database in which the transaction is being commited.
 *  This is the handle that was returned to the user when 
 *  clDbalTxnOpen() call was successfully executed.
 *   
 *  \retval CL_OK The API executed successfully.  
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.  
 *  \retval CL_DBAL_ERR_COMMIT_FAILED If the commit of underlying database fails.  
 *  \retval CL_ERR_NOT_SUPPORTED If the underlying database does not support transactions. 
 *
 *  \par Description:
 *  This API commits the recently started transaction. All operations performed after calling
 *  \e  clDbalTransactionBegin, will be committed in the database. 
 *
 *  \note Once you have committed a transaction, the transaction
 *  handle that is used for the transaction is no longer valid. To perform
 *  database activities under the control of a new transaction, you must
 *  update the DB handle using clDbalTransactionBegin().
 *
 *  \sa clDbalTransactionBegin(), clDbalTransactionAbort()
 *
 */
ClRcT
clDbalTransactionCommit(CL_IN ClDBHandleT  dbHandle);
/*****************************************************************************/

/**
 ***************************************** 
 *  \brief Aborts a transaction on a database instance.
 *
 *  \par Header File: 
 *   clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle to the Database in which the transaction is being aborted.
 *  This is the handle that was returned to the user when 
 *  clDbalTxnOpen() call was successfully executed.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.  
 *  \retval CL_DBAL_ERR_ABORT_FAILED If the abort of underlying database fails.  
 *  \retval CL_ERR_NOT_SUPPORTED If the underlying database does not support transactions.
 *
 *  \par Description:
 *  This API is used to abort the recently started transaction. This removes
 *  the transaction ID specified in the handle.
 *
 *  \note Once you have aborted a transaction, the transaction
 *  handle that is used for the transaction is no longer valid. To perform
 *  database activities under the control of a new transaction, you must
 *  update the DB handle using clDbalTransactionBegin().
 *
 *  \sa clDbalTransactionBegin(), clDbalTransactionCommit()
 *
 */
ClRcT
clDbalTransactionAbort(CL_IN ClDBHandleT  dbHandle);
/*****************************************************************************/

/**
 ***************************************** 
 *  \brief Frees the database record.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle to the Database.
 *  This is the handle that was returned to the user when 
 *  clDbalOpen() call was successfully executed.
 *  \param dbRec Record to be freed.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.  
 *
 *  \par Description:
 *  This API is used to free the record returned by clDbalRecordGet(),
 *  clDbalNextRecordGet(), clDbalFirstRecordGet().
 *  Since the record is created by clDbalRecordInsert(), it can not
 *  be freed by clHeapFree. So for proper cleanup of records, this API must 
 *  be called after every successful call of clDbalRecordGet(), 
 *  clDbalFirstRecordGet(), clDbalNextRecordGet().
 *
 *  \sa clDbalRecordGet(), clDbalFirstRecordGet(), clDbalNextRecordGet()
 *
 */
ClRcT
clDbalRecordFree(CL_IN ClDBHandleT          dbHandle,
                 CL_IN ClDBRecordHandleT    dbRec);
/*****************************************************************************/

/**
 ***************************************** 
 *  \brief Frees the database key.
 *
 *  \par Header File: 
 *  clDbalApi.h
 *
 *  \par Library Files:
 *   ClDbal
 *
 *  \param dbHandle Handle to the Database.
 *  This is the handle that was returned to the user when 
 *  clDbalOpen() call was successfully executed.
 *  \param dbKey Key to be freed.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.  
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.  
 *
 *  \par Description:
 *  This API is used to free the Key returned by \e clDbalFirstRecordGet(),
 *  clDbalNextRecordGet().
 *  Since the key is created by clDbalRecordInsert(), it can not
 *  be freed by clHeapFree. So for proper cleanup of keys, this API must 
 *  be called after every successful call of clDbalFirstRecordGet(),
 *  clDbalNextRecordGet(). 
 *
 *  \sa clDbalFirstRecordGet(), clDbalNextRecordGet() 
 *
 */
ClRcT
clDbalKeyFree(CL_IN ClDBHandleT     dbHandle,
              CL_IN ClDBKeyHandleT  dbKey);
/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _CL_DBAL_API_H_ */


/** \} */
