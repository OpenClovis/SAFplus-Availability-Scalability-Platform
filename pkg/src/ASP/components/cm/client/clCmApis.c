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
 * ModuleName  : cm                                                            
 * File        : clCmApis.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * This file contains the implementation of client side apis of Chassis   
 * Manager Module.	
 *******************************************************************************/

#undef __SERVER__
#define __CLIENT__

#include <clCmApi.h>
#include <include/clCmDefs.h>
#include <clIocApi.h>
#include <clIocIpi.h>
#include <clIocServices.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clDebugApi.h>
#include <clCommonErrors.h>
#include <string.h>
#include <clCpmApi.h>
#include <clCpmExtApi.h>
#include <clCmServerFuncTable.h>
#include <clXdrApi.h>

static ClRcT 
clCmServerAddressGet(ClIocNodeAddressT *pIocAddress);

static ClRcT 
clCmRmdWithMsg(ClUint32T funcId,
                    ClBufferHandleT inMsgHdl,
                    ClBufferHandleT outMsgHdl,
                    ClUint32T flags,
                    ClRmdAsyncOptionsT *pAsyncOptions);

static ClRcT clCmSlotIdFromMOIdGet(CL_IN ClCorMOIdPtrT hMoId,
                                   CL_OUT ClUint32T * pChassisId,
                                   CL_OUT ClUint32T * pSlotId);

ClRcT clCmLibInitialize(void)
{
    return clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, CM), CL_IOC_CM_PORT);
}

ClRcT clCmLibFinalize(void)
{
    /* empty function to satisfy linker. */
    return CL_OK;
}

static ClRcT
clCmSlotIdFromMOIdGet(CL_IN ClCorMOIdPtrT hMoId,
                            CL_OUT ClUint32T * pChassisId, 
                            CL_OUT ClUint32T * pSlotId)
{
    ClCpmSlotInfoT cpmSlotInfo; 
    ClRcT rc=CL_OK; 

    if(pChassisId==NULL || pSlotId==NULL)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NULL POINTER passed in clCmSlotIdFromMOIdGet"));
        return CL_ERR_NULL_POINTER;
    }

    memset(&cpmSlotInfo,0,sizeof(ClCpmSlotInfoT )); 
    memcpy(&(cpmSlotInfo.nodeMoId), hMoId,sizeof(ClCorMOIdT));

    rc=clCpmSlotInfoGet(CL_CPM_NODE_MOID, & cpmSlotInfo);
    if(CL_OK!=rc)
    {
        *pSlotId=0;
        *pChassisId=0;
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clCpmSlotInfoGet Failed rc:%x",rc));
    }
    else 
    {
        *pSlotId=cpmSlotInfo.slotId;
        *pChassisId=0;/* No Mutli Chassis Support in RC2.2 .*/
    }
    return rc;
}

                            


static
ClRcT clCmRmdWithMsg(ClUint32T funcId,
                    ClBufferHandleT inMsgHdl,
                    ClBufferHandleT outMsgHdl,
                    ClUint32T flags,
                    ClRmdAsyncOptionsT *pAsyncOptions)
{
	ClRcT rc = CL_OK;
	ClIocAddressT cmServerAddress ;
	ClRmdOptionsT rmdOptions = CL_RMD_DEFAULT_OPTIONS; 
    ClUint8T status = 0;

	rc=clCmServerAddressGet(&cmServerAddress.iocPhyAddress.nodeAddress); 
	if(rc != CL_OK )
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clCmServerAddressGet Failed rc:0x%x",rc));
		return rc;
	}
    CL_CM_CLIENT_DEBUG_PRINT(("CmServer IocNode Address =%d",
    cmServerAddress.iocPhyAddress.nodeAddress));

	cmServerAddress.iocPhyAddress.portId= CL_IOC_CM_PORT;
    clIocCompStatusGet(cmServerAddress.iocPhyAddress, &status);
    if(status == CL_IOC_NODE_DOWN)
    {
        return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
    }

	memset((void*)&rmdOptions, 0 ,sizeof(rmdOptions));
    
	rmdOptions.timeout = CL_RMD_DEFAULT_TIMEOUT ;
	rmdOptions.priority = CL_RMD_DEFAULT_PRIORITY ;
	rmdOptions.retries = CL_RMD_DEFAULT_RETRIES;
   
    
    rc = clRmdWithMsg(cmServerAddress, 
                      /*clCmServerCpmResponseHandle */   
                      funcId,
					  inMsgHdl,
					  (ClBufferHandleT)outMsgHdl,
                      flags,
					  &rmdOptions,
                      pAsyncOptions);

	if(rc != CL_OK)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nRMD Call in to CM failed with rc:0x%x",rc));
	}

    return rc;
}

/**Description:
 * ------------
 * This API is used to get the state of the FRU . The states are defined in
 * the SaHpi.h header. First argument is the moid of the FRU for which the
 * state to be set. Second argument is a pointer to location to store the
 * state of the FRU. 
 *
 * @param in hMoId	:	moid of the FRU for which state to be set.
 * @param out pState:	Pointer to store the state of the FRU. 
 *
 * @returns CL_OK           :  on success full retrieval of the state.
 *			CL_ERR_INVALID_PARAMETER :  If any input parameter is invalid.
 * 			CL_ERR_NULL_POINTER    :  If NULL is passed to store the state.
 *			CL_ERR_VERSION_MISMATCH : If client and server are version
 *									  incompatible.
 */
ClRcT
clCmFruStateGet (ClCorMOIdPtrT hMoId, SaHpiHsStateT *pState)
{

	ClRcT rc = CL_OK;
	ClBufferHandleT inputMsg , outputMsg;
	ClUint32T outputLen = (ClUint32T)sizeof(SaHpiHsStateT);
    /*ClUint32T physicalSlot = 0;*/
	ClCmFruAddressT fruSlotInfo;
	CL_CM_VERSION_SET(fruSlotInfo.version);


	if(( rc = clCorMoIdValidate(hMoId))!= CL_OK ){
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nInvalid MOID passed "));
		return CL_CM_ERROR(CL_ERR_INVALID_PARAMETER);
	}

	if(pState == NULL )
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nNULL passed for storing state"));
		return CL_CM_ERROR(CL_ERR_NULL_POINTER);
	}


	/* FIXME : As of now we assume the physical slot is same as the logical
	 *  slot , Need to fix this later */
    rc = clCmSlotIdFromMOIdGet(hMoId,
                               &(fruSlotInfo.chassisId),
                               &(fruSlotInfo.physicalSlot));
	if(rc != CL_OK)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nGetting Slot id from moid failed rc:0x%x",rc));
		return rc;
	}




	/* form the message with api request */ 
	if((rc = clBufferCreate( &inputMsg ))!= CL_OK ){
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nMessage Creation failed rc:0x%x",rc));
		return rc;
	}

	if((rc = clBufferCreate( &outputMsg ))!= CL_OK ){
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nMessage Creation failed rc:0x%x",rc));
		clBufferDelete ( &inputMsg );
		return rc;
	}

	/* copy the api req info to the Message */ 
    /* Mahesh. changed the message to include Moid
     *  instead of structure having unions */
	clBufferNBytesWrite( inputMsg , 
			(ClUint8T *)&fruSlotInfo,
			(ClUint32T)sizeof(ClUint32T));

	if((rc = clCmRmdWithMsg( 
					CL_CHASSIS_MANAGER_FRU_STATE_GET,
					inputMsg,
					outputMsg,
					(ClUint32T)(CL_RMD_CALL_ATMOST_ONCE | CL_RMD_CALL_NEED_REPLY),
					NULL
					))!= CL_OK ){
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nclCmRmdWithMsg Call in to CM failed with rc:0x%x",rc));
		clBufferDelete ( &inputMsg );
		clBufferDelete ( &outputMsg );
		return rc;
	}


	clBufferNBytesRead (
			outputMsg, 
			(ClUint8T*)pState, 
			(ClUint32T*)&outputLen 
			);

	clBufferDelete ( &inputMsg );
	clBufferDelete ( &outputMsg );

	return rc;
}


/** Description:
 *	------------
 * This api is used to operate on the FRU or any platform hardware.API takes 2
 * arguments , first argument is the moid of the FRU to operate , second
 * argument is the operation requested. Operations are listed in the
 * enumeration cmRequestT. 
 *
 * @param hMoId				:	moid of the platform hardware to operate.
 * @param request 			:   Operation requested on the hardware.
 * 
 * @returns	CL_OK 		   : 	on successfull return.
 * 			CL_ERR_INVALID_PARAMETER :	If any of the input arguments is invalid.
 *			CL_ERR_VERSION_MISMATCH : If client and server are version
 *									  incompatible.
 *
 */
ClRcT
clCmFruOperationRequest (ClCorMOIdPtrT hMoId, ClCmFruOperationT request)
{

	ClRcT rc = CL_OK;
    ClCmOpReqT cmOpReq;

	if(( rc = clCorMoIdValidate(hMoId))!= CL_OK )
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nInvalid MOID passed "));
		return CL_CM_ERROR(CL_ERR_INVALID_PARAMETER);
	}
    if(request < CL_CM_POWERON_REQUEST || request > CL_CM_RESET_DEASSERT_REQUEST )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nInvalid Request passed "));
		return CL_CM_ERROR(CL_ERR_INVALID_PARAMETER);
    }


	memset((void*)&cmOpReq, 0 , sizeof(ClCmOpReqT));
	/* FIXME : As of now we assume the physical slot is same as the logical
	 *  slot , Need to fix this later */
     rc = clCmSlotIdFromMOIdGet(hMoId,
                               &(cmOpReq.chassisId),
                               &(cmOpReq.physicalSlot));
	if(rc != CL_OK)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nGetting Slot id from moid failed rc:0x%x",rc));
		return rc;
	}

	
    rc=clCmBladeOperationRequest (cmOpReq.chassisId, 
                                  cmOpReq.physicalSlot,
                                  request);
	/* form the message with api request */ 
	if(rc != CL_OK )
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nclCmBladeOperationRequest failed rc:0x%x",rc));
	}
    return rc;
}

ClRcT
clCmBladeOperationRequest (ClUint32T chassisId, ClUint32T slotId, ClCmFruOperationT request)
{
	ClRcT rc = CL_OK;
    ClCmOpReqT bladeOpReq;
	ClBufferHandleT inputMsg;

    if(request < CL_CM_POWERON_REQUEST || request > CL_CM_RESET_DEASSERT_REQUEST )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nInvalid Request passed "));
		return CL_CM_ERROR(CL_ERR_INVALID_PARAMETER);
    }
	memset((void*)&bladeOpReq, 0 , sizeof(bladeOpReq));
	CL_CM_VERSION_SET(bladeOpReq.version); 

    bladeOpReq.chassisId = chassisId;
    bladeOpReq.physicalSlot = slotId;
	bladeOpReq.request =  request;

    CL_CM_CLIENT_DEBUG_PRINT(("chassisId=%d,slotId=%d,request=%d",chassisId,slotId,request));

	/* form the message with api request */ 
	if((rc = clBufferCreate( &inputMsg ))!= CL_OK )
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nMessage Creation failed rc:0x%x",rc));
		return rc;
	}


	/* copy the blade req info to the Message */ 
	rc=clBufferNBytesWrite( 
			inputMsg , 
			(ClUint8T *)&bladeOpReq,
			(ClUint32T)sizeof(bladeOpReq));
    if(rc != CL_OK )
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nclBufferNBytesWrite Failed rc:0x%x",rc));
		return rc;
	}


	if((rc = clCmRmdWithMsg( 
					CL_CHASSIS_MANAGER_OPERATION_REQUEST,
					inputMsg,
					(ClBufferHandleT)NULL,
					(ClUint32T)CL_RMD_CALL_ATMOST_ONCE,
					NULL
					))!= CL_OK )
   {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nclCmRmdWithMsg Call in to CM failed with rc:0x%x",rc));
		clBufferDelete ( &inputMsg );
		return rc;
	}

	clBufferDelete ( &inputMsg );
	return rc;
}



static ClRcT clCmServerAddressGet(ClIocNodeAddressT *pIocAddress)
{

	ClRcT rc=CL_OK;
	/*FIXME: Need to use the name service apis to resolve the CM address for
	 * now assuming that the cm will be colocated with Masters*/
	rc=clCpmMasterAddressGet(pIocAddress); 
	if(rc != CL_OK )
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nclCpmMasterAddressGet failed rc:0x%x",rc));
	}
	return rc ;
}

ClRcT clCmCpmResponseHandle(ClCpmCmMsgT * pClCpmCmMsg)
{
    ClRcT rc = CL_OK;
	ClBufferHandleT inputMsg;
    ClRmdAsyncOptionsT  rmdAsyncOptions;
	
	if(NULL == pClCpmCmMsg )
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NULL POINTER"));
		return CL_CM_ERROR(CL_ERR_NULL_POINTER);
	}

	if((rc = clBufferCreate( &inputMsg ))!= CL_OK ){
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nMessage Creation failed rc:0x%x",rc));
		return rc;
	}

	/* copy the pClCpmCmMsg to the Message */ 
	rc = clBufferNBytesWrite(inputMsg ,
                                    (ClUint8T *)pClCpmCmMsg,
                                    (ClUint32T)sizeof(ClCpmCmMsgT));
    if(CL_OK !=rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clBufferNBytesWrite Failed \
                                         rc:%x",rc));
		return rc;
    }

    rmdAsyncOptions.pCookie=NULL;
    rmdAsyncOptions.fpCallback=NULL;
    
    rc = clCmRmdWithMsg( /*clCmServerCpmResponseHandle */   
                      CL_CHASSIS_MANAGER_CPM_RESPONSE_HANDLE,
					  inputMsg,
					  (ClBufferHandleT)NULL,
					  (ClUint32T)(CL_RMD_CALL_ASYNC|CL_RMD_CALL_ATMOST_ONCE),
                      &rmdAsyncOptions);

	if(rc != CL_OK)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nclCmRmdWithMsg Call in to CM failed with rc:0x%x",rc));
	}

	clBufferDelete ( &inputMsg );
	return rc;
}

/**
 *  Determine the supported cm Version.  
 *
 *  This Api checks the version is supported or not. If the version check fails,
 *	the closest version is passed as the out parameter.
 *                                                                        
 *  @param version[IN/OUT] : version value.
 *
 *  @returns CL_OK  - Success<br>
 *    On failure, the error CL_ERR_VERSION_MISMATCH is returned with the 
 * 	  closest value of the version supported filled in the version as out parameter.
 */
ClRcT 
clCmVersionVerify(ClVersionT *version)
{
    ClRcT rc = CL_OK;
	if(version == NULL)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed as version \
					address.with rc: 0x%x", rc));
		return CL_ERR_NULL_POINTER;
	}
    CL_FUNC_ENTER();
    
	if((rc = clVersionVerify(&gCmClientToServerVersionDb, version)))
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("The CM version is not supported. 0x%x", rc));
		return CL_ERR_VERSION_MISMATCH;
	}
    CL_FUNC_EXIT();
    return rc;
}

ClRcT
clCmThresholdStateGet(ClUint32T slot, ClCmThresholdLevelT *pLevel, ClBoolT *pAsserted)
{
	ClRcT rc = CL_OK;
    ClCmThresholdStateReqT stateReq = {{0}};
    SaHpiEventStateT eventState = 0;
    ClBufferHandleT inMsg = 0;
    ClBufferHandleT outMsg = 0;

    if(!slot || (!pLevel && !pAsserted))
    {
        return CL_CM_ERROR(CL_ERR_INVALID_PARAMETER);
    }

	CL_CM_VERSION_SET(stateReq.version);
    stateReq.slot = slot;
    stateReq.sensorNum = 0;

    rc = clBufferCreate(&inMsg);
    rc |= clBufferCreate(&outMsg);

    if(rc != CL_OK)
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nMessage Creation failed rc:0x%x",rc));
        goto out_delete;
    }
    
    rc =  clXdrMarshallClVersionT((void*)&stateReq.version, inMsg, 0);
    rc |= clXdrMarshallClUint32T((void*)&stateReq.slot, inMsg, 0);
    rc |= clXdrMarshallClUint32T((void*)&stateReq.sensorNum, inMsg, 0);
    
    if(rc != CL_OK )
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Xdr marshall of threshold request failed with [%#x]", rc));
        goto out_delete;
	}

	if((rc = clCmRmdWithMsg( 
                    CL_CHASSIS_MANAGER_THRESHOLD_STATE_GET,
					inMsg,
					outMsg,
                    CL_RMD_CALL_ATMOST_ONCE | CL_RMD_CALL_NEED_REPLY,
					NULL
					))!= CL_OK )
   {
       if(CL_GET_ERROR_CODE(rc) != CL_IOC_ERR_COMP_UNREACHABLE)
       {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nclCmRmdWithMsg Call in to CM failed with rc:0x%x",rc));
       }
       goto out_delete;
	}

    rc = clXdrUnmarshallClUint16T(outMsg, (void*)&eventState);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Umarshall of event state failed with [%#x]", rc));
        goto out_delete;
    }

    if(pLevel)
        *pLevel = (ClCmThresholdLevelT)eventState;
    
    if(pAsserted)
    {
        *pAsserted = CL_FALSE;
        if((eventState & (CL_CM_THRESHOLD_LOWER_MAJOR | CL_CM_THRESHOLD_LOWER_CRIT | \
                          CL_CM_THRESHOLD_UPPER_MAJOR | CL_CM_THRESHOLD_UPPER_CRIT)))
           *pAsserted = CL_TRUE;
    }

    out_delete:
    if(inMsg) clBufferDelete(&inMsg);
    if(outMsg) clBufferDelete(&outMsg);

	return rc;
}
