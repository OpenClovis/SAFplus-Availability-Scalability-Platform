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
#include <string.h>
#include <sys/statvfs.h>
#include <signal.h>
#include <errno.h>


static ClRcT 
clLogSvrInitialize(ClUint32T  argc, ClCharT  *argv[]);

extern ClRcT
clLogSvrDefaultStreamCreate(void);

extern ClRcT
clLogSvrDefaultStreamDestroy(void);

static ClRcT
clLogSvrShutdown(void);

static ClRcT 
clLogSvrStateChange(ClEoStateT  eoState);

static ClRcT 
clLogSvrHealthCheck(ClEoSchedFeedBackT  *pSchFeedback);

ClEoConfigT clEoConfig = {
    "LOG",      			    /* EO Name*/
    1,				            /* EO Thread Priority */
    5,            				/* No of EO thread needed */
    CL_IOC_LOG_PORT,        	/* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START, 
    CL_EO_USE_THREAD_FOR_RECV, 	/* Use main thread for eo Recv */
    clLogSvrInitialize,  		/* CB to initialize the Application */
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

#define LOGD_RESTART_COUNT_FILE "logd_restart_count"
#define CL_LOG_DEFAULT_MAX_RESTART_COUNT 5
#define CL_LOG_DEFAULT_MAX_DISK_SPACE_USAGE 80 
static ClHandleT shLogDummyIdl = CL_HANDLE_INVALID_VALUE;
ClBoolT gClLogSvrExiting = CL_FALSE;

static int clLogGetRestartCount(ClCharT *fileName)
{ 
    FILE *file = NULL;
    int  restartCount = 0;

    file = fopen(fileName, "r");
    if (file)
    {   int count = 0;
        count = fscanf(file, "%d", &restartCount);
        fclose(file);
    }
    return restartCount;
}

static void clLogSetRestartCount(ClCharT *fileName, int restartCount)
{
    FILE *file = NULL;

    file = fopen(fileName, "w+");
    if (file)
    {
        fprintf(file, "%d", restartCount);
        fclose(file);
    }
}
 
static void clLogSigintHandler(ClInt32T signum)
{
    const ClCharT    *filePath = NULL;
    unsigned long    blocks, freeblks;
    int              clLogMaxDiskSpaceUsage = CL_LOG_DEFAULT_MAX_DISK_SPACE_USAGE;
    int              restartCount = 0;
    int              clLogMaxRestartCount = CL_LOG_DEFAULT_MAX_RESTART_COUNT;
    int              filePathLen = 0;
    struct statvfs   buf;
    ClCharT          *fileName = NULL;
    ClCharT          *maxDiskSpaceUsage = NULL;
    ClCharT          *maxRestartCount = NULL;
    static ClBoolT   sigHndlInProgress = CL_FALSE;
    ClBoolT          shutdownNode = CL_FALSE;

    if (CL_TRUE == sigHndlInProgress)
    {/* SIGBUS Handling is already in-progress, so skip it */  
       return;
    }
    sigHndlInProgress = CL_TRUE;

    CL_LOG_DEBUG_ERROR(("Caught signal [%d]. Should be SIGBUS",signum));

   /* Check for Disk Space Usage */ 
    if ((maxDiskSpaceUsage = getenv("CL_LOG_MAX_DISK_SPACE_USAGE")))
    {
        clLogMaxDiskSpaceUsage = atoi(maxDiskSpaceUsage); 
    }
    filePath = getenv("CL_LOG_DISK_SPACE_FILE_PATH");
    if (!statvfs(filePath, &buf))
    {
        blocks = buf.f_blocks;
        freeblks = buf.f_bfree;
        if  ( (freeblks * 100) /(blocks)  < (100 - clLogMaxDiskSpaceUsage))
        {   /* Disk space usage is exceeded maximum usage limit so halt the system */
            shutdownNode = CL_TRUE;
        }
    }

    /* Get Logd Restart Count And Shutdown Node if it exceeds maximum restart count else commit suicide by using exit() */
    if ((maxRestartCount = getenv("CL_LOG_MAX_RESTART_COUNT")))
    {
        clLogMaxRestartCount = atoi(getenv("CL_LOG_MAX_RESTART_COUNT"));
    }
    if (!(filePath = getenv("ASP_RUNDIR")))
    {
        filePath = ".";
    }
    filePathLen = strlen(filePath);
    fileName = (ClCharT *) malloc(filePathLen + strlen(LOGD_RESTART_COUNT_FILE) + 5);
    sprintf(fileName, "%s/%s", filePath, LOGD_RESTART_COUNT_FILE);  

    restartCount = clLogGetRestartCount(fileName); 
    if (clLogMaxRestartCount <= restartCount)
    { /* Exceeded Maximum Restart Count, So Halt Node */
            shutdownNode = CL_TRUE;
    }

    if (CL_TRUE == shutdownNode)
    {
        restartCount = 0;
        clLogSetRestartCount(fileName, restartCount);
        clCpmNodeShutDown(clIocLocalAddressGet());
    }
    else
    {
        restartCount++;
        clLogSetRestartCount(fileName, restartCount);
        exit(1);
    }
}

static void clLogSigHandlerInstall(void)
{
    struct sigaction newAction;

    newAction.sa_handler = clLogSigintHandler;
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = SA_RESTART;

    if (-1 == sigaction(SIGBUS, &newAction, NULL))
    {
        perror("sigaction for SIGBUS failed");
        CL_LOG_DEBUG_ERROR(("Unable to install signal handler for SIGBUS"));
    }

    return;
}

ClRcT 
clLogSvrTerminate(ClInvocationT invocation,
				  const ClNameT  *compName)
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
        return rc;
    }
    hCpm = pSvrEoEntry->hCpm;
    CL_LOG_CLEANUP(clCpmComponentUnregister(pSvrEoEntry->hCpm, 
                                            compName, NULL), CL_OK);
    CL_LOG_CLEANUP(clLogSvrShutdown(), CL_OK);
    CL_LOG_CLEANUP(clCpmClientFinalize(hCpm), CL_OK);
	clCpmResponse(hCpm, invocation, CL_OK);

	CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
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
clLogSvrStateChange(ClEoStateT eoState)
{
    return CL_OK;
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
clLogSvrInitialize(ClUint32T argc,  
				   ClCharT   *argv[])
{
    ClCpmCallbacksT	 callbacks     = {0};
    ClVersionT		 version       = gLogVersion;
    ClNameT          logServerName = {0};  
    ClRcT            rc            = CL_OK;
    ClLogSvrEoDataT  *pSvrEoEntry  = NULL;
    ClBoolT          *pCookie      = NULL;
    ClIocAddressT    invalidAddr   = {{0}};
	
    clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
              "Log Server initialization is started...");

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
    clLogSigHandlerInstall();
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
    
    callbacks.appHealthCheck                 = NULL; 
    callbacks.appTerminate                   = clLogSvrTerminate;
    callbacks.appCSISet                      = NULL;
    callbacks.appCSIRmv                      = NULL;
    callbacks.appProtectionGroupTrack        = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup     = NULL;

    rc = clCpmClientInitialize(&pSvrEoEntry->hCpm, &callbacks, &version);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCpmClientInitialize(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrEoDataFree(), CL_OK);
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
        CL_LOG_CLEANUP(clLogStreamOwnerEoDataFree(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        return rc;
    }    

    rc = clCpmComponentNameGet(pSvrEoEntry->hCpm, &logServerName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCpmComponentNameGet(): rc[0x %x]", rc));
        clCpmClientFinalize(pSvrEoEntry->hCpm);
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrEoDataFree(), CL_OK);
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
        CL_LOG_CLEANUP(clLogStreamOwnerEoDataFree(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
        return rc;
    }    

    rc = clCpmComponentRegister(pSvrEoEntry->hCpm, &logServerName, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCpmComponentRegister(): rc[0x %x]", rc));
        clCpmClientFinalize(pSvrEoEntry->hCpm);
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrEoDataFree(), CL_OK);
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clLogStreamOwnerLocalShutdown(), CL_OK);
        CL_LOG_CLEANUP(clLogStreamOwnerEoDataFree(), CL_OK);
        CL_LOG_CLEANUP(clLogSvrCommonDataFinalize(), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(shLogDummyIdl), CL_OK);
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
        clCpmComponentUnregister(pSvrEoEntry->hCpm, &logServerName, NULL);
        clCpmClientFinalize(pSvrEoEntry->hCpm);
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
