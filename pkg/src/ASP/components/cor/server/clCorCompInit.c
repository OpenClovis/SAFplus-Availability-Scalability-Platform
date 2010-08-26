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
 * ModuleName  : cor
 * File        : clCorCompInit.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module is responsible for COR's libarary initilizations and finalizations
 *****************************************************************************/

/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <clDebugApi.h>
#include <clCpmApi.h>
#include <clLogApi.h>
#include <clEoQueue.h>
#include <clCorApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clCorConfigApi.h>
#include <clOmCommonClassTypes.h>
#include <clOampRtApi.h>
/* Internal Api*/
#include "clCorRMDWrap.h"
#include "clCorTreeDefs.h"
#include "clCorDmProtoType.h"
#include "clCorNiIpi.h"
#include "clCorPvt.h"
#include "clCorRmDefs.h"
#include "clCorStats.h"
#include "clCorNotify.h"
#include "clCorSync.h"
#include "clCorDeltaSave.h"
#include "clCorMoIdToNodeNameParser.h"
#include "clCorMoIdToNodeNameTable.h"
#include "clCorNiLocal.h"
#include "clCorObj.h"
#include "clCorEO.h"
#include "clCorTxnInterface.h"
#include "xdrClIocAddressIDLT.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif
#include "clCorLog.h"
#include <clCorAmf.h>

/************************************ GLOBAL************************************************/

ClUint32T corRunningMode = 0;

ClNameT gCompName ;

ClCorSyncStateT pCorSyncState = CL_COR_SYNC_STATE_INVALID;

/* COR save type, will be specified by CW */
ClUint32T gClCorSaveType;

/* Definition containing all the mutexes used on the server side. */
_ClCorServerMutexT gCorMutexes = {0};

ClEoExecutionObjT *pEOObj = NULL;

ClCpmHandleT             cpmHandle;

ClCorComponentConfigT corCfg;

/* COR db path, will be exported in the env variable ASP_DBDIR */
ClCharT gClCorPersistDBPath[CL_MAX_NAME_LENGTH] = {0};

#ifdef CL_COR_MEASURE_DELAYS
_ClCorOpDelayT gCorOpDelay = {0};
#endif

ClCorInitStageT gCorInitStage = CL_COR_INIT_INCOMPLETE;
ClCharT gClCorMetaDataFileName[CL_MAX_NAME_LENGTH] = {0};
static ClBoolT gClCorConfigLoad = CL_FALSE;
static CL_LIST_HEAD_DECLARE(gClCorCompResourceList);
/*********************************************************************************************/

/************************************* EXTERNS **********************************************/

/* This should be freed when the component is finalized */
extern ClCorClassTypeT gClCorClassIdAlloc;

/* Forward Declartions */
extern ClRcT corEOInit();
extern void corEoFinalize();
extern void corNiFinalize(void);
extern ClRcT corDebugRegister(ClEoExecutionObjT* pEoObj);
extern ClRcT corDebugDeregister(ClEoExecutionObjT* pEoObj);
extern ClRcT corSetClassIdAllocBase(void);

/********************************************************************************************/


/*************************************** STATICS *********************************************/
/* This function shall deallocated the static memory allocated 
      during COR init. */
static ClRcT corAppFinalize();

static ClRcT clCorComponentConfigure(void) ;
/********************************************************************************************/

/******************************************* MACROS ******************************************/
#define DEFAULT_TIMEPERIOD  5

/********************************************************************************************/


void corSaveInit(ClUint32T saveType)
{
	 if(saveType == CL_COR_DELTA_SAVE)
                gClCorSaveType = CL_COR_DELTA_SAVE;
     else
                gClCorSaveType = CL_COR_NO_SAVE;
			
}

ClRcT clCorLoadData(ClCorComponentConfigPtrT pThis)
{
	ClRcT rc = CL_OK;
	ClNameT	nodeName = {0};
	ClCharT fileExt[] =".corClassDb";
	ClUint32T	fileSize = 0;

	rc = clCpmLocalNodeNameGet(&nodeName);
	if(rc != CL_OK)
	{
		clLogError("INI", "LOD", 
                "Failed to get local node name for CPM. rc[0x%x]", rc);
		return rc;
	}
    
    /* The extra "1" byte is for NULL character */
	fileSize = nodeName.length + strlen(fileExt) + 1; 

	strncat(gClCorMetaDataFileName, nodeName.value, nodeName.length);
	strcat(gClCorMetaDataFileName, fileExt);

    clLogInfo("INI", "CLS", "Creating DB File Name : [%s], file name size : [%d]", 
            gClCorMetaDataFileName, fileSize);

    if(!clCpmIsMaster())
    {
        if (( rc = clCorDeltaDbsOpen(CL_DB_CREAT)) != CL_OK) 
        {
		    if(rc != CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST))
       	    {	
                clLogError("INI", "LOD", 
                        "FAILED to create delta DB in slave .. rc [0x%x]\n", rc);
			 	return rc;
			}
        }

        /* This is slave. Get the data from Master.*/
        if((rc =  synchronizeWithMaster()) != CL_OK)
        {
            clLogError("INI", "LOD", CL_LOG_MESSAGE_1_DATA_SYNC_MASTER_COR, rc);
            return(rc);
        }

	    if((rc =  _clCorDataSave()) != CL_OK)
	    {
       	    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to save class Info in slave. rc[0x%x]", rc)); 
			return rc;
	    }

        rc = clCorOmInfoBuild();
        if(CL_OK != rc)
        {
            clLogError("INI", "LOD", "Failed while reading the OM"
                  " class-Id information from the xml file. rc[0x%x]", rc);
            return rc;
        }

        if((rc = ClCorDeltaDbSlaveCreate() )!= CL_OK )
	    {
            clLogError("INI", "LOD",  
				CL_LOG_MESSAGE_1_DATA_SYNC_MASTER_COR, rc);
            return(rc);
	    }
       
        clLogInfo("INI", "LOD", CL_LOG_MESSAGE_0_SYNC_COMPLETE);
        return (CL_OK); 
    }
    /* In here only when as a master. This is restoring the MO tree, dm Class, Name-Interface Table */
 		rc = _clCorDataRestore();
		if(rc != CL_OK)
    	{
            /* If _clCorDataRestore fails, then try loading from classTable */
              clLogNotice("INI", "LOD", CL_LOG_MESSAGE_1_DATA_RESTORE, rc);
             if(rc == CL_ERR_NOT_EXIST)
	              clLogTrace("INI","LOD", "The database file is not present.");
             else
	              clLogTrace("INI", "LOD", 
                          "FAILED to restore COR data from database file. rc [0x%x]", rc);

            /* Load from the Default IM.*/
	        clLogNotice("INI", "LOD", "Loading  default Information model to COR.");
        
            rc = clCorInformationModelBuild(NULL);

            if (rc != CL_OK)
    	    { 
                /* Information model build failed */
                clLogError("INI", "LOD", CL_LOG_MESSAGE_1_INFORMATION_MODEL, rc);
                return rc;
     	    }
            gClCorConfigLoad = CL_TRUE;
        }          
	    else
        {
            rc = clCorOmInfoBuild();
            if(CL_OK != rc)
            {
                clLogError("INI", "LOD", "Failed while reading the OM"
                             " class-Id information from the xml file. rc[0x%x]", rc);
                return rc;
            }

            clLogNotice("INI", "LOD", CL_LOG_MESSAGE_0_DATA_RESTORE);
        }

        if (( rc = clCorDeltaDbsOpen(CL_DB_APPEND)) != CL_OK) 
        {
		    if(rc != CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST))
       	    {	
                clLogError("INI", "LOD", 
                        "FAILED to open delta DB. rc [0x%x]", rc);
			 	return rc;
			}
            rc = CL_OK ;
        }
	    else
	 	    rc = clCorDeltaDbRestore();
	  return rc;
}

/**
 *  Initialize the Clovis Object Registry component.
 *
 *  This API creates the COR EO And initializes the Clovis Object Registry
 *  component. This needs to be called before COR can be used.
 *                                                                        
 *  @param
 *
 *  @returns CL_OK  - Success<br>
 *           MO_NO_MEM - Failed to allocate memory<br>
 */

ClRcT 
corInitComponent(ClCorComponentConfigPtrT pThis)
{
    ClRcT rc = CL_OK;
    ClIocPhysicalAddressT myAddr;
	const ClCharT *aspDbPath = NULL;

	if (pThis == NULL)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL argument passsed in"));
		return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
	}

    myAddr.nodeAddress    = clIocLocalAddressGet();
    myAddr.portId= CL_IOC_COR_PORT;

    if ((rc = (clOsalMutexCreate(&gCorMutexes.gCorServerMutex))) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to allocate walk semaphore"));
        return (rc);
    }
                                
    if ((rc = (clOsalMutexCreate(&gCorMutexes.gCorSyncStateMutex))) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed during creation of CorTxnJob Mutex "));
        return (rc);
    }

    if((rc = clOsalMutexCreate(&gCorMutexes.gCorDeltaObjDbMutex)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the Object Db lock. rc[0x%x]", rc));
        return rc;
    }

    if((rc = clOsalMutexCreate(&gCorMutexes.gCorDeltaRmDbMutex)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the Route Db lock. rc[0x%x]", rc));
        return rc;
    }

    if((rc = clOsalMutexCreate(&gCorMutexes.gCorTxnValidation)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the Txn inprogress validation lock. rc[0x%x]", rc));
        return rc;
    }

    /*Initialize module for defining the cor save type*/
    corSaveInit(pThis->saveType);

    aspDbPath = getenv("ASP_DBDIR");
	
    if ((aspDbPath == NULL) || (strlen(aspDbPath) == 0))
    {
        aspDbPath = ".";
    }

	snprintf (gClCorPersistDBPath, CL_MAX_NAME_LENGTH - 1, "%s/cor", aspDbPath);
	
    if (gClCorSaveType != CL_COR_NO_SAVE)
    {
        clLogInfo("INT", "DBP", "Creating the COR-Db path: [%s]",gClCorPersistDBPath);
      
        rc = mkdir (gClCorPersistDBPath, 0755);
        if ((-1 == rc) && (EEXIST != errno))
        {
            clLogError("INT", "DBP", 
                    "Failed while creating the DB directory [%s] - [%s]", 
                    gClCorPersistDBPath, strerror(errno));
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_STATE);
        }
    }

	/* Initalize the COR statistics module */
	clCorStatisticsInitialize();

	/* Initalize the Data Manager sub-component within COR */
	   if((rc = dmInit()) != CL_OK)
           {
                   clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
					CL_LOG_MESSAGE_2_LIB_INIT, "Data Manager", rc);
                   CL_DEBUG_PRINT(CL_DEBUG_ERROR,  
                       ( "\n Data Manager lib initialization Failed. rc [0x%x]\n", rc));
                   return (rc);
           } 

    /* 
	 * TODO: For now a quick fix is card id is assumed to be the
	 * first byte in the ioc addr,so giving that for now, later to
	 * be replaced with actual card id.
	 */

	/* Initialize the Route Manager */
          if((rc = rmInit((myAddr.nodeAddress&0xFFFF), myAddr)) != CL_OK)
           {

                   clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
					CL_LOG_MESSAGE_2_LIB_INIT, "Route Manager", rc); 
                   CL_DEBUG_PRINT(CL_DEBUG_ERROR,  
                       ( "\n Route Manager lib initialization Failed. rc [0x%x]\n", rc));
                   return (rc);
           } 

	/* Initialize the MO Tree Library module */
	   if((rc = corMOTreeInit()) != CL_OK)
           {
                   clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
				CL_LOG_MESSAGE_2_LIB_INIT, "MOClass Tree" , rc);
                   CL_DEBUG_PRINT(CL_DEBUG_ERROR,  
                       ( "\nMOClass Tree lib initialization Failed. rc [0x%x]\n", rc));
                   return (rc);
           } 

	/* Initialize the Object Instance Tree Library module */
	   if((rc = corObjTreeInit()) != CL_OK)
           {
                   clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
					CL_LOG_MESSAGE_2_LIB_INIT, "Object Tree", rc);
                   CL_DEBUG_PRINT(CL_DEBUG_ERROR,  
                       ( "\nObject Tree lib initialization Failed. rc [0x%x]\n", rc));
                   return (rc);
           } 

	/* Initialize the Transaction Manager module */
	   if((rc = clCorTxnInterfaceInit()) != CL_OK)
           {
                   clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
				CL_LOG_MESSAGE_2_INIT, "COR Transaction interface", rc);
                   CL_DEBUG_PRINT(CL_DEBUG_ERROR,  
                       ( "\nCOR Transaction interface  initialization Failed. rc [0x%x]\n", rc));
                   return (rc);
           } 

        if((rc = clCorBundleDataContCreate()) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the container for storing the session info. rc[0x%x]", rc));
            return rc;
        }

	    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nBooting COR.............."));


   /* Initialize the COR EO thread */
       if((rc = corEOInit()) != CL_OK)
       {
               clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
					CL_LOG_MESSAGE_2_INIT, "EO Function Table", rc);	
               CL_DEBUG_PRINT(CL_DEBUG_ERROR,  
                    ( "\n  EO function table  initialization Failed. rc [0x%x]\n", rc));
                return (rc);
       } 

       if((rc = corEventInit()) != CL_OK)
        {
               clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
				CL_LOG_MESSAGE_2_INIT, "Event", rc);
               CL_DEBUG_PRINT(CL_DEBUG_ERROR,  
                    ( "\n Event initialization Failed. rc [0x%x]\n", rc));
                return (rc);
        }

        /* Initialize the COR Name Interface (NI) module */
        if((rc = corNiInit()) != CL_OK)
        {
               clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_WARNING, NULL, 
				CL_LOG_MESSAGE_2_INIT, "Name Interface Table", rc);
               CL_DEBUG_PRINT(CL_DEBUG_ERROR,  
                    ( "\n Name Interface table initialization Failed. rc [0x%x]\n", rc));
        }

        /* Loads the IM  to COR*/
        if((rc = clCorLoadData(pThis)) != CL_OK)
        {
               clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_WARNING, NULL,
					CL_LOG_MESSAGE_1_INFORMATION_MODEL_ABSENT, rc);
               CL_DEBUG_PRINT(CL_DEBUG_ERROR,  
                ( "\n COR could not get data from any source. No Information model present. rc [0x%x]\n", rc));
                return rc;
        }


   /* Register with DBG Infra */
    if(CL_OK != clEoMyEoObjectGet(&pEOObj))
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_WARNING, NULL, 
				CL_LOG_MESSAGE_1_EO_OBJECT_GET, rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get EO object [%x]",rc));
    }
    corDebugRegister(pEOObj);

    rc = _clCorAttrWalkRTContCreate();  
    if (CL_OK != rc)
    {
        clLogError("INT", "AWC", 
                "Failed while creating the container to store the information about runtime attributes. rc[0x%x]", rc);
        return rc;
    }
	 
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, NULL, CL_LOG_MESSAGE_0_COR_INITIALIZATION_COMPLETED);

    return (CL_OK);
} /* corCompInit */


ClRcT  corFinalize(ClInvocationT invocation,
                const ClNameT  *compName)
{
    gCompName = *compName;
    /* Cor finalize */
    corAppFinalize();

    clCpmComponentUnregister(cpmHandle, &gCompName, NULL);
    clCpmClientFinalize(cpmHandle);
    clCpmResponse(cpmHandle, invocation, CL_OK);
    /* clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, NULL, CL_LOG_MESSAGE_0_COR_FINALIZATION_COMPLETED); */
    return CL_OK;
}

static ClRcT corCreateResource(ClOampRtResourceT *pResources, 
                               ClUint32T noOfResources,
                               ClCorMOIdT *pMoids)
{
    ClRcT rc = CL_OK;
    register ClInt32T i;
    ClCorMOClassPathT moClassPath;
    CORMOClass_h moClassHandle;
    CORMSOClass_h msoClassHandle;

    for(i = 0; i < noOfResources; ++i)
    {
        if(pResources[i].autoCreateFlag == CL_FALSE
           ||
           pResources[i].wildCardFlag == CL_TRUE)
            continue;

        rc = corXlateMOPath((ClCharT*)pResources[i].resourceName.value, pMoids+i);
        if(rc != CL_OK)
        {
            clLogError("COR", "CREATE", "COR name to moid get for resource [%s] returned [%#x]",
                       pResources[i].resourceName.value, rc);
            return rc;
        }

        /*
         * Create the MO. Ignore if instance already exist
         */
        rc = _corMOObjCreate(pMoids+i);
        if(rc != CL_OK            
           && 
           CL_GET_ERROR_CODE(rc) != CL_COR_INST_ERR_MO_ALREADY_PRESENT)
        {
            pMoids[i].depth = 0;
            clLogError("COR", "CREATE", "COR object create for resource [%s] returned [%#x]",
                       pResources[i].resourceName.value, rc);
            return rc;
        }

        if(rc != CL_OK) 
        {
            clLogDebug("COR", "CREATE", "Skipping MO create of duplicated resource [%s]",
                       pResources[i].resourceName.value);
            continue;
        }

        clLogDebug("COR", "CREATE", "COR [%s] MO created", 
                   pResources[i].resourceName.value);

        /*
         * Create the MSOs if it is modeled.
         */
        clCorMoIdToMoClassPathGet( pMoids+i, &moClassPath );
        rc = corMOClassHandleGet(&moClassPath, &moClassHandle);
        if(rc != CL_OK) 
        {
            pMoids[i].depth = 0;
            continue;
        }

        /*
         * Create the provisioning mso.
         */
        rc = corMSOClassHandleGet(moClassHandle, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT,
                                  &msoClassHandle);
        if (rc == CL_OK)
        {
            rc = _corMSOObjCreate(pMoids + i, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
            if(rc != CL_OK
               &&
               CL_GET_ERROR_CODE(rc) != CL_COR_INST_ERR_MSO_ALREADY_PRESENT)
            {
                pMoids[i].depth = 0;
                clLogError("COR", "CREATE", "COR Prov MSO object create for resource [%s] returned [%#x]",
                           pResources[i].resourceName.value, rc);
                return rc;
            }
        
            if(rc != CL_OK)
            {
                clLogDebug("COR", "CREATE", "Skipping Prov MSO create of duplicated resource [%s]",
                            pResources[i].resourceName.value);
            }
            else
            {
                clLogDebug("COR", "CREATE", "COR [%s] Prov MSO created", 
                       pResources[i].resourceName.value);
            }
        }

        /*
         * Create the alarm mso. 
         */
        rc = corMSOClassHandleGet(moClassHandle, CL_COR_SVC_ID_ALARM_MANAGEMENT,
                                  &msoClassHandle);
        if (rc == CL_OK)
        {
            rc = _corMSOObjCreate(pMoids + i, CL_COR_SVC_ID_ALARM_MANAGEMENT);
            if(rc != CL_OK
               &&
               CL_GET_ERROR_CODE(rc) != CL_COR_INST_ERR_MSO_ALREADY_PRESENT)
            {
                pMoids[i].depth = 0;
                clLogError("COR", "CREATE", "COR Alarm MSO object create for resource [%s] returned [%#x]",
                           pResources[i].resourceName.value, rc);
                return rc;
            }
        
            if(rc != CL_OK)
            {
                clLogDebug("COR", "CREATE", "Skipping Alarm MSO create of duplicated resource [%s]",
                            pResources[i].resourceName.value);
            }
            else
            {
                clLogDebug("COR", "CREATE", "COR [%s] Alarm MSO created", 
                       pResources[i].resourceName.value);
            }
        }

        /*
         * Create the pm mso. 
         */
        rc = corMSOClassHandleGet(moClassHandle, CL_COR_SVC_ID_PM_MANAGEMENT,
                                  &msoClassHandle);
        if (rc == CL_OK)
        {
            rc = _corMSOObjCreate(pMoids + i, CL_COR_SVC_ID_PM_MANAGEMENT);
            if(rc != CL_OK
               &&
               CL_GET_ERROR_CODE(rc) != CL_COR_INST_ERR_MSO_ALREADY_PRESENT)
            {
                pMoids[i].depth = 0;
                clLogError("COR", "CREATE", "COR PM MSO object create for resource [%s] returned [%#x]",
                           pResources[i].resourceName.value, rc);
                return rc;
            }
        
            if(rc != CL_OK)
            {
                clLogDebug("COR", "CREATE", "Skipping PM MSO create of duplicated resource [%s]",
                            pResources[i].resourceName.value);
            }
            else
            {
                clLogDebug("COR", "CREATE", "COR [%s] PM MSO created", 
                       pResources[i].resourceName.value);
            }
        }

    }

    return CL_OK;
}

/*
 * Uncomment below to get resource creation stats.
 *#define RESOURCE_STATS
 */

ClRcT clCorCreateResources(const ClCharT *pDirName, const ClCharT *pSuffixName, ClInt32T suffixLen)
{
    ClOampRtResourceArrayT *pResourceArray = NULL;
    ClUint32T numScannedResources = 0;
    ClRcT rc = CL_OK;
    register ClInt32T i;
    ClCorMOIdT **pMoidArray = NULL;
    CORMOClass_h moClassHandle = NULL;
    CORMSOClass_h msoClassHandle = NULL;
    ClCorMOClassPathT moClassPath = {{0}};

#ifdef RESOURCE_STATS
    ClTimeT startTime, endTime;
    ClTimeT startDbTime = 0;
    ClTimeT endDbTime = 0;
    ClUint32T totalResources = 0;
#endif

    rc = clOampRtResourceScanFiles(pDirName, pSuffixName, suffixLen, &pResourceArray, &numScannedResources, 
                                   &gClCorCompResourceList);
    if(rc != CL_OK) return rc;

#ifdef RESOURCE_STATS
    startTime = clOsalStopWatchTimeGet();
#endif

    pMoidArray = clHeapCalloc(numScannedResources, sizeof(*pMoidArray));
    CL_ASSERT(pMoidArray != NULL);

    for(i = 0; i < numScannedResources; ++i)
    {
        pMoidArray[i] = clHeapCalloc(pResourceArray[i].noOfResources, sizeof(**pMoidArray));
        CL_ASSERT(pMoidArray[i] != NULL);
        clLogDebug("COR", "CREATE", "Read [%d] resources from [%s]",
                   pResourceArray[i].noOfResources, pResourceArray[i].resourceFile);
        rc = corCreateResource(pResourceArray[i].pResources, pResourceArray[i].noOfResources, 
                               pMoidArray[i]);
        if(rc != CL_OK) 
        {
            goto out_free;
        }
#ifdef RESOURCE_STATS
        totalResources += pResourceArray[i].noOfResources;
#endif
    }
    
    /*
     * We have to commit to persistent DB only if all the resources were created
     * successfully.
     */
    
#ifdef RESOURCE_STATS
    startDbTime = clOsalStopWatchTimeGet();
#endif

    for(i = 0; i < numScannedResources; ++i)
    {
        ClInt32T j;
        ClCorMOIdT *pMoids = pMoidArray[i];

        if(!pResourceArray[i].pResources) continue;

        for(j = 0; j < pResourceArray[i].noOfResources; ++j)
        {
            if(!pMoids[j].depth) continue;
            clCorDeltaDbStore(pMoids[j], NULL, CL_COR_DELTA_MO_CREATE);

            clCorMoIdToMoClassPathGet(&pMoids[j], &moClassPath);
            corMOClassHandleGet(&moClassPath, &moClassHandle);

            if (corMSOClassHandleGet(moClassHandle, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT, &msoClassHandle) == CL_OK)
            {
                pMoids[j].svcId = CL_COR_SVC_ID_PROVISIONING_MANAGEMENT;
                clCorDeltaDbStore(pMoids[j], NULL, CL_COR_DELTA_MSO_CREATE);
            }

            if(corMSOClassHandleGet(moClassHandle, CL_COR_SVC_ID_ALARM_MANAGEMENT, &msoClassHandle) == CL_OK)
            {
                pMoids[j].svcId = CL_COR_SVC_ID_ALARM_MANAGEMENT;
                clCorDeltaDbStore(pMoids[j], NULL, CL_COR_DELTA_MSO_CREATE);
            }

            if(corMSOClassHandleGet(moClassHandle, CL_COR_SVC_ID_PM_MANAGEMENT, &msoClassHandle) == CL_OK)
            {
                pMoids[j].svcId = CL_COR_SVC_ID_PM_MANAGEMENT;
                clCorDeltaDbStore(pMoids[j], NULL, CL_COR_DELTA_MSO_CREATE);
            }
        }
    }

#ifdef RESOURCE_STATS
    endDbTime = clOsalStopWatchTimeGet();
    clLogNotice("COR", "CREATE", "Time taken to store [%d] resources is : [%lld] usecs",
            totalResources, (endDbTime - startDbTime));
#endif

#ifdef RESOURCE_STATS
    endTime = clOsalStopWatchTimeGet();
    clLogNotice("COR", "CREATE", "Created [%d] resources in [%lld] usecs. "
                "Average resource creation time is [%lld] usecs", 
                totalResources, endTime - startTime,
                (endTime - startTime)/(totalResources ? totalResources : 1));
#endif

    out_free:
    if(pResourceArray)
    {
        for(i = 0; i < numScannedResources; ++i)
        {
            clHeapFree(pResourceArray[i].pResources);
            clHeapFree(pMoidArray[i]);
        }
        clHeapFree(pResourceArray);
        clHeapFree(pMoidArray);
    }

    return rc;
}

ClOampRtResourceArrayT *corComponentResourceGet(ClNameT *compName)
{
    register ClListHeadT *iter;
    CL_LIST_FOR_EACH(iter, &gClCorCompResourceList)
    {
        ClOampRtComponentResourceArrayT *array = CL_LIST_ENTRY(iter, ClOampRtComponentResourceArrayT, list);
        if(!strcmp(array->compName, compName->value))
            return &array->resourceArray;
    }
    return NULL;
}

ClRcT   corAppInitialize(ClUint32T argc, ClCharT *argv[])
{ 
    ClRcT  rc = CL_OK;
    
    ClNameT            appName = {0};
    ClCpmCallbacksT     callbacks = {0};
    ClVersionT  version = {0};
    ClIocPortT  iocPort = {0};
    ClUint32T master = 0;

    clCorClassConfigInitialize();

    /* This function takes external configurations(compile time) and initializes
     * cor libraries.  
     */
    if((rc = clCorComponentConfigure()) != CL_OK)
    {
	 clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, NULL,
					CL_LOG_MESSAGE_2_INIT, "COR Component", rc);
         CL_DEBUG_PRINT(CL_DEBUG_ERROR,("COR component Initialisation FAILED !!!!!  rc = 0x%x\n", rc));
 	return (rc);
    }
    /*  Do the CPM client init/Register */
    version.releaseCode = 'B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
                                                                                                                             
    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = corFinalize;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;
    
    clEoMyEoIocPortGet(&iocPort);
    rc = clCpmClientInitialize(&cpmHandle, &callbacks, &version);
    /** 
     ** MoId/NodeName configuration. and node object create
     **/
    if((rc = clCorMoIdToNodeNameTableFromConfigCreate()) != CL_OK)
    {
        clLogError("COR", "CREATE", 
                   "COR failed to get MOId/NodeName mapping information from XML. [%x]",rc);
        return rc;
    }
    master = clCpmIsMaster();

    if(gClCorConfigLoad && master)
    {
        rc = clCorCreateResources(NULL, NULL, 0);
        if(rc != CL_OK)
        {
            clLogError("COR", "INI", "COR resource creation returned [%#x]", rc);
            return rc;
        }
    }

    /* COR master is fully initialized for the slave to sync up */
    gCorInitStage = CL_COR_INIT_DONE;

    /* Set the application class id base */
    rc = corSetClassIdAllocBase();
    if (rc != CL_OK)
    {
        clLogError("COR", "INI", "Failed to set the application base class id. rc [0x%x]", rc);
        return rc;
    }

    /*
     * If loaded from config, initialize the AMF obj. tree on the master
     */
    if(gClCorConfigLoad && master)
    {
        rc = clCorBuiltinModuleRun();
        if(rc != CL_OK)
        {
            clLogError("COR", "MODULE", "COR builtin module run returned [%#x]", rc);
            return rc;
        }
    }

    rc = clCpmComponentNameGet(cpmHandle, &appName);
    rc = clCpmComponentRegister(cpmHandle, &appName, NULL);

    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_NOTICE, NULL,
        CL_LOG_MESSAGE_0_COR_COMPONENT_INIT);
    
    return rc;
}

/* This function shall deallocated the static memory allocated 
      during COR init. */
ClRcT corAppFinalize()
{
 
       /* Finalize cor - client (disables set/create/delete requests) */      
		/* clCorClientFinalize(); */

    /* Wait for all the other threads to complete its job */
    clEoQueuesQuiesce();

		clCorStationDisable();
        /* Deregister with DBG Infra */
        corDebugDeregister(pEOObj);
	/* Finalize the COR statistics module */
	clCorStatisticsFinalize();

	/* Finalize the Route Manager */
	rmFinalize();

        /* Finalize cor-txn interface*/
        clCorTxnInterfaceFini();

	/************** Finalize the object tree, mo tree and Dm, 
							in that order *********/
							
	/* Finalize the Object Instance Tree Library module */
	 corObjectTreeFinalize();

	/* Finalize the MO Tree Library module */
	 corMoTreeFinalize(); 

	/* Finalize the Data Manager sub-component within COR */
	 dmFinalize();

       /* Finalize the COR EO thread */
         corEoFinalize();
	    
      /* Finalize the COR Name Interface (NI) module */
         corNiFinalize();

	 /* Delete the mutex before going down. */
	clOsalMutexDelete(gCorMutexes.gCorServerMutex);

	clOsalMutexDelete(gCorMutexes.gCorSyncStateMutex);

	clOsalMutexDelete(gCorMutexes.gCorDeltaRmDbMutex);

	clOsalMutexDelete(gCorMutexes.gCorDeltaObjDbMutex);

    clOsalMutexDelete(gCorMutexes.gCorTxnValidation);

    /* 
       Delete the container created to store the information about the
       runtime attributes. 
     */
    _clCorAttrWalkRTContDelete();  

	 /* Finalize the moId to Node Name map table */
		clCorMoIdToNodeNameTableFinalize();
     /*Finalize the cor Event */
         corEventFinalize();

    /* Close the delta Dbs */
       clCorDeltaDbsClose() ;

    /* Deleting the container used for storing the session details. */
    clCorBundleDataContFinalize();

	return CL_OK;
}

ClRcT   corAppStateChange(ClEoStateT eoState)
{
    return CL_OK;
}
                                                                                                                             
                                                                                                                             
ClRcT   corAppHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
    return CL_OK;
}

ClEoConfigT clEoConfig = {
                   "COR",                         /* EO Name*/
                    1,                            /* EO Thread Priority */
                    7,                            /* No of EO thread needed */
                    CL_IOC_COR_PORT,              /* Required Ioc Port */
                    CL_EO_USER_CLIENT_ID_START,
                    CL_EO_USE_THREAD_FOR_RECV  ,      /* Whether to use main thread for eo Recv or not */
                    corAppInitialize,               /* Function CallBack  to initialize the Application */
                    NULL,         /* Function Callback to Terminate the Application */
                    corAppStateChange,                           /* Function Callback to change the Application state */
                    corAppHealthCheck /* appHealthCheck*/,       /* Function Callback to change the Application state */
                    NULL
                    };

/* What basic and client libraries do we need to use? */
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,			/* osal */
    CL_TRUE,			/* timer */
    CL_TRUE,			/* buffer */
    CL_TRUE,			/* ioc */
    CL_TRUE,			/* rmd */
    CL_TRUE,			/* eo */
    CL_FALSE,			/* om */
    CL_FALSE,			/* hal */
    CL_TRUE,			/* dbal */
};
  
ClUint8T clEoClientLibs[] = {
    CL_TRUE,			/* cor */
    CL_FALSE,			/* cm */
    CL_FALSE,			/* name */
    CL_TRUE,			/* log */
    CL_FALSE,			/* trace */
    CL_FALSE,			/* diag */
    CL_TRUE,			/* txn */
    CL_FALSE,			/* hpi */
    CL_FALSE,			/* cli */
    CL_FALSE,			/* alarm */
    CL_TRUE,			/* debug */
    CL_FALSE,			/* gms */
    CL_FALSE,           /* pm */ 
};


ClRcT 
clCorStationDisable()
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT sdAddr = 0, nodeAddr = 0, localAddr = 0;
	ClIocAddressIDLT idlIocAddress;

    CL_FUNC_ENTER();

    idlIocAddress.discriminant = CLIOCADDRESSIDLTIOCPHYADDRESS;
	localAddr = clIocLocalAddressGet();

	CL_CPM_MASTER_ADDRESS_GET(&nodeAddr);
    if (CL_OK != rc)
    {
        clLogError("FIN", "STD", "Failed to get the master address in the standby COR. rc[0x%x]", rc);
        return rc;
    }

	if(nodeAddr != localAddr)
		sdAddr = nodeAddr;
	else
	{
		clCorSlaveIdGet(&sdAddr);
	}
		
    idlIocAddress.clIocAddressIDLT.iocPhyAddress.nodeAddress = localAddr;
	idlIocAddress.clIocAddressIDLT.iocPhyAddress.portId = CL_IOC_COR_PORT; 
	
    /* make a RMD call to peer COR for disabling itself from its corlist request */
    if (sdAddr != 0)
        COR_CALL_RMD_WITH_DEST(sdAddr,
                               COR_EO_STATION_DIABLE,
                               VDECL_VER(clXdrMarshallClIocAddressIDLT, 4, 0, 0),
                               &idlIocAddress, sizeof(ClIocAddressIDLT), 
                               VDECL_VER(clXdrUnmarshallClIocAddressIDLT, 4, 0, 0),
                               NULL,
                               NULL,
                               rc);

    CL_FUNC_EXIT();
    return rc;
}


ClRcT 
clCorComponentConfigure(void)
{
	ClRcT rc = CL_OK;
    ClUint32T corSaveType = 0;

	ClCorComponentConfigT corCfg = {0};

    CL_FUNC_ENTER();

	memset(&corCfg, 0, sizeof(ClCorComponentConfigT));

    rc = clCorConfigGet(&corSaveType);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the configuration for COR. rc [0x%x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }
    
    corCfg.saveType = corSaveType;

	if ((rc = corInitComponent(&corCfg)) != CL_OK )
	{
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to initialize COR"));
            CL_FUNC_EXIT();
            return(rc);
	}



    CL_FUNC_EXIT();
	return (rc);
}

