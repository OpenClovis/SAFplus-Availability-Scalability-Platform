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
#include "clCkptSvrIpi.h"


static ClInt32T gClCkptPersistentDBDisabled = -1;
static ClBoolT gClCkptDBInitialized = CL_FALSE;
//extern ClRcT ckptMasterDatabasePack(ClBufferHandleT  outMsg);
//extern ClRcT ckptDbPack();
static  ClCkptSvcHdlT         gCkptHandle;
static ClOsalMutexIdT         gMutex;
//ClTimerHandleT         gTimerHdl;

#define CL_CKPT_MASTERDB_CKPT_NAME  "Cl_CkptMasterDB_Point"
//#define CL_CKPT_CHECKPOINTS         2
#define CL_CKPT_METADATA            1

/* Checkpoint name */
ClNameT gCkptName = {sizeof(CL_CKPT_MASTERDB_CKPT_NAME),
                     CL_CKPT_MASTERDB_CKPT_NAME};


ClRcT ckptDataBackupInitialize(ClUint8T *pFlag)
{
    ClRcT           rc      = CL_OK;
    ClBoolT         retVal  = CL_TRUE;
    //ClTimerTimeOutT timeOut = {0};
    
    CL_FUNC_ENTER();

    /* Create the mutex for controlling access to global message handles */
    /*rc = clOsalMutexCreate(&gMutex);
    if (rc != CL_OK)
    {
    	clLogError("PERSISTENT", "DB", "cannot create mutex. rc [%x]", rc);
    	goto out;
    }
    clLogDebug("PERSISTENT", "DB", "mutex created ok rc [%x]", rc);*/

    gClCkptPersistentDBDisabled = (ClInt32T)clParseEnvBoolean("CL_CKPT_PERSISTENT_DB_DISABLED");

        if(gClCkptPersistentDBDisabled)
        {
            clLogNotice("PERSISTENT", "DB", "CKPT persistent db disabled");
            return CL_CKPT_RC(CL_ERR_NOT_EXIST);
        }

        if(gClCkptDBInitialized == CL_TRUE) return CL_OK;

    /* Initialize the Check Point Service Library */
    rc = clCkptLibraryInitialize(&gCkptHandle);
    if(CL_OK != rc)
    {
        clLogError("PERSISTENT", "INIT", "CKPT Library Initialize Failed rc = %x", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    /* Check whether db files are present or not and set the 
     * flag accordingly*/
    rc = clCkptLibraryDoesCkptExist(gCkptHandle, &gCkptName, &retVal);
    if(rc != CL_OK)
    {
    	clLogError("PERSISTENT", "INIT", "Ckpt: CKPT Initialize Failed,rc = %x", rc);
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
    	clLogError("PERSISTENT", "INIT", "CKPT Check Point Create Failed, rc = %x", rc);
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
    	clLogError("PERSISTENT", "INIT", "CKPT Data Set Create Failed, rc = %x", rc);
        goto label3;
    }
    gClCkptDBInitialized = CL_TRUE;
    clLogNotice("PERSISTENT", "INIT", "CKPT Library Initialize successfully");
    return rc;
    
label3:
    clCkptLibraryCkptDelete(gCkptHandle, &gCkptName);
label4:
    clCkptLibraryFinalize(gCkptHandle);
//label0:
    clDbgResourceNotify(clDbgComponentResource, clDbgAllocate, 0, CL_CID_CKPT, ("Checkpoint data backup library initialized"));

    CL_FUNC_EXIT();
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
    clLogDebug("HUNG80","---","enter %s", __FUNCTION__);
#if 1
    /* take the semaphore */
    //if (gMutex)
    //  {
        clDbgResourceNotify(clDbgComponentResource, clDbgRelease, 0, CL_CID_CKPT, ("Checkpoint data backup library shut down"));

        clOsalMutexLock(gMutex); 
        /* Delete the Data Sets */
#if 0
        clCkptLibraryCkptDataSetDelete(gCkptHandle,
                                       &gCkptName,
                                       CL_CKPT_METADATA);
#endif

//        clCkptLibraryCkptDataSetDelete(gCkptHandle,
//                                       &gCkptName,
//                                       CL_CKPT_CHECKPOINTS);
        /* Delete the timer */
        //clTimerDelete(&gTimerHdl);
        /* Delete the Check Point */
        clCkptLibraryCkptDelete(gCkptHandle, &gCkptName);
        /* Finalize the Check Point Service Library */
        clCkptLibraryFinalize(gCkptHandle);

        /* Release the semaphore */
        //clOsalMutexUnlock(gMutex);
        //clOsalMutexDelete(gMutex);
   //   }
    //else
    //  {
        /* Passing OK to the error because calling finalize 2x is actually ok */
   //     clDbgCodeError(CL_OK, ("Double call to checkpoint data backup finalize, or initialize never called"));
    //  }

    CL_FUNC_EXIT();
#endif
    clLogDebug("HUNG80","---", "leave %s", __FUNCTION__);
    return;
}

ClRcT ckptPersistentMemoryWrite()
{
	ClRcT rc = CL_OK;
	static ClRcT dbInitialized = CL_OK;
    ClUint8T flag;
    /*rc = clOsalMutexCreate(&gMutex);
        if (rc != CL_OK)
        {
        	clLogError("PERSISTENT", "DB", "cannot create mutex. rc [%x]", rc);
        	goto out;
        }*/
	/*if((rc = clOsalMutexLock(gMutex))  != CL_OK)
	    {
		clLogError(CL_CKPT_AREA_DEPUTY,"DSK.WRI", "Ckpt: ckptPersistentMemoryWrite: Could not get Lock");
		goto out;
	    }*/
#if 1
	if(gClCkptPersistentDBDisabled == -1)
	    {
	        gClCkptPersistentDBDisabled = (ClInt32T)clParseEnvBoolean("CL_CKPT_PERSISTENT_DB_DISABLED");
	    }

	    if(gClCkptPersistentDBDisabled)
	    {
	        return CL_OK;
	    }

	    if(gClCkptDBInitialized == CL_FALSE)
	    {
	        if( (dbInitialized == CL_OK) &&
	            (rc = ckptDataBackupInitialize(&flag)) != CL_OK)
	        {
	            dbInitialized = rc;
	        }
	        if(dbInitialized != CL_OK)
	            return dbInitialized;
	    }
#endif
	    /* Write to metadata dataset */
	    rc = clCkptLibraryCkptDataSetWrite(gCkptHandle, &gCkptName,
	                CL_CKPT_METADATA, 0);
	    if(CL_OK != rc)
	    {
	    	clLogError("PERSISTENT","DSK.WRI","CKPT DataSet Write Failed, "
	                                       "rc = %x \n", rc);
	        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
	                   CL_LOG_MESSAGE_1_CKPT_WRITE_FAILED, rc);

	        CL_FUNC_EXIT();
	    }
	//clOsalMutexUnlock(gMutex);
//out:
    clLogDebug("PERSISTENT","---", "leave %s with retcode [0x%x]", __FUNCTION__,rc);
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
    /*if((rc = clOsalMutexLock(gMutex))  != CL_OK)
    {
    	clLogError(CL_CKPT_AREA_DEPUTY,"DSK.READ", "Ckpt: ckptPersistentMemoryRead: Could not get Lock");
    	goto out;
    }*/

    /* Read checkpoint meta data */
    rc = clCkptLibraryCkptDataSetRead(gCkptHandle, &gCkptName,
           CL_CKPT_METADATA, 0);
    if(CL_OK != rc)
    {
    	clLogError("PERSISTENT","DSK.READ","CKPT DataSet Read Failed rc = [%x]", rc);
        /* Release the semaphore */
        //clOsalMutexUnlock(gMutex);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    /* Release the semaphore */
    //clOsalMutexUnlock(gMutex);
//out:
    clLogDebug("HUNG","---","leave %s with retcode [0x%x]", __FUNCTION__,rc);
    return rc;
}
