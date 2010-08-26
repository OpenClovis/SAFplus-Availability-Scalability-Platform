
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

#include <fcntl.h>
#include <clCommon.h>

#if OS_VERSION_CODE < OS_KERNEL_VERSION(2,6,16)
#include <tipc.h>
#else
#include <linux/tipc.h>
#endif
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clIocApi.h>
#include <clIocErrors.h>
#include <clTipcMaster.h>
#include <clTipcSetup.h>
#include <clIocConfig.h>
#include <clTipcNeighComps.h>


#define CL_TIPC_MASTER_ADDRESS_RETRIES (10)


extern ClBoolT gIsNodeRepresentative;
static ClIocNodeAddressT *gpClTipcMasterSeg;
static ClOsalSemIdT gClTipcMasterSem;

/*
 * Locate the master through the LA availability.
 */
static ClRcT clIocTryMasterAddressGet(ClIocLogicalAddressT logicalAddress,
        ClIocPortT portId,
        ClIocNodeAddressT *pIocNodeAddress
        )
{
    struct sockaddr_tipc topsrv;
    ClInt32T fd;
    ClInt32T ret;
    struct tipc_event event;
    struct tipc_subscr subscr;
    ClRcT rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);

    fd = socket(AF_TIPC,SOCK_SEQPACKET,0);
    if(fd < 0 )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error creating socket.errno=%d\n",errno));
        goto out;
    }
    ret = fcntl(fd, F_SETFD, FD_CLOEXEC);
    CL_ASSERT(ret == 0);
    memset(&topsrv,0,sizeof(topsrv));
    memset(&event,0,sizeof(event));
    memset(&subscr,0,sizeof(subscr));

    topsrv.family = AF_TIPC;
    topsrv.addrtype = TIPC_ADDR_NAME;
    topsrv.addr.name.name.type = TIPC_TOP_SRV;
    topsrv.addr.name.name.instance = TIPC_TOP_SRV;
    if(connect(fd,(struct sockaddr*)&topsrv,sizeof(topsrv))<0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in connecting to topology server.errno=%d\n",errno));
        goto out_close;
    }

    /*
     *Make a subscription filter:
     *The master is expected to have bound
     *on a master address type.
     */
    subscr.seq.type = CL_IOC_TIPC_MASTER_TYPE(portId);
    subscr.seq.lower = CL_IOC_MIN_NODE_ADDRESS;
    subscr.seq.upper = CL_IOC_MAX_NODE_ADDRESS;
    subscr.timeout = CL_TIPC_TOP_SRV_TIMEOUT;
    subscr.filter = TIPC_SUB_SERVICE;
    if(send(fd, (const char*)&subscr,sizeof(subscr),0) != sizeof(subscr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in send. errno=%d\n",errno));
        goto out_close;
    }
    if(recv(fd, (char*)&event,sizeof(event),0) != sizeof(event))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in recv. errno=%d\n",errno));
        goto out_close;
    }
    if(event.event != TIPC_PUBLISHED)
    {
        goto out_close;
    }
    CL_ASSERT(event.found_lower == event.found_upper);
    *pIocNodeAddress = (ClIocNodeAddressT)event.found_lower;
    rc = CL_OK;

out_close:
    close(fd);
out:
    return rc;
}


/*
 * Locate the master through the LA availability.
 */
ClRcT clIocMasterAddressGetExtended(ClIocLogicalAddressT logicalAddress,
                                    ClIocPortT portId,
                                    ClIocNodeAddressT *pIocNodeAddress,
                                    ClInt32T numRetries,
                                    ClTimerTimeOutT *pDelay)
{
    ClRcT rc = CL_OK;
    ClInt32T retryCnt = CL_TIPC_MASTER_ADDRESS_RETRIES;
    ClTimerTimeOutT delay = {.tsMilliSec = 100, .tsSec = 0 }; 
    ClIocNodeAddressT node = 0;

    if(!gpClTipcMasterSeg)
        return CL_IOC_RC(CL_ERR_NOT_INITIALIZED);

    if(pIocNodeAddress == NULL)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    if(numRetries)
        retryCnt = numRetries;

    if(pDelay)
        delay = *pDelay;

    clOsalSemLock(gClTipcMasterSem);
    node = gpClTipcMasterSeg[portId]; 

    if (node && !CL_IOC_NEIGH_NODE_STATUS_GET(node))
    {
        node = 0;
    }

    clOsalSemUnlock(gClTipcMasterSem);


    if(node == 0)
    {
        do {    
            /*
             * if this call is made during transcient time of switchover/failover,
             * this may fail, so retrying to ensure that master is 
             * not there 
             */
            rc = clIocTryMasterAddressGet(logicalAddress, portId, &node);
            if( CL_OK == rc ) 
            {
                clOsalSemLock(gClTipcMasterSem);
                *pIocNodeAddress = gpClTipcMasterSeg[portId] = node;
                clOsalSemUnlock(gClTipcMasterSem);
                clLogInfo("IOC", "MASTER", "Setting node [%d] as master for comp [%d].", node, portId);
                break;
            }
            CL_DEBUG_PRINT(CL_DEBUG_WARN,("Cannot get IOC master, return code [0x%x]",rc));

            clOsalTaskDelay(delay);
        } while(--retryCnt > 0);
    }
    else
    {
        *pIocNodeAddress = node;
    }

    return rc;
}


ClRcT clIocMasterAddressGet(ClIocLogicalAddressT logicalAddress,
        ClIocPortT portId,
        ClIocNodeAddressT *pIocNodeAddress)
{
    return clIocMasterAddressGetExtended(logicalAddress, portId, pIocNodeAddress, 0, NULL);
}


void clTipcMasterSegmentInitialize(void *pMasgerSegment, ClOsalSemIdT masterSem)
{
    gpClTipcMasterSeg = (ClIocNodeAddressT *)pMasgerSegment;
    gClTipcMasterSem = masterSem;
}

void clTipcMasterSegmentFinalize(void)
{
    gpClTipcMasterSeg = NULL;
}

void clTipcMasterSegmentUpdate(ClIocPhysicalAddressT compAddr)
{
    ClUint32T i;

    if(!gpClTipcMasterSeg) return;

    clOsalSemLock(gClTipcMasterSem);
    if(compAddr.portId == CL_IOC_TIPC_PORT)
    {
        for(i = CL_IOC_MIN_COMP_PORT ; i < CL_IOC_MAX_COMPONENTS_PER_NODE ; i++)
        {
            if(gpClTipcMasterSeg[i] == compAddr.nodeAddress)
            {
                gpClTipcMasterSeg[i] = 0;
                clLogInfo("IOC", "MASTER", "Resetting node info of master for comp [%d].", i);
            } 
        }
    }
    else if(gpClTipcMasterSeg[compAddr.portId] == compAddr.nodeAddress)
    {
        gpClTipcMasterSeg[compAddr.portId] = 0;
        clLogInfo("IOC", "MASTER", "Resetting node info of master for comp [%d].", compAddr.portId);
    }
    clOsalSemUnlock(gClTipcMasterSem);
}
