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
 * ModuleName  : alarm
 * File        : clAlarmClient.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 *
 *          This file provide implemantation of the Eonized application main function
 *          This C file SHOULD NOT BE MODIFIED BY THE USER
 *
 *****************************************************************************/

#undef __SERVER__
#define __CLIENT__

/* Standard Inculdes */
#include <string.h>  /* strcpy is needed for eo name */
#include <time.h>
#include <sys/time.h>

/* ASP Includes */
#include <clCommon.h>
#include <clEoApi.h>
#include <clDebugApi.h>
#include <clOampRtApi.h>
#include <clCpmApi.h>
#include <clOmApi.h>
#include <clCntApi.h>
#include <clHalApi.h>
#include <clParserApi.h>
#include <clEventExtApi.h>
#include <clBitApi.h>
#include <clOampRtErrors.h>
#include <clVersion.h>

/* cor apis */
#include <clCorIpi.h>
#include <clCorServiceId.h>
#include <clCorUtilityApi.h>
#include <clCorNotifyApi.h>
#include <clCorApi.h>
#include <clCorMetaData.h>
#include <clCorTxnApi.h>
#include <clAlarmApi.h>
#include <clCorErrors.h>
#include <clLeakyBucket.h>
#include <xdrClCorAlarmConfiguredDataIDLT.h>
#include <xdrClCorAlarmResourceDataIDLT.h>
#include <xdrClCorAlarmProfileDataIDLT.h>

/* alarm includes */
#include "clAlarmUtil.h"
#include <clAlarmClient.h>
#include <clAlarmOMIpi.h>
#include "clAlarmOmClass.h"
#include <clAlarmCommons.h>
#include <clAlarmErrors.h>
#include <ipi/clAlarmIpi.h>
#include <clAlarmServer.h>

#include <clIdlApi.h>
#include <clXdrApi.h>
#include <xdrClAlarmInfoIDLT.h>
#include <xdrClAlarmHandleInfoIDLT.h>
#include <xdrClCorMOIdT.h>

#include <clAlarmClientFuncTable.h>
#include <clAlarmServerFuncTable.h>


/*
 * Disabling alarm polling as it is not supported now.
 */
//#define ALARM_POLL

/******************************************************************************
 *  Global
 *****************************************************************************/
static ClBoolT gClAlarmLibInitialize = CL_FALSE;
ClBoolT gClAlarmLibFinalizeFlag  = CL_FALSE;
ClEventInitHandleT gAlarmClntEvtHandle = 0;
ClEventChannelHandleT gAlmServerEvtChannelHandle = 0;
ClAlarmEventCallbackFuncPtrT gAlarmClientEventCallbackFunc = NULL;

/*************************** Mutex variables ********************************/

ClOsalMutexIdT gClAlarmCntMutex;
ClOsalMutexIdT gClAlarmPollMutex;

/*****************************************************************************/

/**************************** Timer variable ********************************/

ClTimerHandleT gTimerHandle;
/* 
 * This can be made a local variable but just in case one needs to stop the
 * timer anywhere else this provides the flexibility to do so.
 */

/*
 * Alarm resource list handle which is obtained on creation of container.
 */
ClCntHandleT     gresListHandle = 0;

ClEventChannelHandleT gAlarmEvtChannelHandle;
ClEventInitHandleT gAlarmEvtHandle;

ClEventSubscriptionIdT gAlarmMOCreateSubsId;
ClEventSubscriptionIdT gAlarmMOChangeSubsId;

ClAlarmPollInfoT gAlarmsToPoll[CL_ALARM_MAX_ALARMS];
VDECL_VER(ClCorAlarmConfiguredDataIDLT, 4, 1, 0) gAlarmConfig = {0};

#if ALARM_TXN 
/*
 * Alarm service handle which is obtained on registering with the 
 * transaction agent
 */
ClTxnAgentServiceHandleT gAlarmServiceHandle;    
#endif


/*****************************************************************************/

#if ALARM_TXN 
/* Callback functions for transaction-agent */
static ClTxnAgentCallbacksT gAlarmTxnCallbacks = {
    .fpTxnAgentJobPrepare   = clAlarmClientTxnValidate,
    .fpTxnAgentJobCommit    = clAlarmClientTxnUpdate,
    .fpTxnAgentJobRollback  = clAlarmClientTxnRollback,
};
#endif

#if ALARM_TXN 
ClRcT clAlarmClientTxnValidate(
    CL_IN ClTxnTransactionHandleT txnHandle,
    CL_IN ClTxnJobDefnHandleT     jobDefn, 
    CL_IN ClUint32T               jobDefnSize,
    CL_INOUT ClTxnAgentCookieT       *pCookie)
{

    return CL_OK;
}

ClRcT clAlarmClientTxnUpdate(
    CL_IN ClTxnTransactionHandleT txnHandle,
    CL_IN ClTxnJobDefnHandleT     jobDefn, 
    CL_IN ClUint32T               jobDefnSize,
    CL_INOUT ClTxnAgentCookieT       *pCookie)
{

   ClRcT rc = CL_OK;
   ClCorTxnIdT  corTxnId;

   rc = clCorTxnJobHandleToCorTxnIdGet(jobDefn, jobDefnSize, &corTxnId);
   if(CL_OK != rc)
   {
       CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Handle2CorTxnId Get failed with rc: [0x%x]\r\n",rc));
       return rc;
   }

   rc = clCorTxnJobWalk(corTxnId,clAlarmClientTxnJobWalk, 0);
   if(CL_OK != rc)
   {
       CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Txn Job Walk failed with rc: [0x%x]\r\n",rc));
       return rc;
   }

   rc = clCorTxnIdTxnFree(corTxnId);
   if(CL_OK != rc)
   {
       CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Txn free failed with rc: [0x%x]\r\n",rc));
       return rc;
   }

   return CL_OK;
}

ClRcT clAlarmClientTxnRollback(
    CL_IN       ClTxnTransactionHandleT txnHandle,
    CL_IN       ClTxnJobDefnHandleT     jobDefn,
    CL_IN       ClUint32T               jobDefnSize,
    CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    return CL_OK;
}


ClRcT clAlarmClientTxnJobWalk(ClCorTxnIdT trans,
                    ClCorTxnJobIdT jobId,
                            void          *arg)
{
    ClCorOpsT         corOp = CL_COR_OP_RESERVED;
    ClCorMOIdT        moId;
    ClCorMOIdPtrT     hMoId = &moId;
    ClRcT               rc = CL_OK;
    ClCorMOServiceIdT svcId;
	ClHandleT            handleOm;    

    /* TODO - COR-TXN (check errors) */
    rc = clCorTxnJobMoIdGet(trans, hMoId);

    svcId = clCorMoIdServiceGet(hMoId);

    if (CL_COR_SVC_ID_ALARM_MANAGEMENT != svcId)
    {
        return rc;        
    }
        
    rc = clCorTxnJobOperationGet(trans, jobId, &corOp);
    if (corOp == CL_COR_OP_CREATE)
    {

        ClOmClassTypeT    omClass;
        void             *pOmObj = NULL;    
        /*ClHandleT            hOm = 0;    */
        ClCorClassTypeT   myMOClassType;

        rc = clCorMoIdToClassGet(hMoId, CL_COR_MO_CLASS_GET,  &myMOClassType);

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorMoIdToClassGet failed w rc:0x%x\n", rc));
            return rc;
        }

        rc = clOmClassFromInfoModelGet(myMOClassType, 
                                       CL_COR_SVC_ID_ALARM_MANAGEMENT,&omClass);
        
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\n Om class type for alarm MSO is:%d\n", omClass));
        if ( rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorClassTableToOmClassGet failed \
                                   to return om class rc:%0x \n", rc));
            return (rc);
        }

        if (( rc=clOmMoIdToOmHandleGet(hMoId, &handleOm)) != CL_OK)
        {
            rc = clOmObjectCreate(omClass, 1, &handleOm, &pOmObj, hMoId, sizeof(ClCorMOIdT));
            if (pOmObj == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOmObjectCreate failed .. \n"));
                return CL_ALARM_RC(CL_ALARM_ERR_OM_CREATE_FAILED);
            }

            if((rc= clOsalMutexCreate( &((CL_OM_ALARM_CLASS*)pOmObj)->cllock))!=
                    CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n unable to create the mutex rc \
                            0x%x",rc));

                if ((rc = clOmObjectByReferenceDelete(pOmObj)) != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete OM object!\r\n"));
                    return rc;
                }
                return rc;
                
            }
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Successfully created the OM object \n"));

            if ( (rc=clOmMoIdToOmIdMapInsert(hMoId, handleOm)) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to install OM/COR table entry\n"));
                                                                                                
                /* delete the lock first */
                if ( (rc = clOsalMutexDelete(((CL_OM_ALARM_CLASS*)pOmObj)->cllock))!=
                        CL_OK){
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n unable to delete the mutex rc 0x%x\
                                ",rc));
                    return rc;
                }

                /* delete OM obj */
                if ((rc = clOmObjectByReferenceDelete(pOmObj)) != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete OM object!\r\n"));
                    return rc;
                }
            }
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("error, OM object already present w/rc:%x \n", rc));
            /* return rc;*/
        }
    }
/*
 * Currently we are checking for only CL_COR_OP_CREATE, but we will also have to check for
 * other operations especially CL_COR_OP_DELETE and remove the corresponding entries from 
 * the container. This will avoid the errors which are being thrown out as a result of the
 * delay in the event being delivered.
 */
    return CL_OK;
}
#endif




/**
 *  Determine the supported alarm Version.  
 *
 *  This Api checks the version is supported or not. If the version check fails,
 *    the closest version is passed as the out parameter.
 *                                                                        
 *  @param version[IN/OUT] : version value.
 *
 *  @returns CL_OK  - Success<br>
 *    On failure, the error CL_ALARM_ERR_VERSION_UNSUPPORTED is returned with the 
 *       closest value of the version supported filled in the version as out parameter.
 */
ClRcT 
clAlarmVersionVerify(ClVersionT *version)
{
    ClRcT rc = CL_OK;
    
	clLogTrace( "ALM", "VER", "Entering [%s]", __FUNCTION__);

    if(version == NULL)
    {
		rc = CL_ALARM_RC(CL_ERR_NULL_POINTER);
		clLogError( "ALM",  
             "VER", "NULL pointer passed for version. rc[0x%x]", rc);
        return rc;
    }
    CL_FUNC_ENTER();
    
    if((rc = clVersionVerify(&gAlarmClientToServerVersionDb, version)))
    {
		clLogError( "ALM",  
             "VER", "Failed to verify the version. rc[0x%x] \
			The version is [0x%x, 0x%x, 0x%x] ", rc, version->releaseCode, \
			version->majorVersion, version->minorVersion);
        return CL_ALARM_RC(CL_ALARM_ERR_VERSION_UNSUPPORTED);
    }
	
	clLogTrace( "ALM", "VER", "Leaving [%s]", __FUNCTION__);

    CL_FUNC_EXIT();
    return rc;
}

ClRcT clAlarmLibInitialize(void)
{
    ClRcT                   rc              = CL_OK;
    ClEoExecutionObjT*      eoObj           = NULL;
    ClOmClassControlBlockT  *pOmClassEntry  = NULL;

    CL_FUNC_ENTER();

    pOmClassEntry = &omAlarmClassTbl[0];

	if (gClAlarmLibInitialize == CL_TRUE)
	{
		clLogNotice("ALM", "INT", "The alarm client library intialization has "
						"already been done successfully, so skipping");
		return CL_OK;
	}
     
    clLogNotice( "ALM", "INT", "Initializing the alarm client library ...");

    /* Leaky Bucket implementation */
    rc = clAlarmLeakyBucketInitialize();
    if (rc != CL_OK)
    {
        clLogError("ALM", "INT", "Failed to initialize the Leaky Bucket. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clEoMyEoObjectGet(&eoObj);
    if(CL_OK != rc)
    {
        clLogError( "ALM", 
                  "INT", "Failed while getting the Execution Object Data. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    clLogTrace("ALM", "INT", "Before doing client to server table registration.");

    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, ALM), CL_IOC_ALARM_PORT);
    if (rc != CL_OK)
    {
        clLogError("ALM", "INT", "Alarm client EO table register failed. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    clLogTrace("ALM", "INT", "Before doing client to owner table registration.");

    /* Register the alarm owner's table with its own IOC port. */
    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, ALM_CLIENT), eoObj->eoPort);
    if (rc != CL_OK)
    {
        clLogError("ALM", "INT", "Alarm client EO table register failed. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    clLogTrace("ALM", "INT",
            "Before doing alarm client table initialization.");

    rc = clAlarmClientNativeClientTableInit(eoObj);
    if (rc != CL_OK)
    {
        clLogError("ALM", "INT", "Failed to install the alarm client table. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    clLogTrace( "ALM", "INT",
            "Before doing OM Base class Initialization ");

    rc = clAlarmClientBaseOmClassInit();
    if(CL_OK != rc)
    {
		clEoClientUninstall(eoObj, CL_ALARM_CLIENT_TABLE_ID);
        clLogError( "ALM", "INT", 
                "Failed while initializing the OM base class. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }    

#if ALARM_TXN
    
    clLogTrace( "ALM", "INT",
            "Before doing TXN service Callbacks registration ");

    rc = clTxnAgentServiceRegister(CL_COR_SVC_ID_ALARM_MANAGEMENT, 
                                   gAlarmTxnCallbacks, &gAlarmServiceHandle);
    if(rc != CL_OK)
    {
        clLogError( "ALM", "INT",
                "Failed to register as transaction agent. rc [0x%x]", rc);
	    return rc;
    }

#endif

    clLogTrace( "ALM", "INT",
            "Before creating the client side Resource List");

    rc = clAlarmClientResListCreate();
    if(CL_OK != rc)
    {
		clOmClassFinalize(pOmClassEntry,pOmClassEntry->eMyClassType);
		clEoClientUninstall(eoObj, CL_ALARM_CLIENT_TABLE_ID);
        clLogError( "ALM", "INT", 
                "Failed while creating the resource list. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }    

    clLogTrace( "ALM", "INT",
            "Before opening the event channel [COR_EVT_CHANNEL] ");

    rc = clAlarmChannelOpen();    
    if(CL_OK != rc)
    {
		clCntDelete(gresListHandle);
		clOmClassFinalize(pOmClassEntry,pOmClassEntry->eMyClassType);
		clEoClientUninstall(eoObj, CL_ALARM_CLIENT_TABLE_ID);
        clLogError( "ALM", "INT", 
                "Failed while opening the alarm event channel. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }    

    rc = clOsalMutexCreate(&gClAlarmCntMutex);    
    if(CL_OK != rc)
    {
		clEventFinalize(gAlarmEvtHandle);
		clCntDelete(gresListHandle);
		clOmClassFinalize(pOmClassEntry,pOmClassEntry->eMyClassType);
		clEoClientUninstall(eoObj, CL_ALARM_CLIENT_TABLE_ID);
        clLogError( "ALM", "INT", "Creation of Alarm Container Mutex failed. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    clLogTrace( "ALM", "INT",
            "Processing the resorce information of the rt.xml file");

    rc = clAlarmClientResTableProcess(eoObj);
    if(CL_OK != rc)
    {
		clOsalMutexDelete(gClAlarmCntMutex);    
		clEventFinalize(gAlarmEvtHandle);
		clCntDelete(gresListHandle);
		clOmClassFinalize(pOmClassEntry,pOmClassEntry->eMyClassType);
		clEoClientUninstall(eoObj, CL_ALARM_CLIENT_TABLE_ID);
        clLogError( "ALM", "INT", 
                " Failed while processing the alarm client resource table. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }    

    clLogTrace( "ALM", "INT",
            "Rt.xml file processed successfully. Now installing the timer.");

    rc = clAlarmClientTimerInit(eoObj);
    if(CL_OK != rc)
    {
		clEventFinalize(gAlarmEvtHandle);
		clOsalMutexDelete(gClAlarmCntMutex);    
		clCntDelete(gresListHandle);
        clAlarmClientConfigTableDelete();
        clAlarmClientConfigDataFree(&gAlarmConfig);
		clOmClassFinalize(pOmClassEntry,pOmClassEntry->eMyClassType);
		clEoClientUninstall(eoObj, CL_ALARM_CLIENT_TABLE_ID);
        clLogError( "ALM", "INT", " Failed while initializing the timer. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    clLogTrace( "ALM",  "INT", "Timer initialized successfully.");

    rc = clOsalMutexCreate(&gClAlarmPollMutex);    
    if(CL_OK != rc)
    {
		clOsalMutexDelete(gClAlarmCntMutex);    
        if(gTimerHandle != 0)
            clTimerDelete(&gTimerHandle);
		clEventFinalize(gAlarmEvtHandle);
		clCntDelete(gresListHandle);
        clAlarmClientConfigTableDelete();
        clAlarmClientConfigDataFree(&gAlarmConfig);
		clOmClassFinalize(pOmClassEntry,pOmClassEntry->eMyClassType);
		clEoClientUninstall(eoObj, CL_ALARM_CLIENT_TABLE_ID);
        clLogError( "ALM",  
                         "INT", "Creation of Alarm Polling Mutex failed. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

#ifdef ALARM_POLL    
    /* get the  current alarms */

    clLogTrace( "ALM", "INT", "Processing the alarms at the boot up time....");

    rc = clAlarmClientBootTimeResAlarmGet();
    if(CL_OK != rc)
    {
		clOsalMutexDelete(gClAlarmCntMutex);    
		clOsalMutexDelete(gClAlarmPollMutex);    
        if(gTimerHandle != 0)
            clTimerDelete(&gTimerHandle);
		clEventFinalize(gAlarmEvtHandle);
		clCntDelete(gresListHandle);
        clAlarmClientConfigTableDelete();
        clAlarmClientConfigDataFree(&gAlarmConfig);
		clOmClassFinalize(pOmClassEntry,pOmClassEntry->eMyClassType);
		clEoClientUninstall(eoObj, CL_ALARM_CLIENT_TABLE_ID);
        clLogError( "ALM", "INT", 
                "Failed while processing the alarm information at the bootup time. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }
#endif

    clLogNotice( "ALM", "INT", "Completed the alarm client library Initialization.");
	gClAlarmLibInitialize = CL_TRUE;

    CL_FUNC_EXIT();

    return rc;
}


#ifdef ALARM_POLL

ClRcT clAlarmClientBootTimeResAlarmGet()
{
    ClRcT rc = CL_OK;

    clLogTrace( "ALM", "INT", " Entering [%s]", __FUNCTION__);

    clOsalMutexLock(gClAlarmCntMutex);
    rc = clCntWalk(gresListHandle,clAlarmClientResAlarmStausGet, NULL,0);
    clOsalMutexUnlock(gClAlarmCntMutex);    

    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT",
                "Failed while walking the container having the alarms. rc[0x%x]", rc);
    }
    
    clLogTrace( "ALM", "INT", " Leaving [%s]", __FUNCTION__);
    return rc;
}




ClRcT clAlarmClientResAlarmStausGet(ClCntKeyHandleT     key,
                   ClCntDataHandleT    pData,
                   void                *dummy,
                   ClUint32T           dataLength)
{

    ClRcT               rc = CL_OK;
    ClCorObjectHandleT  hMSOObj;
    ClCorServiceIdT     svcid = CL_COR_SVC_ID_ALARM_MANAGEMENT;
    CL_OM_ALARM_CLASS*  omhandle = NULL ; 
    ClCorMOIdT          moid ;

    clLogTrace( "ALM", "INT"," Entering [%s]", __FUNCTION__);

    rc = clCorObjectHandleGet((ClCorMOIdPtrT)key, &hMSOObj);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT",
                "Failed while getting the object handle from MoId. rc[0x%x]", rc);
        return rc;
    }

    rc = clAlarmClientPolledAlarmUpdate((ClCorMOIdPtrT)key, hMSOObj, 1 /* boot up time is true*/);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT", 
                " Failed while updating the polled alarm information. rc[0x%x]", rc);
        clCorObjectHandleFree(&hMSOObj);
        return rc;
    }

    /* TODO : The moid is already available as the Key. Do we need to again call Cor here for it ?*/
    /* Get the moid from the corobject handle */      
    rc = clCorObjectHandleToMoIdGet(hMSOObj , &moid, &svcid);
    if(rc != CL_OK )
    {
        clLogError( "ALM", "INT", 
                "Failed while getting the moId from the Object handle. rc[0x%x]", rc);
        clCorObjectHandleFree(&hMSOObj);
        return rc;
    }

    /* Get the omobject handle from the moid */ 
    rc = clOmMoIdToOmObjectReferenceGet(&moid, (void **)&omhandle );
    if(rc != CL_OK ) {
        clLogError( "ALM", "INT",
                "Failed to get the OM Object handle for the MoId. rc[0x%x]", rc);
        clCorObjectHandleFree(&hMSOObj);
       return rc;
    }

    if (((CL_OM_ALARM_CLASS*)omhandle)->fpAlarmObjectPoll)
    {
        clOsalMutexLock(gClAlarmPollMutex);
        rc = ((CL_OM_ALARM_CLASS*)omhandle)->fpAlarmObjectPoll(
                                          (CL_OM_ALARM_CLASS*) omhandle,
                                          &moid,
                                          gAlarmsToPoll);
        clOsalMutexUnlock(gClAlarmPollMutex);    
        if (rc != CL_OK)
        {
            clLogError( "ALM", "INT",
                    "Failed after calling the polling function.  rc[0x%x]", rc);
            clCorObjectHandleFree(&hMSOObj);
            return rc;
        }         
    }
    else
    {
#if CL_ALARM_POLL_LOG /* No need to print this message. */
        clLogError( "ALM", "INT",
                "No virtual function fpAlarmObjectPoll provided for MO.");
#endif
        clCorObjectHandleFree(&hMSOObj);
        return CL_OK;        
    }

    /* process the data read from HAL */
    rc = clAlarmClientPollDataProcess((ClCorMOIdPtrT)key, hMSOObj);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT", 
                "Failed while processing the alarm poll data. rc[0x%x]", rc);
        clCorObjectHandleFree(&hMSOObj);
        return rc;
    }         
        
    clCorObjectHandleFree(&hMSOObj);
    clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__);
    return rc;
}
#endif


ClRcT clAlarmClientResListWalk(ClCntKeyHandleT     key,
                   ClCntDataHandleT    pData,
                   void                *dummy,
                   ClUint32T           dataLength)
{

    ClRcT rc = CL_OK;
    ClCorObjectHandleT     hMSOObj;

#ifdef ALARM_POLL
    ClUint32T        size = 0;
    ClUint8T        almsToPoll=0;
    ClUint32T lockApplied = 1;
#endif

#ifdef CL_ALARM_POLL_LOG
    clLogTrace( "ALM", "INT", "Entering [%s]", __FUNCTION__);
#endif

    rc = clAlarmStaticInfoCntMoIdToHandleGet((ClCorMOIdPtrT)key, &hMSOObj);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT", 
                " Failed while getting the object handle form the static container. rc[0x%x]", rc);
        return rc;
    }        


#ifdef CL_ALARM_POLL_LOG
    clLogTrace( "ALM",  "INT","Updating the Soaking time.");
#endif

    clAlarmClientUpdateSoakingTime((ClCorMOIdPtrT)key, hMSOObj);

#ifdef ALARM_POLL
    size = sizeof(almsToPoll);
    if ( (rc = GetAlmAttrValue((ClCorMOIdPtrT)key,
                               hMSOObj, 
                               CL_ALARM_MSO_TO_BE_POLLED, 
                               0,
                               &almsToPoll, 
                               &size,
                               lockApplied)) != CL_OK )
    {
        clLogError( "ALM", "INT",
                "Could not retrieve the value of attribute [CL_ALARM_MSO_TO_BE_POLLED]. rc[0x%x]", rc);
        return rc;
    }

#ifdef CL_ALARM_POLL_LOG
    clLogTrace( "ALM", "INT",
            "After getting the value of almsToPoll [%d]", almsToPoll);
#endif
    
    if (almsToPoll)
    {
#ifdef CL_ALARM_POLL_LOG
        clLogTrace("ALM", "INT", "The alarm has polling enabled starting the polling. ");
#endif

        clAlarmClientAlarmPoll((ClCorMOIdPtrT)key,hMSOObj);
    }
#endif

#ifdef CL_ALARM_POLL_LOG
    clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__);
#endif

    return rc;
}



#ifdef ALARM_POLL
void 
clAlarmClientAlarmPoll(ClCorMOIdPtrT pMoId, ClCorObjectHandleT hMSOObj)
{

    /* For all alarms, if they are under soaking, update the soaking time 
     * poll the hardware, if the elapsed polling time has reached the provisioned
     * polling time. Process any change in defect condition 
     */

    ClUint32T                  pollingInterval = 0;
    ClUint32T                  elapsedPollingInterval = 0;
    ClRcT                      rc=CL_OK;
    ClUint32T                  size=0;
    CL_OM_ALARM_CLASS* omhandle = NULL ; 
    ClUint32T lockApplied = 1;
    
#ifdef CL_ALARM_POLL_LOG
    clLogTrace( "ALM", "INT", "Entering [%s]", __FUNCTION__);
#endif
				
    size = sizeof(ClUint32T);
    rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_POLLING_INTERVAL,
                    0, &pollingInterval, &size, lockApplied);
	if(CL_OK != rc)
	{
    	clLogError( "ALM", "INT", 
                "Failed while getting the polling interval. rc[0x%x]", rc);
		return;
	}
	
    size = sizeof(ClUint32T);
    rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_ELAPSED_POLLING_INTERVAL,
                    0, &elapsedPollingInterval, &size, lockApplied);

	if(CL_OK != rc)
	{
    	clLogError( "ALM", "INT",
                "Failed while getting elapsed polling interval. rc[0x%x]", rc);
		return;
	}

    elapsedPollingInterval += ALARM_CLIENT_CALLBACK_TIME;

    if (elapsedPollingInterval >= pollingInterval)
    {

        /* time to poll */
        /* fill out the alarm structure */
        if ( (rc = clAlarmClientPolledAlarmUpdate(pMoId, hMSOObj, 0/* not bootup time*/) ) != CL_OK )
        {
            /* nothing to query. skipping this object */
    		clLogTrace( "ALM", "INT", "There are no alarm to query in polling .. so skipping. rc[0x%x]", rc);
            return;
        }
                        
        if ( (rc = clOmMoIdToOmObjectReferenceGet(pMoId, (void **)&omhandle ))!= CL_OK )
        {
    		clLogError( "ALM",  "INT", " Failed while getting the omObject \
							reference for the MoId. rc[0x%x]", rc);
            return ;
        }

        if (((CL_OM_ALARM_CLASS*)omhandle)->fpAlarmObjectPoll)
        {
#ifdef CL_ALARM_POLL_LOG
			clLogTrace( "ALM",  "INT","Executing the fpAlarmObjectPoll callback function."); 
#endif

            clOsalMutexLock(gClAlarmPollMutex);
            rc = ((CL_OM_ALARM_CLASS*)omhandle)->fpAlarmObjectPoll(
                                              (CL_OM_ALARM_CLASS*) omhandle,
                                              pMoId,
                                              gAlarmsToPoll);
            clOsalMutexUnlock(gClAlarmPollMutex);
            if (rc != CL_OK)
            {
    			clLogError( "ALM", "INT", " Failed while executing the polling callback function. rc[0x%x]", rc); 
                return;
            }         
        }
        else
        {
			clLogTrace( "ALM", "INT","No virtual function for fpAlarmObjectPoll provided for MO"); 
            return;        
        }

        /* process the data read from HAL */
        rc = clAlarmClientPollDataProcess(pMoId, hMSOObj);
        if (rc != CL_OK)
        {
			clLogError( "ALM", "INT", 
                    "Failed while processing the data returned from the polling function. rc[0x%x]", rc ); 
            return;
        }         
        elapsedPollingInterval = 0;

    }

#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM",  "INT","Setting the value of elapsed polling interval"); 
#endif

    /* update the elapsed polling time for the alarm MSO */
    rc = SetAlmAttrValue(pMoId,NULL, hMSOObj, CL_ALARM_ELAPSED_POLLING_INTERVAL,
                       0, &elapsedPollingInterval, sizeof(ClUint32T));
 	if(CL_OK != rc)
	{
		clLogError( "ALM", "INT", 
                "Failed while setting the elapsed polling interval. rc[0x%x]", rc ); 
		return;
	}
    
#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__); 
#endif

}

ClRcT clAlarmClientPolledAlarmUpdate(ClCorMOIdPtrT pMoId, ClCorObjectHandleT objH, ClUint8T bootUpTime)
{
    /* fill out the global structure with the 
       alarms being monitored for an MSO object */
    ClRcT        rc = CL_OK;
    ClUint32T    i=0;
    ClUint32T    size=0;
    ClUint32T    pollAlarm=0;
    ClUint32T    lockApplied = 1;

    /* Reducing the number of calls made to cor server */
    ClAlarmProbableCauseT alarmId[CL_ALARM_MAX_ALARMS] = {0};

#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "INT", "Entering [%s]", __FUNCTION__); 
#endif
	
    size = sizeof(alarmId);
    if ( (rc = GetAlmAttrValue(pMoId, objH, CL_ALARM_ID,-1, &alarmId, &size, lockApplied) != CL_OK ))
    {
		clLogError( "ALM", "INT", "Not able to get the probable cause from COR. rc[0x%x] ", rc); 
        return rc;
    }

    clOsalMutexLock(gClAlarmPollMutex);
    memset(gAlarmsToPoll, 0, sizeof(gAlarmsToPoll));
    clOsalMutexUnlock(gClAlarmPollMutex);

    for (i=0; i< CL_ALARM_MAX_ALARMS; i++)
    {
#ifdef CL_ALARM_POLL_LOG
/* 
 * This is related to bug 5230. This will be uncommented after 
 * bug 5010 is fixed.
 */
        size = sizeof(ClUint32T);
        if ( (rc = GetAlmAttrValue(objH, CL_ALARM_ACTIVE,
                            i, &gAlarmsToPoll[i].alarmState , &size) != CL_OK ))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the alarm state from COR\n"));
            return rc;
        }
#endif

#ifdef CL_ALARM_POLL_LOG
		clLogTrace( "ALM", "INT", "Getting the probable cause for the alarm Index [%d]", i); 
#endif

        if (alarmId[i] != CL_ALARM_ID_INVALID)
        {

#ifdef CL_ALARM_POLL_LOG
			clLogTrace( "ALM", "INT", "Alarm Id valid for the alarm index [%d]", i); 
#endif

            if (bootUpTime)
            {
#ifdef CL_ALARM_POLL_LOG
				clLogTrace( "ALM", "INT", 
                        "Getting the probable cause [%d] at the bootup-time for index [%d].", alarmId[i], i); 
#endif

                clOsalMutexLock(gClAlarmPollMutex);
                gAlarmsToPoll[i].probCause = alarmId[i];            
                clOsalMutexUnlock(gClAlarmPollMutex);
            }
            else
            {
                size = sizeof(pollAlarm);
                rc = clAlarmClientEngineProfAttrValueGet(pMoId, objH,CL_ALARM_FETCH_MODE, i,
                                                         &pollAlarm, &size);
                if (pollAlarm)
                {
#ifdef CL_ALARM_POLL_LOG
				    clLogTrace( "ALM", "INT",
                            "Getting the probable cause [%d] at the runtime for index [%d].", alarmId[i], i); 
#endif
                    clOsalMutexLock(gClAlarmPollMutex);
                    gAlarmsToPoll[i].probCause = alarmId[i];            
                    clOsalMutexUnlock(gClAlarmPollMutex);
                }
            }

        }        
        else
        {
#ifdef CL_ALARM_POLL_LOG
			clLogTrace( "ALM", "INT",
                    "Putting the alarm Id as invalid for all unconfigured alarms"); 
#endif
            clOsalMutexLock(gClAlarmPollMutex);
            gAlarmsToPoll[i].probCause = CL_ALARM_ID_INVALID;
            clOsalMutexUnlock(gClAlarmPollMutex);
        }
    }

#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__); 
#endif
	
    return rc;
}




ClRcT
clAlarmClientPollDataProcess(ClCorMOIdPtrT pMoId, ClCorObjectHandleT objH)
{
    ClRcT         rc = CL_OK;
    ClAlarmInfoT alarmInfo;

    ClUint32T     idx;
    ClUint64T    alarmStatus;
    ClUint32T    size=0;
    ClCorServiceIdT    svcId = CL_COR_SVC_ID_ALARM_MANAGEMENT;
    ClAlarmStateT alarmState;
    ClAlarmProbableCauseT probCause;
    ClUint32T lockApplied = 1;

#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "INT", "Entring %s", __FUNCTION__); 
#endif

    /* get the status bit for the alarm MSO */
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, objH, CL_ALARM_CURRENT_ALMS_BITMAP,
                    0, &alarmStatus, &size, lockApplied);
    if (rc != CL_OK)
    {
		clLogError( "ALM",  
			 "INT", "Failed while getting the current alarms bitmap. rc[0x%x]", rc); 
        return rc;
    }                                

    for(idx=0; idx < CL_ALARM_MAX_ALARMS; idx++)
    {
#ifdef CL_ALARM_POLL_LOG
		clLogTrace( "ALM",  
			 "INT", "Getting the alarm State and probable cause for the index [%d]", idx); 
#endif
		
        clOsalMutexLock(gClAlarmPollMutex);
        alarmState = gAlarmsToPoll[idx].alarmState;
        probCause = gAlarmsToPoll[idx].probCause;
        clOsalMutexUnlock(gClAlarmPollMutex);

        if (probCause == CL_ALARM_ID_INVALID)
        {
#ifdef CL_ALARM_POLL_LOG
            /* no alarms to process*/
			clLogTrace( "ALM",  "INT", 
                    "Processed all the alarms in the alarm structure, and got the "
                    "probable cause as invalid for index: [%d]", idx); 
#endif
            break;
        }
        else
        {
#ifdef CL_ALARM_POLL_LOG
			clLogTrace( "ALM", "INT", "checking for any change in the state of the alarm ");
#endif

            if ( (alarmStatus & ((ClUint64T) 1<<idx)) != alarmState )
            {                    
#ifdef CL_ALARM_POLL_LOG
				clLogTrace( "ALM", "INT", "The alarm State change for the alarm [%d] ", idx);
#endif
				
                /* defect condition changed*/
                memset(&alarmInfo, 0, sizeof(ClAlarmInfoT));
                
                clOsalMutexLock(gClAlarmPollMutex);
                alarmInfo.alarmState = gAlarmsToPoll[idx].alarmState ;
                alarmInfo.probCause = gAlarmsToPoll[idx].probCause;
                clOsalMutexUnlock(gClAlarmPollMutex);

#ifdef CL_ALARM_POLL_LOG
				clLogTrace( "ALM", "INT", "Getting the MoId from Object handle");
#endif

                /* set the MOID as well */
                rc = clCorObjectHandleToMoIdGet(objH, &(alarmInfo.moId), &svcId);
                if (rc != CL_OK)
                {
					clLogError( "ALM", "INT", "Failed while getting the MoId from Object Handle . rc[0x%x]", rc);
                    return rc;
                }                                

#ifdef CL_ALARM_POLL_LOG
				clLogTrace( "ALM", "INT", "Processing the alarms for the index [%d]", idx);
#endif
				
                alarmInfo.len = 0;
                rc = clAlarmClientEngineAlarmProcess(pMoId, objH, &alarmInfo, NULL, lockApplied);
                if (rc != CL_OK)
                {
					clLogTrace( "ALM", "INT", "Failed while processing the alarm at the client "
                            "for alarm index[%d]. rc[0x%x]", idx, rc);
                }
            } 
        }
    }
#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "INT", " Leaving [%s]", __FUNCTION__);
#endif
    return rc;
}
#endif



ClRcT clAlarmClientResListCreate()
{
    ClRcT rc = CL_OK;
    
	clLogTrace( "ALM", "INT", " Entering [%s]", __FUNCTION__);

    rc = clCntLlistCreate(clAlarmResCompare, 
                          clAlarmResEntryDeleteCallback,
                          clAlarmResEntryDestroyCallback, 
                          CL_CNT_UNIQUE_KEY, 
                          &gresListHandle );
	if(CL_OK != rc)
	{
		clLogError( "ALM", "INT", " Resource List creation failed with rc[0x%x]",rc);
	}
	
	clLogTrace( "ALM", "INT", " Leaving [%s] ", __FUNCTION__);
	
    return rc;
}

ClInt32T clAlarmResCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return(clCorMoIdCompare((ClCorMOIdPtrT)key1,(ClCorMOIdPtrT)key2));
}

void clAlarmResEntryDeleteCallback(ClCntKeyHandleT key, ClCntDataHandleT userData)
{
    ClHandleT       handle  = (( ClAlarmResCntDataT *)userData)->omHandle;

	clLogTrace( "ALM", "FIN", " Entering [%s] ", __FUNCTION__);
	
    clOmObjectDelete( handle, 1, NULL, 0 );
    clCorObjectHandleFree(& (((ClAlarmResCntDataT *) userData)->hMSOObj));
    clHeapFree(((ClAlarmResCntDataT *)userData)->alarmContInfo);
    clHeapFree((ClAlarmResCntDataT *)userData);
    clCorMoIdFree((ClCorMOIdPtrT)key);
	
	clLogTrace( "ALM", "FIN", " Leaving [%s] ", __FUNCTION__);
    return;
}

void clAlarmResEntryDestroyCallback(ClCntKeyHandleT key, ClCntDataHandleT userData)
{
    ClHandleT       handle  = (( ClAlarmResCntDataT *)userData)->omHandle;

	clLogTrace( "ALM", "FIN", " Entering [%s] ", __FUNCTION__);
	
    clOmObjectDelete( handle, 1, NULL, 0 );
    clHeapFree(((ClAlarmResCntDataT *)userData)->alarmContInfo);
    clHeapFree((ClAlarmResCntDataT *)userData);
    clCorMoIdFree((ClCorMOIdPtrT)key);

	clLogTrace( "ALM", "FIN", " Leaving [%s] ", __FUNCTION__);

    return;
}

ClRcT clAlarmClientResTableProcess(ClEoExecutionObjT* pEoObj)
{
    ClRcT    rc = CL_OK;
    ClCorAddrT alarmAddr = {0};        
    ClOampRtResourceArrayT resourcesArray={0,NULL};    
    ClOampRtResourceArrayT alarmResArr={0,NULL};
    ClCorMOClassPathT   moPath;
    ClCorMOIdListPtrT pMoIdList = NULL;
    ClUint32T resIdx = 0;
    ClUint32T moIdIdx = 0;
    ClUint32T i = 0;
    ClCorClassTypeT classId = 0;
#ifdef ALARM_CONFIG_STATS
    ClTimeT startTime = 0;
    ClTimeT endTime = 0;
#endif

	clLogTrace( "ALM", "INT", " Entering [%s] ", __FUNCTION__);
	
    rc = clAlarmClientResourceTableInfoGet(&alarmAddr, &resourcesArray);
    if(CL_OK != rc)
    {
        if ((rc == CL_OAMP_RT_RC(CL_ERR_DOESNT_EXIST) || (rc == CL_OAMP_RT_RC(CL_OAMP_RT_ERR_INTERNAL))) && 
                (pEoObj->eoPort == CL_IOC_ALARM_PORT))
        {
            clLogTrace("ALM", "INT", "The alarm server doesn't have any resources configured, so returning...");
            return CL_OK;
        }

		clLogError( "ALM", "INT", "Failed while getting the informaiton from Rt.xml file. rc [0x%x] ", rc);
        return rc;
    }

    if ( (resourcesArray.pResources == NULL) || (resourcesArray.noOfResources == 0) )
    {
        clLogInfo("ALM", "INT", "No alarm resources configured for this application.");
        return CL_OK;
    }

    pMoIdList = (ClCorMOIdListPtrT) clHeapAllocate(sizeof(ClCorMOIdListT) + 
            (resourcesArray.noOfResources * sizeof(ClCorMOIdT)));
    if (pMoIdList == NULL)
    {
        clLogError("ALM", "INT", "Failed to allocate memory.");
        if (resourcesArray.pResources != NULL)
            clHeapFree(resourcesArray.pResources);
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }

    memset(pMoIdList, 0, sizeof(ClCorMOIdListT) + (resourcesArray.noOfResources * sizeof(ClCorMOIdT)));

    /* Allocate memory for the alarm resources */
    alarmResArr.pResources = (ClOampRtResourceT *) clHeapAllocate(
            (resourcesArray.noOfResources * sizeof(ClOampRtResourceT)));
    if (alarmResArr.pResources == NULL)
    {
        clLogError("ALM", "INT", "Failed to allocate memory.");
        if (resourcesArray.pResources != NULL)
            clHeapFree(resourcesArray.pResources);
        clHeapFree(pMoIdList);
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }

    alarmResArr.noOfResources = 0;
    memset(alarmResArr.pResources, 0, (resourcesArray.noOfResources * sizeof(ClOampRtResourceT)));

    clLogTrace("ALM", "INT", "Creating alarm configuration info table.");
    rc = clAlarmClientConfigTableCreate();
    if (rc != CL_OK)
    {
        clLogError("ALM", "INT", "Failed to create alarm config info table. rc [0x%x]", rc);
        if (resourcesArray.pResources != NULL)
            clHeapFree(resourcesArray.pResources);
        clHeapFree(pMoIdList);
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }

    clLogTrace("ALM", "INT", "Fetching alarm configuration data.");

    rc = clAlarmClientConfigDataGet(&gAlarmConfig);
    if (rc != CL_OK)
    {
        clLogError("ALM", "CONFIG", "Failed to get alarm configuration data. rc [0x%x]", rc);
        if(resourcesArray.pResources != NULL)
            clHeapFree(resourcesArray.pResources);
        clHeapFree(pMoIdList);
        clAlarmClientConfigTableDelete();
        return rc;
    }

    for(resIdx=0; resIdx < resourcesArray.noOfResources; resIdx++)
	{
		if ( (resourcesArray.pResources == NULL)
			 || ((resourcesArray.pResources + resIdx) == NULL) )
        {
            clLogError("ALM", "INT", "Failed as the pointer to the resource array is NULL.");
            if (resourcesArray.pResources != NULL)
                clHeapFree(resourcesArray.pResources);
            clHeapFree(pMoIdList);
            clHeapFree(alarmResArr.pResources);
            clAlarmClientConfigTableDelete();
            clAlarmClientConfigDataFree(&gAlarmConfig);
            return CL_ALARM_RC(CL_ALARM_ERR_NULL_POINTER);
        }

		if ( (resourcesArray.pResources != NULL)
			 && ((resourcesArray.pResources + resIdx) != NULL) 
			 && (resourcesArray.pResources[resIdx].objCreateFlag == CL_TRUE) 
			 && ( resourcesArray.pResources[resIdx].wildCardFlag == CL_TRUE) )
		{
			clLogError( "ALM", "INT"," Invalid entry in the rt.xml file ");
            if(resourcesArray.pResources != NULL)
                clHeapFree(resourcesArray.pResources);

            clHeapFree(pMoIdList);
            clHeapFree(alarmResArr.pResources);
            clAlarmClientConfigTableDelete();
            clAlarmClientConfigDataFree(&gAlarmConfig);
			return CL_ALARM_RC(CL_ALARM_ERR_INTERNAL_ERROR);
		}

        clLogTrace("ALM", "INT", "Resource name [%s], Creation Flag [%d], Wildcard flag [%d]", 
                resourcesArray.pResources[resIdx].resourceName.value, resourcesArray.pResources[resIdx].objCreateFlag,
                resourcesArray.pResources[resIdx].wildCardFlag);

        rc = clCorMoIdInitialize(&(pMoIdList->moId[moIdIdx]));
        if(CL_OK != rc)
        {
			clLogError( "ALM", "INT", 
                    "Failed while initializing the MoId. rc[0x%x]", rc);
            if(resourcesArray.pResources != NULL)
                clHeapFree(resourcesArray.pResources);
            clHeapFree(pMoIdList);
            clHeapFree(alarmResArr.pResources);
            clAlarmClientConfigTableDelete();
            clAlarmClientConfigDataFree(&gAlarmConfig);
            CL_FUNC_EXIT();
            return rc;
        }

        rc = clCorMoIdNameToMoIdGet(&(resourcesArray.pResources[resIdx].resourceName),
                &(pMoIdList->moId[moIdIdx]));
        if(CL_OK != rc)
        {
            clLogError( "ALM", "INT", " Failed while getting the MoId from resource " 
                        "name string [%s]. rc[0x%x]", resourcesArray.pResources[resIdx].resourceName.value, rc);
            if(resourcesArray.pResources != NULL)
                clHeapFree(resourcesArray.pResources);
            clHeapFree(pMoIdList);
            clHeapFree(alarmResArr.pResources);
            clAlarmClientConfigTableDelete();
            clAlarmClientConfigDataFree(&gAlarmConfig);
            CL_FUNC_EXIT();
            return CL_ALARM_RC(CL_ALARM_ERR_INTERNAL_ERROR);
        }

        /**
         * If given Alarm MSO is not associated for given resource then don't add the rule
         * silently supperss the information.
         */
        rc = clCorMoIdToMoClassPathGet(&(pMoIdList->moId[moIdIdx]), &moPath );
        if(CL_OK != rc)
        {
            clLogError( "ALM", "INT", "Failed while getting the Mo Class path \
                        from the MO [%s]. rc [0x%x]", resourcesArray.pResources[resIdx].resourceName.value, rc);
            if(resourcesArray.pResources != NULL)
                clHeapFree(resourcesArray.pResources);
            clHeapFree(pMoIdList);
            clHeapFree(alarmResArr.pResources);
            clAlarmClientConfigTableDelete();
            clAlarmClientConfigDataFree(&gAlarmConfig);
            CL_FUNC_EXIT();
            return rc;
        }

        rc = clCorMSOClassExist( &moPath, CL_COR_SVC_ID_ALARM_MANAGEMENT );
        if ( CL_OK != rc )
        {
            clLogInfo( "ALM", "INT", " The MO class do not have Alarm MSO . rc[0x%x]", rc); 
            continue;
        }
        
        if ( (rc=clCorMoIdServiceSet(&(pMoIdList->moId[moIdIdx]), CL_COR_SVC_ID_ALARM_MANAGEMENT))
              != CL_OK)
        {
            clLogError( "ALM", "INT", "Failed while setting the service Id to \
                    the MO[%s]. rc[0x%x]", resourcesArray.pResources[resIdx].resourceName.value, rc);
            if(resourcesArray.pResources != NULL)
                clHeapFree(resourcesArray.pResources);
            clHeapFree(pMoIdList);
            clHeapFree(alarmResArr.pResources);
            clAlarmClientConfigTableDelete();
            clAlarmClientConfigDataFree(&gAlarmConfig);
            return rc;
        }        

        clLogTrace( "ALM", "INT", "Adding the service rule for the component [0x%x:0x%x]",
		alarmAddr.nodeAddress, alarmAddr.portId);

        if ( (rc=clCorOIRegisterAndDisable(&(pMoIdList->moId[moIdIdx]), alarmAddr)) != CL_OK)
        {
            clLogError( "ALM", "INT", "Failed while adding the component[0x%x:0x%x] \
                    in the route list.  rc[0x%x]", alarmAddr.nodeAddress, 
                    alarmAddr.portId, rc);
            if(resourcesArray.pResources != NULL)
                clHeapFree(resourcesArray.pResources);
            clHeapFree(pMoIdList);
            clHeapFree(alarmResArr.pResources);
            clAlarmClientConfigTableDelete();
            clAlarmClientConfigDataFree(&gAlarmConfig);
            return rc;
        }

        rc = clCorMoIdToClassGet(&(pMoIdList->moId[moIdIdx]), CL_COR_MO_CLASS_GET, &classId);
        if (rc != CL_OK)
        {
            clLogError( "ALM", "INT", "Failed to get mo classId from moId. rc [0x%x]", rc);
            if(resourcesArray.pResources != NULL)
                clHeapFree(resourcesArray.pResources);
            clHeapFree(pMoIdList);
            clHeapFree(alarmResArr.pResources);
            clAlarmClientConfigTableDelete();
            clAlarmClientConfigDataFree(&gAlarmConfig);
            return rc;
        }

        rc = clAlarmClientConfigTableResAdd(classId, &(pMoIdList->moId[moIdIdx]));
        if (rc != CL_OK)
        {
            clLogError("ALM", "INT", "Failed to add resources into alarm configuration table. rc [0x%x]", rc);
            if(resourcesArray.pResources != NULL)
                clHeapFree(resourcesArray.pResources);
            clHeapFree(pMoIdList);
            clHeapFree(alarmResArr.pResources);
            clAlarmClientConfigTableDelete();
            clAlarmClientConfigDataFree(&gAlarmConfig);
            return rc;
        }

        /* Add the resource into alarm resources array */
        alarmResArr.pResources[moIdIdx] = resourcesArray.pResources[resIdx];
        moIdIdx++;

        pMoIdList->moIdCnt++; 
        alarmResArr.noOfResources++;
	}

	clLogTrace( "ALM",  "INT", "The number of resources to be managed are [%d]", 
				resourcesArray.noOfResources);

    for(i=0; i < alarmResArr.noOfResources; i++)
    {    
        rc = clAlarmClientAlarmObjectCreate(alarmResArr.pResources[i].resourceName,
                                          &(pMoIdList->moId[i]),
                                          &alarmAddr,
                                          alarmResArr.pResources[i].objCreateFlag,
                                          alarmResArr.pResources[i].autoCreateFlag,
                                          alarmResArr.pResources[i].wildCardFlag);
        if(CL_OK != rc)
        {
			clLogTrace( "ALM",  "INT", "Ignorable failure while executing "
                    "working through the resource entries, so going to the next one.. rc[0x%x]", rc);
            continue;
        }            
    }

#ifdef ALARM_CONFIG_STATS
    startTime = clOsalStopWatchTimeGet();
#endif

    rc = clCorAlarmInfoConfigure(&gAlarmConfig);
    if (rc != CL_OK)
    {
        clLogError("ALM", "CONFIG", "Failed to configure the alarm data in COR. rc [0x%x]", rc);
        if(resourcesArray.pResources != NULL)
            clHeapFree(resourcesArray.pResources);
        clHeapFree(pMoIdList);
        clHeapFree(alarmResArr.pResources);
        clAlarmClientConfigTableDelete();
        clAlarmClientConfigDataFree(&gAlarmConfig);
        return rc;
    }

#ifdef ALARM_CONFIG_STATS
    endTime = clOsalStopWatchTimeGet();
    clLogNotice("ALM", "PERF", "Resource configuration in COR took [%lld] usecs.", (endTime - startTime));
#endif

    rc = clAlarmClientCorSubscribe(pMoIdList);
    if(CL_OK != rc)
    {
        clLogError( "ALM", "INT", "Failed while subscribing for the \
                 MO creation and deletion events. rc[0x%x]",rc);
        if(resourcesArray.pResources != NULL)
            clHeapFree(resourcesArray.pResources);
        clHeapFree(pMoIdList);
        clHeapFree(alarmResArr.pResources);
        clAlarmClientConfigTableDelete();
        clAlarmClientConfigDataFree(&gAlarmConfig);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clAlarmClientMOChangeNotifSubscribe(pMoIdList);
    if (rc != CL_OK)
    {
        clLogError("ALM", "INT", "Failed to subscribe for SET events. rc [0x%x]", rc);
        if(resourcesArray.pResources != NULL)
            clHeapFree(resourcesArray.pResources);
        clHeapFree(pMoIdList);
        clHeapFree(alarmResArr.pResources);
        clAlarmClientConfigTableDelete();
        clAlarmClientConfigDataFree(&gAlarmConfig);
        return rc;
    }

    if(resourcesArray.pResources != NULL)
        clHeapFree(resourcesArray.pResources);
	
    clHeapFree(pMoIdList);
    clHeapFree(alarmResArr.pResources);

	clLogTrace( "ALM", "INT", " Leaving [%s] ", __FUNCTION__);

    return rc;
}

ClRcT clAlarmClientBaseOmClassInit()
{
    ClRcT                rc = CL_OK;
    ClOmClassControlBlockT  *pOmClassEntry = NULL;
    pOmClassEntry = &omAlarmClassTbl[0];

	clLogTrace( "ALM", "INT", " Entering [%s] ", __FUNCTION__);
	
    if ((rc = clOmClassInitialize (pOmClassEntry,
                            pOmClassEntry->eMyClassType,
                            pOmClassEntry->maxObjInst,
                            pOmClassEntry->maxSlots)) != CL_OK)
    {
		clLogError( "ALM", "INT", 
                "Failed to initialize OM class. rc[0x%x]", rc);
    }

	clLogTrace( "ALM", "INT", " Leaving [%s] ", __FUNCTION__);
	
    return rc;
}


ClRcT clAlarmChannelOpen()
{
    ClRcT    rc = CL_OK;
    ClEventCallbacksT  evtCallBack;
    ClVersionT emVersion = CL_EVENT_VERSION;
    ClNameT channelName;

	clLogTrace( "ALM", "AEM", " Entering [%s] ", __FUNCTION__);
	
    evtCallBack.clEvtChannelOpenCallback = NULL;
    evtCallBack.clEvtEventDeliverCallback = clAlarmClientNotificationHandler;

    if(CL_OK!=( rc =  clEventInitialize( &gAlarmEvtHandle, &evtCallBack, &emVersion)))
    {
        /* Over my head. Just pass the error */
		clLogError( "ALM", "AEM", 
                "Failed to initialize Event with rc[0x%x] ", rc);
        return rc;
    }    


    channelName.length = strlen(clCorEventName);
    strncpy(channelName.value, clCorEventName, channelName.length);
    rc = clEventChannelOpen(gAlarmEvtHandle, 
                            &channelName, 
                            CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL, 
                            -1, 
                            &gAlarmEvtChannelHandle);
    if(CL_OK != rc)
    {
		clLogError( "ALM", "AEM", 
                "Event Channel Open failed with rc [0x%x] ", rc);
        return rc;
    }

	clLogTrace( "ALM", "AEM", "Leaving [%s] ", __FUNCTION__);
	
    return rc;
}




ClRcT clAlarmClientCorSubscribe(ClCorMOIdListPtrT pMoIdList)
{
    ClRcT    rc = CL_OK;

	clLogTrace( "ALM", "AEM", " Entering [%s] ", __FUNCTION__);

    rc = GetAlarmUniqueId(&gAlarmMOCreateSubsId);
    if(rc != CL_OK)
    {
		clLogError( "ALM", "AEM", 
                "Unable to get unique subscription Id. rc[0x%x]", rc);
        return rc;
    }
    
	clLogTrace( "ALM", "AEM", 
            "Alarm Client subscribing for MO CREATE/DELETE Events");

    rc = clCorMOListEventSubscribe(gAlarmEvtChannelHandle,
                    pMoIdList,
                    NULL,
                    NULL,
                    CL_COR_OP_CREATE | CL_COR_OP_DELETE,
                    NULL,
                   (ClEventSubscriptionIdT) gAlarmMOCreateSubsId);

    if (CL_OK != rc)
    {
		clLogError( "ALM",  
			 "AEM", " Failed to subscribe for MO creation and deletion. rc[0x%x]", rc);
    }

	clLogTrace( "ALM", "AEM", " Leaving [%s] ", __FUNCTION__);

    return rc;
}


ClRcT clAlarmClientTimerInit(ClEoExecutionObjT* peoObj)
{
    ClRcT    rc = CL_OK;
    ClTimerTimeOutT     alarmTimeOut = {0,0};
    
    ClUint32T isPollingEnabled = 0, isSoakingTimeNonzero = 0;
    
	clLogTrace( "ALM",  "INT", " Entering [%s] ", __FUNCTION__);
	
	clLogTrace( "ALM",  "INT"," Getting the soaking time and polling from \
			the configuration.");

    rc = clAlarmSoakTimeStatusGet(&isSoakingTimeNonzero);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "INT",
                "Failed while getting the soaking status. rc[0x%x] ", rc);
        return rc;
    }

    rc = clAlarmPollingStatusGet(&isPollingEnabled);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "INT", 
                "Failed while getting the polling status. rc[0x%x] ", rc);
        return rc;
    }
    /**
    * This check is being made so that if polling is not enabled and soaking time is zero
    * for every alarm then we do not start the timer.
    */
    if(isSoakingTimeNonzero == 0 && isPollingEnabled == 0)
	{
		clLogTrace( "ALM", "INT"," Neither soaking time nor polling confiugred, \
				so not starting the timer. ");
        return rc;
	}

    alarmTimeOut.tsSec = 0;
    alarmTimeOut.tsMilliSec = ALARM_CLIENT_CALLBACK_TIME;
    
    rc= clTimerCreate(alarmTimeOut,
                      CL_TIMER_REPETITIVE,
                      CL_TIMER_SEPARATE_CONTEXT,
                      _clAlarmClientTimerCallback,
                      (void*)peoObj,
                      &gTimerHandle);

    if (CL_OK != rc)
    {
		clLogError( "ALM", "INT", 
                "Failed while creating the timer. rc[0x%x] ", rc);
        return rc;
    }

    clLogTrace("ALM", "INT", "Successfully created the timer handle [%p]", ( ClPtrT ) gTimerHandle);

    rc = clTimerStart(gTimerHandle);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "INT", 
                "Failed while starting the timer for timer handle [%p]. rc[0x%x] ", 
                (ClPtrT)gTimerHandle, rc);
        return rc;
    }

	clLogTrace( "ALM",  "INT", " Leaving [%s] ", __FUNCTION__);
			
    return rc;
}


/**
 * Fix for bug 5356: Timer thread need to be optimized.
 *
 * The new callback function is introduced which is going 
 * to the actual function which would raise the alarm 
 * after the soaking time expires.
 * The logic of updating the soaking time has been changed.
 * The time when the alarm is raise (start time) is stored.
 * Whenever the timer pops-up, the difference is calculated
 * between the current time and the start time. If the diff
 * is more than the soaking time, then the alarm is published.
 * During all this calculation, the timer is stoped, which is
 * started once calculation of the timer is done.
 */ 
ClRcT _clAlarmClientTimerCallback(ClPtrT arg)
{
    ClRcT   rc = CL_OK;

    rc = clTimerStop(gTimerHandle);
    if (CL_OK != rc)
        clLogError("ALM", "INT", "Failed while stopping the timer. rc[0x%x]", rc);

    rc = clAlarmClientPeriodicCallBack(arg);
    if (CL_OK != rc)
        clLogError("ALM", "INT", "Failed while calling the timer callback function. rc[0x%x]", rc);

    rc = clTimerStart(gTimerHandle);
    if (CL_OK != rc)
        clLogError("ALM", "INT", "Failed while starting the timer. rc[0x%x]", rc);

    return rc;
}



ClRcT clAlarmClientPeriodicCallBack(void *arg)
{

    ClRcT rc = CL_OK;

#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "INT", " Entering [%s] ", __FUNCTION__);
#endif

    if(gClAlarmLibFinalizeFlag == CL_TRUE)
	{
		clLogTrace( "ALM", "INT",
                "Periodic callback called while finalizing, so ignoring...");
        return rc;
	}

    ClEoExecutionObjT* peoObj=(ClEoExecutionObjT *)arg;
    if ( (rc = clEoMyEoObjectSet(peoObj)) != CL_OK)
    {
		clLogError( "ALM", "INT", 
                "Getting the EO object failed. rc[0x%x] ", rc);
        return rc;
    }

#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "INT",
            "Walking all the alarms configured in the timer callback function.");
#endif

    clOsalMutexLock(gClAlarmCntMutex);
    rc = clCntWalk(gresListHandle,clAlarmClientResListWalk, NULL,0);
    clOsalMutexUnlock(gClAlarmCntMutex);    
    if (CL_OK != rc)
    {
		clLogError( "ALM", "INT", 
                "Walking all the alarms failed. rc[0x%x] ", rc);
        return rc;
    }

#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "INT", "Leaving [%s] ", __FUNCTION__);
#endif

    return rc;
}



void 
clAlarmClientUpdateSoakingTime(ClCorMOIdPtrT pMoId, ClCorObjectHandleT hMSOObj)
{
    /* check if there are any alarms under soaking */
    ClUint64T     almsUnderSoaking = 0;
    ClUint64T     updatedAlmsUnderSoaking = 0;
    ClUint64T     orgAlmsUnderSoaking = 0;
    ClUint64T     almsAfterSoaking=0;
    ClUint64T     pendingAlarms = 0;
    ClUint32T     assertSoakingTime=0;
    ClUint32T     elapsedAssertSoakingTime=0;
    ClUint32T     clearSoakingTime=0;
    ClUint32T     elapsedClearSoakingTime=0;
    ClUint32T     idx=0;
    ClUint32T     size=0;
    struct        timeval startTime = {0,0};
    struct        timeval endTime = {0,0};    
    ClUint32T	  msec = 0;
    ClUint32T     lockApplied = 1;
	ClRcT		  rc = CL_OK;
    ClUint32T     isAssert = 0, isCleared = 0;

#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "INT", "Entering [%s] ", __FUNCTION__);
#endif


    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_ALMS_UNDER_SOAKING,
                    0, &almsUnderSoaking, &size, lockApplied);
    if(CL_OK != rc)
	{
		clLogError( "ALM", "INT", "Failed to get the value of alarms \
				under Soaking bit map. rc[0x%x] ", rc);
		return;
	}

    updatedAlmsUnderSoaking = almsUnderSoaking;
    orgAlmsUnderSoaking = almsUnderSoaking;

    while (almsUnderSoaking)
    {

        if ( almsUnderSoaking & 1 )
        {
#ifdef CL_ALARM_POLL_LOG
            /* update the soaking time for this alarm*/
			clLogTrace( "ALM", "INT", " UpdateSoakingTime, alm idx: [%d] \
								is under soaking .....", idx);
#endif

            /* get the current assert soaking time */
            size = sizeof(ClUint32T);
            rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_ELAPSED_ASSERT_SOAK_TIME,
                            idx, &isAssert, &size, lockApplied);
			if(CL_OK != rc)
			{
				clLogError( "ALM",  "INT", 
                        "Failed while getting the value of elapsed assert soaking \
							time. rc [0x%x] ", rc);
				return;
			}

            /* get the current clear soaking time */
            size = sizeof(ClUint32T);
            rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_ELAPSED_CLEAR_SOAK_TIME,
                            idx, &isCleared, &size, lockApplied);
			if(CL_OK != rc)
			{
				clLogError( "ALM", "INT", 
                        "Failed while getting the value of elapsed clear soaking \
							time. rc [0x%x] ", rc);
				return;
			}

            /* get the latest alarm status*/
            size = sizeof(ClUint64T);
            rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_CURRENT_ALMS_BITMAP,
                            0, &pendingAlarms, &size, lockApplied);
			if(CL_OK != rc)
			{
				clLogError( "ALM", "INT", 
                        "Failed while getting the value of current alarms bitmap. rc [0x%x] ", rc);
				return;
			}

            if ( (0 == isCleared) && (0 ==isAssert) )
            {
                /* error*/
				clLogTrace( "ALM", "INT",
                        "UpdateSoakingTime: Both assert and clear soaking taking place. error "
                        "condition ...");
            }
            else if ( 0 != isAssert)
            {
#ifdef CL_ALARM_POLL_LOG
				clLogTrace( "ALM", "INT", "Updating the assert soaking time "
                        "for alarm Index [%d]", idx);
#endif

                /* update the soaking time*/
                size = sizeof(ClUint32T);
                rc = clAlarmClientEngineProfAttrValueGet(pMoId, hMSOObj, CL_ALARM_ASSERT_SOAKING_TIME,
                                idx, &assertSoakingTime, &size);
				if(CL_OK != rc)
				{
					clLogError( "ALM", "INT", 
                            "Failed while getting the assert soak time from configuration. rc[0x%x]", rc);
					return;
				}
            
                    
                /* now find the correct initial elapsed time */
                rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_SOAKING_START_TIME,
                            idx, &startTime, &size, lockApplied);
                if(CL_OK != rc)
                {
                    clLogError( "ALM", "INT", 
                            "Failed while getting soaking start time for alarm index [%d]. rc[0x%x]", idx, rc);
                    return;
                }
							
                gettimeofday(&endTime, NULL);
                msec = clAlarmClientEngineTimeDiffCalc(&startTime, &endTime);

                elapsedAssertSoakingTime = msec;

                if (elapsedAssertSoakingTime >= assertSoakingTime)
                {
					clLogDebug( "ALM",  "INT", "Elapsed soak time exceeded the assert soak \
									time for alarm index [%d]", idx);

                    /* look at the current status of the alarm */
                    if (pendingAlarms & ((ClUint64T) 1<<idx))
                    {
#ifdef CL_ALARM_POLL_LOG
						clLogTrace( "ALM", "INT", "In Assert Soak time the alarm persisted \
									for the alarm index [%d]", idx);
#endif

                        /* alarm still exists*/
                        size = sizeof(ClUint64T);
                        rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
                                        0, &almsAfterSoaking, &size, lockApplied);
						if(CL_OK != rc)
						{
							clLogError( "ALM", "INT", "Failed while getting the value for alarms \
									after soaking bitmap. rc[0x%x]", rc);
							return;
						}
#ifdef CL_ALARM_POLL_LOG
						clLogTrace( "ALM", "INT", "Updating the alarms after soaking bitmap for \
										alarm index [%d]", idx);
#endif

                        almsAfterSoaking |= ((ClUint64T) 1<<idx);
                        size = sizeof(ClUint64T);
                        rc = SetAlmAttrValue(pMoId,NULL,hMSOObj, 
                                       CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
                                        0, &almsAfterSoaking, size);
						if(CL_OK != rc)
						{
							clLogError( "ALM", "INT", "Failed while updating the alarms after \
										soaking bitmap for alarm index [%d]. rc[0x%x]", idx, rc);
							return ;
						}

						clLogDebug( "ALM", "INT", 
                                "Processing the assert alarm request for alarm index [%d]", idx);
						
                        /* process the alarm for affected alarms*/
                         clAlarmClientEngineAffectedAlarmsProcess(pMoId, hMSOObj, idx, 
                                                           1 /*assert*/,lockApplied,
                                                           NULL);
                    }
                    /* clear the alarm from soaking*/
                    isAssert = 0;
                    size = sizeof(ClUint32T);
                    rc = SetAlmAttrValue(pMoId,NULL,hMSOObj, CL_ALARM_ELAPSED_ASSERT_SOAK_TIME,
                                       idx, &isAssert, size);
                    if(CL_OK != rc)
                    {
                        clLogError( "ALM", "INT", "Failed while updating the value of elapsed \
                                assert soak time for the alarm index [%d]. rc[0x%x]", idx, rc);
                        return ;
                    }

                    updatedAlmsUnderSoaking &= ~((ClUint64T) 1<<idx);
#ifdef CL_ALARM_POLL_LOG
					clLogTrace( "ALM", "INT", "UpdateSoakingTime: in assert.. \
                                          updatedAlmsUnderSoaking [%d] for alarm index [%d]\n", 
                                          updatedAlmsUnderSoaking, idx) ;
#endif
                    
                }
				
#ifdef CL_ALARM_POLL_LOG
				clLogTrace( "ALM", "INT", "Updating the elapsed assert soak time for alarm \
						index [%d]", idx);
#endif
            
            }
            else if (0 != isCleared)
            {
#ifdef CL_ALARM_POLL_LOG
				clLogTrace( "ALM", "INT", "Updating the clear soak time for the alarm \
						index [%d]", idx);
#endif

                /* update the soaking time*/
                size = sizeof(ClUint32T);
                rc = clAlarmClientEngineProfAttrValueGet(pMoId, hMSOObj, CL_ALARM_CLEAR_SOAKING_TIME,
                                                    idx, &clearSoakingTime, &size);
				if(CL_OK != rc)
				{
					clLogError( "ALM", "INT", "Failed while getting the clear soak time from \
						   configuration for alarm index [%d]. rc[0x%x]", idx, rc);
					return;
				}
                
                rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_SOAKING_START_TIME,
                                    idx, &startTime, &size, lockApplied);
                if(CL_OK != rc)
                {
                    clLogError( "ALM", "INT", "Failed while getting the alarm soak start \
                                    time for alarm index [%d]. rc[0x%x]", idx, rc); 
                    return;	
                }

                gettimeofday(&endTime, NULL);
                msec = clAlarmClientEngineTimeDiffCalc(&startTime, &endTime);

                elapsedClearSoakingTime = msec;
                    
                if (elapsedClearSoakingTime >= clearSoakingTime)
                {
					clLogDebug( "ALM", "INT", "Elapsed soaking time has exceeded the \
						        the clear soaking time for alarm index [%d]", idx);
                    
                    /* look at the current status of the alarm */
                    if (! (pendingAlarms & ((ClUint64T) 1<<idx)) )
                    {
#ifdef CL_ALARM_POLL_LOG
						clLogTrace( "ALM", "INT", "UpdateSoakingTime: in clearsoakingtime, \
							alarm has cleared suring soaking for idx: [%d]... \n", idx );
#endif

                        /* alarm has cleared */
                        size = sizeof(ClUint64T);
                        rc = GetAlmAttrValue(pMoId, hMSOObj, CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
                                        0, &almsAfterSoaking, &size, lockApplied);
						if(CL_OK != rc)
						{
							clLogError( "ALM", "INT", "Failed while getting the value of alarms after \
								   soaking bitmap. rc[0x%x]", rc);
							return;
						}
                                                                 
                        almsAfterSoaking &= ~((ClUint64T) 1<<idx);
                        size = sizeof(ClUint64T);
                        rc = SetAlmAttrValue(pMoId,NULL,hMSOObj, CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
                                        0, &almsAfterSoaking, size);
						if(CL_OK != rc)
						{
							clLogError( "ALM", "INT", "Failed while updating the value of alarms after \
								   soaking bitmap. rc[0x%x]", rc);
							return;
						}

						clLogDebug( "ALM", "INT", "Clearing the alarm persisted after clear soak time \
								for alarm index [%d].", idx);

                        /* process the alarm for affected alarms*/
                        clAlarmClientEngineAffectedAlarmsProcess(pMoId, hMSOObj,
                                                    idx, 0 /*clear*/,lockApplied, NULL);
                                                         
                    }
#ifdef CL_ALARM_POLL_LOG				
                    clLogTrace( "ALM", "INT", "UpdateSoakingTime: in clear..\
                                           updatedAlmsUnderSoaking: [%d]\n", 
                                           updatedAlmsUnderSoaking);
#endif
                    /* clear the alarm from soaking*/
                    isCleared = 0;
                    size = sizeof(ClUint32T);
                    rc = SetAlmAttrValue(pMoId,NULL,hMSOObj, CL_ALARM_ELAPSED_CLEAR_SOAK_TIME,
                                    idx, &isCleared, size);
                    if(CL_OK != rc)
                    {
                        clLogError( "ALM", "INT", "Failed while updating the elapsed soak time \
                                for alarm index [%d]. rc[0x%x]", idx, rc);
                        return ;	
                    }

                    updatedAlmsUnderSoaking &= ~((ClUint64T) 1<<idx);
                }
#ifdef CL_ALARM_POLL_LOG
				clLogTrace( "ALM", "INT", "UpdateSoakingTime: elapsedClearSoakingTime: [%d] \n", 
                                      elapsedClearSoakingTime);
#endif
            }
        }
        idx++;
        almsUnderSoaking >>= 1;
    }

    /* put back the updated updatedAlmsUnderSoaking value*/
    if(updatedAlmsUnderSoaking != orgAlmsUnderSoaking)
    {
#ifdef CL_ALARM_POLL_LOG
		clLogTrace( "ALM", "INT","Updating the alarms undex soaking bitmap ");
#endif
		
        size = sizeof (ClUint64T);
        rc = SetAlmAttrValue(pMoId,NULL,hMSOObj, CL_ALARM_ALMS_UNDER_SOAKING,
                        0, &updatedAlmsUnderSoaking, size);
		if(CL_OK != rc)
		{
			clLogError( "ALM", "INT", "Failed while updating the alarms under \
					soaking bit map. rc[0x%x]", rc);
			return;
		}
    }
	
#ifdef CL_ALARM_POLL_LOG
	clLogTrace( "ALM", "INT", " Leaving [%s] ", __FUNCTION__);
#endif

}



ClRcT clAlarmClientResourceTableInfoGet(ClCorAddrT* pAlarmAddr, ClOampRtResourceArrayT* pResourcesArray)
{
    ClRcT           rc = CL_OK;
    ClCpmHandleT    cpmHandle = 0;
    ClNameT         compName = {0};
    ClParserPtrT    top = NULL;
    ClCharT         fileName[CL_MAX_NAME_LENGTH] = {0};
    ClCharT         *pFileName = fileName;
    ClCharT         *aspPath = NULL;
    ClCharT         *nodeName = NULL;

    CL_FUNC_ENTER();

	clLogTrace( "ALM", "INT", " Entering [%s] ", __FUNCTION__);
			
    aspPath = getenv("ASP_CONFIG");
    nodeName = getenv("ASP_NODENAME");
    
    if(!aspPath || !nodeName)
    {
        clLogError("ALM", "INT", "ASP config variables are not set in the environment");
        return CL_ALARM_RC(CL_ERR_DOESNT_EXIST);
    }

    if(gClOIConfig.oiDBReload)
    {
        if(!gClOIConfig.pOIRouteFile)
        {
            clLogError("ALM", "INT", "OI has db reload flag set but hasn't specified the route config file");
            return CL_RC(CL_CID_ALARMS, CL_ERR_NULL_POINTER);
        }
        pFileName = (ClCharT*)gClOIConfig.pOIRouteFile;
        if(gClOIConfig.pOIRoutePath)
            aspPath = (ClCharT*)gClOIConfig.pOIRoutePath;
        
    }
    else
        snprintf(fileName, sizeof(fileName), "%s_%s", nodeName, "rt.xml");

    clLogTrace( "ALM", "INT", "Opening the file [%s] containing resorce information. ", 
                pFileName);

    top = clParserOpenFile(aspPath, pFileName);
    if (top == NULL)
    {
        clLogError( "ALM", "INT", "Error while opening the route file [%s] at path [%s]",
                    pFileName, aspPath);

        return CL_RC(CL_CID_ALARMS, CL_ERR_NULL_POINTER);
    }

	clLogTrace( "ALM", "INT",
                "Getting the node address and ioc port");

    /* Get the local ioc address. this will be used to get the moid for an node */
    pAlarmAddr->nodeAddress = clIocLocalAddressGet();
    rc = clEoMyEoIocPortGet(&pAlarmAddr->portId);
    if(CL_OK != rc)
    {
        clParserFree(top);
		clLogError( "ALM",  
                    "INT", "Not able to get IOC port identifier from Exection object data. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clCpmComponentNameGet(cpmHandle, &compName);
    if(CL_OK != rc)
    {
        clParserFree(top);
        clLogError("ALM", "INT", "Failed while getting component name. rc[0x%x]",rc);
        CL_FUNC_EXIT();
        return rc;
    }

	clLogTrace( "ALM", "INT", "Got the component name [%s]. Now looking for that name in "
                "the file [%s] ", compName.value, fileName);
			
    rc = clOampRtResourceInfoGet(top, &compName, pResourcesArray);
    if(CL_OK != rc)
    {
        if ( ! ((rc == CL_OAMP_RT_RC(CL_ERR_DOESNT_EXIST) ||
                 rc == CL_OAMP_RT_RC(CL_OAMP_RT_ERR_INTERNAL)) && 
                (pAlarmAddr->portId == CL_IOC_ALARM_PORT)) )
        {
		    clLogError( "ALM", "INT", "Failed while getting the resource information from \
                file [%s] for component [%s]. rc[0x%x] ", fileName, compName.value, rc);
        }

        CL_FUNC_EXIT();
        clParserFree(top);
        return rc;
    }

	clLogTrace( "ALM", "INT", " Leaving [%s] ", __FUNCTION__);

    CL_FUNC_EXIT();
    clParserFree(top);
    return CL_OK;
}



                                        // coverity[pass_by_value]
ClRcT clAlarmClientAlarmObjectCreate(   ClNameT moIdName,
                                        ClCorMOIdPtrT pFullMoId,
                                        ClCorAddrT* pAlarmAddr,
                                        ClUint32T createFlag,
                                        ClBoolT   autoCreateFlag,
                                        ClUint32T wildCardFlag)
{
    ClRcT rc = CL_OK;
    ClCorObjectHandleT corObjHandle;    
    ClCorAddrT     alarmClientAddr;
    ClEoExecutionObjT*   eoObj;

    CL_FUNC_ENTER();

	clLogTrace( "ALM", "INT", " Entering [%s] ", __FUNCTION__);

	if(pFullMoId == NULL || pAlarmAddr == NULL)
	{
		clLogError( "ALM", "INT", "NULL pointer passed. rc[0x%x]", CL_ERR_NULL_POINTER );
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
	}
	
    rc = clEoMyEoObjectGet(&eoObj);
    if(CL_OK != rc)
    {
		clLogError( "ALM", "INT", " Failed while getting the IOC port Identifier from \
                Exection Object data . rc[0x%x]",rc);
		CL_FUNC_EXIT();
        return rc;
    }

    alarmClientAddr.nodeAddress = clIocLocalAddressGet();
    alarmClientAddr.portId = eoObj->eoPort;

    if (createFlag == CL_TRUE)    
    {
		clLogTrace("ALM", "INT","The creation flag is set to TRUE");

        rc = clCorMoIdServiceSet( pFullMoId, CL_COR_INVALID_SRVC_ID);
        if ( CL_OK != rc )
        {
			clLogError( "ALM", "INT", "Failed while resetting service id for the OM. rc[0x%x]", rc);
            return rc;
        }

        /**
         * Get the object, if the object is not there then create the object.
         */
		
		clLogTrace( "ALM", "INT","Checking the MO is already created ");

        if (!autoCreateFlag)
        {
            rc = clCorObjectHandleGet( pFullMoId, &corObjHandle );
            if ( CL_OK != rc )
            {
                clLogTrace( "ALM", "INT", "MO[%s] not present so creating the object ", moIdName.value);

                rc = clCorObjectCreate( NULL, pFullMoId, NULL );
                if ( CL_OK != rc )
                {
                    clLogError( "ALM",  
                         "INT", "Failed while creating the object. rc[0x%x]", rc);
                    return rc;
                }
            }
            else
            {
                clCorObjectHandleFree(&corObjHandle);
            }
        }

        rc = clCorMoIdServiceSet( pFullMoId, CL_COR_SVC_ID_ALARM_MANAGEMENT );
        if ( CL_OK != rc )
        {
			clLogError( "ALM", "INT", "Failed putting the service Id for the MO[%s]. rc[0x%x]", 
                    moIdName.value, rc);
            return rc;
        }
		
		clLogTrace( "ALM", "INT",
                "Checking the Alarm MSO is already created ");

        if (!autoCreateFlag)
        {
            /* check if the object is already created */
            rc = clCorObjectHandleGet( pFullMoId, &corObjHandle );
            if ( CL_OK != rc )
            {
                clLogTrace( "ALM", "INT",
                        "Alarm MSO not present so creating the object ");

                rc = clCorObjectCreate( NULL, pFullMoId, NULL);
                if ( CL_OK != rc )
                {
                    clLogTrace( "ALM", "INT",
                            "Alarm MSO not present [%s] so creating the object ", moIdName.value);
                     return rc;
                }
            }
            else
                clCorObjectHandleFree(&corObjHandle);
        }
       
        clLogTrace( "ALM", "INT",
                "Configuring the alarms as MSO is already created ");

        /*
         * Object is already created in COR, configure the alarm msos and don't rely on the 
         * event delivery callback.
         */
        rc = clAlarmBootUpAlarmMsoProcess(pFullMoId);
        if(CL_OK != rc)
        {
            clLogError( "ALM", "INT", 
                    "Failed while configuring the alarm mso at the bootup time.  rc[0x%x]",rc);
            return rc;
        }                
    }
    else
    {
        if (wildCardFlag)
        {
			clLogTrace( "ALM", "INT"," The resource [%s] is a wild card entry.  ", moIdName.value);

            ClUint16T depth;
            ClCorMOIdPtrT rootMoId;
            clCorMoIdServiceSet(pFullMoId, CL_COR_SVC_ID_ALARM_MANAGEMENT);
            clCorMoIdAlloc(&rootMoId);
            clCorMoIdInitialize(rootMoId); 
            clCorMoIdClone(pFullMoId, &rootMoId);
            depth = clCorMoIdDepthGet(rootMoId);
            clCorMoIdTruncate(rootMoId, clCorMoIdDepthGet(rootMoId) - 1);
            rc = clCorObjectWalk(rootMoId, pFullMoId,  clAlarmWildCardMoIdObjWalk, CL_COR_MSO_WALK, NULL);
            if(CL_OK != rc)
            {
				clLogError( "ALM", "INT", "Failed while doing a walk on all the resource \
						matching the wild carded resource [%s]. rc[0x%x] ", moIdName.value, rc);
            }                        
            clCorMoIdFree(rootMoId);

            return rc;
        }

        /* MOID does not have wild card */
		clLogTrace( "ALM", "INT", "For the resource [%s] the create flag is FALSE", moIdName.value);

        rc = clCorObjectHandleGet(pFullMoId, &corObjHandle);
        if (CL_OK == rc)
        {
			clLogTrace( "ALM", "INT", "Object is alreay present so configuring it.");

            rc = clAlarmBootUpAlarmMsoProcess(pFullMoId);
            if(CL_OK != rc)
            {
				clLogTrace( "ALM",  
					 "INT", "Failed while configuring the alarm mso. rc[0x%x]",rc);
                return rc;            
            }            

            clCorObjectHandleFree(&corObjHandle);
        }
        else
        {
			clLogTrace( "ALM", "INT", "The MSO for the resource [%s] is not present so returning from here.", 
                    moIdName.value);

            /* MSO not present. dont bother */
            rc = CL_OK;
        }
    }
	
	clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__);

    CL_FUNC_EXIT();
    return rc;
}




ClRcT clAlarmWildCardMoIdObjWalk(void*  pData, void *cookie)
{
    ClRcT rc = CL_OK;
    ClCorObjectHandleT corObjHandle = *(ClCorObjectHandleT*)pData; 
    ClCorMOIdT moId;
    ClCorServiceIdT srvcId = CL_COR_INVALID_SVC_ID;

	clLogTrace( "ALM", "INT", "Entering [%s]", __FUNCTION__);

    rc = clCorObjectHandleToMoIdGet(corObjHandle, &moId, &srvcId);
    if (CL_OK != rc)
    {
		clLogError( "ALM",  "INT", "Failed while getting the MoId from Object \
			handle. rc[0x%x]", rc);
        return rc;
    }    

    if (srvcId != CL_COR_SVC_ID_ALARM_MANAGEMENT)
        return CL_OK;

	clLogTrace( "ALM", "INT", "Configuring the MSO which is matching the \
		wildcarded resource found after object walk. ");

    rc = clAlarmBootUpAlarmMsoProcess(&moId);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "INT", "Failed while configuring the alarm MSO, found while \
			doing object walk. rc [0x%x]", rc);
			
        return rc;
    }

	clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__);

    return rc;
    
}




ClRcT clAlarmBootUpAlarmMsoProcess(ClCorMOIdPtrT pMoId)
{

    ClRcT rc = CL_OK;
	clLogTrace( "ALM", "INT", "Entering [%s]", __FUNCTION__);

    clLogTrace( "ALM", "INT", "Before doing the Alarm OM create for the MSO");

    rc = clAlarmClientAlarmOMCreate(pMoId);
    if(CL_OK != rc)
    {
        clLogError( "ALM", "INT", "Failed while creating the Alarm OM Class. rc[0x%x]", rc);
        return rc;
    }
    
	clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__);

    return rc;
}


ClRcT clAlarmClientMOChangeNotifSubscribe(ClCorMOIdListPtrT pMoIdList)
{
    ClCorAttrPathPtrT pAttrPath = NULL;        
    ClCorAttrListT*   pAttrList = NULL;
    ClUint32T           count;     
    ClRcT          rc = CL_OK;    
    ClInt32T myAttrIds[] = {CL_ALARM_CLEAR, CL_ALARM_ENABLE};

	clLogTrace( "ALM", "AEM", "Entering [%s]", __FUNCTION__);

    rc = clCorAttrPathAlloc(&pAttrPath);
    if (rc != CL_OK)
    {
        clLogError( "ALM", "AEM", "Failed while allocating the memory for \
            attribute path. rc[0x%x] \n", rc);
        return rc;
    }
    
    rc = clCorAttrPathAppend( pAttrPath, CL_ALARM_INFO_CONT_CLASS, CL_COR_INSTANCE_WILD_CARD);
    if (rc != CL_OK)
    {
        clLogError( "ALM", "AEM", "Failed while appending the attribute id \
            in the attribute path. rc[0x%x]", rc);
        clCorAttrPathFree(pAttrPath);                
        return rc;
    }

    pAttrList = (ClCorAttrListT *) clHeapAllocate(sizeof(ClCorAttrListT) + ((CL_ALARM_MAX_RES_ATTR_SUBS_ID) * sizeof(ClInt32T)));
    if (pAttrList == NULL)
    {
        clLogError("ALM", "AEM", "Failed to allocate memory.");
        clCorAttrPathFree(pAttrPath);
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }

    pAttrList->attrCnt = CL_ALARM_MAX_RES_ATTR_SUBS_ID;

    count = 0;
    while (count < CL_ALARM_MAX_RES_ATTR_SUBS_ID)
    {
        pAttrList->attr[count] = myAttrIds[count];
        count++;
    }

    rc = GetAlarmUniqueId(&gAlarmMOChangeSubsId);
    if(rc != CL_OK)
    {
        clCorAttrPathFree(pAttrPath);                
        clHeapFree(pAttrList);
        clLogError( "ALM",  "AEM", " Failed to get the unique subscription \
            identifier, rc[0x%x]", rc);
         return rc;
    }

    clLogTrace( "ALM", "AEM", 
            "Before doing event subscribe for the attributes."
            "Subscription Id [0x%x]", gAlarmMOChangeSubsId);
    
    rc = clCorMOListEventSubscribe(gAlarmEvtChannelHandle,
                        pMoIdList,
                        pAttrPath,
                        pAttrList,
                        CL_COR_OP_SET,
                        NULL,
                        gAlarmMOChangeSubsId);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", 
                "Failed while subscribing for Attribute Id [0x%x]. rc[0x%x]", pAttrList->attr[0], rc);
    }

    rc = clCorAttrPathFree(pAttrPath);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", 
                "Failed while freeing the attribute path. rc[0x%x]", rc);
        clHeapFree(pAttrList);
        return rc;
    }

    clHeapFree(pAttrList);

	clLogTrace( "ALM", "AEM", "Leaving [%s]", __FUNCTION__);

    return rc;
}



ClRcT clAlarmClientAlarmOMCreate(ClCorMOIdPtrT hMoId)
{
    ClOmClassTypeT    omClass;
    void             *pOmObj = NULL;    
    ClCorClassTypeT   myMOClassType;
    ClRcT            rc = CL_OK, tempRc = CL_OK;
	ClHandleT            handleOm;    
    ClAlarmResCntDataT* pResCntData;
    ClCorMOIdPtrT 	   pMoIdKey = NULL;
	
	clLogTrace( "ALM", "INT", " Entering [%s]", __FUNCTION__);

    rc = clCorMoIdToClassGet(hMoId, CL_COR_MO_CLASS_GET,  &myMOClassType);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT", "Failed while getting the class-id for a \
            given moId. rc[0x%x]", rc);
        return rc;
    }    
            
    rc = clOmClassFromInfoModelGet(myMOClassType, 
                                   CL_COR_SVC_ID_ALARM_MANAGEMENT,&omClass);
    if ( rc != CL_OK)
    {
        clLogError("ALM", "INT", "Failed while getting the value of the OM Class id for the \
                               MSO class [0x%x]. rc[0x%x]", myMOClassType, rc);
        return rc;
    }

	clLogTrace( "ALM", "INT", " The alarm Mso class Id [0x%x] has the Om class "
            "id [0x%x]", myMOClassType, omClass);

    if (( rc=clOmMoIdToOmHandleGet(hMoId, &handleOm)) != CL_OK)
    {

	    clLogTrace( "ALM", "INT", "The OM entry is not present for the "
                "Alarm MSO so creating it");

        rc = clOmObjectCreate(omClass, 1, &handleOm, &pOmObj, hMoId, sizeof(ClCorMOIdT));
        if (pOmObj == NULL)
        {
	        clLogError( "ALM", "INT", "Failed while creating the OM object. rc[0x%x]", rc);
            return CL_ALARM_RC(CL_ALARM_ERR_OM_CREATE_FAILED);
        }

        if((rc= clOsalMutexCreate( &((CL_OM_ALARM_CLASS*)pOmObj)->cllock))!=
                CL_OK)
        {
	        clLogError( "ALM", "INT", "Failed while creating the mutex. rc[0x%x]", rc);

            if ((rc = clOmObjectByReferenceDelete(pOmObj)) != CL_OK)
            {
	            clLogError( "ALM",  
		             "INT", " Failed while deleting the OM object. rc[0x%x]", rc);
                return rc;
            }

            return rc;
            
        }

	    clLogTrace( "ALM", "INT", "Successfully created the OM object");
        
        if ( (rc=clOmMoIdToOmIdMapInsert(hMoId, handleOm)) != CL_OK)
        {
            clLogError("ALM", "INT", "Failed to install OM/COR table entry. rc[0x%x]", rc);
                                                                                            
            /* delete the lock first */
            if ( (tempRc = clOsalMutexDelete(((CL_OM_ALARM_CLASS*)pOmObj)->cllock))!= CL_OK)
            {
	            clLogError("ALM", "INT", "Failed while deleting the mutex. rc[0x%x]", tempRc);
                return rc;
            }

            /* delete OM obj */
            if ((tempRc = clOmObjectByReferenceDelete(pOmObj)) != CL_OK)
            {
	            clLogError( "ALM", "INT", " Failed while deleting the OM object. rc[0x%x]",tempRc);
                return rc;
            }
            return rc;
        }

		pResCntData = (ClAlarmResCntDataT*)clHeapAllocate (sizeof (ClAlarmResCntDataT));
		if(pResCntData == NULL)
		{
            clLogError( "ALM", "INT", " Failed while allocating the memory for the \
                resource container data. rc[0x%x]", rc);
			return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
		}

/* Earlier the omhanlde was being used as the key and the moid happened to be one of the data
 * members of the container data. Now the moid is being made the key and omhandle as one of 
 * data members */
		rc = clAlarmStaticInfoCntAdd(hMoId,pResCntData);
		if(CL_OK !=rc )
		{
            clLogError( "ALM", "INT", "Failed while adding the resource container data. rc[0x%x]", rc);
			clHeapFree(pResCntData);
			return rc;
		}

		rc = clCorMoIdClone(hMoId, &pMoIdKey);
		if(CL_OK !=rc )
		{
            clLogError( "ALM", "INT", "Failed while cloning the MoId. rc[0x%x]", rc);
			clHeapFree(pResCntData);
			return rc;
		}

		pResCntData->omHandle = handleOm;

		clOsalMutexLock(gClAlarmCntMutex);        
		rc = clCntNodeAdd((ClCntHandleT)gresListHandle,(ClCntKeyHandleT)pMoIdKey,(ClCntDataHandleT)pResCntData,NULL);
		clOsalMutexUnlock(gClAlarmCntMutex);
		if (CL_OK != rc)
		{
            clLogError( "ALM", "INT", "Failed while adding the resource container data. rc[0x%x]", rc);
			clHeapFree(pResCntData);
			clCorMoIdFree(pMoIdKey);
			
            if ((tempRc = clOmObjectByReferenceDelete(pOmObj)) != CL_OK)
            {
                clLogError( "ALM", "INT", "Failed to delete OM object. rc[0x%x]", tempRc);
                return rc;
            }
            /* delete the lock first */
            if ( (tempRc = clOsalMutexDelete(((CL_OM_ALARM_CLASS*)pOmObj)->cllock))!= CL_OK)
            {
                clLogError( "ALM", "INT", "Failed to delete mutex. rc[0x%x]", tempRc);
                return rc;
            }
		}

    }
    else
    {
        clLogTrace( "ALM", "INT", "OM Object already present. rc[0x%x]", rc);
    }

	clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__);

    return rc;
}


ClRcT clAlarmClientCorNodeAddressToMoIdGet(ClCorMOIdPtrT moId)
{
    ClRcT rc = CL_OK;
    ClNameT nodeName;

	clLogTrace( "ALM", "INT", "Entering [%s]", __FUNCTION__);

    rc = clCpmLocalNodeNameGet(&nodeName);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT", " Failed while getting the local node name \
            from CPM. rc[0x%x]", rc);
        return rc;
    }

    rc = clCorNodeNameToMoIdGet(nodeName,moId);
    if (CL_OK != rc)
    {
        clLogError( "ALM",  
		     "INT", " Failed to get the MoId corresponding to the nodeName[%s] \
            from COR. rc[0x%x]", nodeName.value, rc);
        return rc;
    }

	clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__);

    return rc;
}



void clAlarmClientNotificationHandler( ClEventSubscriptionIdT subscriptionId, 
                                       ClEventHandleT eventHandle, 
                                       ClSizeT eventDataSize )
{
    ClCorOpsT      operation;
    ClCorTxnIdT    corTxnId;
    ClCorTxnJobIdT jobId;
    ClRcT rc = CL_OK;    

	clLogTrace( "ALM", "AEM", " Entering [%s]", __FUNCTION__);

    rc = clCorEventHandleToCorTxnIdGet(eventHandle, eventDataSize, &corTxnId);
    if(CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed while getting cor-txn handle from the \
            event handle. rc[0x%x]", rc);
        return;
    }

    rc = clCorTxnFirstJobGet(corTxnId, &jobId);
    if(CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed to get the first job from the COR \
            job-list. rc[0x%x]", rc);
        return;
    }

    do{ 
        clLogTrace( "ALM", "AEM", "Processing the COR jobs form the list.");

        rc = clCorTxnJobOperationGet(corTxnId, jobId, &operation);
        if(CL_OK != rc)
        {
            clLogError( "ALM", "AEM", "Failed to get the operation type from \
                the COR job. rc[0x%x]", rc);
            return;
        }    

        switch(operation)
        {
            case CL_COR_OP_CREATE:
                
                clLogTrace( "ALM", "AEM", "The Operation type is CREATE");
                
                rc = clAlarmClientCreateNotifProcess(corTxnId, jobId);
                if(rc != CL_OK)
                    clLogError("ALM", "AEM", "Failed while processing the MO creation event. rc[0x%x]", rc);

                break;
            case CL_COR_OP_DELETE:
                
                clLogTrace( "ALM", "AEM", "Operation type is DELETE");
                
                rc = clAlarmClientDeleteNotifProcess(corTxnId, jobId);            
                if(CL_OK != rc)
                    clLogError("ALM", "AEM", "Failed while processing the MO deletion event. rc[0x%x]", rc);

                break;
            case CL_COR_OP_SET:
                
                clLogTrace( "ALM",  "AEM", "Operation type is SET");
                
                rc = clAlarmSetJobProcess(corTxnId, jobId);
                if(CL_OK != rc)
                    clLogError("ALM", "AEM", "Failed while processing the attribute set event. rc[0x%x]", rc);

                break;
            default:
                break;
        }
    } while(clCorTxnNextJobGet(corTxnId, jobId, &jobId) == CL_OK);

    rc = clCorTxnIdTxnFree(corTxnId);    
    if(CL_OK != rc)
    {
        clLogTrace( "ALM",  
             "AEM", "Failed while freeing the cor-txn handle. rc[0x%x]", rc);
        return;
    }    
	
    clLogTrace( "ALM", "AEM", " Leaving [%s]", __FUNCTION__);
}

ClRcT clAlarmSetJobProcess(ClCorTxnIdT  corTxnId,
                      ClCorTxnJobIdT     jobId)
{
    /* TODO - COR-TXN */

    ClUint8T alarmEnabled = 0;
    ClUint8T alarmSuspend = 0;    
    ClUint8T alarmClear = 0;    
    ClCorAttrPathPtrT pAttrPath;    
    ClCorMOIdPtrT  hMoId = NULL;    
    ClCorObjectHandleT hMSOObj;        
    ClUint32T size = 0;
    ClRcT    rc = CL_OK;
    ClAlarmInfoT* pAlarmInfo;    
    ClAlarmProbableCauseT        probCause;
    ClCorAttrIdT           attrId;
    ClInt32T               index;
    void                  *pValue;
    ClUint32T primaryOI = 0;

    clLogTrace( "ALM", "AEM", " Entering [%s]", __FUNCTION__);
 
    rc = clCorTxnJobAttrPathGet(corTxnId, jobId, &pAttrPath);
    if(pAttrPath == NULL)
    {
        clLogError( "ALM", "AEM", "Failed to get the attribute path . rc[0x%x]", rc);
        return rc;
    }


    if ( (rc = clCorMoIdAlloc(&hMoId)) != CL_OK)
    {
        clLogError( "ALM", "AEM", "Failed while allocating the MoId . rc[0x%x]", rc);
        return rc;
    }    

    if ((rc = clCorMoIdInitialize(hMoId)) != CL_OK)
    {
        clLogError( "ALM", "AEM", "Failed while initializing the MoId . rc[0x%x]", rc);
        clCorMoIdFree (hMoId);
        return rc;
    }            

    if (CL_OK != (rc = clCorTxnJobMoIdGet(corTxnId, hMoId)))
    {
        /* Things are real bad.   Return failure */
        clLogError( "ALM", "AEM", "Failed to get the MoId from the COR job. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        clCorMoIdFree (hMoId);        
        return rc;
    }

    rc = clAlarmCheckIfPrimaryOI(hMoId, &primaryOI);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed while checking it the resource is the primary OI. rc[0x%x]", rc);
        clCorMoIdFree (hMoId);        
        return rc;
    }

    if(primaryOI != 1)
    {
        clLogTrace( "ALM", "AEM", "The component is not a primary OI, so returning simply.");
        return CL_OK;
    }

    clLogTrace( "ALM", "AEM", "Proceeding further as it is a primary OI.");
    
    rc = clCorTxnJobObjectHandleGet(corTxnId, &hMSOObj);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed to get object handle from the COR job. rc[0x%x]", rc); 
        clCorMoIdFree (hMoId);
        return rc;
    }        
        
    rc = clCorTxnJobSetParamsGet(corTxnId,
                                 jobId,
                                 &attrId,
                                 &index,
                                 &pValue,
                                 &size);    
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed to get the different arguments of the set job. rc[0x%x]", rc);
        clCorMoIdFree (hMoId);
        return rc;
    }        
 
    switch(attrId)
    {
        case CL_ALARM_ENABLE:
            
            alarmEnabled =  *(ClUint8T*)pValue;

            /* now set alarmSuspend attr appropriately */
            if (alarmEnabled)
                alarmSuspend = 0;
            else
                alarmSuspend = 1;
                    
            clLogTrace("ALM", "AEM", "The CL_ALARM_ENABLE attribute is set to [%d], "
                    "so setting the CL_ALARM_SUSPEND to [%d]", alarmEnabled, alarmSuspend);

            rc = clCorObjectAttributeSet(NULL, hMSOObj, pAttrPath ,  
                CL_ALARM_SUSPEND, CL_COR_INVALID_ATTR_IDX, 
                &alarmSuspend, sizeof(alarmSuspend));

            if (rc != CL_OK)
            {
                clLogError( "ALM",  "AEM", "Failed while setting the CL_ALARM_ENABLE attribute. rc[0x%x]", rc);
                clCorMoIdFree (hMoId);                            
                return rc;
            }                
            
             break;
        case CL_ALARM_CLEAR:

            clLogTrace( "ALM", "AEM", "CL_ALARM_CLEAR attribute is set to [%d]",
                *(ClUint8T *)pValue);
                
            size = sizeof(ClUint32T);
            rc = clCorObjectAttributeGet(hMSOObj, 
                                         pAttrPath, 
                                         CL_ALARM_PROBABLE_CAUSE, 
                                         CL_COR_INVALID_ATTR_IDX,  
                                         &probCause, 
                                         &size);
            if (rc != CL_OK)
            {
                clLogError( "ALM",  "AEM", " Failed to get the value of  \
                    CL_ALARM_PROBABLE_CAUSE from COR. rc[0x%x]", rc);
                clCorMoIdFree (hMoId);                                       
                return rc;
            }            

            clLogTrace("ALM", "AEM", "Clearing the alarm with probable cause [%d]", probCause);

            /* now process the alarm */
            pAlarmInfo = (ClAlarmInfoT *)clHeapAllocate(sizeof(ClAlarmInfoT));            
            pAlarmInfo->probCause = probCause;
            pAlarmInfo->alarmState = CL_ALARM_STATE_CLEAR;
            pAlarmInfo->moId = *hMoId;

            rc = clAlarmClientEngineAlarmProcess(hMoId, hMSOObj, pAlarmInfo,NULL, 0/*no lock applied*/);

			if(*(ClUint8T *)pValue == 1)
			{
                clLogTrace( "ALM", "AEM", " The value of CL_ALARM_CLEAR was set to "
                    "one so resetting it to zero");

				alarmClear = 0;
				rc = clCorObjectAttributeSet(NULL, hMSOObj, pAttrPath ,  
					CL_ALARM_CLEAR, CL_COR_INVALID_ATTR_IDX, &alarmClear, sizeof(alarmClear));
				if (CL_OK != rc)
				{
                    clLogError( "ALM", "AEM", 
                            "Failed while setting CL_ALARM_CLEAR attribute to [0]. rc[0x%x]", rc);
					clHeapFree(pAlarmInfo);
					clCorMoIdFree(hMoId);
					return rc;
				}    
			}
			clHeapFree(pAlarmInfo);
            break;
        default:
            clLogTrace( "ALM", "AEM", " The Attribute Id [0x%x] is set which is not taken care by alarm ", attrId);
            break;
    }
    clCorMoIdFree (hMoId);          	

    clLogTrace( "ALM", "AEM", " Leaving [%s]", __FUNCTION__);

    return CL_OK;
}


ClRcT clAlarmClientDeleteNotifProcess(ClCorTxnIdT corTxnId, ClCorTxnJobIdT jobId)
{
    ClCorMOIdPtrT  hMoId = NULL;  /* MOID of the object being changed*/
    ClRcT         rc = CL_OK; /* Return code */
    void*        pObj = NULL;     
    ClHandleT omHandle;    

 /* TODO - COR-TXN */

#if 0
   if (eventHandle == 0)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rNull Event")); 
     return CL_ERR_NULL_POINTER;
   }
#endif

    clLogTrace( "ALM", "AEM", " Entering [%s]", __FUNCTION__);

    if ( (rc = clCorMoIdAlloc(&hMoId)) != CL_OK)
    {
        clLogError( "ALM", "AEM", 
                "Failed to allocate memory for MoId . rc[0x%x]", rc);
        return rc;
    }

    if ((rc = clCorMoIdInitialize(hMoId)) != CL_OK)
    {
        clLogError( "ALM", "AEM", 
                "Failed to initialize the MoId. rc[0x%x] ", rc);
        clCorMoIdFree (hMoId);
        return rc;
    }            

    if (CL_OK != (rc = clCorTxnJobMoIdGet(corTxnId, hMoId)))
    {
        /* Things are real bad.   Return failure */
        clLogError( "ALM", "AEM", 
                "Failed to get the MoId from COR job. rc[0x%x] ", rc);
        CL_FUNC_EXIT();
        clCorMoIdFree (hMoId);         
        return rc;
    }
    
    /* 
     * The below api can be used to get the object handle 
     */
    /*rc = clCorTxnJobObjectHandleGet(corTxnId, &hMSOObj);*/

    /* unsubscribe for the attr change notif */

    /* TODO - hargagan : Can remove this function call as it seems */
    rc = clOmMoIdToOmHandleGet(hMoId, &omHandle);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", 
                "Failed to get the MoId to om handle. rc[0x%x] ", rc);
        clCorMoIdFree (hMoId);                 
        return rc;
    }

    clLogTrace( "ALM", "AEM"," Deleting all the resources belong to the MSO been deleted");

    /* delete the OM object */
    rc = clOmMoIdToOmObjectReferenceGet(hMoId, &pObj);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed to get the MoId to Om reference.  rc [0x%x] \n", rc);
        clCorMoIdFree (hMoId);        
        return rc;
    }

    rc = clOsalMutexDelete(((CL_OM_ALARM_CLASS*)pObj)->cllock);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", " Failed to delete the resource container mutex. rc[0x%x] \n", rc);
        clCorMoIdFree (hMoId);        
        return rc;
    }

    rc = clOmMoIdToOmIdMapRemove(hMoId);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed to remove MoId to OM mapping. rc [0x%x] ", rc);
        clCorMoIdFree (hMoId);        
        return rc;
    }
    
#if 0
    if ((rc = clOmObjectByReferenceDelete(pObj)) != CL_OK)
    {
		clCorMoIdFree (hMoId);    
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete OM object!\r\n"));
		return rc;
    }
#endif
    clOsalMutexLock(gClAlarmCntMutex);        
    rc = clCntAllNodesForKeyDelete(gresListHandle, (ClCntKeyHandleT)hMoId);        
    clOsalMutexUnlock(gClAlarmCntMutex);    
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed  to remove all the entries \
            from the resource container for the MSO been deleted.. rc [0x%x] ", rc);
        clCorMoIdFree (hMoId);        
        return rc;
    }        

    clLogTrace( "ALM", "AEM", " Leaving [%s]", __FUNCTION__);

    clCorMoIdFree (hMoId);    
    return rc;
}

ClRcT clAlarmClientCreateNotifProcess(ClCorTxnIdT corTxnId, ClCorTxnJobIdT jobId)
{

    ClCorMOIdPtrT  hMoId = NULL;  /* MOID of the object being changed*/
    ClRcT         rc = CL_OK; /* Return code */
    ClHandleT omHandle;
    ClCorAddrT     alarmClientAddr;
    ClEoExecutionObjT*   eoObj;
    ClUint32T primaryOI = 0;

#if 0
   if (eventHandle == 0)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rNull Event")); 
     return CL_ERR_NULL_POINTER;
   }
#endif

    clLogTrace( "ALM", "AEM", " Entering [%s]", __FUNCTION__);

    if ( (rc = clCorMoIdAlloc(&hMoId)) != CL_OK)
    {
        clLogError( "ALM", "AEM", "Failed while allocating the MoId. rc[0x%x]", rc);
        return rc;
    }

    if ((rc = clCorMoIdInitialize(hMoId)) != CL_OK)
    {
        clLogError( "ALM", "AEM", "Failed to initialize the MoId. rc[0x%x]", rc);
        clCorMoIdFree (hMoId);
        return rc;
    }            

    if (CL_OK != (rc = clCorTxnJobMoIdGet(corTxnId, hMoId)))
    {
        /* Things are real bad.   Return failure */
        clLogError( "ALM", "AEM", "Failed to get the moId from COR job. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        clCorMoIdFree (hMoId);         
        return rc;
    }    

    rc = clEoMyEoObjectGet(&eoObj);
    if(CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed to get the EO object. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    alarmClientAddr.nodeAddress = clIocLocalAddressGet();
    alarmClientAddr.portId = eoObj->eoPort;

    clLogTrace( "ALM", "AEM", "Adding the component addres [0x%x: 0x%x] in the route list", 
        alarmClientAddr.nodeAddress, alarmClientAddr.portId);

    if ( (rc=clCorOIRegisterAndDisable(hMoId, alarmClientAddr)) != CL_OK)
    {
        clLogError( "ALM", "AEM", "Failed to add the component address \
            [0x%x:0x%x] in the route list. rc[0x%x]", alarmClientAddr.nodeAddress, 
            alarmClientAddr.portId, rc);
        return rc;
    }

    rc = clAlarmCheckIfPrimaryOI(hMoId, &primaryOI);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed to get the primary OI status. rc[0x%x]", rc);
        clCorMoIdFree (hMoId);        
        return rc;
    }

    if(primaryOI == 1)
    {
        clLogTrace( "ALM", "AEM", 
                "The component is a primary OI so configuring the alarm MSO");
        
        rc = clAlarmClientAlarmMsoConfigure(hMoId);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "AEM", "Failed to configure the alarm MSO. rc[0x%x]", rc); 
            clCorMoIdFree (hMoId);        
            return rc;
        }
    }

/* Moving the alarm om create functionality below alarm mso configure as now the alarm om create
 * function will also involve getting the configuration attributes and populating the cache */
    rc = clAlarmClientAlarmOMCreate(hMoId);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed to create the alarm MSO OM entry. rc[0x%x]", rc); 
        clCorMoIdFree (hMoId);        
        return rc;
    }

    /* TODO - hargagan : Can remove this function call. */

    rc = clOmMoIdToOmHandleGet(hMoId, &omHandle);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "AEM", "Failed to get the om-handle from the MoId. rc[0x%x]", rc); 
        clCorMoIdFree (hMoId);                        
        return rc;
    }

    clLogTrace( "ALM", "AEM", " Leaving [%s]", __FUNCTION__);

    clCorMoIdFree (hMoId);                
    return rc;
}

ClRcT clAlarmClientAlarmMsoConfigure(ClCorMOIdPtrT hMoId)
{

    ClRcT           rc = CL_OK; /* Return code */
    ClCorClassTypeT   myMOClassType;
    ClUint32T            i=0, index=0;
    ClCorObjectHandleT hMSOObj;
    ClUint8T             almValid = 0;
    ClUint32T            size;
    ClUint8T            bMSOToBePolled= 0;
    ClCorTxnSessionIdT txnSessionId = 0 ;

    clLogTrace( "ALM", "INT", " Entering [%s]", __FUNCTION__);

    rc = clCorMoIdToClassGet(hMoId, CL_COR_MO_CLASS_GET,  &myMOClassType);    
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT", "Failed while getting the class identifier \
            from the MoId. rc[0x%x] ", rc);
        return rc;
    }
    
    index = 0;

    while( (appAlarms[index].moClassID != myMOClassType))
    { 
        /* fix for bug 3640 */
        if(appAlarms[index].moClassID == 0)
        {
            clLogError( "ALM", "INT", "The class identifier is zero ");
            return CL_ALARM_RC(CL_ALARM_ERR_INTERNAL_ERROR);
        }
        /* fix for bug 3640 */
        index++; 
    }

    /* get the MSO object */
    rc = clCorObjectHandleGet(hMoId, &hMSOObj);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT", "Failed to get the object handle for the MOId. rc[0x%x] ", rc);
        return rc;
    }

    size = sizeof(appAlarms[index].pollingTime);
    rc = SetAlmAttrValue(hMoId, &txnSessionId,
                        hMSOObj,
                        CL_ALARM_POLLING_INTERVAL, 
                        i, 
                        &(appAlarms[index].pollingTime), 
                        size);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT", "Failed to set the value of CL_ALARM_POLLING_INTERVAL. \
            rc[0x%x] ", rc);
        clCorObjectHandleFree(&hMSOObj);
        return rc;
    }
    
    clLogTrace( "ALM", "INT", "The alarm MSO being configure by reading the "
            "configuration file. ");

    /*set the prob_cause and CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY attr in MSO */

    for (i=0; i< CL_ALARM_MAX_ALARMS; i++)
    {
        clLogTrace( "ALM", "INT", "Adding the set jobs for the attribute of alarm index [%d]", i);

        clLogTrace("ALM", "INT", "Verifying the value of probable cause ...");

        /* set the prob cause */
        if (appAlarms[index].MoAlarms[i].probCause == CL_ALARM_ID_INVALID)
        {
            clLogTrace( "ALM", "INT", "The probable cause is invalid so breaking, \
                so stop processing");
            break;
        }

        /* check if any MSO has alarms to be polled */
        clLogTrace( "ALM", "INT", "Checking if any MSO has alarm to be polled ...");

        if (appAlarms[index].MoAlarms[i].fetchMode == CL_ALARM_POLL)
        {
            clLogTrace( "ALM", "INT", "The alarm configuration has polling enabled.");
            bMSOToBePolled = 1;
        }
        
        size = sizeof(appAlarms[index].MoAlarms[i].probCause);
        rc = SetAlmAttrValue(hMoId, &txnSessionId,
                            hMSOObj,
                            CL_ALARM_PROBABLE_CAUSE,
                            i,
                            &(appAlarms[index].MoAlarms[i].probCause), 
                            size);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", "Failed while adding set job of probable \
                cause to the COR job. rc[0x%x]", rc);
            clCorObjectHandleFree(&hMSOObj);
            clCorTxnSessionFinalize(txnSessionId);
            return rc;
        }

        size = sizeof(appAlarms[index].MoAlarms[i].category);

        rc = SetAlmAttrValue(hMoId, &txnSessionId,
                            hMSOObj,
                            CL_ALARM_CATEGORY,
                            i,
                            &(appAlarms[index].MoAlarms[i].category),
                            size);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", "Failed while adding the set job of alarm \
                category to the COR job. rc[0x%x]", rc);
            clCorObjectHandleFree(&hMSOObj);
            clCorTxnSessionFinalize(txnSessionId);
            return rc;
        }        

        /* set the severity */

        size = sizeof(appAlarms[index].MoAlarms[i].severity);
        rc = SetAlmAttrValue(hMoId, &txnSessionId,
                            hMSOObj, 
                            CL_ALARM_SEVERITY, 
                            i, 
                            &(appAlarms[index].MoAlarms[i].severity), 
                            size);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", "Failed while adding the set job of alarm \
                severity to the COR job. rc[0x%x]", rc);
            clCorObjectHandleFree(&hMSOObj);
            clCorTxnSessionFinalize(txnSessionId);
            return rc;
        }        


        size = sizeof(appAlarms[index].MoAlarms[i].probCause);
        rc = SetAlmAttrValue(hMoId, &txnSessionId,
                            hMSOObj, 
                            CL_ALARM_ID, 
                            i, 
                            &(appAlarms[index].MoAlarms[i].probCause), 
                            size);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", "Failed while adding the set job of \
                CL_ALARM_ID to the COR job. rc[0x%x]", rc);
            clCorObjectHandleFree(&hMSOObj);
            clCorTxnSessionFinalize(txnSessionId);
            return rc;
        }
        
        size = sizeof(appAlarms[index].MoAlarms[i].specificProblem);
        rc = SetAlmAttrValue(hMoId, &txnSessionId,
                            hMSOObj, 
                            CL_ALARM_SPECIFIC_PROBLEM, 
                            i, 
                            &(appAlarms[index].MoAlarms[i].specificProblem), 
                            size);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", "Failed while adding the set job of \
                CL_ALARM_SPECIFIC_PROBLEM to the COR job. rc[0x%x]", rc);
            clCorObjectHandleFree(&hMSOObj);
            clCorTxnSessionFinalize(txnSessionId);
            return rc;
        }
            
        almValid = 1;

        size = sizeof(almValid);
        
        rc = SetAlmAttrValue(hMoId, &txnSessionId,
                            hMSOObj, 
                            CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY, 
                            i, 
                            &almValid, 
                            size);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", "Failed while adding the set job of \
                CONTAINMENT_ATTR_VALID_ENTRY to the COR job. rc[0x%x]",rc);
            clCorObjectHandleFree(&hMSOObj);
            clCorTxnSessionFinalize(txnSessionId);
            return rc;
        }
    }

    if (bMSOToBePolled)
    {
        clLogTrace( "ALM", "INT", "Polling is enabled so setting the "
                "CL_ALARM_MSO_TO_BE_POLLED to true.");
        
        size = sizeof(bMSOToBePolled);
        rc = SetAlmAttrValue(hMoId, &txnSessionId,
                            hMSOObj, 
                            CL_ALARM_MSO_TO_BE_POLLED, 
                            0, 
                            &bMSOToBePolled, 
                            size);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", "Failed while adding the set job of \
                CL_ALARM_MSO_TO_BE_POLLED to the COR job. rc[0x%x]",rc);
            clCorObjectHandleFree(&hMSOObj);
            clCorTxnSessionFinalize(txnSessionId);
            return rc;
        }
    }

    clLogTrace( "ALM", "INT", "Committing the COR-Transaction jobs ");

    rc = clCorTxnSessionCommit(txnSessionId);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT", "Failed to commit the set jobs for all "
            "the alarm attributes. rc[0x%x]",rc);
        clCorObjectHandleFree(&hMSOObj);
        clCorTxnSessionFinalize(txnSessionId);
        return rc;
    }

    clLogTrace( "ALM", "INT", 
            "Calculating the generation and suppression rule for the alarm profile"
            "index [%d] ", index);
    
    rc = clAlarmClientSetAffectedAlarm(hMoId, hMSOObj, index);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "INT", "Failed while calculating the GENERATION "
            " or SUPPRESSION RULE for the alarm profile index [%d]. rc[0x%x]", index, rc);
        clCorObjectHandleFree(&hMSOObj);
        return rc;
    }

    clLogTrace( "ALM", "INT", " Leaving [%s]", __FUNCTION__);
    clCorObjectHandleFree(&hMSOObj);

    return rc;

}




ClRcT clAlarmClientSetAffectedAlarm(ClCorMOIdPtrT pMoId, ClCorObjectHandleT objH, ClUint32T index)
{
    ClRcT                       rc = CL_OK;
    ClUint32T                   alarmIndex = 0;        
    ClUint64T                   orgAlmGenBM = 0, orgAlmSupBM = 0;
    ClUint64T                   affectedAlmsBM = 0;
    ClUint32T                   affAlarmIndex = 0;    
    ClUint32T                   size = 0;
    ClUint32T                   i = 0, j = 0;
    ClUint32T                   relation = 0;
    ClUint32T                   relationAttrId=0;
    ClAlarmProbableCauseT       affGenAlarmId = 0, affSupAlarmId = 0, alarmId = 0;
    ClAlarmRuleEntryT           alarmKeyGen = {0}, alarmKeySup = {0};
    ClCorTxnSessionIdT          sessionId = 0;

    i = j = 0;
    
    clLogTrace( "ALM", "INT", " Entering [%s]", __FUNCTION__);

    while (appAlarms[index].MoAlarms[i].probCause != CL_ALARM_ID_INVALID)
    {
        alarmKeyGen.probableCause = alarmId = appAlarms[index].MoAlarms[i].probCause;
        alarmKeyGen.specificProblem = appAlarms[index].MoAlarms[i].specificProblem;

        clLogTrace( "ALM", "INT", "Preparing the rule for the alarm [%d]", alarmId);

        rc = alarmUtilAlmIdxGet(pMoId, objH, alarmKeyGen, &alarmIndex, 0/*no lock applied*/);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", 
                    "Failed to get the alarm index for the alarm [%d]. rc[0x%x]",
                    alarmId, rc);
            goto exitError;
        }

        orgAlmSupBM =  orgAlmGenBM = 0;

        clLogTrace( "ALM", "INT", "Got the alarm index [%d] for the alarm [%d]",
                alarmIndex, alarmId);

        for (j = 0; j < CL_ALARM_MAX_ALARM_RULE_IDS; j++)
        {
            memset(&alarmKeyGen, 0, sizeof(ClAlarmRuleEntryT));
            memset(&alarmKeySup, 0, sizeof(ClAlarmRuleEntryT));
            affGenAlarmId = affSupAlarmId = CL_ALARM_ID_INVALID;

            clLogTrace( "ALM", "INT", 
                    "Calculating the generation and suppression rule for alarm [%d]", alarmIndex);

            if ((appAlarms[index].MoAlarms[i].generationRule != NULL) || 
                    (appAlarms[index].MoAlarms[i].suppressionRule != NULL))
            {
                if (appAlarms[index].MoAlarms[i].generationRule != NULL)
                {
                    alarmKeyGen.probableCause = affGenAlarmId = 
                        appAlarms[index].MoAlarms[i].generationRule->alarmIds[j].probableCause;
                    alarmKeyGen.specificProblem = 
                        appAlarms[index].MoAlarms[i].generationRule->alarmIds[j].specificProblem;
                }
                else
                    clLogTrace("ALM", "INT", "The Generation rule for alarm index [%d] is NULL", alarmIndex);
                
                if (appAlarms[index].MoAlarms[i].suppressionRule != NULL) 
                {
                    alarmKeySup.probableCause = affSupAlarmId  =
                        appAlarms[index].MoAlarms[i].suppressionRule->alarmIds[j].probableCause;
                    alarmKeySup.specificProblem = 
                        appAlarms[index].MoAlarms[i].suppressionRule->alarmIds[j].specificProblem;
                }
                else
                    clLogTrace("ALM", "INT", "The Suppression rule for alarm index [%d] is NULL", alarmIndex);
            }
            else
            {
                clLogTrace( "ALM", "INT", 
                        "No Generation or Suppression rule supplied for alarm [%d]", alarmId);
                break;
            }
            
            clLogTrace( "ALM",  "INT", 
                    " The affected alarm Identifier for the alarm index [%d] is [%d]", 
                    alarmIndex, affGenAlarmId);

            /* For putting the Generation rule */
            if (affGenAlarmId == CL_ALARM_ID_INVALID)
            {
                clLogTrace( "ALM", "INT", 
                        "Gen Rule : The alarm [%d] has no more affected alarms", alarmId);
                if (affSupAlarmId == CL_ALARM_ID_INVALID)
                    break;
            }
            else
            {
                clLogTrace( "ALM", "INT", "Gen Rule: The alarm index [%d] being updated as "
                    "the affected alarm of [%d]", alarmIndex, affGenAlarmId);

                affAlarmIndex = affectedAlmsBM = 0;

                rc = alarmUtilAlmIdxGet(pMoId, objH, alarmKeyGen, &affAlarmIndex, 0/*no lock applied*/);
                if(CL_OK != rc)
                {
                    clLogError( "ALM", "INT", "Gen Rule : Failed to get the alarm index "
                        "of the affected alarm [%d]. rc[0x%x]", affGenAlarmId, rc);
                    goto exitError;
                }
                
                clLogTrace( "ALM", "INT", "Gen Rule: Got the affected alarm index [%d] ", 
                        affAlarmIndex);

                orgAlmGenBM |= ((ClUint64T) 1 << affAlarmIndex);

                size = sizeof(ClUint64T);

                rc = GetAlmAttrValue (pMoId, objH, CL_ALARM_AFFECTED_ALARMS, affAlarmIndex,
                            &affectedAlmsBM, &size, 0/*no lock applied*/);
                if (CL_OK != rc)
                {
                    clLogError( "ALM", "INT", "Gen Rule : Failed to get the value of the "
                            "AFFECTED_ALARMS for the alarm index [%d]. rc[0x%x]", affAlarmIndex, rc);
                    goto exitError;
                }                

                affectedAlmsBM |= ((ClUint64T) 1<< alarmIndex);

                clLogTrace( "ALM", "INT", "Gen rule : Affected alarm bitmap for alarm "
                        "index [%d] is [0x%llx]", affAlarmIndex, affectedAlmsBM);

                rc = SetAlmAttrValue (pMoId, NULL, objH, CL_ALARM_AFFECTED_ALARMS, affAlarmIndex,
                                        &affectedAlmsBM, sizeof(ClUint64T));

                if (CL_OK != rc)
                {
                    clLogError( "ALM", "INT", "Gen Rule : Failed to set the value of the "
                            "affect alarm bitmap [0x%llx] for alarm index [%d]. rc[0x%x]",
                        affectedAlmsBM, affAlarmIndex, rc);
                    goto exitError;
                }                
            }

            /* For putting the Suppression rule */

            if (affSupAlarmId == CL_ALARM_ID_INVALID)
            {
                clLogTrace( "ALM", "INT", 
                        "Sup Rule : The alarm [%d] has no more affected alarms", alarmId);
                if (affGenAlarmId == CL_ALARM_ID_INVALID)
                    break;
            }
            else
            {
                affAlarmIndex = affectedAlmsBM = 0;

                clLogTrace( "ALM", "INT", " Sup Rule: The alarm index [%d] being updated as "
                                "the affected alarm of [%d]", alarmIndex, affSupAlarmId);

                rc = alarmUtilAlmIdxGet(pMoId, objH, alarmKeySup, &affAlarmIndex, 0/*no lock applied*/);
                if(CL_OK != rc)
                {
                    clLogError( "ALM", "INT", " Sup Rule: Failed to get the alarm index "
                        "of the affected alarm [%d]. rc[0x%x]", affGenAlarmId, rc);
                    goto exitError;
                }
                
                clLogTrace( "ALM", "INT", 
                        "Sup Rule: Got the affected alarm index [%d] ", affAlarmIndex);

                orgAlmSupBM |= ((ClUint64T) 1 << affAlarmIndex);

                size = sizeof(ClUint64T);

                rc = GetAlmAttrValue(pMoId, objH, CL_ALARM_AFFECTED_ALARMS, affAlarmIndex,
                            &affectedAlmsBM, &size, 0/*no lock applied*/);
                if (CL_OK != rc)
                {
                    clLogError( "ALM", "INT", " Sup Rule: Failed to get the value of the "
                        "AFFECTED_ALARMS for the alarm index [%d]. rc[0x%x]", affAlarmIndex, rc);
                    goto exitError;
                }                

                affectedAlmsBM |= ((ClUint64T) 1<< alarmIndex);

                clLogTrace( "ALM", "INT", 
                        "Sup Rule: Affected alarm bitmap for alarm index [%d] "
                        "is [0x%llx]", affAlarmIndex, affectedAlmsBM);

                rc = SetAlmAttrValue(pMoId, NULL, objH, CL_ALARM_AFFECTED_ALARMS, affAlarmIndex,
                                &affectedAlmsBM, sizeof(ClUint64T));

                if (CL_OK != rc)
                {
                    clLogError( "ALM", "INT", "Sup Rule: Failed to set the value of the "
                            "affect alarm bitmap [0x%llx] for alarm index [%d]. rc[0x%x]",
                        affectedAlmsBM, affAlarmIndex, rc);
                    goto exitError;
                }                
            }
        }

        clLogTrace( "ALM", "INT", "Generation rule BM [%lld],  Suppression rule bitmap [%lld] for "
                "the alarm index [%d]",  orgAlmGenBM, orgAlmSupBM, alarmIndex); 

        rc = SetAlmAttrValue (pMoId, &sessionId ,objH, CL_ALARM_GENERATION_RULE, 
                                alarmIndex, &orgAlmGenBM, sizeof(ClUint64T));
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", "Failed to set the value of the GEN_RULE "
                "for the alarm index [%d], rc [0x%x]", alarmIndex, rc); 
            goto exitError;
        }

        rc = SetAlmAttrValue (pMoId, &sessionId ,objH, CL_ALARM_SUPPRESSION_RULE , 
                alarmIndex, &orgAlmSupBM, sizeof(ClUint64T));
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", "Failed to set the value of the SUP_RULE "
                    "for the alarm index [%d], rc [0x%x]", alarmIndex, rc); 
            goto exitError;
        }


        /* set the rule relation */
        relation = 0;
        relationAttrId = CL_ALARM_GENERATE_RULE_RELATION;
        if (appAlarms[index].MoAlarms[i].generationRule)
            relation = appAlarms[index].MoAlarms[i].generationRule->relation;

        size = sizeof(relation);
        rc = SetAlmAttrValue(pMoId, &sessionId ,objH, relationAttrId, alarmIndex, &relation, size);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", 
                    "Failed to set the relation GEN_RULE for alarm index [%d]. rc [0x%x]",
                    alarmIndex, rc); 
            goto exitError;
        }
    
        relation = 0;
        relationAttrId = CL_ALARM_SUPPRESSION_RULE_RELATION;

        if (appAlarms[index].MoAlarms[i].suppressionRule)
            relation = appAlarms[index].MoAlarms[i].suppressionRule->relation;

        rc = SetAlmAttrValue(pMoId, &sessionId ,objH, relationAttrId, alarmIndex, &relation, size);
        if (CL_OK != rc)
        {
            clLogError( "ALM", "INT", 
                    "Failed to set relation SUP_RULE for the alarm index [%d]. rc [0x%x]",
                    alarmIndex, rc); 
            goto exitError;
        }

        clLogTrace( "ALM", "INT", "Generation/Suppression rule (attrId [%d]) "
                "relation is [%d] for alarm index [%d]", relationAttrId, relation, alarmIndex);

        i++;
    }

    rc = clCorTxnSessionCommit(sessionId);
    if (CL_OK != rc)
    {
       clLogError("ALM", "INT", "Failed to commit the COR attributes. rc[0x%x]", rc);
       goto exitError;
    }

    clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__);
    return rc;

exitError:
    if (sessionId != 0)
        clCorTxnSessionFinalize(sessionId);

    clLogTrace( "ALM", "INT", "Leaving [%s]", __FUNCTION__);
    return rc;        
}


ClRcT clAlarmOmCleanUp(ClCntKeyHandleT     key,
                   ClCntDataHandleT    pData,
                   void                *dummy,
                   ClUint32T           dataLength)
{

    ClRcT rc = CL_OK;
    void *pOmObj=NULL;

    clLogTrace( "ALM", "FIN", " Entering [%s]", __FUNCTION__);

    rc = clOmMoIdToOmObjectReferenceGet((ClCorMOIdPtrT)key, &pOmObj);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "FIN", "Failed to get the OM Object reference \
            from MoId. rc[0x%x] ", rc);
        return rc;
    }        

    if ( (rc = clOsalMutexDelete(((CL_OM_ALARM_CLASS*)pOmObj)->cllock))!=CL_OK)
    {
        clLogError( "ALM", "FIN", "Unable to delete the OM container Mutex. rc[0x%x]", rc);
        return rc;
    }

    clLogTrace( "ALM", "FIN", " Leaving [%s]", __FUNCTION__);

    return rc;
}

ClRcT clAlarmLibFinalize(void)
{
    ClRcT                   rc = CL_OK;
    ClEoExecutionObjT*   eoObj;
    ClOmClassControlBlockT  *pOmClassEntry = NULL;
    pOmClassEntry = &omAlarmClassTbl[0];

    clLogTrace( "ALM", "FIN", " Entering [%s]", __FUNCTION__);
    gClAlarmLibFinalizeFlag = CL_TRUE;

    if(gTimerHandle != 0)
    {
        clLogTrace( "ALM", "FIN","Timer handle was intialized so finalizing it.");

        rc = clTimerDelete(&gTimerHandle);
        if(CL_OK != rc)
        {
            clLogError( "ALM", "FIN", "Failed while deleting the timer handle. rc[0x%x] ", rc);
            return rc;
        }
    }
    
    clLogTrace( "ALM", "FIN","Getting the EO object to uninstall it.");

    rc = clEoMyEoObjectGet(&eoObj);
    if(CL_OK != rc)
    {
        clLogError( "ALM", "FIN", "Failed to get the EO object. rc[0x%x]", rc);
        return rc;
    }

    clLogTrace( "ALM", "FIN", "Finalizing the OM container now ...");

    clOsalMutexLock(gClAlarmCntMutex);
    rc = clCntWalk(gresListHandle,clAlarmOmCleanUp, NULL,0);
    clOsalMutexUnlock(gClAlarmCntMutex);    
    if(rc != CL_OK)
    {
        clLogError( "ALM", "FIN", "Finalizing the OM container failed. rc[0x%x]", rc);
        return rc;
    }

    clLogTrace( "ALM", "FIN", "Deleting the resource list container.");

    clOsalMutexLock(gClAlarmCntMutex);
    rc = clCntDelete(gresListHandle);
    clOsalMutexUnlock(gClAlarmCntMutex);    
    if(CL_OK != rc)
    {
        clLogError( "ALM", "FIN", "Failed while deleting the resource \
                list container. rc[0x%x]", rc);
        return rc;
    }
    
    clLogTrace( "ALM", "FIN", "Finalizing the mutexes used in the client.");

    rc = clOsalMutexDelete(gClAlarmPollMutex);    
    if(CL_OK != rc)
    {
        clLogError( "ALM", "FIN", "Failed to delete the Alarm Poll Mutex. rc[0x%x]", rc);
        return rc;
    }

    rc = clOsalMutexDelete(gClAlarmCntMutex);    
    if(CL_OK != rc)
    {
        clLogError( "ALM", "FIN", "Failed to delete the Alarm Container Mutex. rc[0x%x]", rc);
        return rc;
    }
	
    clLogTrace( "ALM", "FIN", "Finalizing the Event related resources.");

    /* Unsubscribe for the SET events */
    rc = clCorEventUnsubscribe(gAlarmEvtChannelHandle, gAlarmMOChangeSubsId);
    if (rc != CL_OK)
    {
        clLogError("ALM", "FIN", "Failed to unsubscribe for SET events from COR. rc [0x%x]", rc);
        return rc;
    }

    /* Unsubscribe for the CREATE/DELETE events */
    rc = clCorEventUnsubscribe(gAlarmEvtChannelHandle, gAlarmMOCreateSubsId);
    if (rc != CL_OK)
    {
        clLogError("ALM", "FIN", "Failed to unsubscribe for CREATE/DELETE events from COR. rc [0x%x]", rc);
        return rc; 
    }

    rc = clEventChannelClose(gAlarmEvtChannelHandle);
    if(rc != CL_OK)
    {
        clLogError( "ALM", "FIN", "Failed to close the event channel. rc[0x%x]", rc);
        return rc;
    }
    
    rc = clEventFinalize(gAlarmEvtHandle);
    if(rc != CL_OK)
    {
        clLogError( "ALM", "FIN", "Failed to finalize the event handle. rc[0x%x]", rc);
        return rc;
    }

    /* Delete the configuration information stored. */
    rc = clAlarmClientConfigTableDelete();
    if (rc != CL_OK)
    {
        clLogError("ALM", "FIN", "Failed to delete alarm config info table. rc [0x%x]", rc);
        return rc;
    }

    rc = clAlarmClientConfigDataFree(&gAlarmConfig);
    if (rc != CL_OK)
    {
        clLogError("ALM", "FIN", "Failed to remove alarm config info. rc [0x%x]", rc);
        return rc;
    }

#if ALARM_TXN
    rc = clTxnAgentServiceUnRegister(gAlarmServiceHandle);
    if(rc != CL_OK)
    {
        clLogError( "ALM", "FIN", 
                "Failed to un-register the Transaction service callbacks. rc[0x%x]", rc);
        return rc;
    }
#endif

    clLogTrace( "ALM", "FIN", "Finalizing the OM Library resources");

    rc = clOmClassFinalize(pOmClassEntry,pOmClassEntry->eMyClassType);
    if(rc != CL_OK)
    {
		clLogError( "ALM", "FIN", "Failed to finalize om class. rc [0x%x] \n", rc);
        return rc;
    }

    rc = clAlarmClientNativeClientTableFinalize(eoObj);
    if (CL_OK != rc)
    {
        clLogError( "ALM", "FIN", "Failed while finalizing the EO. rc [0x%x] \n", rc);
        return rc;
    }

	clLogTrace( "ALM", "FIN", "Leaving [%s]", __FUNCTION__);

    return rc;
}


ClRcT VDECL(clAlarmSvcLibDebugCliAlarmProcess) (ClEoDataT data, ClBufferHandleT  inMsgHandle,
        ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClAlarmInfoT *pAlarmInfo=NULL; 
    ClCorObjectHandleT hMSOObj;
	ClAlarmHandleT alarmHandle = 0;
	ClAlarmInfoIDLT alarmInfoIdl  = {0};
    
	clLogTrace( "ALM", "ALR", "Entering [%s]", __FUNCTION__);

    rc = VDECL_VER(clXdrUnmarshallClAlarmInfoIDLT, 4, 0, 0)(inMsgHandle,(void *)&alarmInfoIdl);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "ALR", "Failed while unmarshalling the alarmInfo. rc[0x%x]", rc);
        return rc;
    }

    pAlarmInfo = clHeapAllocate(sizeof(ClAlarmInfoT) + (alarmInfoIdl).len);
    if (NULL == pAlarmInfo)
    {
        rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
		clLogError( "ALM", "ALR", "Failed while allocating the memory. rc[0x%x]"
                , rc);
        return rc;
    }

	pAlarmInfo->probCause = alarmInfoIdl.probCause;
	pAlarmInfo->compName.length = (alarmInfoIdl.compName).length;
    memcpy(pAlarmInfo->compName.value,(alarmInfoIdl.compName).value,pAlarmInfo->compName.length);
	pAlarmInfo->moId = (alarmInfoIdl).moId;
	pAlarmInfo->alarmState = (alarmInfoIdl).alarmState;
	pAlarmInfo->category = (alarmInfoIdl).category;
	pAlarmInfo->specificProblem = (alarmInfoIdl).specificProblem;
	pAlarmInfo->severity = (alarmInfoIdl).severity;
	pAlarmInfo->eventTime = (alarmInfoIdl).eventTime;
	pAlarmInfo->len = (alarmInfoIdl).len;
    memcpy(pAlarmInfo->buff,alarmInfoIdl.buff,pAlarmInfo->len);

	clLogMultiline(CL_LOG_NOTICE, "ALM", "ALR", " Alarm Info of the alarm being [%s] is:\n"
				"Probable cause [%d] \n"
			    "compName [%s] \n"
			    "category [%d] \n"
			    "specific problem [%d] \n"
			    "severity [%d] \n"
			    "eventTime [%lld] \n"
			    "payload length [%d]",
			    alarmInfoIdl.alarmState ? "RAISED" : "CLEARED",
			    alarmInfoIdl.probCause,
			    alarmInfoIdl.compName.value,
			    alarmInfoIdl.category,
			    alarmInfoIdl.specificProblem,
			    alarmInfoIdl.severity,
			    alarmInfoIdl.eventTime,
			    alarmInfoIdl.len);	
    /* need to free the pointer allocated inside idl */
    clHeapFree(alarmInfoIdl.buff);
    alarmInfoIdl.len = 0;
	
	rc = clCorMoIdServiceSet(&(pAlarmInfo->moId),CL_COR_SVC_ID_ALARM_MANAGEMENT);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "ALR", 
                "Failed while setting the service Id of MoId. rc[0x%x]", rc);
        clHeapFree(pAlarmInfo);
        return rc;
    }
    
    rc = clCorObjectHandleGet(&(pAlarmInfo->moId), &hMSOObj);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "ALR", 
                "Failed while getting the object handle. rc[0x%x]", rc);
        clHeapFree(pAlarmInfo);
        return rc;
    }
    
    rc = clAlarmClientEngineAlarmProcess(&(pAlarmInfo->moId), hMSOObj,pAlarmInfo,&alarmHandle, 0/*no lock applied*/);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "ALR", 
                "Failed while processing the alarm . rc[0x%x]", rc);
        clHeapFree(pAlarmInfo);
        return rc;
    }

	clLogInfo("ALM", "ALR", 
            "Getting the alarm handle [0x%x]", alarmHandle);

	rc = clXdrMarshallClUint32T((void *)&alarmHandle,outMsgHandle,0);
	if (CL_OK != rc)
	{
		clLogError( "ALM",  
			 "ALR", "Failed while marshalling the alarm handle.\
		   	rc [0x%x]", alarmHandle);
        clHeapFree(pAlarmInfo);
		return rc;
	}

	alarmInfoIdl.severity = pAlarmInfo->severity;
	alarmInfoIdl.category = pAlarmInfo->category;
	rc = VDECL_VER(clXdrMarshallClAlarmInfoIDLT, 4, 0, 0)((void *)&alarmInfoIdl,outMsgHandle,0);
	if (CL_OK != rc)
	{
		clLogError( "ALM",  
			 "ALR", "Failed while marshalling the alarm info. rc[0x%x]", rc);
        clHeapFree(pAlarmInfo);
		return rc;
	}

	clLogTrace( "ALM", "ALR", " Leaving [%s]", __FUNCTION__);

    clHeapFree(pAlarmInfo);
    return rc;

}

ClRcT clAlarmRaise(ClAlarmInfoT *alarmInfo,ClAlarmHandleT *pAlarmHandle)
{
    ClRcT    rc = CL_OK;
    ClCorObjectHandleT hMSOObj;
    ClBufferHandleT inMsgHandle;
    ClBufferHandleT outMsgHandle;
    ClIocAddressT          iocAddress;
    ClRmdOptionsT           opts;
    ClCorAddrT         addr;    
	ClAlarmInfoIDLT alarmInfoIdl  = {0};

	clLogTrace( "ALM", "ALR", " Entering [%s]", __FUNCTION__);

	if ((NULL == alarmInfo) || (NULL == pAlarmHandle))
	{
		rc = CL_ALARM_RC(CL_ERR_NULL_POINTER);
		clLogError( "ALM", "ALR", "NULL pointer passed. rc[0x%x]", rc);
		return rc;
	}

    if (alarmInfo->len > 0)
    {
	    alarmInfoIdl.buff = clHeapAllocate(alarmInfo->len);
	    if(NULL == alarmInfoIdl.buff)
	    {
		    rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
		    clLogError( "ALM", "ALR", "Failed while allocating the memory. rc[0x%x]", rc);
		    return rc;
	    }
    }

    rc = clCorMoIdServiceSet(&(alarmInfo->moId) , CL_COR_SVC_ID_ALARM_MANAGEMENT);
	if (CL_OK != rc)
	{
		clLogError( "ALM", "ALR", "Failed while doing service set for the MoId. rc[0x%x]", rc); 
        clHeapFree(alarmInfoIdl.buff);
		return rc;
	}

	alarmInfoIdl.probCause = alarmInfo->probCause;
	alarmInfoIdl.compName.length = (alarmInfo->compName).length;
	if(alarmInfoIdl.compName.length > 0)
	    memcpy(alarmInfoIdl.compName.value,(alarmInfo->compName).value,alarmInfoIdl.compName.length);
	alarmInfoIdl.moId = alarmInfo->moId;
	alarmInfoIdl.alarmState = alarmInfo->alarmState;
	alarmInfoIdl.category = alarmInfo->category;
	alarmInfoIdl.specificProblem = alarmInfo->specificProblem;
	alarmInfoIdl.severity = alarmInfo->severity;
	alarmInfoIdl.eventTime = alarmInfo->eventTime;
	alarmInfoIdl.len = alarmInfo->len;

    if ( alarmInfo->len > 0)
	    memcpy(alarmInfoIdl.buff,alarmInfo->buff,alarmInfoIdl.len);

	clLogMultiline( CL_LOG_TRACE, "ALM",  "ALR", " Alarm Info of the alarm being [%s] is:\n"
				"Probable cause [%d] \n"
			    "compName [%s] \n"
			    "category [%d] \n"
			    "specific problem [%d] \n"
			    "severity [%d] \n"
			    "eventTime [%lld] \n"
			    "payload length [%d]",
			    alarmInfoIdl.alarmState ? "Raised" : "Cleared",
			    alarmInfoIdl.probCause,
			    alarmInfoIdl.compName.value,
			    alarmInfoIdl.category,
			    alarmInfoIdl.specificProblem,
			    alarmInfoIdl.severity,
			    alarmInfoIdl.eventTime,
			    alarmInfoIdl.len);	

    rc = clBufferCreate(&outMsgHandle);
    if (rc != CL_OK)
    {
		clLogError( "ALM", "ALR", " Failed while creating the buffer for output \
			message. rc[0x%x]", rc);
        clHeapFree(alarmInfoIdl.buff);
        return rc;
    }
    
    rc = clCorObjectHandleGet(&(alarmInfoIdl.moId), &hMSOObj);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "ALR", " Failed while getting the object handle. rc[0x%x]", rc); 
        clBufferDelete(&outMsgHandle); 
        clHeapFree(alarmInfoIdl.buff);
        return CL_ALARM_RC(CL_ALARM_ERR_INVALID_MOID);
    }        

	rc = clCorMoIdToComponentAddressGet(&(alarmInfoIdl.moId), &addr);
	if (CL_OK != rc)
	{
		clLogError( "ALM", "ALR", "Failed while getting the component address. rc[0x%x]", rc); 
        clBufferDelete(&outMsgHandle); 
        clHeapFree(alarmInfoIdl.buff);
		return CL_ALARM_RC(CL_ALARM_ERR_NO_OWNER);
	}

	clLogTrace( "ALM", "ALR", " Sending the RMD to the component [0x%x:0x%x] which "
            "owns the alarm resource.", addr.nodeAddress, addr.portId);

	opts.timeout = CL_RMD_DEFAULT_TIMEOUT; 
	opts.priority = CL_RMD_DEFAULT_PRIORITY;
	opts.retries = CL_RMD_DEFAULT_RETRIES;

	rc = clBufferCreate(&inMsgHandle);
	if (rc != CL_OK )
	{
		clLogError( "ALM", "ALR", "Failed while creating buffer. rc[0x%x]", rc); 
        clBufferDelete(&outMsgHandle); 
        clHeapFree(alarmInfoIdl.buff);
		return rc;
	}

	rc = VDECL_VER(clXdrMarshallClAlarmInfoIDLT, 4, 0, 0)((void *)&alarmInfoIdl,inMsgHandle,0);
	if (CL_OK != rc)
	{
        clHeapFree(alarmInfoIdl.buff);
		clLogError( "ALM", "ALR", "Failed while writing the data to the buffer. \
			rc[0x%x]", rc); 
        goto exitOnError;
	}

    clHeapFree(alarmInfoIdl.buff);
	iocAddress.iocPhyAddress.nodeAddress = addr.nodeAddress;
	iocAddress.iocPhyAddress.portId    = addr.portId;

	rc = clRmdWithMsg (iocAddress,
					   CL_ALARM_COMP_ALARM_RAISE_IPI,
					   inMsgHandle,
					   outMsgHandle,
					   CL_RMD_CALL_ATMOST_ONCE|CL_RMD_CALL_NEED_REPLY,
					   &opts,
					   NULL);
	if(rc != CL_OK)
	{
		clLogError( "ALM", "ALR", "Failed while Sending RMD to the client \
		    owning the alarm resource. rc[0x%x]", rc);
        goto exitOnError;
	}	


	/* Get the alarm Info from the outAlarmMsgHdl */
	rc = clXdrUnmarshallClUint32T(outMsgHandle,(void *)pAlarmHandle);
	if (CL_OK != rc)
	{
		clLogError( "ALM", "ALR", "Failed while unmarshalling the alarm handle. rc[0x%x]", rc);
	    goto exitOnError;
	}

	clLogTrace( "ALM", "ALR", "Got the alarm handle as [0x%x]", *pAlarmHandle);

	rc = VDECL_VER(clXdrUnmarshallClAlarmInfoIDLT, 4, 0, 0)(outMsgHandle,(void *)&alarmInfoIdl);
	if (CL_OK != rc)
	{
		clLogError( "ALM", "ALR", "Failed while unmarshalling the alarm info. rc[0x%x]", rc);
		goto exitOnError;
	}

	alarmInfo->severity = alarmInfoIdl.severity;
	alarmInfo->category = alarmInfoIdl.category;

	clLogTrace( "ALM", "ALR", "Got the new severity [%d] and category [%d]", 
		alarmInfoIdl.severity, alarmInfoIdl.category);

	clLogTrace( "ALM", "ALR", "Leaving [%s]", __FUNCTION__);

exitOnError:

	clBufferDelete(&inMsgHandle);
	clBufferDelete(&outMsgHandle);

    return rc;
}


ClRcT
clAlarmClientEventDataGet(ClUint8T *pData, ClSizeT size, ClAlarmHandleInfoT *pAlarmHandleInfo)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT   msgHdl;
    ClAlarmHandleInfoIDLT alarmHandleInfoIdl = {0};

	clLogTrace( "ALM", "AEM", " Entering [%s]", __FUNCTION__);

    if(pData == NULL || pAlarmHandleInfo == NULL)
	{
		rc = CL_ALARM_RC(CL_ERR_NULL_POINTER);
		clLogError( "ALM", "AEM", "NULL pointer passed as input. rc[0x%x]", rc);
        return rc;
	}

    if((rc = clBufferCreate(&msgHdl)) != CL_OK)
    {
		clLogError( "ALM",  "AEM", 
                "Failed while creating the buffer message. rc[0x%x]", rc);
        return (rc);
    }

    if((rc = clBufferNBytesWrite(msgHdl, (ClUint8T *)pData, size))!= CL_OK)
    {
        clBufferDelete(&msgHdl);
		clLogError( "ALM", "AEM", "Failed while writing the data in the \
			buffer. rc[0x%x]", rc);
        return (rc);
    }

    rc = VDECL_VER(clXdrUnmarshallClAlarmHandleInfoIDLT, 4, 0, 0)(msgHdl,(void *)&alarmHandleInfoIdl);
    if (CL_OK != rc)
    {
        clBufferDelete(&msgHdl);
		clLogError( "ALM", "AEM", "Failed while unmarshalling alarm info \
			from buffer. rc[0x%x]", rc);
        return rc;
    }

	pAlarmHandleInfo->alarmHandle = alarmHandleInfoIdl.alarmHandle;
	pAlarmHandleInfo->alarmInfo.probCause = alarmHandleInfoIdl.alarmInfo.probCause;
	pAlarmHandleInfo->alarmInfo.compName.length = (alarmHandleInfoIdl.alarmInfo.compName).length;
    memcpy(pAlarmHandleInfo->alarmInfo.compName.value,(alarmHandleInfoIdl.alarmInfo.compName).value,pAlarmHandleInfo->alarmInfo.compName.length);
	pAlarmHandleInfo->alarmInfo.moId = (alarmHandleInfoIdl.alarmInfo).moId;
	pAlarmHandleInfo->alarmInfo.alarmState = (alarmHandleInfoIdl.alarmInfo).alarmState;
	pAlarmHandleInfo->alarmInfo.category = (alarmHandleInfoIdl.alarmInfo).category;
	pAlarmHandleInfo->alarmInfo.specificProblem = (alarmHandleInfoIdl.alarmInfo).specificProblem;
	pAlarmHandleInfo->alarmInfo.severity = (alarmHandleInfoIdl.alarmInfo).severity;
	pAlarmHandleInfo->alarmInfo.eventTime = (alarmHandleInfoIdl.alarmInfo).eventTime;
	pAlarmHandleInfo->alarmInfo.len = (alarmHandleInfoIdl.alarmInfo).len;
    memcpy(pAlarmHandleInfo->alarmInfo.buff,alarmHandleInfoIdl.alarmInfo.buff,pAlarmHandleInfo->alarmInfo.len);

	clLogMultiline(CL_LOG_TRACE, "ALM", "AEM", "Alarm Info of the alarm being [%s] is:\n"
				"Probable cause [%d] \n"
			    "compName [%s] \n"
			    "category [%d] \n"
			    "specific problem [%d] \n"
			    "severity [%d] \n"
			    "eventTime [%lld] \n"
			    "payload length [%d]",
			    alarmHandleInfoIdl.alarmInfo.alarmState ? "Raised" : "Cleared",
			    alarmHandleInfoIdl.alarmInfo.probCause,
			    alarmHandleInfoIdl.alarmInfo.compName.value,
			    alarmHandleInfoIdl.alarmInfo.category,
			    alarmHandleInfoIdl.alarmInfo.specificProblem,
			    alarmHandleInfoIdl.alarmInfo.severity,
			    alarmHandleInfoIdl.alarmInfo.eventTime,
			    alarmHandleInfoIdl.alarmInfo.len);	

	clBufferDelete(&msgHdl);
    clHeapFree(alarmHandleInfoIdl.alarmInfo.buff);
	
	clLogTrace( "ALM", "AEM", " Leaving [%s]", __FUNCTION__);

    return rc;
}



/**
 * Event Delivery Callback function. This function should be called once the event is 
 * recievied by the alarm client. This event will be published by the alarm server in 
 * case of alarm raise or clear.
 */

static void 
clAlarmEventDeliveryCallback ( ClEventSubscriptionIdT subscriptionId, ClEventHandleT eventHandle, 
                                            ClSizeT eventDataSize )
{
    ClRcT               rc = CL_OK;
    ClUint32T           eventType = 0;
    ClPtrT              pData = NULL;
    ClAlarmHandleInfoT  *pAlarmInfo  = NULL;

    clLogTrace("ALM", "EVT", "Got event notification : EventDataSize [%lld] ", eventDataSize);
     
    rc = clEventExtAttributesGet( eventHandle,&eventType, NULL, NULL, NULL, NULL, NULL);
    if (CL_OK != rc)
    {
        clLogError("ALM", "EVT", "Failed while getting the attribute information from event. rc[0x%x]",rc); 
        goto evtCallbackError;
    }

    eventType = CL_BIT_N2H32(eventType);

    if(eventType == CL_ALARM_EVENT)
    {
		pData = clHeapAllocate(eventDataSize);
        if(NULL == pData)
        {
            rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
            clLogError("ALM", "EVT", "Failed while allocating memory for the event Data.");
            goto evtCallbackError;
        }
    
        rc = clEventDataGet (eventHandle, (void*)pData,  &eventDataSize);
        if(rc != CL_OK)
        {
            clLogError("ALM", "EVT", "Failed while getting the event data from event. rc[0x%x]",rc); 
            clHeapFree(pData);
            goto evtCallbackError;
        }

        pAlarmInfo = clHeapAllocate(eventDataSize);
        if(NULL == pAlarmInfo)
        {
            rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
            clLogError("ALM", "EVT", "Failed while allocating the memory for alarm Info. rc[0x%x]", rc);
            clHeapFree(pData);
            goto evtCallbackError;
        }

        rc = clAlarmClientEventDataGet(pData, eventDataSize, pAlarmInfo);
		if(rc != CL_OK)
		{
		    clLogError("ALM", "EVT" ,"Failed while doing the alarm event data umarshalling. rc[0x%x]",rc); 
            clHeapFree(pData);
            clHeapFree(pAlarmInfo);
            goto evtCallbackError;
		}

        clHeapFree(pData);

        if(gAlarmClientEventCallbackFunc != NULL)
        {
            rc = gAlarmClientEventCallbackFunc(pAlarmInfo);
            if(CL_OK != rc)
            {
                clLogError("ALM", "EVT", "Failed after calling the callback function. rc[0x%x]", rc);
                clHeapFree(pAlarmInfo);
                goto evtCallbackError;
            }
        }
        else
        {
            clLogNotice("ALM", "EVT", "Alarm Callback is not registered. Please call clAlarmEventSubscribe to register it.");
            clHeapFree(pAlarmInfo);
            goto evtCallbackError;
        }
        
        clHeapFree(pAlarmInfo);
    }

evtCallbackError:
    clEventFree(eventHandle);
    CL_ALARM_FUNC_EXIT();
    return;
}

/**
 * Function to subscribe for the event on behalf of the user. This 
 * takes a function pointer as a parameter.
 */ 

ClRcT 
clAlarmEventSubscribe(ClAlarmEventCallbackFuncPtrT pAlarmEvtCallbackFuncFP)
{
    ClRcT               rc = CL_OK;
    ClNameT             almClntChannelName = {0};
    ClVersionT          almEvtVersion = CL_EVENT_VERSION;
    ClUint32T           eventType = CL_ALARM_EVENT;
    ClEventCallbacksT   almEvtCallBack;

    if(pAlarmEvtCallbackFuncFP == NULL)
    {
        clLogError("ALM", "SUB", "The function pointer passed for alarm event subscription is NULL. ");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    if(gAlarmClntEvtHandle != 0)
    {
        clLogError("ALM", "SUB", "The subscription is already done for the alarm event channel.");
        return CL_ALARM_RC(CL_ALARM_ERR_EVT_SUBSCRIBE_AGAIN);
    }

    almEvtCallBack.clEvtChannelOpenCallback = NULL;
    almEvtCallBack.clEvtEventDeliverCallback = clAlarmEventDeliveryCallback;

    clLogTrace("ALM", "SUB", "Inside the event Subscription ...");

    rc =  clEventInitialize( &gAlarmClntEvtHandle, &almEvtCallBack, & almEvtVersion);
    if(CL_OK != rc)
    {
         clLogError("ALM", "SUB", "Failed while initializing event handle. rc[0x%x]", rc);
         return CL_ALARM_RC(CL_ALARM_ERR_EVT_INIT);  
    }

    /*Open another event channel. This will be used for getting any alarm events.*/
    almClntChannelName.length = strlen(ClAlarmEventName);
    strncpy(almClntChannelName.value, ClAlarmEventName, almClntChannelName.length);

    rc = clEventChannelOpen(gAlarmClntEvtHandle, &almClntChannelName, 
            CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL, -1, &gAlmServerEvtChannelHandle);
    if(CL_OK != rc)
    {
        clLogError("ALM", "SUB", "Failed while opening the alarm event channel.  rc[0x%x]", rc);
        return CL_ALARM_RC(CL_ALARM_ERR_EVT_CHANNEL_OPEN); 
    }

    eventType = CL_BIT_H2N32(eventType);
      
    rc = clEventExtSubscribe(gAlmServerEvtChannelHandle, eventType, 1, NULL);
    if(CL_OK != rc)
    {
        clLogError("ALM", "SUB", "Failed while doing the event subscription. rc[0x%x]", rc);
        return CL_ALARM_RC(CL_ALARM_ERR_EVT_SUBSCRIBE);
    }

    gAlarmClientEventCallbackFunc = pAlarmEvtCallbackFuncFP;

    return rc;
}


/**
 * Function to un-subscribe for the event subscribed earlier.
 */ 
ClRcT
clAlarmEventUnsubscribe()
{
    ClRcT   rc = CL_OK;

    if(gAlmServerEvtChannelHandle != 0)
    {
        rc = clEventChannelClose(gAlmServerEvtChannelHandle);
        if(CL_OK != rc)
        {
            clLogError("ALM", "USB", "Failed while closing the event channel handle. rc[0x%x]", rc);
            return CL_ALARM_RC(CL_ALARM_ERR_EVT_CHANNEL_CLOSE);
        }
        gAlmServerEvtChannelHandle = 0;
    }

    if(gAlarmClntEvtHandle != 0)
    {
        rc = clEventFinalize(gAlarmClntEvtHandle);
        if(CL_OK != rc)
        {
            clLogError("ALM", "USB", "Failed while doing event finalize for the alarm client. rc[0x%x]", rc);
            return CL_ALARM_RC(CL_ALARM_ERR_EVT_FINALIZE);
        }
        gAlarmClntEvtHandle = 0;
    }

    gAlarmClientEventCallbackFunc = NULL;

    return rc;
}


/**
 * Function to be used by the fault server to get the resource MOId
 * while providing the alarm handle.
 */ 

ClRcT clAlarmClientResourceInfoGet (CL_IN ClAlarmHandleT alarmHandle, 
                           CL_OUT  ClAlarmInfoPtrT* const ppAlarmInfo)
{
    ClRcT               rc = CL_OK;
    ClBufferHandleT     inMsgHdl;
    ClBufferHandleT     outMsgHdl;
    ClRmdOptionsT       opts = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT       destAddress = {{0}};
    ClAlarmInfoIDLT     alarmInfoIdl = {0};

    if (NULL == ppAlarmInfo)
    {
        clLogError("ALM", "AIG", "The pointer to the AlarmInfo structure passed is NULL. ");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    rc = clBufferCreate(&inMsgHdl);
    if (rc != CL_OK)
    {
        clLogError("ALM", "AIG", "Failed while creating the incoming message header. rc[0x%x]", rc);
        return rc;
    }
    
    rc = clBufferCreate(&outMsgHdl);
    if (rc != CL_OK)
    {
        clBufferDelete(&inMsgHdl);
        clLogError("ALM", "AIG", "Failed while creating the outgoing message header. rc[0x%x]", rc);
        return rc;
    }
    
    rc = clXdrMarshallClUint32T(&alarmHandle, inMsgHdl, 0);
    if (rc != CL_OK)
    {
        clLogError("ALM", "AIG", "Failed while writing the alarm handle into the buffer. rc[0x%x]", rc);
        goto handleError;
    }

    destAddress.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    destAddress.iocPhyAddress.portId = CL_IOC_ALARM_PORT;

     /* rmd call to alarm server */
     rc = clRmdWithMsg (destAddress,
                        CL_ALARM_HANDLE_TO_INFO_GET,
                        inMsgHdl,
                        outMsgHdl,
                        CL_RMD_CALL_NEED_REPLY & (~CL_RMD_CALL_ASYNC),
                        &opts,
                        NULL);
    if (rc != CL_OK)
    {
        clLogError("ALM", "AIG", 
                "Failed while getting the alarm information from server. rc[0x%x]", rc);
        goto handleError;
    }

    /* Get the alarm Info from the outAlarmMsgHdl */
    rc = VDECL_VER(clXdrUnmarshallClAlarmInfoIDLT, 4, 0, 0)(outMsgHdl, (void *) &alarmInfoIdl);
    if (rc != CL_OK)
    {
        clLogError("ALM", "AIG", "Failed while reading the incoming message. rc[0x%x]", rc);
        goto handleError;
    }

    *ppAlarmInfo = clHeapAllocate(sizeof(ClAlarmInfoT) + (alarmInfoIdl.len));
    if (*ppAlarmInfo == NULL)
    {
        clLogError("ALM", "ALR", "Failed to allocate memory.");
        goto handleError;
    }

    (*ppAlarmInfo)->probCause = alarmInfoIdl.probCause;
	(*ppAlarmInfo)->compName.length = (alarmInfoIdl.compName).length;
    memcpy((*ppAlarmInfo)->compName.value, (alarmInfoIdl.compName).value, (*ppAlarmInfo)->compName.length);
	(*ppAlarmInfo)->moId = (alarmInfoIdl).moId;
	(*ppAlarmInfo)->alarmState = (alarmInfoIdl).alarmState;
	(*ppAlarmInfo)->category = (alarmInfoIdl).category;
	(*ppAlarmInfo)->specificProblem = (alarmInfoIdl).specificProblem;
	(*ppAlarmInfo)->severity = (alarmInfoIdl).severity;
	(*ppAlarmInfo)->eventTime = (alarmInfoIdl).eventTime;
	(*ppAlarmInfo)->len = (alarmInfoIdl).len;

    if ((*ppAlarmInfo)->len > 0)
    {
        memcpy((*ppAlarmInfo)->buff, alarmInfoIdl.buff, (*ppAlarmInfo)->len);

        /* Free the payload buffer allocated by IDL */
        clHeapFree(alarmInfoIdl.buff);
    }

handleError:
    clBufferDelete(&inMsgHdl);
    clBufferDelete(&outMsgHdl);

    return rc;
}


/**
 * Internal function at the alarm owner side which will 
 * process all the pending alarms in the system and send
 * the response back the requesting party.
 */

ClRcT VDECL(clAlarmPendingAlarmsGet) (ClEoDataT data, ClBufferHandleT  inMsgHandle,
        ClBufferHandleT  outMsgHandle)
{
    ClRcT                   rc = CL_OK;
    ClCorMOIdT              moId;

    clLogDebug("ALM", "GPA", "Got the request to send the pending alarms for a MO.");

    clCorMoIdInitialize(&moId);

    rc = VDECL_VER(clXdrUnmarshallClCorMOIdT, 4, 0, 0)(inMsgHandle, &moId);
    if (CL_OK != rc)
    {
        clLogError("ALM", "GPA", "Failed while unmarshalling the moId. rc[0x%x]", rc);
        return rc;
    }

    /*Find all the pending (suppressed + soaked + raised) alarms for this MO.*/
    rc = _clAlarmClientPendingAlmsForMOGet(&moId, outMsgHandle);
    if (CL_OK != rc)
    {
        clLogError("ALM", "GPA", 
                "Failed while getting the pending alarm list. rc[0x%x]", rc);
        return rc;
    }
    
    return rc;
}


/**
 * Internal function to get the alarm state based on the alarm information
 * provided.
 */

ClRcT VDECL(clAlarmStateGet) (ClEoDataT data, ClBufferHandleT  inMsgHandle,
        ClBufferHandleT  outMsgHandle)
{
    ClRcT               rc = CL_OK;
    ClUint32T           almIdx = 0;
    ClUint32T           size = 0;
    ClCorMOIdT          moId ;
    ClUint64T           publishedAlmBM = 0, supAlmBM = 0, 
                        almAfterSoakingBM = 0, almUnderSoakingBM = 0;
    ClAlarmInfoIDLT     alarmInfoIdl = {0};
    ClAlarmStateT       alarmState = CL_ALARM_STATE_INVALID;
    ClAlarmRuleEntryT   alarmKey = {0}; 
    ClCorObjectHandleT  objH = NULL;

    clLogInfo("ALM", "QAS", "Inside the owner to get the latest state of the alarm. ");

    rc = VDECL_VER(clXdrUnmarshallClAlarmInfoIDLT, 4, 0, 0)(inMsgHandle,(void *)&alarmInfoIdl);
    if (CL_OK != rc)
    {
		clLogError( "ALM", "QAS", "Failed while unmarshalling the alarmInfo. rc[0x%x]", rc);
        return rc;
    }

    moId = alarmInfoIdl.moId;

    rc = clCorObjectHandleGet (&moId, &objH);
    if (CL_OK != rc)
    {
        clLogError("ALM", "QAS", "Failed while getting the object handle for the "
                "given resource MOId. rc[0x%x]", rc);
        return rc;
    }

    alarmKey.probableCause = alarmInfoIdl.probCause;
    alarmKey.specificProblem = alarmInfoIdl.specificProblem;

    rc = alarmUtilAlmIdxGet(&moId, objH, alarmKey, &almIdx, 0 /* No lock applied */);
    if (CL_OK != rc)
    {
        clLogError("ALM", "QAS", "Failed to get the alarm information. rc[0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    clLogDebug ("ALM", "QAS", "Got the alarm index [%d] , for probable cause [%d] and"
            " specific problem [%d]", almIdx, alarmInfoIdl.probCause, alarmInfoIdl.specificProblem);

    /* If the alarm is in published alarm bitmap. */
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(&moId, objH, 
            CL_ALARM_PUBLISHED_ALARMS, 
            CL_COR_INVALID_ATTR_IDX, 
            &publishedAlmBM, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "QAS", "Failed while getting the published alarms bitmap. rc[0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    if (publishedAlmBM & ((ClUint64T) 1 << almIdx))
    {
        alarmState = CL_ALARM_STATE_ASSERT;
        clLogInfo("ALM", "QAS", "Got the alarm in the published alarm bitmap. ");
        goto sendResponse;
    }

    /* If the alarm is in the suppressed alarm bitmap.*/
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(&moId, objH, 
            CL_ALARM_SUPPRESSED_ALARMS, 
            CL_COR_INVALID_ATTR_IDX, 
            &supAlmBM, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "QAS", "Failed while getting the published alarms bitmap. rc[0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    if (supAlmBM & ((ClUint64T) 1 << almIdx))
    {
        alarmState = CL_ALARM_STATE_SUPPRESSED;
        clLogInfo("ALM", "QAS", "Got the alarm in the suppressed alarm bitmap. ");
        goto sendResponse;
    }

    /* Getting the alarms under soaking bitmap. */
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(&moId, objH,
            CL_ALARM_ALMS_UNDER_SOAKING,
            0, 
            &almUnderSoakingBM, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "QAS", 
                "Failed while getting the alarm under soaking bitmap. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    if (almUnderSoakingBM & ((ClUint64T) 1 << almIdx))
    {
        alarmState = CL_ALARM_STATE_UNDER_SOAKING;
        clLogInfo("ALM", "QAS", "Got the alarm in the under soaking bitmap.");
        goto sendResponse;
    }

    /* Getting the alarms after soaking bitmap. */
    size = sizeof(ClUint64T);
    rc = GetAlmAttrValue(&moId, objH,
            CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
            CL_COR_INVALID_ATTR_IDX, 
            &almAfterSoakingBM, &size, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "QAS", 
                "Failed while getting the alarms after soaking bitmap. rc[0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    if (almAfterSoakingBM & ((ClUint64T) 1 << almIdx))
    {
        alarmState = CL_ALARM_STATE_ASSERT;
        clLogInfo("ALM", "QAS", "Got the alarm in the alarm after soaking bitmap.");
        goto sendResponse;
    }

    alarmState = CL_ALARM_STATE_CLEAR;

sendResponse:

    clLogInfo("ALM", "QAS", "Packing the alarm state [%d]", alarmState);

    rc = clXdrMarshallClInt32T(&alarmState, outMsgHandle, 0);
    if (CL_OK != rc)
    {
        clLogError("ALM", "QAS", "Failed while getting the marshalling the state [%d]",
                alarmState);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    clCorObjectHandleFree(&objH);
    return rc;
}

/**
 * Function to query the alarm state of a pending alarms. The pending alarms are the one
 * which are either suppressed, or soaked or they are in the raised state.
 *
 * If the MOId of the resource, the probable cause and specific problem is given, then 
 *    it will give the state of the alarm.
 */

ClRcT 
clAlarmStateQuery (const ClAlarmInfoT *pAlarmInfo,
                   ClAlarmStateT * const pAlarmState)
{
    ClRcT               rc = CL_OK;
    ClCorMOIdT          moId ;
    ClCorAddrT          addr = {0};
    ClIocAddressT       iocAddress;
    ClRmdOptionsT       opts = {0};
    ClBufferHandleT     inMsgHandle = 0, outMsgHandle = 0;
    ClAlarmInfoIDLT     alarmInfoIdl = {0};
    ClCorObjectHandleT  objH = NULL;

    if ((NULL == pAlarmInfo) || (NULL == pAlarmState))
    {
        clLogError("ALM", "QAS", 
                "The pointer to the alarm information or the alarm state passed NULL");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    clCorMoIdInitialize(&moId);

    moId = pAlarmInfo->moId;
    rc = clCorObjectHandleGet (&moId, &objH);
    if (CL_OK != rc)
    {
        clLogError("ALM", "QAS", "Failed while getting the object handle for the "
                "resource MOId passed. rc[0x%x]", rc);
        return CL_ALARM_RC(CL_ALARM_ERR_INVALID_MOID);
    }

    rc = clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_ALARM_MANAGEMENT);
	if (CL_OK != rc)
	{
		clLogError( "ALM", "ALR", 
                "Failed while doing service set for the MoId. rc[0x%x]", rc); 
        clCorObjectHandleFree(&objH);
		return rc;
	}

	rc = clCorMoIdToComponentAddressGet(&moId, &addr);
	if (CL_OK != rc)
	{
		clLogError( "ALM", "ALR", "Failed while getting the component address. rc[0x%x]", rc); 
        clCorObjectHandleFree(&objH);
		return CL_ALARM_RC(CL_ALARM_ERR_NO_OWNER);
	}

	clLogDebug( "ALM", "ALR", " Sending the alarm state query to alarm owner [0x%x:0x%x]",
            addr.nodeAddress, addr.portId);

	alarmInfoIdl.moId = moId;
	alarmInfoIdl.probCause = pAlarmInfo->probCause;
	alarmInfoIdl.category = pAlarmInfo->category;
	alarmInfoIdl.specificProblem = pAlarmInfo->specificProblem;

	rc = clBufferCreate(&inMsgHandle);
	if (rc != CL_OK )
	{
		clLogError( "ALM", "ALR", "Failed while creating buffer. rc[0x%x]", rc); 
        clCorObjectHandleFree(&objH);
		return rc;
	}

    rc = clBufferCreate(&outMsgHandle);
    if (rc != CL_OK)
    {
        clCorObjectHandleFree(&objH);
        clBufferDelete(&inMsgHandle);
		clLogError( "ALM", "ALR", 
                " Failed while creating the buffer for output message. rc[0x%x]",
                rc);
        return rc;
    }

	rc = VDECL_VER(clXdrMarshallClAlarmInfoIDLT, 4, 0, 0)((void *)&alarmInfoIdl,inMsgHandle,0);
	if (CL_OK != rc)
	{
		clLogError( "ALM", "ALR", 
                "Failed while writing the data to the buffer. rc[0x%x]", rc); 
        goto exitOnError;
	}

	iocAddress.iocPhyAddress.nodeAddress = addr.nodeAddress;
	iocAddress.iocPhyAddress.portId    = addr.portId;

	opts.timeout = CL_ALARM_RMDCALL_TIMEOUT;
	opts.priority = CL_RMD_DEFAULT_PRIORITY;
	opts.retries = CL_ALARM_RMDCALL_RETRIES;

	rc = clRmdWithMsg (iocAddress,
					   CL_ALARM_STATUS_GET_IPI,
					   inMsgHandle,
					   outMsgHandle,
					   CL_RMD_CALL_ATMOST_ONCE|CL_RMD_CALL_NEED_REPLY,
					   &opts,
					   NULL);
	if(rc != CL_OK)
	{
		clLogError( "ALM", "ALR", "Failed while Sending RMD to the client "
		    "owning the alarm resource. rc[0x%x]", rc);
        goto exitOnError;
	}	


	/* Get the alarm state information from the outAlarmMsgHdl */
	rc = clXdrUnmarshallClUint32T(outMsgHandle,(void *)pAlarmState);
	if (CL_OK != rc)
	{
		clLogError( "ALM", "ALR", "Failed while unmarshalling the alarm handle. rc[0x%x]", rc);
	    goto exitOnError;
	}

    clLogInfo("ALM", "QAS", "Got the alarm state [%d]", *pAlarmState);

exitOnError:
    clCorObjectHandleFree(&objH);

    clBufferDelete(&inMsgHandle);
    clBufferDelete(&outMsgHandle);

    return rc;
}

/**
 * Function to get the pending alarms list from the owner of the MO.
 * It will marshall the MOId of the resource and send it to the owner.
 * Afer getting the MOId, it sends the request to the owner and gets
 * all the pending alarms on that MO.
 */
ClRcT   
_clAlarmGetPendingAlarmFromOwner (ClCorMOIdPtrT pMoId, 
                                 ClAlarmPendingAlmListPtrT pPendingAlmList)
{
    ClRcT           rc = CL_OK;
    ClUint32T       noOfPendingAlarms = 0, index = 0;
    ClCorAddrT      addr = {0};
    ClCorMOIdT      moId ;
    ClRmdOptionsT   rmdOpts = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT   iocAddr = {{0}};
    ClBufferHandleT inMsgH = 0, outMsgH = 0;

    moId = *pMoId;

    rc = clCorMoIdServiceSet(&moId , CL_COR_SVC_ID_ALARM_MANAGEMENT);
	if (CL_OK != rc)
	{
		clLogError( "ALM", "APG", "Failed while doing service set for the MoId. rc[0x%x]", rc); 
		return rc;
	}

	rc = clCorMoIdToComponentAddressGet(&moId, &addr);
    if (CL_OK != rc && CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT) == rc)
    {
        clLogInfo("ALM", "APG", "No alarm owner present.");
        pPendingAlmList->noOfPendingAlarm = 0;
        return CL_OK;
    }

	if (CL_OK != rc)
	{
		clLogError( "ALM", "APG", "Failed while getting the component address. rc[0x%x]", rc); 
		return rc; 
	}

	clLogDebug( "ALM", "APG", " Sending the RMD to the alarm Owner [0x%x:0x%x] ", 
            addr.nodeAddress, addr.portId);

	rmdOpts.timeout = CL_ALARM_RMDCALL_TIMEOUT;
	rmdOpts.priority = CL_RMD_DEFAULT_PRIORITY;
	rmdOpts.retries = CL_ALARM_RMDCALL_RETRIES;

	rc = clBufferCreate(&inMsgH);
	if (rc != CL_OK )
	{
		clLogError( "ALM", "ALR", "Failed while creating buffer. rc[0x%x]", rc); 
		return rc;
	}

	rc = VDECL_VER(clXdrMarshallClCorMOIdT, 4, 0, 0)(&moId, inMsgH, 0);
	if (CL_OK != rc)
	{
		clLogError( "ALM", "ALR", 
                "Failed while writing the data to the buffer. rc[0x%x]", rc); 
        goto exitOnError;
	}

    rc = clBufferCreate(&outMsgH);
    if (CL_OK != rc)
    {
        clBufferDelete(&inMsgH);
        clLogError("ALM", "APG", "Failed to allocate the out buffer. rc[0x%x]", rc);
        return rc;
    }

	iocAddr.iocPhyAddress.nodeAddress = addr.nodeAddress;
	iocAddr.iocPhyAddress.portId    = addr.portId;

	rc = clRmdWithMsg (iocAddr,
					   CL_ALARM_PENDING_ALARMS_GET_IPI,
					   inMsgH,
					   outMsgH,
					   CL_RMD_CALL_ATMOST_ONCE|CL_RMD_CALL_NEED_REPLY,
					   &rmdOpts,
					   NULL);
	if(rc != CL_OK)
	{
		clLogError( "ALM", "ALR", "Failed while Sending RMD to the client \
		    owning the alarm resource. rc[0x%x]", rc);
        goto exitOnError;
	}

    rc = clXdrUnmarshallClUint32T(outMsgH, &noOfPendingAlarms);
    if (CL_OK != rc)
    {
        clLogError("ALM", "APG", "Failed to unmarshall the count of pending alarms. rc[0x%x]", rc);
        return rc;
    }

    if (0 == noOfPendingAlarms)
    {
        clLogDebug ("ALM", "APG" , "There are no pending alarms for the resource");
        rc = CL_OK;
        goto exitOnError;
    }

    CL_ASSERT(!(noOfPendingAlarms >= CL_ALARM_MAX_ALARMS)); 

    pPendingAlmList->pAlarmList = clHeapAllocate( sizeof(ClAlarmHandleInfoT) * noOfPendingAlarms);
    if (NULL == pPendingAlmList->pAlarmList)
    {
        rc = CL_ALARM_RC(CL_ERR_NO_MEMORY);
        clLogError("ALM", "APG", "Failed to allocate the pending alarm list. rc[0x%x]", rc);
        goto exitOnError;
    }

    memset(pPendingAlmList->pAlarmList, 0, sizeof(ClAlarmHandleInfoT) * noOfPendingAlarms);

    for (index = 0; index < noOfPendingAlarms; index++)
    {
        ClAlarmHandleInfoIDLT alarmHandleInfoIdl = {0};
        ClAlarmPendingAlmInfoPtrT pAlarmHandleList = NULL;

        memset(&alarmHandleInfoIdl, 0, sizeof(ClAlarmHandleInfoIDLT));

        rc = VDECL_VER(clXdrUnmarshallClAlarmHandleInfoIDLT, 4, 0, 0)(outMsgH, &alarmHandleInfoIdl);
        if (CL_OK != rc)
        {
            clLogError("ALM", "APG", 
                    "Failed to un-marshall the alarm information of the pending "
                    "alarm obtained from the owner. rc[0x%x]", rc);
           goto exitOnError;
        }

        pAlarmHandleList = (pPendingAlmList->pAlarmList + pPendingAlmList->noOfPendingAlarm);

        CL_ASSERT(pAlarmHandleList);

        /* Allocate memory for the alarmInfo.
         * This memory needs to be freed by the user by calling clAlarmPendingAlarmListFree().
         */
        pAlarmHandleList->pAlarmInfo = clHeapAllocate(sizeof(ClAlarmInfoT) + (alarmHandleInfoIdl.alarmInfo.len));
        if (pAlarmHandleList->pAlarmInfo == NULL)
        {
            clLogError("ALM", "APG", "Failed to allocate memory");
            goto exitOnError;
        }

        memset(pAlarmHandleList->pAlarmInfo, 0, sizeof(ClAlarmInfoT) + (alarmHandleInfoIdl.alarmInfo.len));

        pAlarmHandleList->alarmHandle = alarmHandleInfoIdl.alarmHandle;
        pAlarmHandleList->pAlarmInfo->moId = alarmHandleInfoIdl.alarmInfo.moId;
        pAlarmHandleList->pAlarmInfo->probCause = alarmHandleInfoIdl.alarmInfo.probCause;
        pAlarmHandleList->pAlarmInfo->category = alarmHandleInfoIdl.alarmInfo.category;
        pAlarmHandleList->pAlarmInfo->severity = alarmHandleInfoIdl.alarmInfo.severity;
        pAlarmHandleList->pAlarmInfo->specificProblem = alarmHandleInfoIdl.alarmInfo.specificProblem;
        pAlarmHandleList->pAlarmInfo->alarmState = alarmHandleInfoIdl.alarmInfo.alarmState;

        /* Copy the payload information */
        pAlarmHandleList->pAlarmInfo->len = alarmHandleInfoIdl.alarmInfo.len;

        if (alarmHandleInfoIdl.alarmInfo.len > 0)
        {
            memcpy(pAlarmHandleList->pAlarmInfo->buff, alarmHandleInfoIdl.alarmInfo.buff, 
                    pAlarmHandleList->pAlarmInfo->len);

            /* Free the memory allocated by IDL */
            clHeapFree(alarmHandleInfoIdl.alarmInfo.buff);
        }

        (pPendingAlmList->noOfPendingAlarm)++;
    }

exitOnError:
    clBufferDelete(&inMsgH);
    clBufferDelete(&outMsgH);
    return rc;
}

/**
 * Function to get all the pending alarms in the system. The pending alarms are the one which are
 * having state as suppressed, under soaked, after soaking state, or they are in the assert state.
 *
 * 1. If only the MOId is given, then it will give the information about all the pending alarms
 *    for that MO.
 * 2. If nothing is provided, then it will look for all the pending alarms in the system and returns
 *    the list of all the alarms in the system.
 *
 */

ClRcT   
clAlarmPendingAlarmsQuery ( ClCorMOIdPtrT const pMoId, 
                            ClAlarmPendingAlmListPtrT const pPendingAlmList)
{
    ClRcT   rc = CL_OK;
    
    if (NULL == pPendingAlmList)
    {
        clLogError("ALM", "GPA", 
                "The pointer to the list for storing the pending alarms is passed NULL. ");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    if (NULL == pMoId)
    {
        rc = _clAlarmAllPendingAlmGet(pPendingAlmList);
        if (CL_OK != rc)
        {
            clLogError("ALM", "GPA", 
                    "Failed to get the pending alarms list. rc[0x%x]", rc);
            clAlarmPendingAlarmListFree(pPendingAlmList);
            return rc;
        }
    }
    else
    {
        rc = _clAlarmGetPendingAlarmFromOwner(pMoId, pPendingAlmList);
        if (CL_OK != rc)
        {
            clLogError("ALM", "GPA", 
                    "Failed to get the pending alarms for the given MO. rc[0x%x]", rc);
            clAlarmPendingAlarmListFree(pPendingAlmList);
            return rc;
        }
    }

    return rc;
}

/**
 * Function to free pending alarm list.
 */
ClRcT
clAlarmPendingAlarmListFree(ClAlarmPendingAlmListPtrT const pPendingAlmList)
{
    ClRcT   rc = CL_OK;
    ClUint32T i = 0;

    if (NULL == pPendingAlmList)
    {
        clLogError("ALM", "PLF", "The pointer to the pending alarm list is NULL");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    if (pPendingAlmList->noOfPendingAlarm != 0)
    {
        /* Free all the alarmInfo allocated */
        for (i=0; i < (pPendingAlmList->noOfPendingAlarm); i++)
        {
            clHeapFree((pPendingAlmList->pAlarmList + i)->pAlarmInfo);
        }

        clHeapFree(pPendingAlmList->pAlarmList);
        pPendingAlmList->noOfPendingAlarm = 0;
    }

    return rc;
}

/* Returns the list of attribute values corresponding to attrId of MO pointed to by objHandle. 
 * The number of attribute values equals the number of alarm profiles associated with the MO being passed*/
ClRcT   
_clAlmBundleAttrListGet(ClCorMOIdPtrT pMoId,
                        ClCorObjectHandleT objHandle, 
                        ClUint32T attrId, 
                        ClUint32T size, 
                        ClCorAttrValueDescriptorListT *pAttrList)
{
    ClRcT               rc = CL_OK;
    ClUint32T           i = 0, j = 0;
    ClUint32T           profileCount = 0;
    ClCorBundleHandleT  bHandle = 0;
    ClCorBundleConfigT  bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};


    if(!pAttrList)
    {
        clLogError("ALM", "INT", "Attribute list pointer is NULL");
        return rc;
    }
    rc = _clAlarmGetAlarmsCountOnResource(pMoId, &profileCount); 
    if(CL_OK != rc)
    {
        clLogError("ALM", "INT", "Error while getting number of alarm profiles, rc [0x%x]",
                rc);
        return rc;
    }
    if(!profileCount)
    {
        clLogInfo("ALM", "INT", "Profile count is zero, nothing to configure");
        return rc;
    }

    clLogDebug("ALM", "INT", "Profile count [%d]", profileCount);

    rc = clCorBundleInitialize(&bHandle, &bundleConfig);
    if(CL_OK != rc)
    {
        clLogError("ALM", "INT", "Error in initializing bundle, rc [0x%x]", rc);
        return rc;
    }

    /* Fill bundle details to send to COR */
    pAttrList->numOfDescriptor = profileCount;
    pAttrList->pAttrDescriptor = (ClCorAttrValueDescriptorT *)clHeapAllocate
        (pAttrList->numOfDescriptor * sizeof(ClCorAttrValueDescriptorT));

    if(!pAttrList->pAttrDescriptor)
    {
        clLogError("ALM", "INT", 
                "Error in allocating memory of size [%zd]",
                (pAttrList->numOfDescriptor * sizeof(ClCorAttrValueDescriptorT)));
        clCorBundleFinalize(bHandle);
        return rc;
    }

    memset(pAttrList->pAttrDescriptor, 0, pAttrList->numOfDescriptor * sizeof(ClCorAttrValueDescriptorT)); 

    for (i = 0; i < profileCount; i++)
    {
        /* Construct attr path */
        rc = clCorAttrPathAlloc(&(pAttrList->pAttrDescriptor[i].pAttrPath));
        if (CL_OK != rc)
        {
            clLogError("ALM", "INT", "Attribute path allocation failed");
            clHeapFree(pAttrList->pAttrDescriptor);
            clCorBundleFinalize(bHandle);
            return rc;
        }
        
        rc = clCorAttrPathAppend(pAttrList->pAttrDescriptor[i].pAttrPath, CL_ALARM_INFO_CONT_CLASS, i);
        if (rc != CL_OK)
        {
            clLogError("ALM", "INT", "Error in appending path with idx[%d], rc [0x%x]",
                    i, rc);
            goto free_and_exit;
        }        
        pAttrList->pAttrDescriptor[i].attrId = CL_ALARM_ENABLE;
        pAttrList->pAttrDescriptor[i].index = -1;
        pAttrList->pAttrDescriptor[i].bufferPtr = clHeapAllocate(sizeof(ClUint8T));
        if(!pAttrList->pAttrDescriptor[i].bufferPtr)
        {
            clLogError("ALM", "INT", "Error allocating memory of size [%zd]",
                    sizeof(ClUint8T));
            goto free_and_exit;
        }
        pAttrList->pAttrDescriptor[i].bufferSize = sizeof(ClUint8T);
        pAttrList->pAttrDescriptor[i].pJobStatus= clHeapAllocate(sizeof(ClCorJobStatusT));
        if(!pAttrList->pAttrDescriptor[i].pJobStatus)
        {
            clLogError("ALM", "INT", "Error allocating [%zd] bytes for job status",
                    sizeof(ClCorJobStatusT));
            goto free_and_exit;
        }
    }
    rc = clCorBundleObjectGet(bHandle, &objHandle, pAttrList);
    if(CL_OK != rc)
    {
        clLogError("ALM", "INT", 
                "Error in adding get job to the bundle, rc [0x%x]", rc);
        goto free_and_exit;
    }
    rc = clCorBundleApply(bHandle);
    if(CL_OK != rc)
    {
        clLogError("ALM", "INT", "Error in applying bundle, rc [0x%x]", rc);
        goto free_and_exit;
    }

    rc = clCorBundleFinalize(bHandle);
    if(CL_OK != rc)
    {
        clLogError("ALM", "INT", "Error in finalizing bundle, rc [0x%x]", rc);
    }
    return rc;

free_and_exit:
    /* Error condition */
    for(j = 0; j <= i; j++)
    {
        clCorAttrPathFree(pAttrList->pAttrDescriptor[j].pAttrPath);
        if(pAttrList->pAttrDescriptor[j].bufferPtr)
            clHeapFree(pAttrList->pAttrDescriptor[j].bufferPtr);
        if(pAttrList->pAttrDescriptor[j].pJobStatus)
            clHeapFree(pAttrList->pAttrDescriptor[j].pJobStatus);
    }
    clHeapFree(pAttrList->pAttrDescriptor);
    clCorBundleFinalize(bHandle);

    return rc;
}

/*
 * Leaky bucket initialize.
 */

#ifdef CL_ENABLE_ASP_TRAFFIC_SHAPING

#define CL_LEAKY_BUCKET_DEFAULT_VOL (200*1024)
#define CL_LEAKY_BUCKET_DEFAULT_LEAK_SIZE (100*1024)
#define CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL (400)

ClRcT clAlarmLeakyBucketInitialize(void)
{
    ClInt64T leakyBucketVol = getenv("CL_LEAKY_BUCKET_VOL") ? 
        (ClInt64T)atoi(getenv("CL_LEAKY_BUCKET_VOL")) : CL_LEAKY_BUCKET_DEFAULT_VOL;
    ClInt64T leakyBucketLeakSize = getenv("CL_LEAKY_BUCKET_LEAK_SIZE") ?
        (ClInt64T)atoi(getenv("CL_LEAKY_BUCKET_LEAK_SIZE")) : CL_LEAKY_BUCKET_DEFAULT_LEAK_SIZE;
    ClTimerTimeOutT leakyBucketInterval = {.tsSec = 0, .tsMilliSec = CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL };
    ClLeakyBucketWaterMarkT leakyBucketWaterMark = {0};

    leakyBucketWaterMark.lowWM = leakyBucketVol/3;
    leakyBucketWaterMark.highWM = leakyBucketVol/2;
    
    leakyBucketWaterMark.lowWMDelay.tsSec = 0;
    leakyBucketWaterMark.lowWMDelay.tsMilliSec = 50;

    leakyBucketWaterMark.highWMDelay.tsSec = 0;
    leakyBucketWaterMark.highWMDelay.tsMilliSec = 100;

    leakyBucketInterval.tsMilliSec = getenv("CL_LEAKY_BUCKET_LEAK_INTERVAL") ? atoi(getenv("CL_LEAKY_BUCKET_LEAK_INTERVAL")) :
        CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL;

    clLogInfo("ALM", "LEAKY-BUCKET-INI", "Creating a leaky bucket with vol [%lld], leak size [%lld], interval [%d ms]",
                 leakyBucketVol, leakyBucketLeakSize, leakyBucketInterval.tsMilliSec);

    return clLeakyBucketCreateSoft(leakyBucketVol, leakyBucketLeakSize, leakyBucketInterval, 
                                   &leakyBucketWaterMark, &gClLeakyBucket);
}

#else

ClRcT clAlarmLeakyBucketInitialize(void)
{
    return CL_OK;
}

#endif
