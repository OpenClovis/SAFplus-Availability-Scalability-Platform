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

/**
 *  This file implements CPM client side functionality related to COR.
 */
 
/*
 * Standard header files 
 */
#include <string.h>

/*
 * ASP header files 
 */
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clCorUtilityApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clXdrApi.h>

/*
 * CPM internal header files 
 */
#include <clCpmClient.h>
#include <clCpmCommon.h>
#include <clCpmLog.h>
#include <clCpmExtApi.h>

/*
 * XDR header files 
 */
#include "xdrClCpmSlotInfoRecvT.h"
#include "xdrClCpmSlotInfoFieldIdT.h"

ClRcT clCpmSlotInfoGet(ClCpmSlotInfoFieldIdT flag, ClCpmSlotInfoT *slotInfo)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT masterIocAddress = 0;
    ClCpmSlotInfoRecvT  slotInfoRecv = {0};
    ClUint32T           bufSize = 0;
    
    /* make a SYNC RMD to CPM/G master */
    
    rc = clCpmMasterAddressGet(&masterIocAddress);
    if (rc != CL_OK)
        goto failure;

    slotInfoRecv.flag = flag;
    switch(slotInfoRecv.flag)
    {
        case CL_CPM_SLOT_ID:
        {
            slotInfoRecv.slotId = slotInfo->slotId;
            break;
        }
        case CL_CPM_IOC_ADDRESS:
        {
            slotInfoRecv.nodeIocAddress = slotInfo->nodeIocAddress;
            break;
        }
#ifdef USE_COR        
        case CL_CPM_NODE_MOID:
        {
            slotInfoRecv.nodeMoIdStr.length = 0;
            slotInfoRecv.nodeMoIdStr.value[0] = 0;                
            rc = clCorMoIdToMoIdNameGet(&slotInfo->nodeMoId, &slotInfoRecv.nodeMoIdStr);
            CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,("MoIdToMoIdNameGet Failed, rc=[0x%x]\n", rc), rc);
            break;
        }
#endif        
        case CL_CPM_NODENAME:
        {
            memcpy(&slotInfoRecv.nodeName, &slotInfo->nodeName, sizeof(SaNameT));
            break;
        }
        default:
        {
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            clLogError(CPM_LOG_AREA_CPM,CL_LOG_CONTEXT_UNSPECIFIED,
                       "Invalid flag passed, rc=[0x%x]\n", rc);
            goto failure;
            break;
        }
    }

    rc = clCpmClientRMDSyncNew(masterIocAddress, CPM_SLOT_INFO_GET,
                               (ClUint8T *) &slotInfoRecv, sizeof(ClUint32T),
                               (ClUint8T *) &slotInfoRecv, &bufSize,
                               CL_RMD_CALL_NEED_REPLY, 0, 0, 0,
                               MARSHALL_FN(ClCpmSlotInfoRecvT, 4, 0, 0),
                               UNMARSHALL_FN(ClCpmSlotInfoRecvT, 4, 0, 0));
    
    CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR, ("Unable to find information about given entity, rc=[0x%x]\n", rc), rc);

    switch(slotInfoRecv.flag)
    {
        case CL_CPM_SLOT_ID:
        {
            slotInfo->nodeIocAddress = slotInfoRecv.nodeIocAddress;
#ifdef USE_COR        
            rc = clCorMoIdNameToMoIdGet(&slotInfoRecv.nodeMoIdStr, &slotInfo->nodeMoId);
            CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                    ("MoIdNameToMoIdGet Failed, rc=[0x%x]\n", rc), rc);
#endif            
            memcpy(&slotInfo->nodeName, &slotInfoRecv.nodeName, sizeof(SaNameT));

            break;
        }
        case CL_CPM_IOC_ADDRESS:
        {
            slotInfo->slotId = slotInfoRecv.slotId;
#ifdef USE_COR  
            rc = clCorMoIdNameToMoIdGet(&slotInfoRecv.nodeMoIdStr, &slotInfo->nodeMoId);
            CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                    ("MoIdNameToMoIdGet Failed, rc=[0x%x]\n", rc), rc);
#endif
            memcpy(&slotInfo->nodeName, &slotInfoRecv.nodeName, sizeof(SaNameT));

            break;
        }
        case CL_CPM_NODE_MOID:
        {
            slotInfo->slotId = slotInfoRecv.slotId;
            
            slotInfo->nodeIocAddress = slotInfoRecv.nodeIocAddress;
            
            memcpy(&slotInfo->nodeName, &slotInfoRecv.nodeName, sizeof(SaNameT));

            break;
        }
        case CL_CPM_NODENAME:
        {
            slotInfo->slotId = slotInfoRecv.slotId;

            slotInfo->nodeIocAddress = slotInfoRecv.nodeIocAddress;
#ifdef USE_COR 
            rc = clCorMoIdNameToMoIdGet(&slotInfoRecv.nodeMoIdStr, &slotInfo->nodeMoId);
            CPM_CLIENT_CHECK(CL_LOG_SEV_ERROR,
                    ("MoIdNameToMoIdGet Failed, rc=[0x%x]\n", rc), rc);
#endif
            break;
        }
        default:
        {
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            clLogError(CPM_LOG_AREA_CPM,CL_LOG_CONTEXT_UNSPECIFIED,
                       "Invalid flag passed, rc=[0x%x]\n", rc);
            goto failure;
            break;
        }
    }

    return rc;

failure:
    return rc;
}
