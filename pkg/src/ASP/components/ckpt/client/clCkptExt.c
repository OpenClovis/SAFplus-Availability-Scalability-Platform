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
 * ModuleName  : ckpt                                                          
 * File        : clCkptExt.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *   This file contains Checkpoint service APIs implementation
 *   The APIs are similar to SAF APIs
 *
 *
 *****************************************************************************/


/* System includes */
#include <string.h>   /* For memcmp */

/* Common includes */
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clCkptErrors.h>

/* Other ASP modules Includes */
#include <clDebugApi.h>
#include <clCpmApi.h>
#include <clCntApi.h>
#include <clDbalApi.h>
#include <clCksmApi.h>
#include <clLogApi.h>

/* Ckpt specific headers */
#include <clCkptApi.h>
#include <clCkptExtApi.h>
#include <clCkptClient.h>
#include <clCkptUtils.h> 
#include <clCkptExt.h>

#define  CKPT_DB_PATH  getenv("ASP_RUNDIR")

#define  CL_CKPT_LIB_NAME_FORM(ckptDB, appName)\
    do\
{\
    ClCharT  *path = CKPT_DB_PATH;      \
    if( NULL != path )\
    {                  \
        snprintf (ckptDB, sizeof(ckptDB), "%s/%d%s%s.db",path, clIocLocalAddressGet (),\
                CL_CKPT_DB_NAME,\
                appName);\
    }\
    else\
    {\
        snprintf (ckptDB, sizeof(ckptDB), "%d%s%s.db", clIocLocalAddressGet (),\
                CL_CKPT_DB_NAME,\
                appName);\
    }\
}\
    while(0)\

#define  CL_CKPT_APP_NAME_FORM(appDB, appName)  do {                    \
       ClCharT appBasePath[CL_MAX_NAME_LENGTH+1];                       \
       ClCharT *appBaseName = NULL;                                     \
       ClCharT  *path = NULL;                                           \
       if(! (path = strrchr(appName, '/') ) )                           \
       {                                                                \
           path = CKPT_DB_PATH;                                         \
           appBaseName = (ClCharT*)appName;                             \
       }                                                                \
       else                                                             \
       {                                                                \
           memset(appBasePath, 0, sizeof(appBasePath));                 \
           strncpy(appBasePath, appName, CL_MIN(path-appName, sizeof(appBasePath)-1)); \
           appBaseName = path + 1;                                      \
           path = appBasePath;                                          \
       }                                                                \
      if(!path)                                                         \
          path = ".";                                                   \
      snprintf (appDB, sizeof(appDB), "%s/%d%s.db",path, clIocLocalAddressGet (), \
                                 appBaseName);                          \
}while(0)

#define CKPT_LIST_GET_KEY(key, name) do {           \
    ClUint32T __len = (name)->length;               \
    ClCharT *__p = NULL ;                           \
    if(__len >= (ClUint32T)sizeof((name)->value))   \
        __len = sizeof((name)->value)-1;            \
    if(!(key))                                      \
        (key) = (name)->value;                      \
    if( (key) != (name)->value)                     \
        strncpy((key), (name)->value, __len);       \
    if( (__p = strrchr ((key), '/')))               \
        key = __p + 1;                              \
}while(0)

extern ClRcT clDbalLibInitialize(void);

/**============================================================**/
/** All static routines. Only to be used with in this file     **/
/**============================================================**/

/* Routine to pack checkpoint library information */
static  ClRcT clCkptPack (
            CkptClnCbT      *pCkptClnt, 
            ClCharT         *pBuffer);

/* Routine to unpack checkpoint lib information */
static  ClRcT clCkptUnpack (
            ClDBRecordT     *dataHdl, 
            ClUint32T       length, 
            CkptClnCbT      **ppCkptClnt);

/* Utility routine to allocate enough memory for ckpt pack */
static  ClRcT clCkptBufferAlloc (
            CkptClnCbT      *pCkptClnt, 
            ClCharT         **ppBuffer, 
            ClUint32T       *length);

/* Utility routine to free the buffer */
static  void  clCkptBufferFree (
            ClCharT         *pBuffer);

/* Key compare routine for data set container */
static ClInt32T ckptDsetCompare (
            ClCntKeyHandleT     key1,  
            ClCntKeyHandleT      key2);

/* Callback function for data set container */
static void ckptDataSetDeleteCallback (
            ClCntKeyHandleT     key,  
            ClCntDataHandleT     userData);    

static ClInt32T ckptCheckptCompare (
            ClCntKeyHandleT     key1,  
            ClCntKeyHandleT      key2);
static ClRcT ckptPackSizeGetCallback(
            ClCntKeyHandleT   userKey,
            ClCntDataHandleT  userData,
            ClCntArgHandleT   userArg,
            ClUint32T         dataLength);

/* Callback function for data set container */
static void ckptCheckptDeleteCallback (
            ClCntKeyHandleT     key,  
            ClCntDataHandleT     userData);    
ClInt32T ckptDsetCompare (
            ClCntKeyHandleT     key1,  
            ClCntKeyHandleT      key2)
{

  return  ((ClWordT)key1 - (ClWordT)key2);
}


/* Callback function for data set container */
void ckptDataSetDeleteCallback (
            ClCntKeyHandleT     key,  
            ClCntDataHandleT     userData)
{
  
    CkptDataSetT   *pDataInfo = NULL;
    pDataInfo = (CkptDataSetT *)userData;
    clHeapFree(pDataInfo);
    return;
}


ClInt32T ckptCheckptCompare (
            ClCntKeyHandleT     key1,  
            ClCntKeyHandleT      key2)
{
    ClCharT *pName1 = (ClCharT *)key1;
    ClCharT *pName2 = (ClCharT *)key2;
    return strcmp(pName1, pName2);
#if 0
    ClUint32T result;
    result = pName1->length - pName2->length;

    if (0 == result)
       result = memcmp (pName1->value, pName2->value,pName1->length);
  return result;
#endif
}

/* Callback function for data set container */
void ckptCheckptDeleteCallback (
            ClCntKeyHandleT     key,  
            ClCntDataHandleT     userData)
{
  
    ClnCkptT    *pCkpt = NULL;
    pCkpt = (ClnCkptT *)userData;
    if(pCkpt != NULL)
    {
        if(pCkpt->pCkptName != NULL)
            clHeapFree(pCkpt->pCkptName);
        if(pCkpt->dataSetList != 0)
            clCntDelete(pCkpt->dataSetList);
        clDbalClose(pCkpt->usrDbHdl);
        clHeapFree(pCkpt);
    }
    return;
}


static ClRcT _clCkptLibraryInitializeDB(ClCkptSvcHdlT *pCkptHdl,
                                        const ClCharT *ckptDB)
{
    CkptClnCbT         *pCkptClnt   =  NULL;        /* Client control block*/
    ClRcT               rc          =  CL_OK;       /* Return code*/
    ClDBHandleT         dbHandle    =  0;           /* Database handle*/
    ClDBRecordT         record      =  NULL;        /* Record*/
    ClUint32T           recSize     =  0;           /* Sizeof the record*/
    ClDBFlagT           dbFlag      =  CL_DB_APPEND;  /* Db flag */
    ClUint32T           key         =  CKPT_DB_KEY; /* Key for CkptDB*/
    
    /*
        Algorithm:
            1)Validations
                - Check the client is already initialized
                - If it is, return CL_ERR_ALREADY_INTIALIZED
            2)Initialise
                - If not,  initialise the client
            3)Memory allocation
                - Allocate memory for client control block
                - Assign values to the variables
                - Assign memory for the specified no of checkpoints
            4)Done
            
    */
    /*Validations */
    CKPT_NULL_CHECK (pCkptHdl);

    /*Initialize dbal if not initialized*/
    rc = clDbalLibInitialize();
    CKPT_ERR_CHECK( CL_CKPT_LIB, CL_DEBUG_ERROR, 
            ("Ckpt: Error during dbal lib initialize [rc=%#x]\n",rc), rc);

    /* Open the DB */
    rc   = clDbalOpen (ckptDB, 
                      ckptDB, 
                      dbFlag, 
                      CKPT_MAX_NUMBER_RECORD,
                      CKPT_MAX_SIZE_RECORD, 
                      &dbHandle);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,  
            ("Ckpt: Error during opening the DB rc[0x %x]\n",rc), rc);

    /* Check if there is some checkpointed information for the client */
    key  = CKPT_DB_KEY;
    rc   = clDbalRecordGet (dbHandle, 
                           (ClDBKeyT)&key, 
                            sizeof (key),
                           (ClDBRecordT *)&record, 
                           &recSize);

    if ( (rc == CL_OK)&&  (record != NULL))
    {
        /* Consume the checkpoint */
        rc = clCkptUnpack ( &record, recSize, &pCkptClnt);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                 ("Ckpt: Error during Unpacking the DB rc[0x %x]\n",rc), rc);
       *pCkptHdl =  (ClWordT)pCkptClnt;
        clDbalRecordFree(dbHandle,record);         
    }     
    else
    {
        /* Initialise the data structures. No checkpoint was done */
        if  (NULL ==  (pCkptClnt= (CkptClnCbT *)clHeapAllocate (
                                                        sizeof (CkptClnCbT))))
        {
            CL_DEBUG_PRINT (CL_DEBUG_CRITICAL, 
                    ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
            clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            return CL_CKPT_ERR_NO_MEMORY;
        }
        memset (pCkptClnt, 0, sizeof (CkptClnCbT));        

        /* Create the mutex */
        clOsalMutexCreate (&pCkptClnt->ckptSem);
        rc = clCntLlistCreate (ckptCheckptCompare,
                               ckptCheckptDeleteCallback,
                               ckptCheckptDeleteCallback,
                               CL_CNT_UNIQUE_KEY,
                               &pCkptClnt->ckptList);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
         ("Ckpt:Error during creating linked list for datasets rc[0x %x]\n",rc),
          rc);
        
        *pCkptHdl =  (ClWordT)pCkptClnt;
    }
    pCkptClnt->ownDbHdl = dbHandle;
    return CL_OK;

exitOnError:
    {
        if  (pCkptClnt != NULL)
        {
            clHeapFree (pCkptClnt);
        }
        if (dbHandle != 0)clDbalClose (dbHandle);
        return rc;
    }

}


ClRcT clCkptLibraryInitializeDB(ClCkptSvcHdlT *pCkptHdl,
                                const ClCharT *dbName)
{
    ClCharT ckptDB[CL_MAX_NAME_LENGTH+1];
    CKPT_NULL_CHECK(pCkptHdl);
    CL_CKPT_APP_NAME_FORM(ckptDB, dbName);
    return _clCkptLibraryInitializeDB(pCkptHdl, ckptDB);
}

/*
 *Function to initializes the client
*/

ClRcT clCkptLibraryInitialize (
        ClCkptSvcHdlT       *pCkptHdl)
{
    ClRcT               rc          =  CL_OK;       /* Return code*/
    ClCharT             ckptDB[CL_MAX_NAME_LENGTH + strlen (CL_CKPT_DB_NAME)]; 
    ClCpmHandleT        cpmHandle   =  0;           /* Cpm handle. */
    ClNameT             appName = {0}; 
    
    /*Validations */
    CKPT_NULL_CHECK (pCkptHdl);

    /* Prepare the name of the ckptDb file. 
    It contains CkptDb<compName><nodeId>.db */
    memset (&appName, 0, sizeof (appName));
    rc = clCpmComponentNameGet (cpmHandle, &appName);
    CKPT_ERR_CHECK( CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Error during name of Component rc[0x %x]\n",rc), rc);

    CL_CKPT_LIB_NAME_FORM(ckptDB, appName.value);

    return _clCkptLibraryInitializeDB(pCkptHdl, ckptDB);
    
    exitOnError:
    return rc;
}


/*
    Function to destroy the client
*/

ClRcT clCkptLibraryFinalize (
            ClCkptSvcHdlT ckptHdl)
{
    CkptClnCbT      *pCkptClnt   =  NULL ;       /*client library handle*/
    ClRcT            rc          =  CL_OK;       /*return code*/
   
                                 
    /*
        Algorithm:
            1)Validations
                - Check the client is already intialized
                - If not, return CL_CKPT_ERR_NOT_INITIALIZED
            2)Deletion
                - Delete all checkpoints
                - Delete the client
            3)Done
    */

    /* Validations */
    CKPT_NULL_CHECK_HDL (ckptHdl);
    pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl ;
    /* Deletion */
    if(pCkptClnt != NULL)
    {
        rc = clCntDelete(pCkptClnt->ckptList);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                ("Ckpt: Error during closing checkpoints rc[0x %x]\n",rc), rc);
        clOsalMutexDelete (pCkptClnt->ckptSem);
       clDbalClose (pCkptClnt->ownDbHdl);
       clHeapFree (pCkptClnt);
    }
    return CL_OK;
exitOnError:
    {
        return rc;
    }
}         
    
    
 /*
    Function for checkpoint open in clientSide
 */
ClRcT clCkptLibraryCkptCreate (ClCkptSvcHdlT  ckptHdl, 
                               ClNameT       *pCkptName) 
                              
{
    CkptClnCbT      *pCkptClnt   =  NULL;        /*checkpoint library handle*/   
    ClRcT            rc          =  CL_OK;       /*return code*/
    ClnCkptT        *pCkpt       =  NULL;        /*checkpoint*/
    ClDBNameT        ckptDbName  =  NULL;        /*DB for storing user data*/
    ClUint32T        length      =  0;           /*length of the data*/
    ClUint32T        key         =  CKPT_DB_KEY;          /*key for CkptDB*/
    ClCharT         *pBuffer     =  NULL;        /*pointer to the buffer*/ 
    ClCharT          tempDb[CL_MAX_NAME_LENGTH]; 
    ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
    ClCharT          *pCkptListKey = ckptListKey;

    /*

      Algorithm:
      1)Validations
      - Check the client is already intialized
      - If not, return CL_CKPT_ERR_NOT_INITIALIZED
      - Check, memory is available for the checkpoint
      - If not, return CL_ERR_INVLD
      2)Initialization
      - Choose the location for the given checkpoint
      - Assign the values to the variables
      3)Alloaction
      - Check the type of checkpoint
      - If it is Disk based, allocate memory for the given no of datasets
      - Else serverbased,  do the server based checkpointing
      4)Done 
    */
    /*Validations*/
    CKPT_NULL_CHECK_HDL (ckptHdl);
    CKPT_NULL_CHECK (pCkptName);
    pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl ;
    CKPT_LOCK (pCkptClnt->ckptSem);
  
    ckptDbName = pCkptName->value;
    
    CL_CKPT_APP_NAME_FORM(tempDb, ckptDbName);

    CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);

    /*checking whether already is existing*/
    
    rc = clCntDataForKeyGet ( pCkptClnt->ckptList,
                              (ClCntKeyHandleT)pCkptListKey,
                              (ClCntDataHandleT *)&pCkpt);
    if (rc == CL_OK)
    {
        ClDBFlagT    openFlag = CL_DB_APPEND;
        if(pCkpt != NULL)
        {
            if  (pCkpt->usrDbHdl == 0)
            {
                rc   = clDbalOpen (tempDb,
                                   tempDb, 
                                   openFlag, 
                                   CKPT_MAX_NUMBER_RECORD,
                                   CKPT_MAX_SIZE_RECORD, 
                                   &pCkpt->usrDbHdl);
            }
            CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                           ("Ckpt:Error during opening the DB rc[0x %x]\n",rc), rc);
        }
        CKPT_UNLOCK (pCkptClnt->ckptSem);
        return rc;
    }
    
    /*Memory allocation and Initialization*/
    pCkpt = (ClnCkptT *)clHeapAllocate(sizeof(ClnCkptT));
    if(pCkpt == NULL)
    {
        CL_DEBUG_PRINT (CL_DEBUG_CRITICAL, 
                        ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CKPT_UNLOCK (pCkptClnt->ckptSem);
        return CL_CKPT_ERR_NO_MEMORY;
    }
    memset(pCkpt, 0,sizeof(ClnCkptT));
    pCkpt->pCkptName  =  (ClNameT *)clHeapAllocate (sizeof (ClNameT));
    if (pCkpt->pCkptName == NULL)
    {
        CL_DEBUG_PRINT (CL_DEBUG_CRITICAL, 
                        ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CKPT_UNLOCK (pCkptClnt->ckptSem);
        return CL_CKPT_ERR_NO_MEMORY;
    }        
    memset (pCkpt->pCkptName, 0, sizeof (ClNameT));
    pCkpt->pCkptName->length = pCkptName->length;
    memcpy (pCkpt->pCkptName->value ,  pCkptName->value,  pCkptName->length);
        
    /*Disk based check pointing*/
    rc = clCntLlistCreate (ckptDsetCompare,
                           ckptDataSetDeleteCallback,
                           ckptDataSetDeleteCallback,
                           CL_CNT_UNIQUE_KEY,
                           &pCkpt->dataSetList);
    if (rc != CL_OK)
    {
        if (pCkpt!=NULL)
        {
            if (pCkpt->pCkptName != NULL)
            {
                clHeapFree(pCkpt->pCkptName);
                pCkpt->pCkptName = NULL;
            }
        }
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                       ("Ckpt: Error during creating linked list for datasets rc[0x %x]\n",rc),
                       rc);
    }
    
    rc   = clDbalOpen ( tempDb, 
                        tempDb,
                        CL_DB_CREAT,
                        CKPT_MAX_NUMBER_RECORD,
                        CKPT_MAX_SIZE_RECORD,
                        &pCkpt->usrDbHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,  
                   ("Ckpt: Error during opening the DB rc[0x %x]\n",rc), rc);
    pCkptListKey = NULL;
    CKPT_LIST_GET_KEY(pCkptListKey, pCkpt->pCkptName);
    rc = clCntNodeAdd ( pCkptClnt->ckptList,
                        (ClCntKeyHandleT)pCkptListKey,
                        (ClCntDataHandleT)pCkpt,
                        NULL);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                   ("Ckpt: Error during adding ckpt rc[0x %x]\n",rc), rc);
    
    key  = CKPT_DB_KEY;         
    rc   = clCkptBufferAlloc (pCkptClnt, &pBuffer, &length);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt:Error during Allocating buffer in checkpoint creation rc[0x %x]\n",rc)
                   ,rc);
    rc   = clCkptPack (pCkptClnt, pBuffer);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                   ("Ckpt: Error during packing the information in"
                    "checkpoint creation rc[0x %x]\n",rc),rc);
    
    rc = clDbalRecordReplace( pCkptClnt->ownDbHdl,
                              (ClDBKeyT)&key,
                              sizeof(key),
                              (ClDBRecordT)pBuffer,
                              length);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Error during inserting into DB rc[0x %x]\n",rc), rc);
    clCkptBufferFree (pBuffer);
      
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return CL_OK;
    exitOnError:
    {
        CKPT_UNLOCK (pCkptClnt->ckptSem);
        return rc;
    }
} 


/*
    Function for Closing the particular checkpoint in  client
*/

ClRcT clCkptLibraryCkptDelete (ClCkptSvcHdlT ckptHdl, 
                              ClNameT     *pCkptName)
{

    CkptClnCbT       *pCkptClnt  =  NULL;        /*checkpoint handle*/
    ClRcT             rc         =  CL_OK;       /*return code */
    ClUint32T        length      =  0;           /*length of the data*/
    ClUint32T        key         =  CKPT_DB_KEY; /*key for CkptDB*/
    ClCharT         *pBuffer     =  NULL;        /*pointer to the Buffer */
    ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *pCkptListKey = ckptListKey;

/*
    Algorithm:
        1)Validations
            -check the client is already initialized
            -if not, return CL_CKPT_ERR_NOT_INITIALIZED
            -check, the checkpoint to be closed is available
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
        2)Deletion
            -delete all datasets exists in the given checkpoint
            -delete the checkpoint
        3)Done.
        
*/
    /*Validations*/
    CKPT_NULL_CHECK_HDL (ckptHdl);
    CKPT_NULL_CHECK (pCkptName);
    pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl ;
    CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);
    CKPT_LOCK (pCkptClnt->ckptSem);
 
    /*Choosing the correct CheckPoint*/
    rc = clCntAllNodesForKeyDelete( pCkptClnt->ckptList,
                                    (ClCntKeyHandleT)pCkptListKey);
   /* rc = clCntAllNodesDelete(pCkpt->dataSetList);*/
                                   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt: Error during deleting Ckpt rc[0x %x]\n",rc), rc);
    /*Deletion*/
    
    rc   = clCkptBufferAlloc ( pCkptClnt,
                               &pBuffer,
                               &length);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
     ("Ckpt: Error during Buffer Allocation in Ckpt deletion rc[0x %x]\n",rc),
     rc);
    rc   = clCkptPack (pCkptClnt, pBuffer);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt: Error during Packing in Ckpt deletion rc[0x %x]\n",rc), rc);
                                                         
    key  = CKPT_DB_KEY ; 
    rc  = clDbalRecordReplace( pCkptClnt->ownDbHdl,
                               (ClDBKeyT)&key,
                               sizeof (key),
                               (ClDBRecordT)pBuffer,
                               length);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Error during inserting into DB rc[0x %x]\n",rc), rc);
    clCkptBufferFree (pBuffer);
    /*Done*/
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return CL_OK;
exitOnError:
    {
        CKPT_UNLOCK (pCkptClnt->ckptSem);
        return rc;
    }
}         



ClRcT clCkptLibraryCkptDataSetVersionCreate (ClCkptSvcHdlT   ckptHdl, 
                                   ClNameT           *pCkptName, 
                                   ClUint32T          dsId, 
                                   ClUint32T          grpId, 
                                   ClUint32T          order, 
                                   ClCkptDataSetCallbackT *pTable,
                                   ClUint32T numTableEntries)
{
    ClnCkptT           *pCkpt      =  NULL;        /*check point object*/ 
    CkptDataSetT       *pDataInfo  =  NULL;        /*dataset*/
    CkptClnCbT         *pCkptClnt  =  NULL;        /*checkpoint client handle*/
    ClRcT               rc         =  CL_OK;       /*return code*/
    ClUint32T           length     =  0;           /*length of the data*/
    ClUint32T           key        =  CKPT_DB_KEY; /*key for CkptDB*/
    ClCharT            *pBuffer    =  NULL;        /*buffer for data*/
    ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *pCkptListKey = ckptListKey;
/*
    Algorithm:
        1)Validations
            -check the client is already intialized
            -if not, return CL_CKPT_ERR_NOT_INITIALIZED
            -check, the given checkpoint is available
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
        2)Initialization
            -choose the location for the given Dataset in the checkpoint
            -assign the values to the variables
        3)Done
*/    
    /*Validations*/
    CKPT_NULL_CHECK_HDL (ckptHdl);
    CKPT_NULL_CHECK (pCkptName);
    CKPT_NULL_CHECK(pTable);

    if (dsId == 0 || numTableEntries == 0) return CL_CKPT_ERR_INVALID_PARAMETER;
    
    pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl ;
    
    CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);
    CKPT_LOCK (pCkptClnt->ckptSem);
    /*Checking the checkpoint*/
    rc = clCntDataForKeyGet ( pCkptClnt->ckptList,
                              (ClCntKeyHandleT)pCkptListKey,
                              (ClCntDataHandleT *)&pCkpt);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,  
            ("Ckpt:Invalid Checkpoint Name rc[0x %x]\n",rc),rc);
    rc = clCntDataForKeyGet (pCkpt->dataSetList,
                            (ClCntKeyHandleT)(ClWordT)dsId,
                            (ClCntDataHandleT *)&pDataInfo);
    if (rc == CL_OK)
    {
        if(pDataInfo->pDataSetCallbackTable)
        {
            if(pDataInfo->numDataSetTableEntries != numTableEntries)
            {
                clHeapFree(pDataInfo->pDataSetCallbackTable);
                pDataInfo->pDataSetCallbackTable = clHeapCalloc(numTableEntries, 
                                                                sizeof(*pDataInfo->pDataSetCallbackTable));
            }
        }
        else
        {
            pDataInfo->pDataSetCallbackTable = clHeapCalloc(numTableEntries,
                                                            sizeof(*pDataInfo->pDataSetCallbackTable));
        }
        CL_ASSERT(pDataInfo->pDataSetCallbackTable);

        memcpy(pDataInfo->pDataSetCallbackTable, pTable, 
               numTableEntries*sizeof(*pDataInfo->pDataSetCallbackTable));
        pDataInfo->numDataSetTableEntries = numTableEntries;
        CKPT_UNLOCK (pCkptClnt->ckptSem);
        return CL_OK;
    }
    pDataInfo =  (CkptDataSetT *)clHeapAllocate (sizeof (CkptDataSetT));
    if (pDataInfo == NULL)
    {
       rc = CL_CKPT_ERR_NO_MEMORY;
       CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
               ("Ckpt: Memory can not be allocated rc[0x %x]\n",rc), rc);
    }
    memset (pDataInfo, 0, sizeof (CkptDataSetT));
    pDataInfo->dsId         =  dsId;
    pDataInfo->pDataSetCallbackTable = clHeapCalloc(numTableEntries, sizeof(*pDataInfo->pDataSetCallbackTable));
    CL_ASSERT(pDataInfo->pDataSetCallbackTable != NULL);
    memcpy(pDataInfo->pDataSetCallbackTable, pTable, numTableEntries * sizeof(*pDataInfo->pDataSetCallbackTable));
    pDataInfo->numDataSetTableEntries = numTableEntries;
    pDataInfo->pElementCallbackTable = NULL;
    pDataInfo->numElementTableEntries = 0;
    pDataInfo->grpId        =  grpId;
    pDataInfo->order        =  order;        
    pDataInfo->size         =  0; 
    pDataInfo->data         =  NULL;
    
    rc = clCntNodeAdd (pCkpt->dataSetList,  (ClCntKeyHandleT)(ClWordT)pDataInfo->dsId,
                       (ClCntDataHandleT)pDataInfo, NULL);
    if (rc != CL_OK)
    {
        clHeapFree(pDataInfo->pDataSetCallbackTable);
        pDataInfo->pDataSetCallbackTable = NULL;
        clHeapFree(pDataInfo);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt: Error during adding dataset rc[0x %x]\n",rc), rc);
        
    /*packing*/
    rc   = clCkptBufferAlloc (pCkptClnt, &pBuffer, &length);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
     ("Ckpt: Error during allocating buffer in DataSetCreation rc[0x %x]\n",rc),
     rc);
    rc   = clCkptPack (pCkptClnt, pBuffer);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
              ("Ckpt: Error during Packing in Dataset creation rc[0x %x]\n",rc),
               rc);
    key  = CKPT_DB_KEY;
    rc  = clDbalRecordReplace ( pCkptClnt->ownDbHdl,
                                (ClDBKeyT)&key,
                                sizeof (key), 
                                (ClDBRecordT)pBuffer,
                                length);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Error during inserting into DB rc[0x %x]\n",rc), rc);
    clCkptBufferFree (pBuffer);
    
    /*Done*/
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return CL_OK;
exitOnError:
    {
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return rc;
    }
}
     
/*
    Function for dataset creation in a particular check point
*/

ClRcT clCkptLibraryCkptDataSetCreate (ClCkptSvcHdlT   ckptHdl, 
                                   ClNameT           *pCkptName, 
                                   ClUint32T          dsId, 
                                   ClUint32T          grpId, 
                                   ClUint32T          order, 
                                   ClCkptSerializeT   dsSerialiser, 
                                   ClCkptDeserializeT dsDeserialiser)
                                   
{
    
    ClCkptDataSetCallbackT table[1] = { 
        {
        .version = {
                .releaseCode = CL_RELEASE_VERSION, 
                .majorVersion = CL_MAJOR_VERSION, 
                .minorVersion = CL_MINOR_VERSION
            },
        .serialiser = dsSerialiser,
        .deSerialiser = dsDeserialiser
        },
    };
    return clCkptLibraryCkptDataSetVersionCreate(ckptHdl, pCkptName, dsId, grpId, order, table, 1);
}     
    
    
/*
    Function for dataset deletion in a particular check point
 
 */
ClRcT clCkptLibraryCkptDataSetDelete (ClCkptSvcHdlT  ckptHdl, 
                                     ClNameT       *pCkptName, 
                                     ClUint32T      dsId)
{
    ClnCkptT          *pCkpt     =  NULL;     /*checkpoint object*/
    CkptDataSetT      *pDataInfo =  NULL;     /*data set*/
    CkptClnCbT        *pCkptClnt =  NULL;     /*checkpoint handle*/
    ClRcT              rc        =  CL_OK;    /*return code*/
    ClUint32T          length    =  0;        /*length*/
    ClUint32T          key       =  CKPT_DB_KEY; /*key for CkptDB*/
    ClDBRecordT        record    =  NULL;      /*record base address*/
    ClUint32T          recSize   =  0   ;      /*length of record*/
    ClCharT           *pBuffer   =  NULL;     /*buffer for data*/ 
    CkptElemKeyT       dbKey;
    ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *pCkptListKey = ckptListKey;
 /*
      Algorithm:
          1)Validations
            -check the client is already intialized
            -if not, return CL_CKPT_ERR_NOT_INITIALIZED
            -check, the given checkpoint is availble
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
            -check, the given dataset is available
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
        2)Deletion
            -choose the dataset to be deleted
            -delete the dataset
        3)Done                                                      
*/    
    /*Validations*/
    CKPT_NULL_CHECK_HDL (ckptHdl);
    CKPT_NULL_CHECK (pCkptName);
    if (dsId == 0) return CL_CKPT_ERR_INVALID_PARAMETER;
    pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl ;
    CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);
    CKPT_LOCK (pCkptClnt->ckptSem);
    /*checking checkpoint*/
    rc = clCntDataForKeyGet ( pCkptClnt->ckptList,
                              (ClCntKeyHandleT)pCkptListKey,
                              (ClCntDataHandleT *)&pCkpt);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt:Invalid Checkpoint Name rc[0x %x]\n",rc),rc);
    /*deletion*/
    rc = clCntDataForKeyGet ( pCkpt->dataSetList,
                              (ClCntKeyHandleT)(ClWordT)dsId,
                              (ClCntDataHandleT *)&pDataInfo);
    if (rc != CL_OK)
    {
       rc = CL_CKPT_ERR_NOT_EXIST;
       CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
               ("Ckpt: Dataset is not exists rc[0x %x]\n",rc), rc);
    }
    memset(&dbKey, 0,sizeof(dbKey));
    dbKey.dsId = dsId;
    dbKey.keyLen  = 0;
    dbKey.elemKey[0] = 0x0;

    rc = clDbalRecordGet ( pCkpt->usrDbHdl,
                           (ClDBKeyT)&dbKey,
                           sizeof (dbKey),
                           &record,
                           &recSize);
    if (rc == CL_OK)
    {    
       rc = clDbalRecordDelete ( pCkpt->usrDbHdl, 
                                 (ClDBKeyT)&dbKey,
                                 sizeof (dbKey));
       CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
               ("Ckpt: Error during deleting the DB rc[0x %x]\n",rc), rc);
       clDbalRecordFree(pCkpt->usrDbHdl,
                             record);
    }
    
    rc = clCntAllNodesForKeyDelete(pCkpt->dataSetList,
                                   (ClCntKeyHandleT)(ClWordT)dsId);
    rc   = clCkptBufferAlloc (pCkptClnt, &pBuffer, &length);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Error during buffer allocation rc[0x %x]\n",rc), rc);
    rc   = clCkptPack (pCkptClnt, pBuffer);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Error during packing ckpt Info rc[0x %x]\n",rc), rc);
    
    key  = CKPT_DB_KEY;
    rc  = clDbalRecordReplace ( pCkptClnt->ownDbHdl, 
                                (ClDBKeyT)&key, 
                                sizeof (key),
                                (ClDBRecordT)pBuffer,
                                length);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Error during inserting into DB rc[0x %x]\n",rc), rc);
    clCkptBufferFree (pBuffer);
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return CL_OK;
exitOnError:
    {
       CKPT_UNLOCK (pCkptClnt->ckptSem);
       return rc;
    }
}           

static ClRcT clCkptDataSetDeSerialize(ClUint32T dsId,
                                      ClDBRecordT record,
                                      ClUint32T recSize,
                                      ClPtrT cookie,
                                      ClCkptDataSetCallbackT *pDataSetCallbackTable,
                                      ClUint32T numDataSetTableEntries)
{
    ClRcT rc = CL_OK;
    ClUint32T versionCode = 0;
    ClUint32T i;

    if(!record || recSize < sizeof(ClUint32T))
    {
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                       ("Ckpt: No data to deserialize\n"), CL_CKPT_ERR_NOT_EXIST);
        
    }
    
    if(!pDataSetCallbackTable || !numDataSetTableEntries)
    {
        rc = CL_CKPT_ERR_NULL_POINTER;
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                       ("Ckpt: haven't registered dataset callback table rc[0x %x]\n",rc), rc);
    }


    memcpy(&versionCode, (ClUint8T*)((ClUint8T*)record + (recSize - sizeof(ClUint32T))),
           sizeof(versionCode));
    versionCode = ntohl(versionCode);
    recSize -= sizeof(ClUint32T);

    /*
     * Invoke the deserializer registered to handle this version.
     */
    for(i = 0; i < numDataSetTableEntries; ++i)
    {
        ClCkptDataSetCallbackT *pCallback = pDataSetCallbackTable + i;
        ClUint32T callbackVersion = CL_VERSION_CODE(pCallback->version.releaseCode,
                                                    pCallback->version.majorVersion,
                                                    pCallback->version.minorVersion);
        if(callbackVersion == versionCode)
        {
            return pCallback->deSerialiser(dsId, (ClAddrT)record, recSize, cookie);
        }
    }
    
    rc = CL_CKPT_ERR_NOT_EXIST;

    exitOnError:
    return rc;
}

static ClRcT clCkptDataSetSerialize(ClVersionT *pVersion,
                                    CkptDataSetT *pDataInfo,
                                    ClPtrT cookie,
                                    ClCkptDataSetCallbackT *pDataSetCallbackTable,
                                    ClUint32T numDataSetTableEntries)
{
    ClRcT rc = CL_OK;
    ClUint32T i;
    ClUint32T versionCode = 0;
    if(!pVersion || !pDataInfo || !pDataSetCallbackTable || !numDataSetTableEntries)
    {
        rc = CL_CKPT_ERR_NULL_POINTER;
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
              ("Ckpt: haven't registered dataset callback table rc[0x %x]\n",rc), rc);
    }
    
    versionCode = CL_VERSION_CODE(pVersion->releaseCode, pVersion->majorVersion,
                                  pVersion->minorVersion);
    /*
     * Invoke the serializer registered to handle this version.
     */
    for(i = 0; i < numDataSetTableEntries; ++i)
    {
        ClCkptDataSetCallbackT *pCallback = pDataSetCallbackTable + i;
        ClUint32T callbackVersion = CL_VERSION_CODE(pCallback->version.releaseCode,
                                                    pCallback->version.majorVersion,
                                                    pCallback->version.minorVersion);
        if(versionCode == callbackVersion)
        {
            /*writing*/
            pDataInfo->size = 0;
            pDataInfo->data = NULL;
            rc = pCallback->serialiser ( pDataInfo->dsId,
                                         &pDataInfo->data,
                                         &pDataInfo->size,
                                         cookie);
        
            CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                           ("Ckpt: Error during serialization rc[0x %x]\n",rc), rc);
            /*
             * Store the version in network order. instead of using the slow
             * marshall/unmarshall version routines which entails a buffer
             */
            versionCode = htonl(versionCode);
            if(pDataInfo->data)
            {
                pDataInfo->data = clHeapRealloc(pDataInfo->data, pDataInfo->size + sizeof(ClUint32T));
                CL_ASSERT(pDataInfo->data != NULL);
                /*
                 * We just append the version to the end since that would avoid a memmove of the
                 * existing serialized data and be fast. 
                 */
                memcpy((ClPtrT)((ClUint8T*)pDataInfo->data + pDataInfo->size), &versionCode, sizeof(ClUint32T));
                pDataInfo->size += sizeof(ClUint32T);
                return rc;
            }
        }
    }

    rc = CL_CKPT_ERR_NOT_EXIST;
    
    exitOnError:
    return rc;
}

 /*
    Function for  writing the dataset from the specified checkpoint
     to some storage with version
*/     
ClRcT clCkptLibraryCkptDataSetVersionWrite (ClCkptSvcHdlT   ckptHdl, 
                                            ClNameT         *pCkptName, 
                                            ClUint32T        dsId, 
                                            ClPtrT        cookie,
                                            ClVersionT    *pVersion)
{                                  
       
     ClnCkptT             *pCkpt     =  NULL;      /*checkpoint object*/
     CkptClnCbT           *pCkptClnt =  NULL;      /*checkpoint handle*/
     CkptDataSetT         *pDataInfo =  NULL;      /*data set */
     ClRcT                 rc        =  CL_OK;     /*return code*/
     ClDBNameT             ckptDbName   =  NULL;      /*dataBase name*/   
     CkptElemKeyT          dbKey = {0};
     ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
     ClCharT *pCkptListKey = ckptListKey;
     /*  
     Algorithm:
        1)Validations
            -check the client is already intialized
            -if not, return CL_CKPT_ERR_NOT_INITIALIZED
            -check, the given checkpoint is availble
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
            -check, the given dataset is available
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
        2)Writing
            -choose the dataset to be written
            -write the dataset to some storage
        3)Done
                                                 
 */
    /*Validations*/      
    CKPT_NULL_CHECK_HDL (ckptHdl);
    CKPT_NULL_CHECK (pCkptName);
    CKPT_NULL_CHECK(pVersion);
    pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl;
    CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);
    CKPT_LOCK (pCkptClnt->ckptSem);
      
    /*choosing the checkpoint*/ 
           
    rc = clCntDataForKeyGet (pCkptClnt->ckptList,
                             (ClCntKeyHandleT)pCkptListKey,
                             (ClCntDataHandleT *)&pCkpt);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Invalid Checkpoint Name [%s:%s] rc[0x %x]\n", pCkptListKey, pCkptName->value, rc),rc);
    /*choosing the dataset*/
    rc = clCntDataForKeyGet ( pCkpt->dataSetList,
                             (ClCntKeyHandleT)(ClWordT)dsId,
                            (ClCntDataHandleT *)&pDataInfo);
    if (rc != CL_OK)
    {
      rc = CL_CKPT_ERR_NOT_EXIST;
      CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
              ("Ckpt: The given dataset is not exists rc[0x %x]\n",rc), rc);
    }
    rc = clCkptDataSetSerialize(pVersion, pDataInfo, cookie, 
                                pDataInfo->pDataSetCallbackTable,
                                pDataInfo->numDataSetTableEntries);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Error during version serialization rc[0x %x]\n",rc), rc);
    ckptDbName = pCkptName->value;
    /*Creating  key*/
    dbKey.dsId = pDataInfo->dsId;
    dbKey.keyLen  = 0;
    dbKey.elemKey[0] = 0x0;
    /*Writing into DBAL*/
    rc  = clDbalRecordReplace (pCkpt->usrDbHdl,  (ClDBKeyT)&dbKey,
                              sizeof (dbKey),
                              (ClDBRecordT)pDataInfo->data, pDataInfo->size);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt: Error during Inserting the record rc[0x %x]\n",rc), rc);
    
    /*Done*/
    clHeapFree (pDataInfo->data);
    pDataInfo->data = NULL;
    pDataInfo->size = 0;
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return CL_OK;
exitOnError:
    {
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return rc;
    }
}


ClRcT clCkptLibraryCkptDataSetWrite (ClCkptSvcHdlT   ckptHdl, 
                                     ClNameT         *pCkptName, 
                                     ClUint32T        dsId, 
                                     ClPtrT        cookie)
{
    ClVersionT version = {.releaseCode = CL_RELEASE_VERSION, 
                          .majorVersion = CL_MAJOR_VERSION,
                          .minorVersion = CL_MINOR_VERSION
    };
    return clCkptLibraryCkptDataSetVersionWrite(ckptHdl, pCkptName, dsId, cookie, &version);
}
                                           

/* 
 * Function for creating the element with version.
*/
ClRcT clCkptLibraryCkptElementVersionCreate(ClCkptSvcHdlT        ckptHdl, 
                                            ClNameT              *pCkptName, 
                                            ClUint32T            dsId,
                                            ClCkptDataSetCallbackT *pTable,
                                            ClUint32T numTableEntries)
{
   ClnCkptT             *pCkpt     =  NULL;      /*checkpoint object*/
   CkptClnCbT           *pCkptClnt =  NULL;      /*checkpoint handle*/
   CkptDataSetT         *pDataInfo =  NULL;      /*data set */
   ClRcT                 rc        =  CL_OK;     /*return code*/
   ClUint32T           length     =  0;           /*length of the data*/
   ClUint32T           key        =  CKPT_DB_KEY; /*key for CkptDB*/
   ClCharT            *pBuffer    =  NULL;        /*buffer for data*/
   ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
   ClCharT *pCkptListKey = ckptListKey;
/*
    Algorithm:
        1)Validations
            -check the client is already intialized
            -if not, return CL_CKPT_ERR_NOT_INITIALIZED
            -check, the given checkpoint is available
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
            -check, the given dataset is available
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
        2)Initialization
            -assign the values to the variables
        3)Done
*/    
   
   /*Validations*/      
   CKPT_NULL_CHECK_HDL (ckptHdl);
   CKPT_NULL_CHECK (pCkptName);
   CKPT_NULL_CHECK(pTable);

   pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl;
   CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);
   CKPT_LOCK (pCkptClnt->ckptSem);
   
   /*choosing the checkpoint*/ 
   rc = clCntDataForKeyGet (pCkptClnt->ckptList,
                            (ClCntKeyHandleT)pCkptListKey,
                            (ClCntDataHandleT *)&pCkpt);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt:Invalid Checkpoint Name rc[0x %x]\n",rc),rc);
    /*choosing the dataset*/
    rc = clCntDataForKeyGet ( pCkpt->dataSetList,
                             (ClCntKeyHandleT)(ClWordT)dsId,
                            (ClCntDataHandleT *)&pDataInfo);
    if (rc != CL_OK)
    {
      rc = CL_CKPT_ERR_NOT_EXIST;
      CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: The given dataset is not exists rc[0x %x]\n",rc), rc);
    }
    
   if(pDataInfo != NULL)
   {
       if(pDataInfo->pElementCallbackTable)
       {
           if(pDataInfo->numElementTableEntries != numTableEntries)
           {
               clHeapFree(pDataInfo->pElementCallbackTable);
               pDataInfo->pElementCallbackTable = clHeapCalloc(numTableEntries,
                                                               sizeof(*pDataInfo->pElementCallbackTable));
           }
       }
       else
       {
           pDataInfo->pElementCallbackTable = clHeapCalloc(numTableEntries,
                                                           sizeof(*pDataInfo->pElementCallbackTable));
       }
       CL_ASSERT(pDataInfo->pElementCallbackTable != NULL);
       memcpy(pDataInfo->pElementCallbackTable, pTable,
              sizeof(*pDataInfo->pElementCallbackTable) * numTableEntries);
       pDataInfo->numElementTableEntries = numTableEntries;
       CKPT_UNLOCK (pCkptClnt->ckptSem);
       return CL_OK;
   }

   rc   = clCkptBufferAlloc (pCkptClnt, &pBuffer, &length);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
     ("Ckpt: Error during allocating buffer in DataSetCreation rc[0x %x]\n",rc),
     rc);
   rc   = clCkptPack (pCkptClnt, pBuffer);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
              ("Ckpt: Error during Packing in Dataset creation rc[0x %x]\n",rc),
               rc);
   key  = CKPT_DB_KEY;
   rc  = clDbalRecordReplace ( pCkptClnt->ownDbHdl,
                                (ClDBKeyT)&key,
                                sizeof (key), 
                                (ClDBRecordT)pBuffer,
                                length);
   CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Error during inserting into DB rc[0x %x]\n",rc), rc);
   clCkptBufferFree (pBuffer);
   CKPT_UNLOCK (pCkptClnt->ckptSem);
   return CL_OK;
exitOnError:
    {
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return rc;
    }
}

ClRcT clCkptLibraryCkptElementCreate(ClCkptSvcHdlT        ckptHdl, 
                                     ClNameT              *pCkptName, 
                                     ClUint32T            dsId,
                                     ClCkptSerializeT     elemSerialiser,
                                     ClCkptDeserializeT   elemDeserialiser)
{
    ClCkptDataSetCallbackT table[1] = {
        { 
          .version = { 
                .releaseCode = CL_RELEASE_VERSION, 
                .majorVersion = CL_MAJOR_VERSION, 
                .minorVersion = CL_MINOR_VERSION 
            },
          .serialiser = elemSerialiser,
          .deSerialiser = elemDeserialiser,
        },
    };

    return clCkptLibraryCkptElementVersionCreate(ckptHdl, pCkptName, dsId, table, 1);
}

/*
    Function for writing the element from the specified Dataset
     to some storage
*/     
                                     
ClRcT clCkptLibraryCkptElementVersionWrite (ClCkptSvcHdlT   ckptHdl, 
                                            ClNameT        *pCkptName, 
                                            ClUint32T       dsId, 
                                            ClPtrT          elemKey,
                                            ClUint32T       keyLen,
                                            ClPtrT       cookie,
                                            ClVersionT   *pVersion)
{                                  
       
     ClnCkptT             *pCkpt     =  NULL;      /*checkpoint object*/
     CkptClnCbT           *pCkptClnt =  NULL;      /*checkpoint handle*/
     CkptDataSetT         *pDataInfo =  NULL;      /*data set */
     ClRcT                 rc        =  CL_OK;     /*return code*/
     CkptElemKeyT          *pDbKey   = NULL;
     ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
     ClCharT *pCkptListKey = ckptListKey;
/*
     Algorithm:
        1)Validations
            -check the client is already intialized
            -if not, return CL_CKPT_ERR_NOT_INITIALIZED
            -check, the given checkpoint is availble
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
            -check, the given dataset is available
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
        2)Writing
            -Create the DBkey
            -insert the record into DB using DBkey
        3)Done
                                                 
 */
    /*Validations*/      
    CKPT_NULL_CHECK_HDL (ckptHdl);
    CKPT_NULL_CHECK (pCkptName);
    if(keyLen == 0) return CL_CKPT_ERR_INVALID_PARAMETER;
    pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl;
    CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);
    CKPT_LOCK (pCkptClnt->ckptSem);
      
    /*choosing the checkpoint*/ 
   rc = clCntDataForKeyGet (pCkptClnt->ckptList,
                            (ClCntKeyHandleT)pCkptListKey,
                            (ClCntDataHandleT *)&pCkpt);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt:Invalid Checkpoint Name rc[0x %x]\n",rc),rc);
    /*choosing the dataset*/
    rc = clCntDataForKeyGet ( pCkpt->dataSetList,
                             (ClCntKeyHandleT)(ClWordT)dsId,
                            (ClCntDataHandleT *)&pDataInfo);
    if (rc != CL_OK)
    {
      rc = CL_CKPT_ERR_NOT_EXIST;
      CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
              ("Ckpt: The given dataset is not exists rc[0x %x]\n",rc), rc);
    }
        
   /*writing*/
    if(pDataInfo != NULL)
    {
        rc = clCkptDataSetSerialize(pVersion, pDataInfo, cookie, 
                                    pDataInfo->pElementCallbackTable,
                                    pDataInfo->numElementTableEntries);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                       ("Ckpt: Error during serialization rc[0x %x]\n",rc), rc);
         
       /*Creating  key*/
       pDbKey = (CkptElemKeyT *)clHeapAllocate(sizeof(CkptElemKeyT) + keyLen - 1);
       memset(pDbKey, 0, sizeof(CkptElemKeyT) + keyLen -1);   
       pDbKey->dsId    = pDataInfo->dsId;
       pDbKey->keyLen  = keyLen;
       memcpy(pDbKey->elemKey, elemKey, keyLen);
       /*Writing into DBAL*/
       rc  = clDbalRecordReplace ( pCkpt->usrDbHdl,
                                   (ClDBKeyT)pDbKey,
                                   sizeof(CkptElemKeyT) + keyLen -1,
                                   (ClDBRecordT)pDataInfo->data,
                                   pDataInfo->size);
       clHeapFree(pDbKey);
    }

    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Error during Inserting the record rc[0x %x]\n",rc), rc);
    
    /*Done*/
    if (pDataInfo != NULL)
    {
        clHeapFree (pDataInfo->data);
        pDataInfo->data = NULL;
        pDataInfo->size = 0;
    }
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return CL_OK;
exitOnError:
    {
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return rc;
    }
}

ClRcT clCkptLibraryCkptElementWrite (ClCkptSvcHdlT   ckptHdl, 
                                    ClNameT        *pCkptName, 
                                    ClUint32T       dsId, 
                                    ClPtrT          elemKey,
                                    ClUint32T       keyLen,
                                    ClPtrT       cookie)
{
    ClVersionT version = {
        .releaseCode = CL_RELEASE_VERSION,
        .majorVersion = CL_MAJOR_VERSION,
        .minorVersion = CL_MINOR_VERSION,
    };
    return clCkptLibraryCkptElementVersionWrite(ckptHdl, pCkptName, dsId,
                                                elemKey, keyLen, cookie, &version);
}

/*
 *  Function for deleting the element from the specified Dataset
 *  and removing the entry from DB also.  
 */     
                                     
ClRcT clCkptLibraryCkptElementDelete(ClCkptSvcHdlT  ckptHdl, 
                                     ClNameT        *pCkptName, 
                                     ClUint32T      dsId, 
                                     ClPtrT         elemKey,
                                     ClUint32T      keyLen)
{                                  
       
     ClnCkptT      *pCkpt     =  NULL;      /*checkpoint object*/
     CkptClnCbT    *pCkptClnt =  NULL;      /*checkpoint handle*/
     CkptDataSetT  *pDataInfo =  NULL;      /*data set */
     ClRcT         rc         =  CL_OK;     /*return code*/
     CkptElemKeyT  *pDbKey    = NULL;
     ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
     ClCharT *pCkptListKey = ckptListKey;
/*
     Algorithm:
        1)Validations
            -check the client is already intialized
            -if not, return CL_CKPT_ERR_NOT_INITIALIZED
            -check, the given checkpoint is availble
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
            -check, the given dataset is available
            -if not, return CL_CKPT_ERR_INVALID_PARAMETER
        2)Deleting
            -Create the DBkey
            -Delete the record from DB using DBkey
        3)Done
                                                 
 */
    /*Validations*/      
    CKPT_NULL_CHECK_HDL (ckptHdl);
    CKPT_NULL_CHECK (pCkptName);
    if(keyLen == 0) return CL_CKPT_ERR_INVALID_PARAMETER;
    pCkptClnt =  (CkptClnCbT *) (ClWordT)ckptHdl;
    CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);
    CKPT_LOCK (pCkptClnt->ckptSem);
      
    /*choosing the checkpoint*/ 
   rc = clCntDataForKeyGet(pCkptClnt->ckptList,
                           (ClCntKeyHandleT) pCkptListKey,
                           (ClCntDataHandleT *) &pCkpt);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt:Invalid Checkpoint Name rc[0x %x]\n",rc),rc);
    /*choosing the dataset*/
    rc = clCntDataForKeyGet(pCkpt->dataSetList,
                            (ClCntKeyHandleT) (ClWordT)dsId,
                            (ClCntDataHandleT *) &pDataInfo);
    if (rc != CL_OK)
    {
      rc = CL_CKPT_ERR_NOT_EXIST;
      CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
              ("Ckpt: The given dataset is not exists rc[0x %x]\n",rc), rc);
    }
        
   /*Deleting*/
    if(pDataInfo != NULL)
    {
        /*Creating  key*/
        pDbKey = (CkptElemKeyT *)clHeapAllocate(sizeof(CkptElemKeyT) + keyLen - 1);
        if( NULL == pDbKey )
        {
            rc = CL_CKPT_ERR_NO_MEMORY;
            CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                    ("Ckpt: No memory errorrc[0x %x]\n",rc), rc);
        }
        memset(pDbKey, 0, sizeof(CkptElemKeyT) + keyLen -1);   
        pDbKey->dsId    = pDataInfo->dsId;
        pDbKey->keyLen  = keyLen;
        memcpy(pDbKey->elemKey, elemKey, keyLen);

        /* Deleting the entry from DB */
        rc  = clDbalRecordDelete(pCkpt->usrDbHdl,
                                 (ClDBKeyT) pDbKey,
                                 sizeof(CkptElemKeyT) + keyLen -1);
        clHeapFree(pDbKey);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                ("Ckpt: Error during deleting the record rc[0x %x]\n",rc), rc);
    }

    /*Done*/
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return CL_OK;
exitOnError:
    {
    CKPT_UNLOCK (pCkptClnt->ckptSem);
    return rc;
    }
}


/* 
    Function for reading the dataset 
*/
                
ClRcT clCkptLibraryCkptDataSetRead (ClCkptSvcHdlT      ckptHdl, 
                                   ClNameT            *pCkptName, 
                                   ClUint32T           dsId, 
                                   ClPtrT           cookie)
                              
{
     ClRcT                 rc         =  CL_OK;     /*return code*/
     ClnCkptT              *pCkpt     =  NULL;      /*checkpoint object*/
     CkptClnCbT            *pCkptClnt =  NULL;      /*checkpoint handle*/
     CkptDataSetT          *pDataInfo =  NULL;      /*data set */
     ClDBRecordT           record    =  NULL;
     ClUint32T             recSize   =  0;
     CkptElemKeyT          *pDbKey   = NULL;
     CkptElemKeyT          *pTempKey = NULL; 
     ClUint32T              keySize  = 0;
     ClBoolT                foundFlag = CL_FALSE;
     ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
     ClCharT *pCkptListKey = ckptListKey;
                         
/*
        Algorithm:
            1)Validations
                -check the client is already intialized
                -if not, return CL_CKPT_ERR_NOT_INITIALIZED
            2)Reading
                -Get key from the DB
                -Extract the dsId from the DBkey
                -Check the DsId with the Given DsId
                -if equal, check it has any elements
                -if it doesn't have return the rcord to Deserialiser of DataSet and break.
                -else call the element deserialiser
             3)Done
 */ 
     
    /*Validations*/
    CKPT_NULL_CHECK_HDL (ckptHdl);
    CKPT_NULL_CHECK (pCkptName);
    pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl;

    CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);
    CKPT_LOCK (pCkptClnt->ckptSem);
    rc = clCntDataForKeyGet (pCkptClnt->ckptList,
                            (ClCntKeyHandleT)pCkptListKey,
                            (ClCntDataHandleT *)&pCkpt);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt:Invalid Checkpoint Name rc[0x %x]\n",rc),rc);
    /*choosing the dataset*/
    rc = clCntDataForKeyGet ( pCkpt->dataSetList,
                             (ClCntKeyHandleT)(ClWordT)dsId,
                            (ClCntDataHandleT *)&pDataInfo);
    if (rc != CL_OK)
    {
      rc = CL_CKPT_ERR_NOT_EXIST;
      CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
           ("Ckpt: The given dataset is not exists rc[0x %x]\n",rc), rc);
    }
    
    rc = clDbalFirstRecordGet( pCkpt->usrDbHdl,
                               (ClDBKeyT *)&pDbKey,
                               &keySize,
                               &record,
                               &recSize);
    if(rc == CL_OK)
    {
    do
    {
        if( (NULL != pDbKey) && (pDbKey->dsId == dsId) )
        {
           foundFlag = CL_TRUE;
           if(pDbKey->keyLen != 0)
           {
               if(!pDataInfo->pElementCallbackTable)
               {
                    clDbalKeyFree(pCkpt->usrDbHdl,(ClDBKeyT)pDbKey);
                    clDbalRecordFree(pCkpt->usrDbHdl,record);
                    rc = CL_CKPT_ERR_NULL_POINTER;
                    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                   ("Ckpt: Haven't registered deserialiser function rc[0x %x]\n",rc), rc);
               }
               rc = clCkptDataSetDeSerialize(dsId, record, recSize, cookie,
                                             pDataInfo->pElementCallbackTable,
                                             pDataInfo->numElementTableEntries);
           }
           else
           {
               
               /*rc = clCksm16bitCompute  ( (ClUint8T *)pDataInfo->data, pDataInfo->size,
                &checkSum);
                printf ("The Check Sum before deserialization: %d", checkSum);*/ 
               if(!pDataInfo->pDataSetCallbackTable)
                {
                    clDbalKeyFree(pCkpt->usrDbHdl,(ClDBKeyT)pDbKey);
                    clDbalRecordFree(pCkpt->usrDbHdl,record);
                    rc = CL_CKPT_ERR_NULL_POINTER;
                    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                   ("Ckpt: Haven't registered Desrialiser function rc[0x %x]\n",rc), rc);
                }
               rc = clCkptDataSetDeSerialize(dsId, record, recSize,
                                             cookie, pDataInfo->pDataSetCallbackTable,
                                             pDataInfo->numDataSetTableEntries);
                
                if (record != NULL)
                {
                    /**
                        Temporory Fix for event service alone
                    **/
       
                    ClIocPortT   evtPort = 0;
#if 1
                    clEoMyEoIocPortGet (&evtPort);
                    if  (evtPort != CL_IOC_EVENT_PORT)
                        clDbalRecordFree(pCkpt->usrDbHdl,record);
#else
                    clDbalRecordFree(pCkpt->usrDbHdl,record);
#endif       
                }
                if(pDbKey != NULL) 
                   clDbalRecordFree(pCkpt->usrDbHdl,(ClDBKeyT)pDbKey);
                break;
           }
        }
        if(record != NULL)
             clDbalRecordFree(pCkpt->usrDbHdl,record);
        pTempKey = pDbKey;
        rc =  clDbalNextRecordGet( pCkpt->usrDbHdl,
                                   (ClDBKeyT)pTempKey,
                                   keySize,
                                  (ClDBKeyT *)&pDbKey,
                                   &keySize,
                                   &record,
                                   &recSize);
        if(pTempKey != NULL) 
            clDbalRecordFree(pCkpt->usrDbHdl,(ClDBKeyT)pTempKey);
        if( rc != CL_OK)
        {
            if(foundFlag)
            {
               rc = CL_OK;
            }
            break;
        }
       
    }while(1);
   }
   else
   {
       rc = CL_CKPT_ERR_NOT_EXIST;
       goto exitOnError;
   }
   CKPT_UNLOCK (pCkptClnt->ckptSem);                                                       
   return rc;
exitOnError:
    {
        CKPT_UNLOCK (pCkptClnt->ckptSem);
        return rc;
    }
}


/*
   Function to find the ckpt is already exist or not
*/
ClRcT clCkptLibraryDoesCkptExist( ClCkptSvcHdlT      ckptHdl, 
                                  ClNameT           *pCkptName, 
                                  ClBoolT           *pRetVal)
{
    CkptClnCbT      *pCkptClnt   = NULL;
    ClCntNodeHandleT  nodeHdl    = 0; 
    ClRcT             rc         = CL_OK;   
    ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *pCkptListKey = ckptListKey;

    /*Validations*/
    CKPT_NULL_CHECK_HDL (ckptHdl);
    CKPT_NULL_CHECK (pCkptName);
    CKPT_NULL_CHECK (pRetVal);
    pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl;

    pCkptListKey = ckptListKey;
    CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);

    CKPT_LOCK (pCkptClnt->ckptSem);
    clLogDebug("DBAL", "FIND", "Looking up ckpt [%s] with key [%s]", 
               pCkptName->value, pCkptListKey);
    rc = clCntNodeFind ( pCkptClnt->ckptList,
                         (ClCntKeyHandleT)pCkptListKey,
                         &nodeHdl);
    if(rc == CL_OK)
    {
        ClnCkptT *pCkpt = NULL;
        rc = clCntNodeUserDataGet(pCkptClnt->ckptList, nodeHdl, (ClCntDataHandleT*)&pCkpt);
        if(rc != CL_OK)
        {
            *pRetVal = CL_FALSE;
        }
        else
        {
            *pRetVal = CL_TRUE;
            if(pCkpt && !pCkpt->usrDbHdl)
            {
                ClCharT  tempDb[CL_MAX_NAME_LENGTH];
                CL_CKPT_APP_NAME_FORM(tempDb, pCkptName->value);
                /* Open the file in APPEND mode */
                clLogDebug("DBAL", "CHECK", "Opening the dbal ckpt at [%s]", tempDb);
                rc   = clDbalOpen (tempDb,
                                   tempDb, 
                                   CL_DB_APPEND, 
                                   CKPT_MAX_NUMBER_RECORD,
                                   CKPT_MAX_SIZE_RECORD, 
                                   &pCkpt->usrDbHdl);
                if(rc != CL_OK)
                {
                    clLogError("DBAL", "CHECK", "DBAL open failed with [%#x]", rc);
                    *pRetVal = CL_FALSE;
                }
            }
        }
    }
    else
        *pRetVal = CL_FALSE;
    CKPT_UNLOCK(pCkptClnt->ckptSem);
    return CL_OK;
}


/*
   Function to find the DataSet is already exist or not
*/
ClRcT clCkptLibraryDoesDatasetExist( ClCkptSvcHdlT    ckptHdl, 
                                     ClNameT            *pCkptName, 
                                     ClUint32T           dsId,
                                     ClBoolT            *pRetVal)
{
    CkptClnCbT      *pCkptClnt   = NULL;
    ClnCkptT        *pCkpt       = NULL;
    ClCntNodeHandleT  nodeHdl    = 0; 
    ClRcT             rc         = CL_OK;   
    ClCharT ckptListKey[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *pCkptListKey = ckptListKey;

    /*Validations*/
    CKPT_NULL_CHECK_HDL (ckptHdl);
    CKPT_NULL_CHECK (pCkptName);
    CKPT_NULL_CHECK (pRetVal);
    pCkptClnt =  (CkptClnCbT *)(ClWordT)ckptHdl;

    CKPT_LIST_GET_KEY(pCkptListKey, pCkptName);
    CKPT_LOCK (pCkptClnt->ckptSem);
    rc = clCntDataForKeyGet (pCkptClnt->ckptList,
                             (ClCntKeyHandleT)pCkptListKey,
                             (ClCntDataHandleT *)&pCkpt);
    if(rc == CL_OK)
    {
        rc = clCntNodeFind ( pCkpt->dataSetList,
                         (ClCntKeyHandleT)(ClWordT)dsId,
                          &nodeHdl);
        if (rc == CL_OK)
        {
              *pRetVal = CL_TRUE;
               CKPT_UNLOCK(pCkptClnt->ckptSem);
               return CL_OK;
         }
    }
    *pRetVal = CL_FALSE;
    clLogDebug("DBAL", "DATACHECK", "Dataset [%d] doesnt exist for checkpoint [%.*s]", dsId, 
               pCkptName->length, pCkptName->value);
               
    CKPT_UNLOCK(pCkptClnt->ckptSem);
    return CL_OK;
}


/*
   Function to pack all parameteres of Checkpoint
*/
ClRcT clCkptPack (CkptClnCbT  *pCkptClnt, 
                 ClCharT     *pBuffer)
                 
{
    ClnCkptT        *pCkpt         = NULL;     
    ClUint32T        nodeCount     = 0 ;
    ClUint32T        dsCount       = 0 ; 
    CkptDataSetT    *pDataInfo     = NULL;
    ClRcT            rc            = CL_OK;
    ClCntNodeHandleT dsNodeHdl     = 0;
    ClCntNodeHandleT nodeHdl       = 0;
    ClCntDataHandleT dataHdl       = 0; 

    memcpy (pBuffer,  (ClCharT*)pCkptClnt, sizeof (CkptClnCbT));
    pBuffer = pBuffer + sizeof (CkptClnCbT);
    
    clCntSizeGet(pCkptClnt->ckptList,&nodeCount); 
    memcpy(pBuffer,(ClCharT *)&nodeCount,sizeof(ClUint32T));
    pBuffer = pBuffer + sizeof (ClUint32T);
    
    clCntFirstNodeGet(pCkptClnt->ckptList,&nodeHdl);
    while(nodeHdl != 0)
    {
          rc = clCntNodeUserDataGet( pCkptClnt->ckptList,
                                     nodeHdl,
                                     &dataHdl);
          CKPT_ERR_CHECK( CL_CKPT_LIB,CL_DEBUG_ERROR,
                  ("Error in getting Data rc[0x %x]\n",rc), rc);
                                                                                     
          pCkpt = (ClnCkptT *)dataHdl;                     
          if (pCkpt != NULL)
          {
              memcpy (pBuffer,  (ClCharT *)pCkpt, sizeof (ClnCkptT));
              pBuffer = pBuffer + sizeof (ClnCkptT);
              if (pCkpt->pCkptName != NULL)
              {
                memcpy (pBuffer,(ClCharT *)pCkpt->pCkptName, sizeof (ClNameT));
                pBuffer = pBuffer + sizeof (ClNameT);
                dsCount = 0;
                clCntSizeGet( pCkpt->dataSetList,
                              &dsCount); 
                memcpy (pBuffer,  (ClCharT *)&dsCount, sizeof (ClUint32T));
                pBuffer = pBuffer + sizeof (ClUint32T);
                clCntFirstNodeGet(pCkpt->dataSetList,&dsNodeHdl);
                while(dsNodeHdl != 0)
                {
                    rc = clCntNodeUserDataGet( pCkpt->dataSetList,
                                               dsNodeHdl,
                                               &dataHdl);
                     CKPT_ERR_CHECK( CL_CKPT_LIB,CL_DEBUG_ERROR,
                             ("Error in getting Data rc[0x %x]\n",rc), rc);
                     pDataInfo = (CkptDataSetT *)dataHdl;
                     if (pDataInfo != NULL)
                     {
                         CkptDataSetT tempDataSet = {0};
                         memcpy(&tempDataSet, pDataInfo, sizeof(tempDataSet));
                         tempDataSet.pDataSetCallbackTable = NULL;
                         tempDataSet.pElementCallbackTable = NULL;
                         tempDataSet.numDataSetTableEntries = 0;
                         tempDataSet.numElementTableEntries = 0;
                         tempDataSet.data = NULL;
                         tempDataSet.size = 0;
                         memcpy (pBuffer, (ClCharT *)&tempDataSet,
                                 sizeof (CkptDataSetT));
                         pBuffer = pBuffer + sizeof (CkptDataSetT);
                     }
                     clCntNextNodeGet( pCkpt->dataSetList,
                                       dsNodeHdl,
                                       &dsNodeHdl);         
                 }
                 
              }
          }
          clCntNextNodeGet(pCkptClnt->ckptList, nodeHdl, &nodeHdl);
    }            
exitOnError:
   return rc; 
}  


/* This function for Unpacking the parameters of ckpt*/       
ClRcT clCkptUnpack (ClDBRecordT  *pBuffer, 
                    ClUint32T    length, 
                    CkptClnCbT  **pCkptClnt)
{
    ClnCkptT      *pCkpt      =  NULL;
    CkptDataSetT  *pDataInfo  =  NULL; 
    ClUint32T      count      =  0;
    ClUint32T      dSetCount  =  0;
    ClUint32T      ckptCount  =  0;
    ClUint32T      dsCount    =  0;
    ClCharT       *dataHdl    =  NULL;
    CkptClnCbT    *pTempClnt  =  NULL;
    ClRcT          rc         =  CL_OK;

    CKPT_NULL_CHECK (pBuffer);
    dataHdl =  (ClCharT*)*pBuffer;    
    /*building client control block*/
    pTempClnt =  (CkptClnCbT *)clHeapAllocate (sizeof (CkptClnCbT));
    if (pTempClnt == NULL)
    {
        CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                        ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        return CL_CKPT_ERR_NO_MEMORY;
    }  
    memset (pTempClnt, 0, sizeof (CkptClnCbT));
    memcpy (pTempClnt, dataHdl, sizeof (CkptClnCbT));
    dataHdl = dataHdl + sizeof (CkptClnCbT);
     
    clOsalMutexCreate (&pTempClnt->ckptSem);
    rc = clCntLlistCreate (   ckptCheckptCompare,
                              ckptCheckptDeleteCallback,
                              ckptCheckptDeleteCallback,
                              CL_CNT_UNIQUE_KEY,
                              &pTempClnt->ckptList);
    if(rc != CL_OK)
    {
        if(pTempClnt != NULL) clHeapFree(pTempClnt);  
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                       ("Ckpt:Error during creating linked list for datasets rc[0x %x]\n",rc),
                       rc);
    }
    memcpy(&ckptCount,dataHdl, sizeof(ClUint32T));
    dataHdl = dataHdl + sizeof (ClUint32T);
    for(count = 0 ; count < ckptCount ; count++)
    {
        ClCharT *pCkptListKey = NULL;
        pCkpt = (ClnCkptT *)clHeapCalloc(1, sizeof(ClnCkptT));
        if(pCkpt == NULL)
        {
            CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                            ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
            clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            return CL_CKPT_ERR_NO_MEMORY;
        }
        memcpy(pCkpt ,dataHdl ,sizeof(ClnCkptT));
        pCkpt->usrDbHdl = 0;
        dataHdl = dataHdl + sizeof(ClnCkptT);
        if(pCkpt->pCkptName != NULL)
        {
            pCkpt->pCkptName  =  (ClNameT *)clHeapCalloc (1, sizeof (ClNameT));
            if (pCkpt->pCkptName == NULL)
            {
                clHeapFree(pCkpt);
                CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                                ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
                clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                return CL_CKPT_ERR_NO_MEMORY;
            }        
            memset (pCkpt->pCkptName, 0, sizeof (ClNameT));
            memcpy(pCkpt->pCkptName,dataHdl,sizeof(ClNameT));

            if(pCkpt->pCkptName->length >= sizeof(pCkpt->pCkptName->value))
                pCkpt->pCkptName->length = sizeof(pCkpt->pCkptName->value)-1;

            pCkpt->pCkptName->value[pCkpt->pCkptName->length] = 0;

            dataHdl = dataHdl + sizeof(ClNameT);

            /* 
             * While opening the dbal later, make sure that the key is the basename
             * on which the existence of db is going to be checked. 
             */
            CKPT_LIST_GET_KEY(pCkptListKey, pCkpt->pCkptName);

            /*Disk based check pointing*/
            if(pCkpt->dataSetList != 0)
            {
                rc = clCntLlistCreate (ckptDsetCompare,
                                       ckptDataSetDeleteCallback,
                                       ckptDataSetDeleteCallback,
                                       CL_CNT_UNIQUE_KEY,
                                       &pCkpt->dataSetList);
                if (rc != CL_OK)
                {
                    if (pCkpt!=NULL)
                    {
                        if (pCkpt->pCkptName != NULL)
                        {
                            clHeapFree(pCkpt->pCkptName);
                            pCkpt->pCkptName = NULL;
                        }
                    }
                    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                                   ("Ckpt: Error during creating linked list"
                                    "for datasets rc[0x %x]\n",rc),
                                   rc);
        
                } 
                rc = clCntNodeAdd (pTempClnt->ckptList,
                                   (ClCntKeyHandleT)pCkptListKey,
                                   (ClCntDataHandleT)pCkpt,
                                   NULL);
                CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                               ("Ckpt: Error during adding ckpt rc[0x %x]\n",rc), rc);
            }
        }
        dSetCount = 0;
        memcpy(&dSetCount,dataHdl,sizeof(ClUint32T));
        dataHdl = dataHdl + sizeof(ClUint32T);
        for(dsCount = 0 ;dsCount < dSetCount ;dsCount++)
        {
            pDataInfo = (CkptDataSetT *)clHeapAllocate (sizeof (CkptDataSetT));
            if (pDataInfo  == NULL)
            {
                if (pCkpt->pCkptName != NULL)
                {
                    clHeapFree (pCkpt->pCkptName);
                    pCkpt->pCkptName = NULL;
                }
                CL_DEBUG_PRINT (CL_DEBUG_CRITICAL, 
                                ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
                clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                return CL_CKPT_ERR_NO_MEMORY;
            }             
            memcpy (pDataInfo, dataHdl, sizeof (CkptDataSetT));
            rc = clCntNodeAdd (pCkpt->dataSetList, 
                               (ClCntKeyHandleT)(ClWordT)pDataInfo->dsId,
                               (ClCntDataHandleT)pDataInfo, NULL);
            CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                           ("Ckpt:Error during Adding dataset to a linked list rc[0x %x]\n",rc),
                           rc);
            dataHdl = dataHdl + sizeof (CkptDataSetT);
        }
    }   
    *pCkptClnt = (CkptClnCbT*)pTempClnt;
    exitOnError:
    return rc;
}     
                                   
                                   
ClRcT clCkptBufferAlloc (CkptClnCbT *pCkptClnt,
                        ClCharT **pBuffer, 
                        ClUint32T *pLen)
{
   ClUint32T  totalSize = 0;
   ClRcT       rc       = CL_OK; 
   ClCharT     *pTemp   = NULL;
   ClUint32T    nodeCount = 0;
     
   totalSize = sizeof (CkptClnCbT) + sizeof(ClUint32T);
   if(pCkptClnt->ckptList != 0)
   {
      rc = clCntSizeGet( pCkptClnt->ckptList,&nodeCount);
      CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
              ("Ckpt: Error in getting size from Container rc[0x %x]\n",rc),rc);
   }
   totalSize = totalSize + nodeCount * sizeof(ClnCkptT) +
                           nodeCount * sizeof(ClNameT) +
                           nodeCount * sizeof(ClUint32T);
   
  rc = clCntWalk(pCkptClnt->ckptList,
                 ckptPackSizeGetCallback,
                 (ClCntArgHandleT)&totalSize,
                 sizeof(ClUint32T *));

                  
  pTemp =  (ClCharT*)clHeapCalloc (1, totalSize);
   if (pTemp == NULL)
   {
      CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
              ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
      clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                 CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
      return CL_CKPT_ERR_NO_MEMORY;
   }
   *pBuffer = pTemp;
   *pLen    = totalSize;
exitOnError:
   return rc;
}  

                          
void clCkptBufferFree (ClCharT *pBuffer)
{
    clHeapFree (pBuffer);
}

ClRcT ckptPackSizeGetCallback(
            ClCntKeyHandleT   userKey,
            ClCntDataHandleT  userData,
            ClCntArgHandleT   userArg,
            ClUint32T         dataLength)
{
    ClnCkptT *pCkpt = NULL;
    ClUint32T  nodeCount = 0;
    ClUint32T  *pTotalSize =  (ClUint32T* ) userArg;
    ClRcT       rc = CL_OK;
    
    pCkpt = (ClnCkptT *)userData;
    if(pCkpt != NULL)
        if(pCkpt->dataSetList != 0)
            rc = clCntSizeGet(pCkpt->dataSetList, &nodeCount);
    *pTotalSize = *pTotalSize + nodeCount * sizeof(CkptDataSetT);
    return CL_OK;
}

