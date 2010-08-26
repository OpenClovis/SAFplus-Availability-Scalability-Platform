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
 * File        : clCorRouteApis.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *  This module contains Cor-Txn Route related APIs.
 *****************************************************************************/

#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>

/** Internal Headers here.**/
#include <clCorRMDWrap.h>
#include <clCorClient.h>

#include <xdrCorRouteApiInfo_t.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/**
 *  Add a service entry.
 *
 *  API to set a service entry for a given service id. 
 *
 *  This entry sets a service entry for the given service id.
 *
 *  @param moh      ClCorMOId handle
 *  @param comm     contains full MSP address and associated properties.
 *
 *  @returns 
 *    CL_OK on success <br/>

 */
ClRcT 
clCorServiceAdd(ClCorServiceIdT id, char *mspName,  ClCorCommInfoPtrT comm)
{
    

	/* This API IS NOT Supported */
	return CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);
#if 0
    ClRcT          rc = CL_OK;
    ClUint32T      mspNameLen =0; 
    corRouteApiInfo_t  param;
    ClUint32T      size =  sizeof(param);

    CL_FUNC_ENTER();
    if ((id <= CL_COR_INVALID_SRVC_ID) || (id > CL_COR_SVC_ID_MAX))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "invalid input parameter") );
    	CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_MSP_ID);
    }

    if ((comm == NULL) ||
        (mspName == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "invalid input parameter") );
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    mspNameLen = strlen(mspName);

    if (mspNameLen >= COR_MSP_NAME_MAX)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "invalid input parameter") );
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }
    memset(&param, '\0', sizeof(corRouteApiInfo_t));

	CL_COR_VERSION_SET(param.version);
    param.reqType = COR_SVC_ADD;
    param.commInfo = *comm;
    strcpy(param.mspName, mspName);
    param.mspNameLen = mspNameLen;
    param.svcId = id;

    COR_CALL_RMD(COR_EO_ROUTE_OP, clXdrMarshallcorRouteApiInfo_t, &param, size, 
								clXdrUnmarshallcorRouteApiInfo_t, &param, &size, rc); 
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorServiceAdd failed  Svc:%x name:%s addr:%04x => RET [%04x]", 
                   id, (NULL == mspName)?"":mspName, comm->addr.portId, rc));
        return rc;
    }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorServiceAdd Svc:%x name:%s addr:%04x => RET [%04x]", 
                   id, (NULL == mspName)?"":mspName, comm->addr.portId, rc));
  

    CL_FUNC_EXIT();
    return (rc);
#endif
}

/**
 *  Add a route rule.
 *
 *  API to create a new rule. 
 *
 *  This rule table overrides all the entries and other routing
 *  information present in the COR.
 *
 *  @param moh      ClCorMOId handle
 *  @param addr     CORAddr (contains card id and EO id)
 *
 *  @returns 
 *    CL_OK on success <br/>

 */
ClRcT 
clCorServiceRuleAdd(ClCorMOIdPtrT moh, ClCorAddrT addr)
{
    ClRcT          rc = CL_OK;
    corRouteApiInfo_t  param;
    ClUint32T      size =  sizeof(param);
    ClNameT        name = {0};
    
    CL_FUNC_ENTER();

    if( NULL == moh )
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memset(&param, '\0', sizeof(corRouteApiInfo_t));

    rc = clCorMoIdValidate(moh);
    if(CL_OK != rc)
    {
        clLogError("COR", "RAD", "Failed while validating the MoId passed. rc[0x%x]", rc);
        return rc;
    }

	CL_COR_VERSION_SET(param.version);
    param.reqType = COR_SVC_RULE_STATION_ENABLE;
    param.moId = *moh;
    param.addr = addr;
    param.status = CL_COR_STATION_ENABLE;

    COR_CALL_RMD(COR_EO_ROUTE_OP,
                 VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 size, 
                 NULL,
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
    {
        (void *)memset(&name, 0, sizeof(name));
        clCorMoIdToMoIdNameGet(moh, &name);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorServiceRuleAdd failed: moh - %s, addr [0x%x:0x%x] => RET [%04x]", 
                            name.value,
                            addr.nodeAddress,
                            addr.portId,
                            rc));
        return rc;
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorServiceRuleAdd addr:%04x => RET [%04x]", 
                          addr.nodeAddress,
                          rc));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Get the component address for a given moId.
 *
 *  This api the first non zero component address 
 *  for a given MoId from the route list. 
 *
 *  @param moh      ClCorMOId handle
 *  @param addr     Address of CORAddrT (contains card id and EO id)
 *
 *  @returns 
 *    CL_OK on success <br/>
 */
ClRcT clCorMoIdToComponentAddressGet(ClCorMOIdPtrT moh, ClCorAddrT* addr)
{
    ClRcT          rc = CL_OK;
    corRouteApiInfo_t  param;
    ClUint32T      size =  sizeof(param);
    ClNameT        name = {0};
    
    CL_FUNC_ENTER();

    if( NULL == moh || addr == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memset(&param, '\0', sizeof(corRouteApiInfo_t));

    rc = clCorMoIdValidate(moh);
    if(CL_OK != rc)
    {
        clLogError("COR", "CAG", "Failed while validating the MoId passed. rc[0x%x]", rc);
        return rc;
    }

	CL_COR_VERSION_SET(param.version);
    param.reqType = COR_SVC_RULE_STATION_ADDR_GET;
    param.moId = *moh;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_ROUTE_OP,
                                     VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0),
                                     &param,
                                     size, 
                                     VDECL_VER(clXdrUnmarshallcorRouteApiInfo_t, 4, 0, 0),
                                     &param,
                                     &size,
                                     rc);
    if (rc != CL_OK)
    {
        (void *)memset(&name, 0, sizeof(name));
        clCorMoIdToMoIdNameGet(moh, &name);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMoIdToComponentAddressGet failed for moh - %s. rc[0x%x]", name.value, rc));
        return rc;
    }

    *addr = param.addr;
    
    CL_FUNC_EXIT();
    return (rc);


}

/**
 *  Disable a route rule.
 *
 *  This Api should be used when a component doesn't want 
 *  to participate in the transaction. 
 *
 *
 *  @param moh      ClCorMOId handle
 *  @param addr     CORAddr (contains card id and EO id)
 *
 *  @returns 
 *    CL_OK on success <br/>
 */
ClRcT clCorServiceRuleDisable(ClCorMOIdPtrT moh, ClCorAddrT addr)
{

    ClRcT   rc = CL_OK;
    corRouteApiInfo_t  param;
    ClUint32T      size =  sizeof(param);
    ClNameT        name = {0};
    
    CL_FUNC_ENTER();

    if( NULL == moh )
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memset(&param, '\0', sizeof(corRouteApiInfo_t));

    rc = clCorMoIdValidate(moh);
    if(CL_OK != rc)
    {
        clLogError("COR", "RDS", "Failed while validating the MoId passed. rc[0x%x]", rc);
        return rc;
    }


	CL_COR_VERSION_SET(param.version);
    param.reqType = COR_SVC_RULE_STATION_DISABLE;
    param.moId = *moh;
    param.addr = addr;
    param.status = CL_COR_STATION_DISABLE;
    param.primaryOI = CL_COR_PRIMARY_OI_DISABLED;

    COR_CALL_RMD(COR_EO_ROUTE_OP,
                 VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 size,
                 NULL,
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
    {
        (void *)memset(&name, 0, sizeof(name));
        clCorMoIdToMoIdNameGet(moh, &name);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorServiceRuleDisable failed: moh - %s, addr [0x%x:0x%x] => RET [%04x]", 
                            name.value,
                            addr.nodeAddress,
                            addr.portId,
                            rc));
        return rc;
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorServiceRuleDisable Succeeded addr:[%x:0x%x] => RET [%04x]", 
                          addr.nodeAddress,
                          addr.portId,
                          rc));
    CL_FUNC_EXIT();
    return (rc);
}


/**
 *  Delete all the station address from the route list a given service Id.
 *
 *  API to delete the station address from the route list for a given service Id.
 *  This Api should be used by the component in its EO finalize callback. This
 *  is basically a cleanup api, which will clean the staion entry from
 *  the route list when the compoent goes down. 
 *
 *  @param moh      ClCorMOId handle
 *  @param addr     CORAddr (contains card id and EO id)
 *
 *  @returns 
 *    CL_OK on success <br/>
 */

ClRcT clCorServiceRuleDeleteAll(ClCorServiceIdT svcId, ClCorAddrT addr)
{

    ClRcT   rc = CL_OK;
    corRouteApiInfo_t  param;
    ClUint32T      size =  sizeof(param);
    
    CL_FUNC_ENTER();

    if( clCorServiceIdValidate(svcId) != CL_OK  )
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    memset(&param, '\0', sizeof(corRouteApiInfo_t));

	CL_COR_VERSION_SET(param.version);
    param.reqType = COR_SVC_RULE_STATION_DELETE_ALL;
    param.srvcId  = svcId;
    param.addr = addr;
    param.status = CL_COR_STATION_DELETE;
    param.primaryOI = CL_COR_PRIMARY_OI_DISABLED;

    COR_CALL_RMD(COR_EO_ROUTE_OP,
                 VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 size,
                 NULL,
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorServiceRuleDeleteAll failed: addr [0x%x:0x%x] => RET [%04x]", 
                            addr.nodeAddress,
                            addr.portId,
                            rc));
        return rc;
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorServiceRuleDeleteAll Succeeded addr:[0x%x:0x%x] => RET [%04x]", 
                          addr.nodeAddress,
                          addr.portId,
                          rc));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Delete the station address from the route list a given moId.
 *
 *  API to delete the station address from the route list. This Api 
 *  should be used by the component in its EO finalize callback. This
 *  is basically a cleanup api, which will clean the staion entry from
 *  the route list when the compoent goes down. 
 *
 *  @param moh      ClCorMOId handle
 *  @param addr     CORAddr (contains card id and EO id)
 *
 *  @returns 
 *    CL_OK on success <br/>
 */

ClRcT clCorServiceRuleDelete(ClCorMOIdPtrT moh, ClCorAddrT addr)
{

    ClRcT   rc = CL_OK;
    corRouteApiInfo_t  param;
    ClUint32T      size =  sizeof(param);
    ClNameT        name = {0};
    
    CL_FUNC_ENTER();

    if( NULL == moh )
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memset(&param, '\0', sizeof(corRouteApiInfo_t));

    rc = clCorMoIdValidate(moh);
    if(CL_OK != rc)
    {
        clLogError("COR", "RDL", "Failed while validating the MoId passed. rc[0x%x]", rc);
        return rc;
    }


	CL_COR_VERSION_SET(param.version);
    param.reqType = COR_SVC_RULE_STATION_DELETE;
    param.moId = *moh;
    param.addr = addr;
    param.status = CL_COR_STATION_DELETE;
    param.primaryOI = CL_COR_PRIMARY_OI_DISABLED;

    COR_CALL_RMD(COR_EO_ROUTE_OP,
                 VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 size,
                 NULL,
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
    {
        (void *)memset(&name, 0, sizeof(name));
        clCorMoIdToMoIdNameGet(moh, &name);
        if(CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT) == rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_WARN, ( "clCorServiceRuleDelete failed: moh - %s, addr [0x%x:0x%x] => RET [%04x]", 
                            name.value,
                            addr.nodeAddress,
                            addr.portId,
                            rc));
            return CL_OK;

        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorServiceRuleDelete failed: moh - %s, addr [0x%x:0x%x] => RET [%04x]", 
                            name.value,
                            addr.nodeAddress,
                            addr.portId,
                            rc));
            return rc;
        }
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorServiceRuleDelete Succeeded addr:[0x%x:0x%x] => RET [%04x]", 
                          addr.nodeAddress,
                          addr.portId,
                          rc));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Api to get the station Status.
 * 
 *  This function get the status of the station given a moId and
 *  the station address.
 * 
 *  @param moh      ClCorMOId handle
 *  @param addr     CORAddr (contains card id and EO id)
 *
 *  @returns 
 *    CL_OK on success <br/>
 */

ClRcT clCorServiceRuleStatusGet(ClCorMOIdPtrT moh, ClCorAddrT addr, ClInt8T *status)
{

    ClRcT   rc = CL_OK;
    corRouteApiInfo_t  param;
    ClUint32T      size =  sizeof(param);
    ClNameT        name = {0};
    
    CL_FUNC_ENTER();

    if( NULL == moh || status == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memset(&param, '\0', sizeof(corRouteApiInfo_t));

    rc = clCorMoIdValidate(moh);
    if(CL_OK != rc)
    {
        clLogError("COR", "RSG", "Failed while validating the MoId passed. rc[0x%x]", rc);
        return rc;
    }

	CL_COR_VERSION_SET(param.version);
    param.reqType = COR_SVC_RULE_STATION_STATUS_GET;
    param.moId = *moh;
    param.addr = addr;

    COR_CALL_RMD(COR_EO_ROUTE_OP,
                 VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 size, 
                 VDECL_VER(clXdrUnmarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
    {
        (void *)memset(&name, 0, sizeof(name));
        clCorMoIdToMoIdNameGet(moh, &name);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorServiceRuleStatusGet failed: moh - %s, addr [0x%x:0x%x] => RET [%04x]", 
                            name.value,
                            addr.nodeAddress,
                            addr.portId,
                            rc));
        *status = CL_COR_STATION_INVALID;
        return rc;
    }

    *status = param.status;
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorServiceRuleStatusGet Succeeded addr:[0x%x:0x%x] => RET [%04x]", 
                          addr.nodeAddress,
                          addr.portId,
                          rc));
    CL_FUNC_EXIT();
    return (rc);
}

static ClRcT 
corPrimaryOISet( CL_IN const ClCorMOIdPtrT pMoId, 
                 CL_IN const ClCorAddrPtrT pCompAddr)
{
    corRouteApiInfo_t  param;
    ClUint32T      size =  sizeof(param);
    ClNameT        name = {0};
    ClRcT rc = CL_OK;

    memset(&param, 0, sizeof(param));
	CL_COR_VERSION_SET(param.version);
    param.reqType = COR_PRIMARY_OI_SET;
    param.moId = *pMoId;
    param.addr = *pCompAddr;
    param.primaryOI = CL_COR_PRIMARY_OI_ENABLED;

    COR_CALL_RMD(COR_EO_ROUTE_OP,
                 VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 size,
                 NULL,
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
    {
        if(CL_ERR_TIMEOUT == CL_GET_ERROR_CODE(rc))
            rc = CL_COR_SET_RC(CL_COR_ERR_TIMEOUT);
        (void *)memset(&name, 0, sizeof(name));
        clCorMoIdToMoIdNameGet(pMoId, &name);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Primary OI set failed: pMoId - %s [svc : %d], addr [0x%x:0x%x] => RET [%04x]", 
                            name.value,
                            pMoId->svcId,
                            pCompAddr->nodeAddress,
                            pCompAddr->portId,
                            rc));
        return rc;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorReadOIRegister Succeeded addr:[0x%x:0x%x] => RET [%04x]", 
                          pCompAddr->nodeAddress,
                          pCompAddr->portId,
                          rc));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Function to set the OI as a primary OI.
 */
ClRcT 
clCorPrimaryOISet( CL_IN const ClCorMOIdPtrT pMoId, 
                   CL_IN const ClCorAddrPtrT pCompAddr)
{
    ClRcT      rc = CL_OK;
    
    CL_FUNC_ENTER();

    if(( NULL == pMoId ) || (NULL == pCompAddr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = clCorMoIdValidate(pMoId);
    if(CL_OK != rc)
    {
        clLogError("COR", "POS", "Failed while validating the MoId passed. rc[0x%x]", rc);
        return rc;
    }

    return corPrimaryOISet(pMoId, pCompAddr);
}

ClRcT clCorNIPrimaryOISet(const ClCharT *pResource)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moid;
    ClNameT moidName = {0};
    ClCorAddrT compAddress;
    if(!pResource) return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);

    clNameSet(&moidName, pResource);
    rc = clCorMoIdNameToMoIdGet(&moidName, &moid);
    if(rc != CL_OK)
    {
        clLogError("PRI", "OI-SET", "Moid not found for name [%s]",
                   pResource);
        return rc;
    }
    rc = clCorMoIdServiceSet(&moid, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
    if(rc != CL_OK)
        return rc;

    compAddress.nodeAddress = clIocLocalAddressGet();
    clEoMyEoIocPortGet(&compAddress.portId);
    return corPrimaryOISet(&moid, &compAddress);
}

static ClRcT 
corPrimaryOIClear( CL_IN const ClCorMOIdPtrT pMoId, 
                   CL_IN const ClCorAddrPtrT pCompAddr)
{
    ClRcT      rc = CL_OK;
    corRouteApiInfo_t  param;
    ClUint32T      size =  sizeof(param);
    ClNameT        name = {0};
    
    CL_FUNC_ENTER();

    memset(&param, 0, sizeof(corRouteApiInfo_t));
    
	CL_COR_VERSION_SET(param.version);
    param.reqType = COR_PRIMARY_OI_UNSET;
    param.moId = *pMoId;
    param.addr = *pCompAddr;
    param.primaryOI = CL_COR_PRIMARY_OI_DISABLED;

    COR_CALL_RMD(COR_EO_ROUTE_OP,
                 VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 size,
                 NULL,
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
    {
        if(CL_ERR_TIMEOUT == CL_GET_ERROR_CODE(rc))
            rc = CL_COR_SET_RC(CL_COR_ERR_TIMEOUT);
        (void *)memset(&name, 0, sizeof(name));
        clCorMoIdToMoIdNameGet(pMoId, &name);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorReadOIUnRegister failed: pMoId - %s, addr [0x%x:0x%x] => RET [%04x]", 
                            name.value,
                            pCompAddr->nodeAddress,
                            pCompAddr->portId,
                            rc));
        return rc;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorReadOIUnRegister Succeeded addr:[0x%x:0x%x] => RET [%04x]", 
                          pCompAddr->nodeAddress,
                          pCompAddr->portId,
                          rc));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Function to unset the OI as a primary Oi.
 */
ClRcT 
clCorPrimaryOIClear( CL_IN const ClCorMOIdPtrT pMoId, 
                     CL_IN const ClCorAddrPtrT pCompAddr)
{
    ClRcT      rc = CL_OK;
    
    CL_FUNC_ENTER();

    if((NULL == pMoId) || (NULL == pCompAddr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL pointer passed.") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = clCorMoIdValidate(pMoId);
    if(CL_OK != rc)
    {
        clLogError("COR", "POC", "Failed while validating the MoId passed. rc[0x%x]", rc);
        return rc;
    }

    return corPrimaryOIClear(pMoId, pCompAddr);
}

ClRcT clCorNIPrimaryOIClear(const ClCharT *pResource)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moid;
    ClNameT moidName = {0};
    ClCorAddrT compAddress;
    if(!pResource) return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);

    clNameSet(&moidName, pResource);
    rc = clCorMoIdNameToMoIdGet(&moidName, &moid);
    if(rc != CL_OK)
    {
        clLogError("PRI", "OI-SET", "Moid not found for name [%s]",
                   pResource);
        return rc;
    }
    rc = clCorMoIdServiceSet(&moid, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
    if(rc != CL_OK)
        return rc;

    compAddress.nodeAddress = clIocLocalAddressGet();
    clEoMyEoIocPortGet(&compAddress.portId);
    return corPrimaryOIClear(&moid, &compAddress);
}

/**
 * Function to get the primary OI from the OI list.
 */

ClRcT 
clCorPrimaryOIGet (  CL_IN const ClCorMOIdPtrT pMoId, 
                    CL_OUT ClCorAddrPtrT pCompAddr)
{
    ClRcT      rc = CL_OK;
    corRouteApiInfo_t  param;
    ClUint32T      size =  sizeof(param);
    ClNameT        name = {0};
    
    CL_FUNC_ENTER();

    if( (NULL == pMoId) || (NULL == pCompAddr) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    memset(&param, '\0', sizeof(corRouteApiInfo_t));

    rc = clCorMoIdValidate(pMoId);
    if(CL_OK != rc)
    {
        clLogError("COR", "POG", "Failed while validating the MoId passed. rc[0x%x]", rc);
        return rc;
    }

	CL_COR_VERSION_SET(param.version);
    param.reqType = COR_PRIMARY_OI_GET;
    param.moId = *pMoId;

    COR_CALL_RMD(COR_EO_ROUTE_OP,
                 VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 size, 
                 VDECL_VER(clXdrUnmarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 &size,
                 rc);

    if (rc != CL_OK)
    {
        if(CL_ERR_TIMEOUT == CL_GET_ERROR_CODE(rc))
            rc = CL_COR_SET_RC(CL_COR_ERR_TIMEOUT);
        (void *)memset(&name, 0, sizeof(name));
        clCorMoIdToMoIdNameGet(pMoId, &name);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorReadOIGet failed: pMoId - %s, pCompAddr [0x%x:0x%x] => RET [%04x]", 
                            name.value,
                            pCompAddr->nodeAddress,
                            pCompAddr->portId,
                            rc));
        return rc;
    }

    *pCompAddr = param.addr;
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorReadOIGet Succeeded pCompAddr:[0x%x:0x%x] => RET [%04x]", 
                          pCompAddr->nodeAddress,
                          pCompAddr->portId,
                          rc));
    CL_FUNC_EXIT();
    return (rc);
}


/**
 * Adding the OI in the OI list.
 */

ClRcT 
clCorOIRegister( CL_IN const ClCorMOIdPtrT pMoId, 
                 CL_IN const ClCorAddrPtrT pCompAddr)
{
    ClRcT   rc = CL_OK;

    if((NULL == pMoId) || (NULL == pCompAddr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed. "));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /* Registring the OI. */
    rc = clCorServiceRuleAdd(pMoId, *pCompAddr);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to add the OI in the OI list. rc[0x%x]", rc));
        return rc;
    }

    return rc;
}

/**
 * Adding the OI in the OI list and also disable.
 * This is required by the alarm client to register itself as alarm owner with COR.
 */
ClRcT
clCorOIRegisterAndDisable(ClCorMOIdPtrT pMoId, ClCorAddrT addr)
{
    ClRcT               rc = CL_OK;
    corRouteApiInfo_t   param;
    ClUint32T           size = sizeof(param);
    ClNameT             moIdName;

    CL_FUNC_ENTER();

    if (NULL == pMoId)
    {
        clLogError("COR", "RAD", "MoId passed is NULL.");
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    memset(&param, 0, sizeof(corRouteApiInfo_t));

    rc = clCorMoIdValidate(pMoId);
    if (CL_OK != rc)
    {
        clLogError("COR", "RAD", "Failed while validating the MoId passed. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_COR_VERSION_SET(param.version);
    param.reqType = COR_SVC_RULE_STATION_ENABLE;
    param.moId = *pMoId;
    param.addr = addr;
    param.status = CL_COR_STATION_DISABLE;

    COR_CALL_RMD(COR_EO_ROUTE_OP,
                 VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0),
                 &param,
                 size,
                 NULL,
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
    {
        clCorMoIdToMoIdNameGet(pMoId, &moIdName);

        clLogError("COR", "RAD", "Failed to register and disable the OI [0x%x:0x%x] for MoId [%s]. rc [0x%x]",
                addr.nodeAddress, addr.portId, moIdName.value, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 * Adding the OI in the OI list.
 */

ClRcT 
clCorOIUnregister ( CL_IN const ClCorMOIdPtrT pMoId, 
                    CL_IN const ClCorAddrPtrT pCompAddr)
{
    ClRcT   rc = CL_OK;

    if((NULL == pMoId) || (NULL == pCompAddr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed. "));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /* Funtion to do un-registration of OI.*/
    rc = clCorServiceRuleDelete(pMoId, *pCompAddr);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to un-register the OI. rc[0x%x]", rc));
        return rc;
    }

    return rc;
}
