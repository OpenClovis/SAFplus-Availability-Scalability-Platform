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
/*******************************************************************************
 * ModuleName  : ckpt
 * File        : clCkptDataBackup.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * File        : clCkptDataBackup.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * related to checkpointing the metadata and created checkpoints
 * (writing/reading from persistent
 * memory).
 *****************************************************************************/

#include "stdio.h"
#include "string.h"
#include "clCommon.h"
#include "clEoApi.h"
#include "clCommonErrors.h"
#include "clTimerApi.h"
#include "clRmdApi.h"
#include "clDebugApi.h"
#include "clCpmApi.h"
#include "clCksmApi.h"
#include "clLogApi.h"
#include "clCkptExtApi.h"
#include "clCkptMaster.h"

extern ClRcT ckptMasterDatabasePack(ClBufferHandleT  outMsg);
extern ClRcT ckptDbPack();
ClCkptSvcHdlT          gCkptHandle;
ClOsalMutexIdT         gMutex = NULL;
ClTimerHandleT         gTimerHdl;

#define CL_CKPT_MASTERDB_CKPT_NAME  "Cl_CkptMasterDB_Point"
#define CL_CKPT_CHECKPOINTS         2
#define CL_CKPT_METADATA            1

/* Checkpoint name */
ClNameT gCkptName = {sizeof(CL_CKPT_MASTERDB_CKPT_NAME),
                     CL_CKPT_MASTERDB_CKPT_NAME};


ClRcT ckptMetaDataSerializer(ClUint32T dataSetID, ClAddrT* ppData,
                        ClUint32T* pDataLen, ClPtrT pCookie);

ClRcT ckptMetaDataDeserializer(ClUint32T dataSetID, ClAddrT pData,
                       ClUint32T dataLen, ClPtrT pCookie);

ClRcT ckptCheckpointsSerializer(ClUint32T dataSetID, ClAddrT* ppData,
                        ClUint32T* pDataLen, ClPtrT pCookie);

ClRcT ckptCheckpointsDeserializer(ClUint32T dataSetID, ClAddrT pData,
                       ClUint32T dataLen, ClPtrT pCookie);



/**
 *  Name: ckptDataPeriodicSave
 *
 *  This function periodically writes into the persistent memory
 *
 *  @param  none
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *
 */

ClRcT ckptDataPeriodicSave()
{
    ClRcT rc = CL_OK;

    /* Write to metadata dataset */
    rc = clCkptLibraryCkptDataSetWrite(gCkptHandle, &gCkptName,
                CL_CKPT_METADATA, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nCKPT DataSet Write Failed, "
                                       "rc = %x \n", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_LOG_MESSAGE_1_CKPT_WRITE_FAILED, rc);

        CL_FUNC_EXIT();
    }

    /* Wriet to checkpoint info dataset */
    rc = clCkptLibraryCkptDataSetWrite(gCkptHandle, &gCkptName,
                CL_CKPT_CHECKPOINTS, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nCKPT DataSet Write Failed, "
                                       "rc = %x \n", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_LOG_MESSAGE_1_CKPT_WRITE_FAILED, rc);

        CL_FUNC_EXIT();
    }
    return rc;
}



/**
 *  Name: ckptDataBackupInitialize
 *
 *  Function for initializing ckpt library, creating data ckeckpoints,
 *  creating datasets and starting the timer for periodic backup.
 *  "pFlag" will carry back the info whether db files are there or not.
 *
 *  @param  none
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *
 */

ClRcT ckptDataBackupInitialize(ClUint8T *pFlag)
{
    ClRcT           rc      = CL_OK;
    ClBoolT         retVal  = CL_TRUE;
    ClTimerTimeOutT timeOut = {0};
    
    CL_FUNC_ENTER();

    /* Create the mutex for controlling access to global message handles */
    clOsalMutexCreate(&gMutex);
    CL_ASSERT(gMutex);
    
    /* Initialize the Check Point Service Library */
    rc = clCkptLibraryInitialize(&gCkptHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n CKPT Library Initialize Failed, "
                                       "rc = %x \n", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    /* Check whether db files are present or not and set the 
     * flag accordingly*/
    rc = clCkptLibraryDoesCkptExist(gCkptHandle, &gCkptName, &retVal);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Ckpt: CKPT Initialize Failed,"
                                       " rc = %x \n", rc));
        CL_FUNC_EXIT();
        goto label4;
    }
    else
    {
        if(retVal == CL_TRUE)
            *pFlag = 1;
        else
            *pFlag = 0;
    }

    /* Create a checkpoint to store both checkpoints and metadata needed by
     * master ckpt server */
    rc = clCkptLibraryCkptCreate(gCkptHandle, &gCkptName); 
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n CKPT Check Point Create Failed, "
                                       "rc = %x \n",
                             rc));
        goto label4;
    }

    /* Create a deta set to store checkpoints' related metadata */
    rc =  clCkptLibraryCkptDataSetCreate(gCkptHandle, &gCkptName,
                        CL_CKPT_METADATA, 
                        0, 0,
                        ckptMetaDataSerializer,
                        ckptMetaDataDeserializer
                        );
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n CKPT Data Set Create Failed, "
                                       "rc = %x \n", rc));
        goto label3;
    }

    /* Create a dataset to store checkpoints' info */
    rc =  clCkptLibraryCkptDataSetCreate(gCkptHandle, &gCkptName,
                        CL_CKPT_CHECKPOINTS, 
                        0, 0,
                        ckptCheckpointsSerializer,
                        ckptCheckpointsDeserializer
                        );
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n CKPT Data Set Create Failed, "
                                       "rc = %x \n", rc));
        goto label2;
    }
                                                                                                                             
    /* Start the periodic timer */
    timeOut.tsSec      = 5;
    timeOut.tsMilliSec = 0;
    rc = clTimerCreateAndStart(timeOut, CL_TIMER_REPETITIVE,
                    CL_TIMER_SEPARATE_CONTEXT, ckptDataPeriodicSave,
                    (void*)NULL, &gTimerHdl);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n CKPT Data Set Create Failed, "
                                       "rc = %x \n", rc));
        goto label1;
    }
    
    goto label0;
    
    /* Cleanup part */
label1:
    clCkptLibraryCkptDataSetDelete(gCkptHandle,
                                   &gCkptName,
                                   CL_CKPT_CHECKPOINTS);
label2:
    clCkptLibraryCkptDataSetDelete(gCkptHandle,
                                   &gCkptName,
                                   CL_CKPT_METADATA);
label3:
    clCkptLibraryCkptDelete(gCkptHandle, &gCkptName);
label4:
    clCkptLibraryFinalize(gCkptHandle);
label0:
    clDbgResourceNotify(clDbgComponentResource, clDbgAllocate, 0, CL_CID_CKPT, ("Checkpoint data backup library initialized"));

    CL_FUNC_EXIT();                                                                                                                             
    return CL_OK;
}


/**
 *  Name: ckptMetaDataSerializer 
 *
 *  Serializer function for ckpt meta data 
 *
 *  @param  none
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *    CL_ERR_INVALID_PARAMETER - Improper dataset id
 *
 */

ClRcT ckptMetaDataSerializer(ClUint32T dataSetID, ClAddrT* ppData,
                        ClUint32T* pDataLen, ClPtrT pCookie)
{
    ClRcT                  rc        = CL_OK;
    ClBufferHandleT outMsgHdl = 0;
                                                                                                                             
    CL_FUNC_ENTER();
    /* take the semaphore */
    clOsalMutexLock(gMutex); 

    rc = clBufferCreate(&outMsgHdl);
    if(rc == CL_OK)
    {
        rc = clBufferLengthGet(outMsgHdl, pDataLen);
        if(rc == CL_OK)
        {
            *ppData = (ClAddrT)clHeapAllocate(*pDataLen);
            rc = clBufferNBytesRead(outMsgHdl, 
                            (ClUint8T*)*ppData, pDataLen); 
            if(rc != CL_OK)
            {
                 /* Release the semaphore */
                 clOsalMutexUnlock(gMutex);
                 return rc;
            }
        }
        else
        {
            /* Release the semaphore */
            clOsalMutexUnlock(gMutex);
            return rc;
        }
    }
    else
    {
         /* Release the semaphore */
         clOsalMutexUnlock(gMutex);
         return rc;
    }
    rc = clBufferDelete(&outMsgHdl);

    /* Release the semaphore */
    clOsalMutexUnlock(gMutex);
    CL_FUNC_EXIT();
    return CL_OK;
}
                                                                                                                             

/**
 *  Name: ckptCheckpointsSerializer 
 *
 *  Serializer function for checkpoints
 *
 *  @param  none
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *
 */

ClRcT ckptCheckpointsSerializer(ClUint32T dataSetID, ClAddrT* ppData,
                        ClUint32T* pDataLen, ClPtrT pCookie)
{
    ClRcT                  rc        = CL_OK;
    ClBufferHandleT outMsgHdl = 0;
    
    CL_FUNC_ENTER();
    
    /* take the semaphore */
    clOsalMutexLock(gMutex);
    
    clBufferCreate(&outMsgHdl);

    /* Pack and serialize the checkpoint info */
    rc = ckptDbPack(&outMsgHdl,0);
    if(rc == CL_OK)
    {
        rc = clBufferLengthGet(outMsgHdl, pDataLen);
        if(rc == CL_OK)
        {
            *ppData = (ClAddrT)clHeapAllocate(*pDataLen);
            rc = clBufferNBytesRead(outMsgHdl, 
                            (ClUint8T*)*ppData, pDataLen); 
            if(rc != CL_OK)
            {
                 /* Release the semaphore */
                 clBufferDelete(&outMsgHdl);
                 clOsalMutexUnlock(gMutex);
                 return rc;
            }
        }
        else
        {
            /* Release the semaphore */
            clBufferDelete(&outMsgHdl);
            clOsalMutexUnlock(gMutex);
            return rc;
        }
    }
    else
    {
         /* Release the semaphore */
         clBufferDelete(&outMsgHdl);
         clOsalMutexUnlock(gMutex);
         return rc;
    }
    rc = clBufferDelete(&outMsgHdl);

    /* Release the semaphore */
    clOsalMutexUnlock(gMutex);
    CL_FUNC_EXIT();
    return CL_OK;
}


                                                                                                                             
/**
 *  Name: ckptMetaDataDeserializer 
 *
 *  Deserializer function for ckpt meta data
 *
 *  @param  none
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *
 */

ClRcT ckptMetaDataDeserializer(ClUint32T dataSetID, ClAddrT pData,
                            ClUint32T dataLen, ClPtrT pCookie)
{
    ClRcT                  rc     = CL_OK; 
    ClBufferHandleT msgHdl = 0;
      
    rc = clBufferCreate (&msgHdl);
    if(rc != CL_OK)
    {
        return rc;
    }
    
    /* Deserialize the metadata info stored in the persistent memory */
    rc = clBufferNBytesWrite(msgHdl, (ClUint8T*)pData, 
                                    (ClUint32T) (dataLen));
    if(CL_OK != rc) {
        clBufferDelete(&msgHdl); 
        return(rc);
    }

#if 0
    /* Unpack the deserialized checkpoint metadata */
    rc =  ckptMasterDatabaseUnpack(msgHdl);
#endif    
    clBufferDelete(&msgHdl); 
    CL_FUNC_EXIT();
    return rc;
}                                                                                                                           



/**
 *  Name: ckptCheckpointsDeserializer 
 *
 *  Deserializer function for checkpoints
 *
 *  @param  none
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *
 */

ClRcT ckptCheckpointsDeserializer(ClUint32T dataSetID, ClAddrT pData,
                            ClUint32T dataLen, ClPtrT pCookie)
{
    ClRcT                  rc     = CL_OK; 
    ClBufferHandleT msgHdl = 0;
    
    rc = clBufferCreate (&msgHdl);
    if(rc != CL_OK)
        return rc;

    /* Deserialize the checkpoint info stored in the persistent memory */
    rc = clBufferNBytesWrite(msgHdl, (ClUint8T*)pData, 
                                    (ClUint32T) (dataLen));
    if(CL_OK != rc) {
        clBufferDelete(&msgHdl); 
        return(rc);
    }

    clBufferDelete(&msgHdl); 
    
    CL_FUNC_ENTER();
    return rc;
}                                                                                                                           


/**
 *  Name: ckptDataBackupFinalize
 *
 *  This function deletes the datasets, the checkpoint and finalizes the
 *  ckpt  library
 *
 *  @param  none
 *
 *  @returns none
 *
 */
  
void ckptDataBackupFinalize(void)
{
    CL_FUNC_ENTER();
    /* take the semaphore */
    if (gMutex)
      {
        clDbgResourceNotify(clDbgComponentResource, clDbgRelease, 0, CL_CID_CKPT, ("Checkpoint data backup library shut down"));

        clOsalMutexLock(gMutex); 
        /* Delete the Data Sets */
        clCkptLibraryCkptDataSetDelete(gCkptHandle,
                                       &gCkptName,
                                       CL_CKPT_METADATA);

        clCkptLibraryCkptDataSetDelete(gCkptHandle,
                                       &gCkptName,
                                       CL_CKPT_CHECKPOINTS);
        /* Delete the timer */
        clTimerDelete(&gTimerHdl);
        /* Delete the Check Point */
        clCkptLibraryCkptDelete(gCkptHandle, &gCkptName);
        /* Finalize the Check Point Service Library */
        clCkptLibraryFinalize(gCkptHandle);

        /* Release the semaphore */
        clOsalMutexUnlock(gMutex);
        clOsalMutexDelete(gMutex);
      }
    else
      {
        /* Passing OK to the error because calling finalize 2x is actually ok */
        clDbgCodeError(CL_OK, ("Double call to checkpoint data backup finalize, or initialize never called"));
      }

    CL_FUNC_EXIT();
    return;
}



/**
 *  Name: ckptDataBackupInit DEPRECATED use ckptDataBackupInitialize
 *
 *  Function for initializing ckpt library and creating the buffers 
 *  carrying the info to be packed.  DEPRECATED use ckptDataBackupInitialize
 *
 *  @param  none
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *
 */

ClRcT ckptDataBackupInit(ClUint8T *pFlag)
{
    ClRcT rc = CL_OK;
    CL_FUNC_ENTER();
    rc = ckptDataBackupInitialize(pFlag);
    CL_FUNC_EXIT();
    return rc;
}



/**
 *  Name: ckptPersistentMemoryRead
 *
 *  Function for reading from persistent memory  
 *
 *  @param  none
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *
 */

ClRcT ckptPersistentMemoryRead()
{
    ClRcT   rc     = CL_OK;
                                                                                                                             
    CL_FUNC_ENTER();

    /* take the semaphore */
    if(clOsalMutexLock(gMutex)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n Ckpt: Could not get Lock \n"));
    }

    /* Read checkpoint meta data */
    rc = clCkptLibraryCkptDataSetRead(gCkptHandle, &gCkptName,
           CL_CKPT_METADATA, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nCKPT DataSet Read Failed, "
                                       "rc = %x \n", rc));
        /* Release the semaphore */
        clOsalMutexUnlock(gMutex);
        CL_FUNC_EXIT();
        return rc;
    }

    /* Read checkpoints */
    rc = clCkptLibraryCkptDataSetRead(gCkptHandle, &gCkptName,
           CL_CKPT_CHECKPOINTS, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nCKPT DataSet Read Failed, "
                                       "rc = %x \n", rc));
        /* Release the semaphore */
        clOsalMutexUnlock(gMutex);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    /* Release the semaphore */
    clOsalMutexUnlock(gMutex);
    return CL_OK;
}
