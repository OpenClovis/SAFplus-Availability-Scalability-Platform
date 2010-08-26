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
#include <string.h>
#include <clCpmApi.h>
#include <clLogErrors.h>
#include <clLogCommon.h>
#include <clLogDebug.h>
#include <clLogClientCommon.h>

/* - clLogClntCompInfoGet
 * - Gets the component name and id
 */
ClRcT
clLogClntCompInfoGet(ClLogCompInfoT  *pCompInfo)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pCompInfo), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clCpmComponentNameGet(0, &pCompInfo->compName);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clCpmComponentNameGet(): rc[0x %x]", rc));
        return rc;
    }

    clEoMyEoIocPortGet(&pCompInfo->compId);

    CL_LOG_DEBUG_TRACE(("Exit: %.*s %u", pCompInfo->compName.length,
                        pCompInfo->compName.value, pCompInfo->compId));
    return rc;
}


/* - clLogClntIdlHandleInitialize
 * - Initializes the IDL handle for use in calls to the server
 */
ClRcT
clLogClntIdlHandleInitialize(ClIocAddressT  destAddr,
                             ClIdlHandleT   *phLogIdl)
{
    ClRcT            rc      = CL_OK;
    ClIdlHandleObjT  idlObj  = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlAddressT    address = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    address.addressType        = CL_IDL_ADDRESSTYPE_IOC;
    address.address.iocAddress = destAddr;

    idlObj.address             = address;
    idlObj.flags               = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout     = CL_LOG_CLIENT_DEFULT_TIMEOUT; 
    idlObj.options.priority    = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries     = CL_LOG_CLIENT_DEFAULT_RETRIES; 

    rc = clIdlHandleInitialize(&idlObj, phLogIdl);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clIdlHandleInitialize(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntIdlHandleFinalize(ClIdlHandleT  *phClntIdl)
{
    ClRcT rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == phClntIdl), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clIdlHandleFinalize(*phClntIdl);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clIdlHandleInitialize(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Enter"));
    return rc;
}

ClRcT
clLogClntCompNameChkNCompIdGet(ClNameT    *pCompName,
                               ClUint32T  *pClientId)
{
    ClRcT      rc           = CL_OK;
    ClUint32T  count        = 0;
    ClUint32T  localAddr    = 0;
    ClCharT    *pCompPrefix = NULL;
    ClUint32T  compLen      = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogCompNamePrefixGet(pCompName, &pCompPrefix);
    if( CL_OK != rc )
    {
        return rc;
    }
    compLen = strlen(pCompPrefix);
    for( count = 0; count < nLogAspComps; count++ )
    {
        if( compLen == strlen(aspCompMap[count].pCompName) )
        {
            if( !(memcmp(pCompPrefix, aspCompMap[count].pCompName, compLen)) )
            {
                localAddr = clIocLocalAddressGet();
                localAddr = localAddr << 16;
                localAddr = localAddr | aspCompMap[count].clntId;
            }
        }
    }
    *pClientId = localAddr;

    clHeapFree(pCompPrefix);
    CL_LOG_DEBUG_TRACE(("Exit: %d", *pClientId));
    return rc;
}
