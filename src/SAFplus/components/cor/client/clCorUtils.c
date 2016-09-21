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
 * ModuleName  : cor
 * File        : clCorUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *  This modules implements Information Loading routines.
 *****************************************************************************/


/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clCpmApi.h>
#include <clCorMetaData.h>
#include <clCorApi.h>
#include <clCorErrors.h>
#include <clCorUtilityApi.h>
#include <clRmdApi.h>
#include <clCorClient.h>
#include <clCorIpi.h>
#include <clCorRMDWrap.h>

#include <xdrClCorServiceIdT.h>
/* Internal Headers*/

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/**
 *  Extracts the OM class from the COR class table.
 *
 *  This API locates the OM class within the information model and 
 * returns it
 *                                                                        
 *  @param pCorClassTbl - Pointer to the COR class table.
 *  @param moClass - MO class type.
 *  @param pOmClass - Buffer to store the returned OM class.
 *
 *  @returns CL_OK  - Success<br>
 *           CL_ERR_NOT_EXIST - OM class type can't be found.
 */

ClRcT clCorOmClassNameFromInfoModelGet(ClCorClassTypeT moClass, ClCorServiceIdT svcId, 
                                       ClOmClassTypeT *pOmClass, ClCharT *pClassName, ClUint32T maxClassSize)
{
    ClRcT rc;
    ClBufferHandleT inMessageHandle = 0;
    ClBufferHandleT outMessageHandle = 0;
    ClVersionT version;
    ClNameT className = {0};
    ClCorUtilExtendedOpT opCode = 0;

    if (!pOmClass || !pClassName || !maxClassSize)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "NULL argument\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    CL_COR_VERSION_SET(version);

    rc = clBufferCreate(&inMessageHandle);

    if(rc != CL_OK)
   	{
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Could not create Input message buffer\n"));
        return rc;
   	}

    /* Start writing on the input message buffer */
	if((rc = clXdrMarshallClVersionT(&version, inMessageHandle, 0)))
		goto HandleError;
    opCode = COR_MO_TO_OM_CLASS_UTIL_OP;
    if((rc = clXdrMarshallClInt32T(&opCode, inMessageHandle, 0)))
      goto HandleError;
    if((clXdrMarshallClInt32T((void *)&moClass, inMessageHandle, 0)) != CL_OK)
      goto HandleError;
    if((VDECL_VER(clXdrMarshallClCorServiceIdT, 4, 0, 0)((void *)&svcId, inMessageHandle, 0)) != CL_OK)
      goto HandleError;
	
    rc = clBufferCreate(&outMessageHandle);
    if(rc != CL_OK)
      goto HandleError;

    COR_CALL_RMD_SYNC_WITH_MSG(COR_UTIL_EXTENDED_OP, inMessageHandle, outMessageHandle, rc);

    if(rc != CL_OK)
      goto HandleError;

	/* Message was successfully delivered. Got back the OM class. */

    if(CL_OK != clXdrUnmarshallClInt32T(outMessageHandle, (ClUint8T*)pOmClass))
      goto HandleError;
    
    if(CL_OK != clXdrUnmarshallClNameT(outMessageHandle, &className))
      goto HandleError;

    strncpy(pClassName, (const ClCharT*)className.value, CL_MIN(className.length, maxClassSize));
    pClassName[CL_MIN(className.length, maxClassSize)] = 0;

HandleError :

    clBufferDelete(&outMessageHandle);
    clBufferDelete(&inMessageHandle);
	return rc;
}

ClRcT clCorOmClassFromInfoModelGet(ClCorClassTypeT moClass, ClCorServiceIdT svcId, ClOmClassTypeT *pOmClass)
{
    ClRcT rc;
    ClBufferHandleT inMessageHandle = 0;
    ClBufferHandleT outMessageHandle = 0;
    ClUint32T exprLen;
    ClVersionT version;

    CL_COR_VERSION_SET(version);

    if (pOmClass == NULL)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "NULL argument\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    rc = clBufferCreate(&inMessageHandle);

    if(rc != CL_OK)
   	{
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Could not create Input message buffer\n"));
        return rc;
   	}

    /* Start writing on the input message buffer */
	if((rc = clXdrMarshallClVersionT(&version, inMessageHandle, 0)))
		goto HandleError;
	if((clXdrMarshallClInt32T((void *)&moClass, inMessageHandle, 0)) != CL_OK) 
		goto HandleError;
	if((VDECL_VER(clXdrMarshallClCorServiceIdT, 4, 0, 0)((void *)&svcId, inMessageHandle, 0)) != CL_OK)
		goto HandleError;
	
    rc = clBufferCreate(&outMessageHandle);

	if(rc != CL_OK)
		goto HandleError;
  
    COR_CALL_RMD_SYNC_WITH_MSG(COR_UTIL_OP, inMessageHandle, outMessageHandle, rc);

	if(rc != CL_OK)
		goto HandleError;

	/* Message was successfully delivered. Got back the OM class. */

    exprLen = sizeof(ClOmClassTypeT );

	if(CL_OK != clXdrUnmarshallClInt32T(outMessageHandle, (ClUint8T*)pOmClass))
		goto HandleError;
   	
HandleError :

    clBufferDelete(&outMessageHandle);
    clBufferDelete(&inMessageHandle);
	return rc;
}

ClRcT clCorConfigLoad(const ClCharT *pConfigFile, const ClCharT *pRouteFile)
{
    ClRcT rc;
    ClBufferHandleT inMessageHandle = 0;
    ClVersionT version;
    ClCorUtilExtendedOpT opCode = 0;
    ClNameT configFile = {0};

    if (!pConfigFile || !pRouteFile)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "NULL argument\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    CL_COR_VERSION_SET(version);

    rc = clBufferCreate(&inMessageHandle);

    if(rc != CL_OK)
   	{
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Could not create Input message buffer\n"));
        return rc;
   	}

    /* Start writing on the input message buffer */
	if((rc = clXdrMarshallClVersionT(&version, inMessageHandle, 0)))
		goto HandleError;

    opCode = COR_CONFIG_LOAD_UTIL_OP;
    clNameSet(&configFile, pConfigFile);
    if((rc = clXdrMarshallClInt32T(&opCode, inMessageHandle, 0)))
        goto HandleError;
    if((rc = clXdrMarshallClNameT(&configFile, inMessageHandle, 0)))
        goto HandleError;
    clNameSet(&configFile, pRouteFile);
    if((rc = clXdrMarshallClNameT(&configFile, inMessageHandle, 0)))
        goto HandleError;

    COR_CALL_RMD_SYNC_WITH_MSG(COR_UTIL_EXTENDED_OP, inMessageHandle, 0, rc);

    HandleError :
    clBufferDelete(&inMessageHandle);
	return rc;
}


/**
 *  Create MO and MSO objects.
 *
 *  This API creates MO as well as associated MSO objects which are 
 * associated with the MO.
 *                                                                        
 *  @param pMoId- MOId of the MO which is to be created.
 *  @param pHandle- [out] Handle to the MO object created.
 *
 *  @returns CL_OK  - Success<br>
 *           CL_ERR_NOT_EXIST - OM class type can't be found.
 */

ClRcT clCorUtilMoAndMSOCreate( ClCorMOIdPtrT pMoId, ClCorObjectHandleT *pHandle)
{
    ClRcT rc;
    ClCorObjectHandleT tempHandle;
    ClCorMOServiceIdT svc;
    ClCorMOClassPathT    moClassPath;
    ClCorMOClassPathPtrT PcorClassPath = &moClassPath;
    ClCorMOIdPtrT   pTempMoId = NULL; 
    int i;


    if(NULL == pMoId)
        {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nNULL parameter passed\n"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

    svc = clCorMoIdServiceGet(pMoId);

    if(svc != CL_COR_INVALID_SRVC_ID)
        {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nMOId of MSO passed instead of MO\n"));
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }

        rc = clCorMoIdClone(pMoId, &pTempMoId);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while cloning the MoId. rc[0x%x]", rc));
            return rc;
        }

        /* First get MO Path from MOId */
        rc = clCorMoClassPathInitialize(PcorClassPath);

    if(CL_OK!=rc)
        {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nCould not Allocate MO Path. Rc = 0x%x\n",rc));
        clCorMoIdFree(pTempMoId);
        return rc;
        }
        
        rc =  clCorMoIdToMoClassPathGet(pTempMoId, PcorClassPath);

    if(CL_OK!=rc)
        {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nCould not get MOPath from MOId. Rc = 0x%x\n",rc));
        clCorMoIdFree(pTempMoId);
        return rc;
        }


    /* First check if the object is already present */
    if(CL_OK == clCorObjectHandleGet(pTempMoId, &tempHandle))
    	{
    	/* The object is already present. Just return */
        clCorMoIdFree(pTempMoId);
	return CL_COR_SET_RC(CL_COR_INST_ERR_MO_ALREADY_PRESENT);
    	}
	
    rc = clCorObjectCreate(CL_COR_SIMPLE_TXN, pTempMoId, pHandle);

    if(CL_OK!=rc)
        {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nCould not create MO. Rc = 0x%x\n",rc));
        clCorMoIdFree(pTempMoId);
        return rc;
        }

    for (i = CL_COR_SVC_ID_DEFAULT  + 1; i < CL_COR_SVC_ID_MAX; i++)
        {
        if((rc = clCorMSOClassExist(PcorClassPath, (ClCorServiceIdT) i))== CL_OK)
            {
            /* The MSO class exists. Create MSO object */
            if( (rc = clCorMoIdServiceSet(pTempMoId, (ClCorServiceIdT)i)) != CL_OK)
                {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nCould not create MO. Rc = 0x%x\n",rc));
                clCorMoIdFree(pTempMoId);
                return rc;
                }
            /* Create MSO */
            if( (rc = clCorObjectCreate(CL_COR_SIMPLE_TXN, pTempMoId, &tempHandle)) != CL_OK)
                {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nCould not create MSO.svcId = %d. Rc = 0x%x\n",i, rc));
                clCorMoIdFree(pTempMoId);
                return rc;
                }
            }
        }
    clCorMoIdFree(pTempMoId);
    return CL_OK;
}


ClRcT clCorUtilMoAndMSODelete( ClCorMOIdPtrT pMoId)
{
    ClRcT rc = CL_OK;
    ClCorObjectHandleT msoObjHandle;
    ClCorObjectHandleT moObjHandle;
    ClCorMOServiceIdT svc = 0;
    ClInt32T i = 0;
    ClCorMOIdPtrT pTempMoId = NULL;

    CL_COR_FUNC_ENTER("UTL", "DEL");
    
    if(NULL == pMoId)
    {
        clLog(CL_LOG_SEV_ERROR, "UTL", "DEL", "NULL parameter passed");        
        CL_COR_FUNC_EXIT("UTL", "DEL");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /* check if MO object exists, before deleting the MSOs. */
    if((rc = clCorObjectHandleGet(pMoId, &moObjHandle)) != CL_OK)
    {
        clLog(CL_LOG_SEV_ERROR, "UTL", "DEL", "Could not get Object Handle for MO. rc [0x%x]",rc);
        CL_COR_FUNC_EXIT("UTL", "DEL");
        return rc;
    }

    svc = clCorMoIdServiceGet(pMoId);
    if(svc != CL_COR_INVALID_SRVC_ID)
    {
        clLog(CL_LOG_SEV_ERROR, "UTL", "DEL", "MOId of MSO passed instead of MO");
        CL_COR_FUNC_EXIT("UTL", "DEL");
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }
	
    rc = clCorMoIdClone(pMoId, &pTempMoId);
    if (rc != CL_OK)
    {
        clLog(CL_LOG_SEV_ERROR, "UTL", "DEL", "Failed to allocate memory for MoId. rc [0x%x]", rc);
        CL_COR_FUNC_EXIT("UTL", "DEL");
        return rc;
    }
    
    for (i = CL_COR_SVC_ID_DEFAULT  + 1; i < CL_COR_SVC_ID_MAX; i++)
    {
        rc = clCorMoIdServiceSet(pTempMoId, (ClCorMOServiceIdT) i);
        if( rc != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, "UTL", "DEL", "Could not set service in MOID. rc [0x%x]",rc);            
            clCorMoIdFree(pTempMoId);
            CL_COR_FUNC_EXIT("UTL", "DEL");
            return rc;
        }
		  
        if(clCorObjectHandleGet(pTempMoId, &msoObjHandle)== CL_OK)
        {
            /* The MSO object exists. Delete it first */
            rc = clCorObjectDelete(CL_COR_SIMPLE_TXN, msoObjHandle);
            if(CL_OK != rc)
            {
                clLog(CL_LOG_SEV_ERROR, "UTL", "DEL", "Could not Delete MSO. rc [0x%x]",rc);
                clCorMoIdFree(pTempMoId);
                CL_COR_FUNC_EXIT("UTL", "DEL");
                return rc;
            }
        }
    }

    clCorMoIdFree(pTempMoId);

    CL_COR_FUNC_EXIT("UTL", "DEL");
    
	/* The MSOs are deleted. Now delete MO */
	return (clCorObjectDelete(CL_COR_SIMPLE_TXN, moObjHandle));
}

/**
 *  Determine the cor Version supported. 
 *
 *  This Api checks whether the version is supported or not. If the version check fails,
 *	the caller will get the version supported.
 *                                                                        
 *  @param version[IN/OUT] : version value.
 *
 *  @returns CL_OK  - Success<br>
 *           CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED) - Failure 
 * 	         Also the  version supported is filled in the 
 * 			 version as out parameter.
 */
ClRcT 
clCorVersionCheck(ClVersionT *version)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
	if( version == NULL)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL value of version is passed."));
		return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
	}
    
	if((rc = clVersionVerify(&gCorClientToServerVersionDb, version)))
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("The Cor Version is not supported. 0x%x", rc));
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);
	}
    CL_FUNC_EXIT();
    return rc;
}


/**
 *  Get the time delays from COR
 * 
 *  Function to get the time delays in certain situations from cor active and standby.
 *  This function sends some information from active COR and in some situations ask
 *  standby COR to send the information back.
 * 
 * /par 
 *  opType : The parameter to determine the information which is needed.
 *  ptime : This parameter will be filled by the client after getting information from server.
 *          The value returned in this parameter is in microseconds.
 *  
 * /return 
 *  CL_COR_ERR_NULL_PTR : null pointer passed for pTime.
 *  CL_COR_ERR_INVALID_PARAM : invalid value of opType passed.
 */ 


ClRcT clCorOpProcessingTimeGet (CL_IN ClCorDelayRequestOptT opType, CL_OUT ClTimeT* pTime)
{
    ClRcT           rc = CL_OK;
    ClBufferHandleT inMsgH = 0, outMsgH = 0;

    if (NULL == pTime)
    {
        clLogError("COR", "DLY", "The time parameter passed is NULL. ");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if (opType < CL_COR_DELAY_INVALID_REQ || opType > CL_COR_DELAY_MAX_REQ)
    {
        clLogError("COR", "DLY", "The value passed in opType parameter is invalid.");
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    rc = clBufferCreate(&inMsgH);
    if(CL_OK != rc)
    {
        clLogError("COR", "DLY", "Failed while allocating the in-message handle. rc[0x%x]", rc);
        return rc;
    }

    rc = clBufferCreate(&outMsgH);
    if(CL_OK != rc)
    {
        clBufferDelete(&inMsgH);
        clLogError("COR", "DLY", "Failed while allocating the out message handle. rc[0x%x]", rc);
        return rc;
    }

    rc = clXdrMarshallClInt32T (&opType, inMsgH, 0);
    if(CL_OK != rc)
    {
        clLogError("COR", "DLY", "Failed while marshalling the operation type. rc[0x%x]", rc);
        clBufferDelete(&inMsgH);
        clBufferDelete(&outMsgH);
        return rc;
    }

    COR_CALL_RMD_SYNC_WITH_MSG(CL_COR_PROCESSING_DELAY_OP, inMsgH, outMsgH, rc);

    if(CL_OK != rc)
    {
        clLogError("COR", "DLY", "Failed while getting the time information for \
                operatoin type [%d]. rc[0x%x]", opType, rc);
        clBufferDelete(&inMsgH);
        clBufferDelete(&outMsgH);
        return rc;
    }

    rc = clXdrUnmarshallClUint64T(outMsgH, pTime);
    if(CL_OK != rc)
    {
        clLogError("COR", "DLY", "Failed while unmarshalling the time information \
                for opType [%d]. rc[0x%x]", opType, rc);
        clBufferDelete(&inMsgH);
        clBufferDelete(&outMsgH);
        return rc;
    }

    clBufferDelete(&inMsgH);
    clBufferDelete(&outMsgH);
    return rc;
}

ClBoolT clCorHandleRetryErrors(ClRcT rc)
{
    if (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE || 
        CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE ||
        CL_GET_ERROR_CODE(rc) == CL_COR_ERR_TRY_AGAIN)
    {
        usleep(CL_COR_TRY_AGAIN_SLEEP_TIME);

        return CL_TRUE;
    }

    return CL_FALSE;
}
