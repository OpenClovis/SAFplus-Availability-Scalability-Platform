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
 * File        : clCorRmExport.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains  Route Manager's exported functionalities
 *****************************************************************************/

/* FILES INCLUDED */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorMetaData.h>
#include <clCorApi.h>
#include <clCorErrors.h>


/* Internal Headers */
#include "clCorDmDefs.h"
#include "clCorRmDefs.h"
#include "clCorDefs.h"
#include "clCorClient.h"
#include "clCorRMDWrap.h"
#include "clCorPvt.h"
#include "clCorLog.h"
#include "clCorDeltaSave.h"

#include <xdrCorRouteApiInfo_t.h>
#include <xdrCorObjFlagNWalkInfoT.h>
#include <xdrClCorObjFlagsT.h>
#include <xdrClIocAddressIDLT.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

extern ClUint32T COR_ISBD;
extern ClRcT clCorSlaveIdGet( ClIocNodeAddressT *corSlaveNodeAddressId);
extern ClRcT _clCorMoIdToMoIdNameGet(ClCorMOIdPtrT moIdh,  ClNameT *moIdName);

ClUint32T gCorSlaveSyncUpDone = CL_FALSE;
extern ClOsalMutexIdT gCorRouteInfoMutex;
extern ClCorInitStageT    gCorInitStage;


/* Todo: Need to add description to these API's along with examples */

ClRcT 
_corRouteRequest(corRouteApiInfo_t *pRt, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle)
{
    ClRcT              rc = CL_OK;
    ClCharT             moIdStr[CL_MAX_NAME_LENGTH] = {0};

    CL_COR_FUNC_ENTER("RMR", "EXP");

	if((rc = clOsalMutexLock(gCorRouteInfoMutex)) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get Lock on route mutex. rc[0x%x]",rc));
		return rc;
	}
 
    clLogTrace("RMR", "EXP", 
            "The route related request comes for [%s]", _clCorMoIdStrGet(&pRt->moId, moIdStr));

    switch(pRt->reqType)
    {

        case COR_SVC_RULE_STATION_ENABLE:
        {
            rc = rmRouteAdd(&pRt->moId, pRt->addr, pRt->status);
            if(rc == CL_OK)
                clCorDeltaDbRouteInfoStore(pRt->moId, *pRt, CL_COR_DELTA_ROUTE_CREATE);
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("_clCorRmRouteStationStatusOp Failed. rc[0x%x]", rc));
                clOsalMutexUnlock(gCorRouteInfoMutex);
                return rc;
            }
        }
        break;
        case COR_SVC_RULE_STATION_ADDR_GET:
        {
            rc = _clCorMoIdToComponentAddressGet(&pRt->moId, &pRt->addr);
            /* Got the component address. Copy it on outbuf */
            if(CL_OK == rc)
            {
                if(outMsgHandle)
                    rc = VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0)(pRt, outMsgHandle, 0);
            }
            else 
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nFailed to get the station address. rc[0x%x]\n", rc));
            }
            if(clOsalMutexUnlock(gCorRouteInfoMutex) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unlock Route Info mutex"));
                return rc;
            }
            return rc;
        }
        break;
        case COR_SVC_RULE_STATION_STATUS_GET:
        {
            rc = _clCorRmRouteStationStatusOp(pRt);
            /* Got the status for the station. Copy it on outbuf */
            if(CL_OK == rc)
            {
                if(outMsgHandle)
                    rc = VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0)(pRt, outMsgHandle, 0);
            }
            else 
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nFailed to get the status. rc[0x%x]\n", rc));
            }
            if(clOsalMutexUnlock(gCorRouteInfoMutex) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unlock Route Info mutex"));
                return rc;
            }
            return rc;
        }
        case COR_SVC_RULE_STATION_DISABLE:
        case COR_SVC_RULE_STATION_DELETE:
        {
            rc = _clCorRmRouteStationStatusOp(pRt);
            if(rc != CL_OK)
            {
                if(rc == CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT))
                    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("_clCorRmRouteStationStatusOp Failed. rc[0x%x]", rc));
                else
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("_clCorRmRouteStationStatusOp Failed. rc[0x%x]", rc));
                clOsalMutexUnlock(gCorRouteInfoMutex);
                return rc;
            }
        }
        break;
        case COR_SVC_RULE_STATION_DELETE_ALL:
        {
            rc = _clCorRmRouteStationDeleteAll(pRt);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("_clCorRmRouteStationDisableAll Failed. rc[0x%x]", rc));
                clOsalMutexUnlock(gCorRouteInfoMutex);
                return rc;
            } 
        }
        break;
        case COR_PRIMARY_OI_SET:
            rc = _clCorPrimaryOISet(pRt);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Failed while registering the OI. rc[0x%x]", rc));
                clOsalMutexUnlock(gCorRouteInfoMutex);
                return rc;
            }

            if(rc == CL_OK)
                clCorDeltaDbRouteInfoStore(pRt->moId, *pRt, CL_COR_DELTA_ROUTE_STATUS_SET);
        break;
        case COR_PRIMARY_OI_UNSET:
            rc = _clCorPrimaryOIClear(pRt);
            if(rc != CL_OK)
            {
                if (CL_GET_ERROR_CODE(rc) != CL_COR_ERR_OI_NOT_REGISTERED)
                    clLogError("RMR", "EXP", "Failed to unregister the primary OI [0x%x:0x%x]. rc [0x%x]",
                            pRt->addr.nodeAddress, pRt->addr.portId, rc);

                clOsalMutexUnlock(gCorRouteInfoMutex);
                return rc;
            }

            if(rc == CL_OK)
                clCorDeltaDbRouteInfoStore(pRt->moId, *pRt, CL_COR_DELTA_ROUTE_STATUS_SET);
        break;
        case COR_PRIMARY_OI_GET:
            rc = _clCorPrimaryOIGet(&pRt->moId, &pRt->addr);

            if(CL_OK == rc)
            {
                if(outMsgHandle)
                    rc = VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0)(pRt, outMsgHandle, 0);
                if (rc != CL_OK)
                {
                    clLogError("RMR", "EXP", "Failed to unmarshall corRouteApiInfo_t. rc [0x%x]", rc);
                }
            }
            else 
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, 
                        CL_LOG_MESSAGE_1_OI_NOT_REGISTERED, _clCorMoIdStrGet(&(pRt->moId), moIdStr));

                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get the OI address. rc [0x%x]", rc));
            }
        
            if(clOsalMutexUnlock(gCorRouteInfoMutex) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unlock Route Info mutex"));
                return rc;
            }

            return rc;
        break;
        default:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid Route related request type. %d", pRt->reqType));
    }

	/*
	 * This snippet is for the runtime syncUp of route list.
	 */
	if((clCpmIsMaster()) && (gCorSlaveSyncUpDone == CL_TRUE))
	{
        ClRcT retCode = CL_OK;
        retCode = clCorSyncDataWithSlave(COR_EO_ROUTE_OP, inMsgHandle);
        if (retCode != CL_OK)
        {
            clLogError("SYNC", "", "Failed to sync data with slave COR. rc [0x%x]", rc);
            /* Ignore the error code. */
        }
    }

	if(clOsalMutexUnlock(gCorRouteInfoMutex)!= CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unlock Route Info mutex"));
		return rc;
	}

    CL_COR_FUNC_EXIT("RMR", "EXP");  
    return rc;
}

ClRcT 
VDECL(_corRouteApi) (ClEoDataT cData, 
                     ClBufferHandleT  inMsgHandle,
                     ClBufferHandleT  outMsgHandle)
{
    ClRcT              rc = CL_OK;
    corRouteApiInfo_t   *pRt = NULL;

    CL_COR_FUNC_ENTER("RMR", "EXP");

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("RMR", "EXP", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    pRt = clHeapAllocate(sizeof(corRouteApiInfo_t));
    if (pRt == NULL)
    {
             CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("malloc failed"));
             return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

	if((rc = VDECL_VER(clXdrUnmarshallcorRouteApiInfo_t, 4, 0, 0)(inMsgHandle, (void *)pRt)) != CL_OK)
	{
		clHeapFree(pRt);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unmarshall corRouteApiInfo_t"));
		return rc;
	}

	clCorClientToServerVersionValidate(pRt->version, rc);
    if(rc != CL_OK)
	{
		clHeapFree(pRt);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

    rc = _corRouteRequest(pRt, inMsgHandle, outMsgHandle);

    CL_COR_FUNC_EXIT("RMR", "EXP");  
    clHeapFree(pRt);
    return rc;
}

/**
 *  Set flags for MO/MSO.
 *
 *  API to set flags associated with an MO Id. The MO Id can be a rule
 *  that contains wild cards like the one used in notification.
 *
 *  @param moh      ClCorMOId handle
 *  @param flags    Flags associated with the MO Id
 *
 *  @returns 
 *    CL_OK on success <br/>

 */
ClRcT   
_clCorObjectFlagsSet(ClCorMOIdPtrT moh, ClCorObjFlagsT flags)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();
    
    if( NULL == moh )
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    ret = rmFlagsSet(moh, flags);
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "_clCorObjectFlagsSet flags:%04x => RET [%04x]", flags, ret));
  
    CL_FUNC_EXIT();
    return (ret);
}


/**
 *  Get flags for MO/MSO.
 *
 *  API to Get flags associated with an MO Id. The rule that closely
 *  matches and its associated flags shall be returned.
 *
 *  @param moh      ClCorMOId handle
 *  @param flags    Flags associated with the MO Id
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT   
_clCorObjectFlagsGet(ClCorMOIdPtrT moh, ClCorObjFlagsT* pFlags)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();
    
    if( ( NULL == moh ) || ( NULL == pFlags ) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    *pFlags = rmFlagsGet(moh);
  
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_clCorObjectFlagsGet flags:%04x", *pFlags));
  
    CL_FUNC_EXIT();
    return (ret);
}

/**
 * [Internal]
 * API to locate the object and retrieve(get) required attributes.
 * It checks the RM Flags for the MO-Id and either makes local function
 * call or does RMD-Call to the remote-COR to retrieve the same.
 *
 * @param  this     ClCorObjectHandleT of the Object
 * @param  attrId   Attribute-ID 
 * @param  attrType Attribute-type (Simple/Assoc/Array/Containment)
 * @param  index    Index - valid for Assoc/Array/Containment attributes
 * @param  pValue   [out] Attribute-value
 * @param  pSize    [out] Size of attribute-value
 *
 * @return  CL_OK on success
 */
#ifdef DISTRIBUTED_COR
ClRcT
corObjAttrFind(ClCorObjectHandleT this, ClCorAttrPathPtrT pAttrPath, ClCorAttrIdT attrId,
                                            ClUint32T index, void * pValue, ClUint32T *pSize)
{
    ClRcT ret = CL_OK;
    ClCorMOIdT      moPath;
    ClCorObjFlagsT  flags;
    ClIocNodeAddressT localAddress;
    ClIocNodeAddressT remoteAddress;

    CL_FUNC_ENTER();

    if ((pValue == NULL) || (pSize == NULL) )
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);

    ret = corOH2MOIdGet((COROH_h)(this).tree, &moPath);
    if (ret != CL_OK)
    {
        return ret;
    }

    ret = _clCorObjectFlagsGet(&moPath, &flags);
    if (ret != CL_OK)
    {
        return ret;
    }

   localAddress=clIocLocalAddressGet();
   ret = _clCorMoIdToLogicalSlotGet(&moPath, &remoteAddress);

        if (ret != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\n Could not get Slot Address for MOId.. Assuming it be present locally.\n"));
            remoteAddress = clIocLocalAddressGet();
        }

    flags &= CL_COR_OBJ_CACHE_MASK;
	
    if ( ( (localAddress != remoteAddress) &&
	( ( flags == CL_COR_OBJ_CACHE_LOCAL ) || 
	( (flags == CL_COR_OBJ_CACHE_ONLY_ON_MASTER) && (COR_ISBD)))) ||
	( (flags == CL_COR_OBJ_CACHE_ON_MASTER) && (COR_ISBD)))
    {
        /* Object exists only on BD. Make an RMD-call */
        Byte_h     dataBuf;
        ClUint32T maxSize = sizeof(ClCorObjectHandleT) + sizeof(ClCorAttrPathT) +
                            sizeof(ClCorAttrIdT) + sizeof(index) + sizeof(*pSize);
        Byte_h  buf;
        if((buf = clHeapAllocate(maxSize) ) == NULL)
        {
	    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_SET_RC(CL_COR_ERR_STR)(CL_COR_SET_RC(CL_COR_ERR_NO_MEM))));
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }
        
        ClUint32T size = maxSize;
        dataBuf = buf;

        STREAM_IN_BOUNDS(buf, &this, sizeof(ClCorObjectHandleT), size);
        STREAM_IN_BOUNDS(buf, pAttrPath, sizeof(ClCorAttrPathT), size);
        STREAM_IN_BOUNDS(buf, &attrId, sizeof(attrId), size);
        STREAM_IN_BOUNDS(buf, &index, sizeof(index), size);
        STREAM_IN_BOUNDS(buf, pSize,  sizeof(ClUint32T), size);

	    /*TODO: RMD call needs to be tested out.*/
        COR_CALL_RMD_WITH_DEST(remoteAddress, COR_EO_DM_ATTR_HDLER, dataBuf, maxSize, pValue, pSize, ret);
        clHeapFree(dataBuf);
    }
    else
    {
        /* Object exists in this COR instance - Local func-call */
        ret = _clCorObjectAttributeGet(this, pAttrPath, attrId, index, pValue, pSize);
    }
    CL_FUNC_EXIT();
    return ret;
}
#endif

/* This might be usefull, if rmRouteGet is to be RMDised. */
#if 0
ClRcT  _corRouteGet(ClUint32T cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT ret = CL_OK;
    corRouteInfo_t *pRouteInfoIn;
 
    CL_FUNC_ENTER();
 
    if((ret = clBufferFlatten(inMsgHandle, (ClUint8T **)&pRouteInfoIn))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to flatten the Message"));
        CL_FUNC_EXIT();
        return ret;
    }   

    if((pRouteInfoIn == NULL) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_corRouteGet : NULL pointer"));
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    
    if(pRouteInfoIn->version > CL_COR_VERSION_NO)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Version mismatch"));
        CL_FUNC_EXIT();
        return CL_ERR_VERSION_MISMATCH;
    }

    ret = rmRouteGet (&(pRouteInfoIn->moId), (pRouteInfoIn->stations), &(pRouteInfoIn->stnCnt));
    if(ret != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "invalid input parameter") );
   
    /* Write to the message*/
    clBufferNBytesWrite (outMsgHandle, (ClUint8T *)pRouteInfoIn, sizeof(corRouteInfo_t));
    CL_FUNC_EXIT();
    return ret;
}
#endif


ClRcT 
VDECL(_corObjectFlagsOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    corObjFlagNWalkInfoT* pData = clHeapAllocate(sizeof(corObjFlagNWalkInfoT));
    ClCorObjFlagsT    outFlag;
    CL_FUNC_ENTER();

    if(pData == NULL) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_corObjectFlagsOp : NULL pointer"));
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

	if((rc = VDECL_VER(clXdrUnmarshallcorObjFlagNWalkInfoT, 4, 0, 0)(inMsgHandle, (void *)pData)) != CL_OK)
	{
		clHeapFree(pData);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unmarshall corObjFlagNWalkInfoT"));
		return rc;
	}
   /* if((rc = clBufferFlatten(inMsgHandle, (ClUint8T **)&pData))!= CL_OK)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to flatten the Message"));
         CL_FUNC_EXIT();
         return rc;
    }*/

    
    /*if(pData->version > CL_COR_VERSION_NO)
    {
		clHeapFree(pData);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Version mismatch"));
        CL_FUNC_EXIT();
        return CL_ERR_VERSION_MISMATCH;
    }*/

	clCorClientToServerVersionValidate(pData->version, rc);
    if(rc != CL_OK)
	{
		clHeapFree(pData);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

    switch(pData->operation)
    {
        case COR_OBJ_FLAG_SET:
            rc = _clCorObjectFlagsSet(&pData->moId, pData->flags);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_clCorObjectFlagsSet failed"));
            }
        break;
        case COR_OBJ_FLAG_GET:
            rc = _clCorObjectFlagsGet(&pData->moId, &outFlag);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "_clCorObjectFlagsGet failed"));
            }

            /* Write to the message*/
		    rc = VDECL_VER(clXdrMarshallClCorObjFlagsT, 4, 0, 0)(&outFlag, outMsgHandle, 0);
            if (CL_OK != rc)
                clLogError("OBF", "GET", "Failed while marshalling the object flags. rc[0x%x]", rc);
        break;
        default:
               CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "INVALID OPERATION, rc = %x", rc) );
               rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        break;
    }
    
    CL_FUNC_EXIT();
	clHeapFree(pData);

    return rc;
}

/*
 *   This call disables the Station's entry from COR's route list and corList.  
 */

ClRcT 
VDECL(_corStationDisable) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                           ClBufferHandleT  outMsgHandle)
{
     ClRcT rc = CL_OK;
	 ClIocAddressIDLT stAddr ;

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("RMR", "CSD", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    CL_COR_FUNC_ENTER("RMR", "CSD");
	stAddr.discriminant = CLIOCADDRESSIDLTIOCPHYADDRESS; 

    if((rc = VDECL_VER(clXdrUnmarshallClIocAddressIDLT, 4, 0, 0)(inMsgHandle, (void *)&stAddr))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to read the message "));
        CL_COR_FUNC_EXIT("RMR", "CSD");
        return (rc);
    }   
    
    if((rc = _clCorStationDisable(stAddr.clIocAddressIDLT.iocPhyAddress))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to delete the station entry in COR."));
        CL_COR_FUNC_EXIT("RMR", "CSD");
        return (rc);
    }   

      CL_COR_FUNC_EXIT("RMR", "CSD");
    return (rc);
}


