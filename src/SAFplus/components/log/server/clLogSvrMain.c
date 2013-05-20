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
#include <clCpmExtApi.h>

#include <clLogServer.h>
#include <clLogMaster.h>
#include <clLogStreamOwner.h>
#include <clLogStreamOwner.h>
#include <clLogFileOwner.h>
#include <clLogSvrCommon.h>
#include <clLogFileEvt.h>
#include <clLogSvrDebug.h>
#include <saAmf.h>

extern ClRcT clLogSvrDefaultStreamCreate(void);

extern ClRcT clLogSvrDefaultStreamDestroy(void);

static ClRcT clLogSvrInitialize(ClUint32T  argc, ClCharT  *argv[]);

static ClRcT clLogSvrShutdown(void);

static ClRcT clLogSvrStateChange(ClEoStateT  eoState);

static ClRcT clLogSvrHealthCheck(ClEoSchedFeedBackT  *pSchFeedback);

ClEoConfigT clEoConfig = {
    1,				            /* EO Thread Priority */
    5,            				/* No of EO thread needed */
    CL_IOC_LOG_PORT,        	/* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START, 
    CL_EO_USE_THREAD_FOR_RECV, 	/* Use main thread for eo Recv */
    NULL,       		/* CB to initialize the Application */
    NULL,                		/* CB to Terminate the Application */ 
    clLogSvrStateChange, 		/* CB to change the Application state */
    clLogSvrHealthCheck, 		/* CB to change the Application state */
    NULL
};

ClUint8T clEoBasicLibs[] = {
    CL_TRUE,     /* osal */
    CL_TRUE,     /* timer */
    CL_TRUE,     /* buffer */
    CL_TRUE,     /* ioc */
    CL_TRUE,     /* rmd */
    CL_TRUE,     /* eo */
    CL_FALSE,    /* om */
    CL_FALSE,    /* hal */
    CL_TRUE      /* dbal */
};

ClUint8T clEoClientLibs[] = {
    CL_FALSE,    /* cor */
    CL_FALSE,    /* cm */
    CL_FALSE,    /* name */
    CL_TRUE,    /* log */ 
    CL_FALSE,    /* trace */
    CL_FALSE,    /* diag */
    CL_FALSE,    /* txn */
    CL_FALSE,    /* hpi */
    CL_FALSE,    /* cli */
    CL_FALSE,    /* alarm */
    CL_TRUE,     /* debug */
    CL_FALSE,    /* gms */
    CL_FALSE,    /* pm */
};

static ClHandleT shLogDummyIdl = CL_HANDLE_INVALID_VALUE;
ClBoolT gClLogSvrExiting = CL_FALSE;

static SaAmfHandleT amfHandle;

void clLogSvrTerminate(SaInvocationT invocation, const SaNameT *compName);

/* Utility functions */
void initializeAmf(void);
void dispatchLoop(void);
int  errorExit(SaAisErrorT rc);

ClBoolT unblockNow = CL_FALSE;

ClInt32T main(ClInt32T argc, ClCharT *argv[])
{
    ClRcT            rc            = CL_OK;
   
       

    rc = clLogSvrInitialize(argc,argv);
    if(rc != CL_OK)
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrInitialize() failed with : rc[0x %x]", rc));
        exit(0);
    }
    dispatchLoop();
    
    

    return 0;
}

ClRcT 
clLogSvrInitialize(ClUint32T argc,ClCharT   *argv[])
{

    SaAmfCallbacksT     callbacks = {0};
    SaVersionT          version;
    SaNameT          logServerName = {0}; 
    SaAisErrorT      rc1 = SA_AIS_OK;
    ClRcT            rc            = CL_OK;
    ClLogSvrEoDataT  *pSvrEoEntry  = NULL;
    ClBoolT          *pCookie      = NULL;
    ClIocAddressT    invalidAddr   = {{0}};
	
    clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
              "Log Server initialization is started...");

   clAppConfigure(&clEoConfig,clEoBasicLibs,clEoClientLibs);



    version.releaseCode  = 'B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;

    callbacks.saAmfHealthcheckCallback          = NULL; /* rarely necessary because SAFplus monitors the process */
    callbacks.saAmfComponentTerminateCallback   = clLogSvrTerminate;
    callbacks.saAmfCSISetCallback               = NULL;
    callbacks.saAmfCSIRemoveCallback            = NULL;
    callbacks.saAmfProtectionGroupTrackCallback = NULL;        
    callbacks.saAmfProxiedComponentInstantiateCallback    = NULL;
    callbacks.saAmfProxiedComponentCleanupCallback    = NULL;

    rc1 = saAmfInitialize(&amfHandle, &callbacks, &version);
    if( SA_AIS_OK != rc1 )
    {
        CL_LOG_DEBUG_ERROR(("saAmfInitialize(): rc[0x %x]", rc1));
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrEoDataFree(), CL_OK);
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
        CL_LOG_CLEANUP(clLogStreamOwnerEoDataFree(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        rc = (ClRcT)rc1;
        return rc;
    }

#if defined(CL_DEBUG) && defined(CL_DEBUG_START)
    clLogDebugLevelSet();
#endif
	CL_LOG_DEBUG_TRACE(("Enter"));	

    clLogSvrMutexModeSet();

    /* 
     * Here dummy initialization of Idl handle to avoid mutiple database 
     * Initialization & finalization databases. Keeping this handle alive 
     * will avoid this. coz Idl library will delete the handle database if the
     * handle count becomes zero. Mutiple times we are initializing & deleting
     * the handles in our log service usage.
     */
    rc = clLogIdlHandleInitialize(invalidAddr, &shLogDummyIdl);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogSvrCommonDataInit();
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrCommonDataInit(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        return rc;
    }

    rc = clLogStreamOwnerLocalBootup();
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogStreamOwnerLocalInit(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        return rc;
    }    

    pCookie = clHeapCalloc(1, sizeof(ClBoolT));
    if( NULL == pCookie )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
        CL_LOG_CLEANUP(clLogStreamOwnerEoDataFree(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    *pCookie = CL_FALSE;
    rc = clLogSvrBootup(pCookie);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrDataInit(): rc[0x %x]", rc));
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        return rc;
    }    
    clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
              "Log server boot type is [%s]", 
              (*pCookie == 1)? "Restart": "Normal");

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrEoDataFree(), CL_OK);
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
        CL_LOG_CLEANUP(clLogStreamOwnerEoDataFree(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        return rc;
    }

    
        
    pSvrEoEntry->hCpm = (ClHandleT)amfHandle;
    rc1 = saAmfComponentNameGet(amfHandle, &logServerName);
    if( SA_AIS_OK != rc1 )
    {
        CL_LOG_DEBUG_ERROR(("saAmfComponentNameGet(): rc[0x %x]", rc1));
        saAmfFinalize(amfHandle);
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrEoDataFree(), CL_OK);
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
        CL_LOG_CLEANUP(clLogStreamOwnerEoDataFree(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        rc = (ClRcT)rc1;
        return rc;
    }    

    rc1 = saAmfComponentRegister(amfHandle, &logServerName, NULL);
    if( SA_AIS_OK != rc1 )
    {
        CL_LOG_DEBUG_ERROR(("saAmfComponentRegister(): rc[0x %x]", rc1));
        saAmfFinalize(amfHandle);
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrEoDataFree(), CL_OK);
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
        CL_LOG_CLEANUP(clLogStreamOwnerEoDataFree(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        rc = (ClRcT)rc1;
        return rc;
    }   

    pSvrEoEntry->hTimer = CL_HANDLE_INVALID_VALUE;
    if( CL_FALSE == *pCookie )
    {
        rc = clLogSvrTimerDeleteNStart(pSvrEoEntry, pCookie);
    }
    else
    {
        rc = clLogTimerCallback((void *) pCookie);
    }
    if( CL_OK != rc )
    {
        saAmfComponentUnregister(amfHandle, &logServerName, NULL);
        saAmfFinalize(amfHandle);
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrEoDataFree(), CL_OK);
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
        CL_LOG_CLEANUP(clLogStreamOwnerEoDataFree(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        return rc;
    }
    rc = clLogDebugRegister();
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogDebugRegister(): rc[0x %x]", rc));
    }
        
    clLogNotice(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                "Log Server partially up");

	CL_LOG_DEBUG_TRACE(("Exit"));
	return CL_OK;					    
}




/*
 * clCompAppTerminate
 * ------------------
 * This function is invoked when the application is to be terminated.
 */

void clLogSvrTerminate(SaInvocationT invocation, const SaNameT *compName)
{
    
    ClRcT            rc           = CL_OK;
    ClLogSvrEoDataT  *pSvrEoEntry = NULL;
    ClHandleT        hCpm         = CL_HANDLE_INVALID_VALUE;

	CL_LOG_DEBUG_TRACE(("Enter"));

	clLogInfo("SVR", "MAI", "Unregistering with cpm...... [%.*s]\n", 
                           compName->length, compName->value);
    gClLogSvrExiting = CL_TRUE;
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if( CL_OK != rc )
    {
        return ;
    }
    hCpm = pSvrEoEntry->hCpm;
    saAmfComponentUnregister(amfHandle,compName, NULL);
    CL_LOG_CLEANUP(clLogSvrShutdown(), CL_OK);
    saAmfFinalize(amfHandle);
    saAmfResponse(amfHandle, invocation, SA_AIS_OK);

	CL_LOG_DEBUG_TRACE(("Exit"));


    unblockNow = CL_TRUE;

    
    
}

ClRcT
clLogSvrShutdown(void)
{
	ClRcT  rc = CL_OK;

	CL_LOG_DEBUG_TRACE(("Enter"));	

    CL_LOG_CLEANUP(clLogDebugDeregister(), CL_OK);
    CL_LOG_CLEANUP(clLogEvtFinalize(CL_TRUE), CL_OK);
    CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
    CL_LOG_CLEANUP(clLogFileOwnerShutdown(), CL_OK);
    CL_LOG_CLEANUP(clLogStreamOwnerGlobalShutdown(), CL_OK);
    CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
    CL_LOG_CLEANUP(clLogMasterShutdown(), CL_OK);
    CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
    CL_LOG_CLEANUP(clLogFileOwnerEoDataFree(), CL_OK);
    CL_LOG_CLEANUP(clLogStreamOwnerEoDataFree(), CL_OK);
    CL_LOG_CLEANUP(clLogSvrEoDataFree(), CL_OK);
    CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);

	CL_LOG_DEBUG_TRACE(("Exit"));	
    return rc;
}

ClRcT
clLogSvrHealthCheck(ClEoSchedFeedBackT* pSchFeedback)
{
    #if 0
    /* Modify following as per App requirement */
    
    pSchFeedback->freq   = CL_EO_DEFAULT_POLL; 
    pSchFeedback->status = CL_CPM_EO_ALIVE;
    pSchFeedback->status = CL_EO_DONT_POLL;
    #endif
    return CL_OK;
}

ClRcT 	
clLogSvrStateChange(ClEoStateT eoState)
{
    return CL_OK;
}


/******************************************************************************
 * Utility functions 
 *****************************************************************************/
/* This simple helper function just prints an error and quits */
int errorExit(SaAisErrorT rc)
{        
    
    exit(-1);
    return -1;
}



void dispatchLoop(void)
{        
  SaAisErrorT         rc = SA_AIS_OK;
  SaSelectionObjectT amf_dispatch_fd;
  int maxFd;
  fd_set read_fds;
 
  /*
   * Get the AMF dispatch FD for the callbacks
   */
  if ( (rc = saAmfSelectionObjectGet(amfHandle, &amf_dispatch_fd)) != SA_AIS_OK)
    errorExit(rc);
    
  maxFd = amf_dispatch_fd;  /* maxFd = max(amf_dispatch_fd,ckpt_dispatch_fd); */
  do
    {
      FD_ZERO(&read_fds);
      FD_SET(amf_dispatch_fd, &read_fds);
            
      if( select(maxFd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
          char errorStr[80];
          int err = errno;
          if (EINTR == err) continue;

          errorStr[0] = 0; /* just in case strerror does not fill it in */
          strerror_r(err, errorStr, 79);
          
          break;
        }
      if (FD_ISSET(amf_dispatch_fd,&read_fds)) saAmfDispatch(amfHandle, SA_DISPATCH_ALL);
      
    }while(!unblockNow);      
}


