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
 * ModuleName  : eo
 * File        : clEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This file provide implementation of the EOnized application main 
 *  function.
 *          This C file SHOULD NOT BE MODIFIED BY THE USER. If a new
 *         client component library is added, then add the init and
 *         finalize functions here, and add a dummy function in
 *         ASP/components/ground/client/clGroundComponent.c
 *
 *
 ****************************************************************************/

/****************************************************************************** 
 * PLEASE DO NOT COPY OR EDIT THIS FILE
 *****************************************************************************/

#include <netinet/in.h>
#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clEoIpi.h>
#include <clEoParser.h>
#include <clEoLibs.h>
#include <clEoEvent.h>

#include <clDebugApi.h>
#include <clDbg.h>
#include <clDispatchApi.h>
#include <clDbg.h>
#include <clLogUtilApi.h>
#include <clCpmIpi.h>
#include <clCmApi.h>
#include <clTransport.h>

#define  CL_LOG_AREA "EO"
#define  CL_LOG_CTXT_INI "INI"
#define  CL_LOG_CTXT_FIN "FIN"


extern void clEoCleanup(ClEoExecutionObjT* pThis);
extern void clEoReceiverUnblock(ClEoExecutionObjT *pThis);
extern ClRcT clEoPriorityQueuesFinalize(ClBoolT force);
extern void clLoadEnvVars();
extern ClUint32T clEoWithOutCpm; 
extern ClInt32T clAspLocalId;

/* Default EO configuration */
ClEoConfigT eoConfig =
{
    CL_OSAL_THREAD_PRI_MEDIUM,    /* EO Thread Priority                       */
    2,                            /* No of EO thread needed                   */
    0,                            /* Required Ioc Port                        */
    (CL_EO_USER_CLIENT_ID_START + 0), 
    CL_EO_USE_THREAD_FOR_APP,     /* Thread Model                             */
    NULL,                         /* Application Initialize Callback          */
    NULL,                         /* Application Terminate Callback           */
    NULL,                         /* Application State Change Callback        */
    NULL                          /* Application Health Check Callback        */
};

ClUint8T eoBasicLibs[] =
{
    CL_TRUE,      /* Lib: Operating System Adaptation Layer   */
    CL_TRUE,      /* Lib: Timer                               */
    CL_TRUE,      /* Lib: Buffer Management                   */
    CL_TRUE,      /* Lib: Intelligent Object Communication    */
    CL_TRUE,      /* Lib: Remote Method Dispatch              */
    CL_TRUE,      /* Lib: Execution Object                    */
    CL_FALSE,     /* Lib: Object Management                   */
    CL_FALSE,     /* Lib: Hardware Adaptation Layer           */
    CL_FALSE      /* Lib: Database Adaptation Layer           */
};

/*
 * Client libraries used by this EO. All are optional and can be
 * enabled or disabled by setting to CL_TRUE or CL_FALSE.
 */

ClUint8T eoClientLibs[] =
{
    CL_FALSE,      /* Lib: Common Object Repository            */
    CL_FALSE,      /* Lib: Chassis Management                  */
    CL_FALSE,      /* Lib: Name Service                        */
    CL_FALSE,      /* Lib: Log Service                         */
    CL_FALSE,      /* Lib: Trace Service                       */
    CL_FALSE,      /* Lib: Diagnostics                         */
    CL_FALSE,      /* Lib: Transaction Management              */
    CL_FALSE,      /* NA */
    CL_FALSE,      /* Lib: Provisioning Management             */
    CL_FALSE,      /* Lib: Alarm Management                    */
    CL_FALSE,      /* Lib: Debug Service                       */
    CL_FALSE,      /* Lib: Cluster/Group Membership Service    */
    CL_FALSE,      /* Lib: PM */
};


void clAppConfigure(ClEoConfigT* clEoConfig,ClUint8T* basicLibs,ClUint8T* clientLibs)
{
    if (clEoConfig) memcpy(&eoConfig, clEoConfig,sizeof(ClEoConfigT));
    if (basicLibs)  memcpy(&eoBasicLibs, basicLibs,sizeof(eoBasicLibs));
    if (clientLibs) memcpy(&eoClientLibs, clientLibs,sizeof(eoClientLibs));
}


/*
 * List of Library Initialize Functions 
 */
typedef ClRcT (*ClInitFinalizeFunc) (void);

typedef struct
{
    ClInitFinalizeFunc fn;
    char*        libName;
} ClInitFinalizeDef;


/*
 * These declarations are present here to satisfy the compiler. The actual
 * function declarations and definitions must be present in the component's
 * Api.h files in the component's <include> directory. 
 */
extern void eoProtoInit(void);

extern ClRcT clIocLibInitialize(ClPtrT pConfig);
extern ClRcT clIocLibFinalize(void);

extern ClRcT clRmdLibInitialize(ClPtrT pConfig);
extern ClRcT clRmdLibFinalize(void);

extern ClRcT clOmLibInitialize(void);
extern ClRcT clOmLibFinalize(void);

extern ClRcT clHalLibInitialize(void);
extern ClRcT clHalLibFinalize(void);

extern ClRcT clDbalLibInitialize(void);
extern ClRcT clDbalLibFinalize(void);

extern ClRcT clCorClientInitialize(void);
extern ClRcT clCorClientFinalize(void);

extern ClRcT clNameLibInitialize(void);
extern ClRcT clNameLibFinalize(void);

extern ClRcT clTraceLibInitialize(void);
extern ClRcT clTraceLibFinalize(void);

extern ClRcT clTxnLibInitialize(void);
extern ClRcT clTxnLibFinalize(void);

extern ClRcT clMsoLibInitialize(void);
extern ClRcT clMsoLibFinalize(void);

extern ClRcT clProvInitialize(void);
extern ClRcT clProvFinalize(void);

extern ClRcT clAlarmLibInitialize(void);
extern ClRcT clAlarmLibFinalize(void);

extern ClRcT clGmsLibInitialize(void);
extern ClRcT clGmsLibFinalize(void);

extern ClRcT clPMLibInitialize(void);
extern ClRcT clPMLibFinalize(void);

#if 0
extern ClRcT clCliLibInitialize(void);
extern ClRcT clCliLibFinalize(void);

extern ClRcT clDebugLibInitialize(void);
extern ClRcT clDebugLibFinalize(void);

extern ClRcT clEoLibInitialize(ClPtrT pConfig);
extern ClRcT clEoLibFinalize(void);

extern ClRcT clHpiLibInitialize(void);
extern ClRcT clHpiLibFinalize(void);

extern ClRcT clLogLibInitialize(void);
extern ClRcT clLogLibFinalize(void);

extern ClRcT clTimerInitialize(ClPtrT pConfig);
extern ClRcT clTimerFinalize(void);
#endif

/*
 * FIXME: Use a single structure or avoid globals completely
 */
ClEoMemConfigT gClEoMemConfig;
ClBufferPoolConfigT gClEoBuffConfig;
ClHeapConfigT gClEoHeapConfig;
ClIocQueueInfoT gClIocRecvQInfo;
ClEoActionInfoT gMemWaterMark[CL_WM_MAX];
ClEoActionInfoT gClIocRecvQActions;
ClEoActionInfoT gIocWaterMark[CL_WM_MAX];

#ifdef CL_INCLUDE_NATIVE_IOC
ClIocConfigT *gpClEoIocConfig;
#endif

/*
 * Global variable to access the executable name for the purpose
 * of debugging through utility libraries or the kind.
 */
ClCharT *clEoProgName = NULL;
/*
 * This should go if IOC doesnt suffer from selective amnesia regarding
 * multiple initialisation of the library.
 */
ClBoolT gIsNodeRepresentative = CL_FALSE;

/*Multiple ASP initialization is supported*/
static ClBoolT gClASPInitialized = CL_FALSE;

static ClUint32T gClASPInitCount = 0;

ClEoEssentialLibInfoT gEssentialLibInfo[] = {
    { .libName = "OSAL", CL_EO_LIB_INITIALIZE_FUNC(clOsalInitialize), clOsalFinalize, NULL},
    { .libName = "MEMORY",CL_EO_LIB_INITIALIZE_FUNC(clMemStatsInitialize), clMemStatsFinalize, (ClPtrT )&gClEoMemConfig, 
        CL_SIZEOF_ARRAY(gMemWaterMark), (ClPtrT)gMemWaterMark},
    { .libName = "HEAP", CL_EO_LIB_INITIALIZE_FUNC(clHeapLibInitialize), clHeapLibFinalize, (ClPtrT)&gClEoHeapConfig},
    { .libName = "BUFFER", CL_EO_LIB_INITIALIZE_FUNC(clBufferInitialize), clBufferFinalize, (ClPtrT)&gClEoBuffConfig},
    { .libName = "TIMER", CL_EO_LIB_INITIALIZE_FUNC(clTimerInitialize), clTimerFinalize, NULL},
    { .libName = "IOC", CL_EO_LIB_INITIALIZE_FUNC(clIocLibInitialize), clIocLibFinalize, (ClPtrT)&gClIocRecvQInfo, 
        CL_SIZEOF_ARRAY(gIocWaterMark), (ClPtrT)gIocWaterMark},
    { .libName = "RMD", CL_EO_LIB_INITIALIZE_FUNC(clRmdLibInitialize), clRmdLibFinalize, NULL},
    { .libName = "EO", CL_EO_LIB_INITIALIZE_FUNC(clEoLibInitialize), clEoLibFinalize, NULL},
    { .libName = "POOL" },  /* Bounded by CL_EO_LIB_ID_RES */
    { .libName = "CPM" }, 
#ifdef CL_EO_TBD 
#endif
};

#ifdef CL_EO_TBD 
static ClRcT eoWaterMarkActionTableInit(void)
{
    ClRcT rc = CL_OK;

    return CL_OK;
}

static ClRcT eoWaterMarkActionTableExit(void)
{
    ClRcT rc = CL_OK;

    return CL_OK;
}
#endif

/*
 * These functions are defined multiply: once in the component and once in
 * component/ground/client/... This is to satisfy the compiler and linker when
 * it compiles libEoClient.a. When linking the actual server modules, the real
 * definitions shadow the dummies in <ground> and everything will work fine.. 
 */
ClInitFinalizeDef gClBasicLibInitTable[] = {
    {NULL, NULL},          /* OSAL moved to Essential */
    {NULL, NULL},          /* Buffer moved to Essential */
    {NULL, NULL},            /* Timer moved to Essential */
    {NULL, NULL},            /* Ioc moved to Essential */
    {NULL, NULL},            /* Rmd moved to Essential */
    {NULL, NULL},            /* Eo moved to Essential */
    { clOmLibInitialize,          "Om"   },
    { clHalLibInitialize,         "Hal"  },
    { clDbalLibInitialize,        "Dbal" }
};

ClInitFinalizeDef gClClientLibInitTable[] = {
    { clCorClientInitialize,      "Cor"             },
    { clCmLibInitialize,          "ChassisManager"  },
    { clNameLibInitialize,        "Name"            },
    { clLogLibInitialize,         "Log"             },
    { clTraceLibInitialize,       "Trace"           },
    { NULL,                       "Diag"            },
    { clTxnLibInitialize,         "Txn"             },
    { clMsoLibInitialize,         "Mso"             },
    { clProvInitialize,           "Prov"            },
    { clAlarmLibInitialize,       "Alarm"           },
    { clDebugLibInitialize,       "Debug"           },
    { clGmsLibInitialize,         "GroupMembership" },
    { clPMLibInitialize,          "PM" }
};


/*
 * List of Library Finalize Functions 
 */
ClInitFinalizeDef gClBasicLibCleanupTable[] = {
    {NULL, NULL},          /* OSAL moved to Essential */
    {NULL, NULL},          /* Buffer moved to Essential */
    {NULL, NULL},            /* Timer moved to Essential */
    {NULL, NULL},            /* Ioc moved to Essential */
    {NULL, NULL},            /* Rmd moved to Essential */
    {NULL, NULL},            /* Eo moved to Essential */
    { clOmLibFinalize,            "Om"    },
    { clHalLibFinalize,           "Hal"   },
    { clDbalLibFinalize,          "Dbal"  }
};

ClInitFinalizeDef gClClientLibCleanupTable[] = {
    { clCorClientFinalize,      "Cor"             },
    { clCmLibFinalize,          "ChassisManager"  },
    { clNameLibFinalize,        "Name"            },
    { clLogLibFinalize,         "Log"             },
    { clTraceLibFinalize,       "Trace"           },
    { NULL,                     "Diag"            },
    { clTxnLibFinalize,         "Txn"             },
    { clMsoLibFinalize,      "Mso"             },
    { clProvFinalize,           "Prov"            },
    { clAlarmLibFinalize,       "Alarm"           },
    { clDebugLibFinalize,       "Debug"           },
    { clGmsLibFinalize,         "GroupMembership" },
    { clPMLibFinalize,          "PM" }
};

/*
 * Gets the EO name.
 */
ClCharT* clEoNameGet(void)
{    
    return ASP_COMPNAME;    
}

/*
 * Gets the EO executable name.
 */
ClRcT clEoProgNameGet(ClCharT *pName,ClUint32T maxSize)
{
    ClRcT rc = CL_ERR_INVALID_PARAMETER;
    if(pName == NULL)
    {
        goto out;
    }
    snprintf(pName,maxSize,clEoProgName);
    rc = CL_OK;
out:
    return rc;
}

/*
 * This function initializes all the Basic ASP library 
 */
static ClRcT clEoEssentialLibInitialize(void)
{
    ClRcT rc = CL_OK;

    ClUint32T i = 0;
    ClUint32T tableSize =  CL_EO_LIB_ID_RES; // CL_SIZEOF_ARRAY(gEssentialLibInfo);

    /*
     * Initializing the Essential Libraries
     */

    for (i = 0; i < tableSize; i++)
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA, CL_LOG_CTXT_INI,
              "Initializing essential library [%s]...", gEssentialLibInfo[i].libName);
        if (CL_OK != (rc = gEssentialLibInfo[i].pLibInitializeFunc(gEssentialLibInfo[i].pConfig)))
        {
            if (CL_GET_ERROR_CODE(rc) != CL_ERR_INITIALIZED) /* Already Initialized is benign */
            {
                
            clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
                  "Failed to initialize essential library [%s], error [0x%x]",
                  gEssentialLibInfo[i].libName, rc);
            return rc;
            }
            
        }
    }
    return CL_OK;
}

static ClRcT clEoEssentialLibFinalize(void)
{
    ClRcT rc = CL_OK;

    ClInt32T i = 0;
    ClUint32T tableSize = CL_EO_LIB_ID_RES; // CL_SIZEOF_ARRAY(gEssentialLibInfo);

    /*
     * Finalizing the Essential Libraries
     */

    for (i = tableSize-1; i >= 0; i--)
    {
        rc = gEssentialLibInfo[i].pLibFinalizeFunc();
        if (CL_OK != rc)
        {
            clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_FIN,
                  "Failed to finalize essential library [%s], error [0x%x]",
                  gEssentialLibInfo[i].libName, rc);
            return rc;
        }
    }

    return CL_OK;
}

static ClRcT clAspBasicLibInitialize()
{
    ClUint32T i = 0;
    ClRcT rc = CL_OK;
    ClUint32T tableSize =
        sizeof(gClBasicLibInitTable) / sizeof(ClInitFinalizeDef);
    for (i = 0; i < tableSize; i++)
    {
        if (gClBasicLibInitTable[i].fn == NULL) continue;

        if (eoBasicLibs[i] == CL_TRUE)
        {
            clLog(CL_LOG_DEBUG, CL_LOG_AREA, CL_LOG_CTXT_INI,
                  "Initializing basic library [%s]...",
                  gClBasicLibInitTable[i].libName);
            if (CL_OK != (rc = gClBasicLibInitTable[i].fn()))
            {
                clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
                      "Failed to initialize basic library [%s], error [0x%x]",
                      gClBasicLibInitTable[i].libName, rc);
                return rc;
            }
        }
    }

    rc = clDispatchLibInitialize();
    if (CL_OK != rc)
    {
	    clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
		      "Error Initializing Basic Library"
			  "clDispatchLibInitialize(), rc=[0x%x]\n",
			  rc);
	    return rc;
    }

    return CL_OK;
}

static ClRcT clAspBasicLibFinalize()
{
    ClInt32T i = 0;
    ClRcT rc = CL_OK;
    ClUint32T tableSize =
        sizeof(gClBasicLibCleanupTable) / sizeof(ClInitFinalizeDef);

    rc = clDispatchLibFinalize();
    if (CL_OK != rc)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_FIN,
              "Error finalizing Dispatch Library "
              "clDispatchLibFinalize(), rc=[0x%x]\n",rc);
        return rc;
    }

    for (i = tableSize - 1; i >= 0; i--)
    {
        if (gClBasicLibCleanupTable[i].fn == NULL) continue;

        if (eoBasicLibs[i] == CL_TRUE)
        {
            if (CL_OK != (rc = gClBasicLibCleanupTable[i].fn()))
            {
                clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_FIN,
                      "Failed to finalize basic library [%s], error [0x%x]",
                      gClBasicLibCleanupTable[i].libName, rc);
            }
        }
    }

    return CL_OK;
}

/*
 * This should Initialize all the client side library of the various Service 
 */
ClRcT clAspClientLibInitialize(void)
{
    ClUint32T i = 0;
    ClRcT rc = CL_OK;
    ClUint32T tableSize =
        sizeof(gClClientLibInitTable) / sizeof(ClInitFinalizeDef);

    for (i = 0; i < tableSize; i++)
    {
        /*
         * Ignore if function not registered to avoid a crash. Refer bug 4061.
         */
        if (gClClientLibInitTable[i].fn == NULL)
            continue;

        if (eoClientLibs[i] == CL_TRUE)
        {
            clLog(CL_LOG_DEBUG, CL_LOG_AREA, CL_LOG_CTXT_INI,
                  "Initializing client library [%s]...",
                  gClClientLibInitTable[i].libName);
            if (CL_OK != (rc = gClClientLibInitTable[i].fn()))
            {
                clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
                      "Failed to initializing client library [%s], error [0x%x]",
                      gClClientLibInitTable[i].libName, rc);
                return rc;
            }
        }
    }
    return CL_OK;
}

/**
 * Called by CPM in clCpmComponentResourceCleanup()
 */
ClRcT clAspClientLibFinalize(void)
{
    ClInt32T i = 0;
    ClRcT rc = CL_OK;
    ClUint32T tableSize =
        sizeof(gClClientLibCleanupTable) / sizeof(ClInitFinalizeDef);

    for (i = tableSize - 1; i >= 0; i--)
    {
        /*
         * Ignore if function not registered to avoid a crash. Refer bug 4061.
         */
        if (gClClientLibCleanupTable[i].fn == NULL) 
            continue;

        if (eoClientLibs[i] == CL_TRUE)
        {
            clLog(CL_LOG_DEBUG, CL_LOG_AREA, CL_LOG_CTXT_FIN,
                  "Finalizing client library [%s]...",
                  gClClientLibCleanupTable[i].libName);
            if (CL_OK != (rc = gClClientLibCleanupTable[i].fn()))
            {
                clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_FIN,
                      "Failed to finalize client library [%s], error [0x%x]",
                      gClClientLibCleanupTable[i].libName, rc);
            }
        }
    }
    return CL_OK;
}

void clEoNodeRepresentativeDeclare(const ClCharT *pNodeName)
{
    gIsNodeRepresentative = CL_TRUE;
}

ClRcT clEoSetup(void)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pThis = NULL;
    
    clDbgInitialize();

    eoProtoInit();

    /*Get node representative if running without the damned CPM*/
    if (clEoWithOutCpm == CL_TRUE) 
    {
        ClCharT *pNodeRepresetativeFlag = getenv("CL_NODE_REPRESENTATIVE");

        if(pNodeRepresetativeFlag && !strcmp(pNodeRepresetativeFlag, "TRUE"))
        {
            clEoNodeRepresentativeDeclare(ASP_NODENAME);
            unsetenv("CL_NODE_REPRESENTATIVE");
        }
    }
    
#ifdef CL_INCLUDE_NATIVE_IOC    
    /*
     * Okay always parse the ioc config file
     */
    clLog(CL_LOG_INFO, CL_LOG_AREA, CL_LOG_CTXT_INI,
          "Reading IOC configuration file, for node [%s]", gClEoNodeName);
    rc = clIocParseConfig(gClEoNodeName,&gpClEoIocConfig);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
              "Failed to parse IOC config file, error [0x%x]", rc);
        return rc;
    }
    
    gpClEoIocConfig->iocConfigInfo.isNodeRepresentative = gIsNodeRepresentative;
    if(gIsNodeRepresentative == CL_TRUE)
    {
        gpClEoIocConfig->iocConfigInfo.iocNodeRepresentative =  eoConfig.reqIocPort;
    }
#endif 

    clLog(CL_LOG_INFO, CL_LOG_AREA, CL_LOG_CTXT_INI,
          "Reading EO configuration file");
    
    if(NULL != clEoProgName)
    {
        char componentCfg[CL_MAX_NAME_LENGTH];            
        snprintf(componentCfg,CL_MAX_NAME_LENGTH,"%s_cfg.xml",clEoProgName);
        rc = clEoGetConfig(componentCfg);
    }
    else
    {
        rc = clEoGetConfig(NULL);
    }
        
    if ( rc != CL_OK )
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
              "Failed to parse EO config file, error [0x%x]", rc);
        return rc;
    }

    clLog(CL_LOG_INFO, CL_LOG_AREA, CL_LOG_CTXT_INI,
          "Initializing essential libraries...");
    rc = clEoEssentialLibInitialize();
    if (rc != CL_OK)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
              "Failed to initialize all essential libraries, error [0x%x]", rc);
        return rc;
    }
    clLogUtilLibInitialize();

    clLog(CL_LOG_INFO, CL_LOG_AREA, CL_LOG_CTXT_INI, "Initializing basic libraries...");
    rc = clAspBasicLibInitialize();
    if (rc != CL_OK)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI, "Failed to initialize all basic libraries, error [0x%x]", rc);
        return rc;
    }

    /*
     * Initialize the signal Handler here 
     */
    clOsalSigHandlerInitialize();

    if (ASP_COMPNAME == NULL)
    {
        if (clEoWithOutCpm == CL_TRUE)
        {
            clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI, "This is an EO running without the CPM.");
        }
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI, "The ASP_COMPNAME environment variable is not set.");
        return CL_ERR_NULL_POINTER;
    }

    rc = clEoCreate(&eoConfig, &pThis);
    if (rc != CL_OK)
    {
        return rc;
    }

    clLog(CL_LOG_INFO, CL_LOG_AREA, CL_LOG_CTXT_INI, "Initializing client libraries...");
    rc = clAspClientLibInitialize();
    if (rc != CL_OK)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
              "Failed to initialize all client libraries, error [0x%x]", rc);
        return rc;
    }

    clLog(CL_LOG_INFO, CL_LOG_AREA, CL_LOG_CTXT_INI,
          "All libraries initialized, progressing to server initialization...");

    return CL_OK;
}

ClRcT clASPInitialize(void)
{
    ClRcT rc = CL_OK;
    ClCharT *iocAddressStr = NULL;

    if(CL_TRUE == gClASPInitialized)
    {
        gClASPInitCount++;
        return CL_OK;
    }
    
    iocAddressStr = getenv("ASP_NODEADDR");
    
    if (iocAddressStr != NULL)
    {
        clAspLocalId = atoi(iocAddressStr);
    }
    else
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
              "ASP_NODEADDR environment variable not set, aborting ASP initialization");
        return CL_ERR_NULL_POINTER; 
    }

    clEoWithOutCpm = clParseEnvBoolean("ASP_WITHOUT_CPM");
    
    clLoadEnvVars();
    
    rc = clEoSetup();
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_LOG_AREA, CL_LOG_CTXT_INI,
              "ASP initialize failed, error[0x%x]", rc);
        return rc;
    }

    clCpmTargetInfoInitialize();

    gClASPInitCount++;
    gClASPInitialized = CL_TRUE;

    return CL_OK;
}

ClRcT clEoTearDown(void)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *pThis = NULL;

    /*
     * We now rip off the EO priority queues. as its safe and outside
     * the dynamic thread context itself.
     */
    clEoPriorityQueuesFinalize(CL_FALSE);

    /*
     * Call the Asp Client Finalize function
     * Before that, finalize the transport layer, low-level gms client
     */
    clTransportLayerGmsFinalize();
    clAspClientLibFinalize();

    /* Release Event Related Resourse if allocated*/
    rc = clEoEventExit();  
    if (rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_LOG_AREA, CL_LOG_CTXT_FIN, "Failed to release event related resources, error [0x%x]", rc);
    }

    clEoMyEoObjectGet(&pThis);

    clEoReceiverUnblock(pThis);

    clLog(CL_LOG_DEBUG, CL_LOG_AREA, CL_LOG_CTXT_FIN, "Cleaning up EO layer...");
    clLogUtilLibFinalize(clEoClientLibs[3]);
    /*
     * In case, the EO didn't have the gmslib flag,
     * attempt to finalize it since the transport layer
     * could have initialized it indirectly
     */
    clGmsLibFinalize();

    if(pThis)
        clEoCleanup(pThis);
    
    clLog(CL_LOG_INFO, CL_LOG_AREA, CL_LOG_CTXT_FIN, "Finalizing basic libraries...");
    rc = clAspBasicLibFinalize();
    if (rc != CL_OK)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_FIN, "Failed to finalize all basic libraries, error [0x%x]", rc);
        return rc;
    }

    clLog(CL_LOG_INFO, CL_LOG_AREA, CL_LOG_CTXT_FIN, "Finalizing essential libraries...");
    rc = clEoEssentialLibFinalize();
    if (rc != CL_OK)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_FIN,
              "Failed to finalize all essential libraries, error [0x%x]", rc);
        return rc;
    }
    clLog(CL_LOG_INFO, CL_LOG_AREA, CL_LOG_CTXT_FIN,
          "All libraries are finalized");

    return CL_OK;
}

ClRcT clASPFinalize(void)
{
    ClRcT rc = CL_OK;
    
    if (CL_FALSE == gClASPInitialized)
    {
        clLog(CL_LOG_ERROR, CL_LOG_AREA, CL_LOG_CTXT_FIN,
              "Called ASP finalize without initializing ASP");
        
        return CL_ERR_NOT_INITIALIZED;
    }

    gClASPInitCount--;

    if(gClASPInitCount > 0)
    {
        return CL_OK;
    }
    
    /*
     * Unblock all threads. first
     */
    clEoUnblock(NULL);

    rc = clEoTearDown();
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_LOG_AREA, CL_LOG_CTXT_FIN,
              "ASP finalize failed, error[0x%x]", rc);
        return rc;
    }
    
    gClASPInitialized = CL_FALSE;

    return CL_OK;
}

ClRcT clEoDebugRegister(void)
{
    ClRcT rc = CL_OK;
    rc |= clTimerDebugRegister();
    rc |= clRmdDebugRegister();
    return rc;
}

ClRcT clEoDebugDeregister(void)
{
    ClRcT rc = CL_OK;
    rc |= clTimerDebugDeregister();
    rc |= clRmdDebugDeregister();
    return rc;
}

ClRcT clEoConfigure(ClEoConfigT *eoConfig, 
                    ClUint8T *basicLibs, ClUint32T numBasicLibs,
                    ClUint8T *clientLibs, ClUint32T numClientLibs)
{
    if(eoConfig)
    {
        memcpy(&clEoConfig, eoConfig, sizeof(clEoConfig));
    }

    if(basicLibs)
    {
        memcpy(clEoBasicLibs, basicLibs, 
               CL_MIN(sizeof(clEoBasicLibs)/sizeof(clEoBasicLibs[0]),
                      numBasicLibs) * sizeof(*basicLibs));
    }

    if(clientLibs)
    {
        memcpy(clEoClientLibs, clientLibs,
               CL_MIN(sizeof(clEoClientLibs)/sizeof(clEoClientLibs[0]),
                      numClientLibs) * sizeof(*clientLibs));
    }

    return CL_OK;
}

/*
 * To all the application we need to pass 1. Component Name 2. Ioc Address So
 * as of now we have decided that these will be passed as environment variable 
 */
ClRcT clEoMain(ClInt32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;

    ClEoExecutionObjT *pThis = NULL;

    ClTimerTimeOutT waitForExit = {.tsSec = 0, .tsMilliSec = 0 };

    clEoProgName = argv[0]; // Make the exe name accessible to aid in debugging

    rc = clEoSetup();
    if (rc != CL_OK)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
              "Exiting : EO setup failed, error [0x%x]", rc);
        exit(1);
    }
    
    rc = clEoMyEoObjectGet(&pThis);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
                "Exiting : EO my object get failed. error [%x0x].\n", rc);
        CL_ASSERT(0);
        exit(1);
    }
        
    /*
     * This should keep track of blocking APP initialize.
     */

    clOsalMutexLock(&pThis->eoMutex);
    ++pThis->refCnt;
    clOsalMutexUnlock(&pThis->eoMutex);

    rc = eoConfig.clEoCreateCallout(argc, argv);
    if (rc != CL_OK)
    {
        clLog(CL_LOG_CRITICAL, CL_LOG_AREA, CL_LOG_CTXT_INI,
              "Server initialization failed, error [0x%x]", rc);
        return rc;
    }

    if(eoConfig.appType == CL_EO_USE_THREAD_FOR_APP)
    {
        clEoUnblock(pThis);
    }

    /*
     * We block on the exit path waiting for a terminate.
     */
    clOsalMutexLock(&pThis->eoMutex);

    while (pThis->refCnt > 0)
    {
        clOsalCondWait(&pThis->eoCond, &pThis->eoMutex, waitForExit);
    }
    clOsalMutexUnlock(&pThis->eoMutex);

    clEoTearDown();

    return CL_OK;
}
