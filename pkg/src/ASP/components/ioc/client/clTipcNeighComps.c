
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <netinet/in.h>

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
#include <clTipcUserApi.h>
#include <clTipcNeighComps.h>
#include <clTipcSetup.h>
#include <clTipcMaster.h>
#include <clIocConfig.h>

#define CL_TIPC_NEIGH_COMPS_SEGMENT_NAME   "/CL_TIPC_NEIGHBOR_COMPONENTS"

#define CL_TIPC_BYTES_FOR_COMPS_IN_SYSTEM  (CL_TIPC_ALLOC_COMPS_IN_SYSTEM >> 3)
#define CL_TIPC_NEIGHBORS_SEGMENT_SIZE     CL_TIPC_ALIGN(CL_TIPC_BYTES_FOR_COMPS_IN_SYSTEM, 8)

/*
 * Just store the next allocated bit.
 */
#define CL_TIPC_CTRL_SEGMENT_SIZE CL_TIPC_ALIGN(sizeof(ClUint32T), 8)
/* Includes both segments, neighbor-info and master-info. */
#define CL_TIPC_MAIN_SEGMENT_SIZE (CL_TIPC_NEIGHBORS_SEGMENT_SIZE + CL_TIPC_MASTER_SEGMENT_SIZE)
#define CL_TIPC_NEIGH_COMPS_SEGMENT_SIZE   (CL_TIPC_MAIN_SEGMENT_SIZE + CL_TIPC_CTRL_SEGMENT_SIZE)

#define CL_TIPC_CTRL_SEGMENT(seg) ( (ClUint32T*)((ClUint8T*)(seg) + CL_TIPC_MAIN_SEGMENT_SIZE) )
ClUint8T *gpClTipcNeighComps;
static ClCharT gpClTipcNeighCompsSegment[CL_TIPC_SEGMENT_NAME_LENGTH + 1];
static ClFdT gClTipcNeighCompsFd;
static ClUint32T gClTipcNeighCompsInitialize;
static ClOsalSemIdT gClTipcNeighborSem;

ClRcT clTipcNeighCompsSegmentCreate(void)
{
    ClRcT rc = CL_OK;
    ClFdT fd;

    rc = clOsalShmUnlink(gpClTipcNeighCompsSegment);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,("shm unlink failed for segment:%s\n",gpClTipcNeighCompsSegment));
    }

    rc = clOsalShmOpen(gpClTipcNeighCompsSegment,O_RDWR|O_CREAT|O_EXCL,0666,&fd);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in osal shm open.rc=0x%x\n",rc));
        goto out;
    }

    rc = clOsalFtruncate(fd, CL_TIPC_NEIGH_COMPS_SEGMENT_SIZE);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in ftruncate.rc=0x%x\n",rc));
        goto out_unlink;
    }
    
    rc = clOsalMmap(0, CL_TIPC_NEIGH_COMPS_SEGMENT_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0, (ClPtrT *)&gpClTipcNeighComps);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in osal mmap.rc=0x%x\n",rc));
        goto out_unlink;
    }
    
    gClTipcNeighCompsFd = fd;

    rc = clOsalSemCreate((ClUint8T*)gpClTipcNeighCompsSegment, 1, &gClTipcNeighborSem);
    if(rc != CL_OK)
    {
        /*Delete existing one and try again*/
        ClOsalSemIdT semId=0;
        if(clOsalSemIdGet((ClUint8T*)gpClTipcNeighCompsSegment, &semId) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in creating sem.rc=0x%x\n",rc));
            goto out_unlink;
        }
        if(clOsalSemDelete(semId) !=CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in creating sem.rc=0x%x\n",rc));
            goto out_unlink;
        }
        rc = clOsalSemCreate((ClUint8T*)gpClTipcNeighCompsSegment, 1, &gClTipcNeighborSem);
    }

    return rc;


out_unlink:
    clOsalShmUnlink(gpClTipcNeighCompsSegment);

    close((ClInt32T)fd);

out:
    return rc;
}

ClRcT clTipcNeighCompsSegmentOpen(void)
{
    ClRcT rc = CL_OK;
    ClFdT fd;

    rc = clOsalShmOpen(gpClTipcNeighCompsSegment,O_RDWR,0666,&fd);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,("Error in shmopen.rc=0x%x\n",rc));
        goto out;
    }

    rc =clOsalMmap(0, CL_TIPC_NEIGH_COMPS_SEGMENT_SIZE, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0,(ClPtrT*)&gpClTipcNeighComps);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in mmap.rc=0x%x\n",rc));
        close((ClInt32T)fd);
        goto out;
    }

    gClTipcNeighCompsFd = fd;

    rc = clOsalSemIdGet((ClUint8T*)gpClTipcNeighCompsSegment, &gClTipcNeighborSem);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error getting semid.rc=0x%x\n",rc));
        close((ClInt32T)fd);
    }

out:
    return rc;
}

ClRcT clTipcNeighCompsInitialize(ClBoolT createFlag)
{
    ClRcT rc = CL_IOC_RC(CL_ERR_INITIALIZED);

    if(gClTipcNeighCompsInitialize)
        goto out;

    sprintf(gpClTipcNeighCompsSegment, "%s_%d",CL_TIPC_NEIGH_COMPS_SEGMENT_NAME, gIocLocalBladeAddress); 

    if(createFlag == CL_TRUE)
    {
        rc = clTipcNeighCompsSegmentCreate();
    }
    else
    {
        rc = clTipcNeighCompsSegmentOpen();
        if(rc == CL_OK)
        {
            gClTipcNeighCompsInitialize=1;
            goto out_master;
        }
    }

    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in segment initialize.rc=0x%x\n",rc));
        goto out;
    }

    CL_ASSERT(gpClTipcNeighComps != NULL);

    rc = clOsalMsync(gpClTipcNeighComps, CL_TIPC_NEIGH_COMPS_SEGMENT_SIZE, MS_ASYNC);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Msync error.rc=0x%x\n",rc));
        goto out;
    }

    gClTipcNeighCompsInitialize = 3;

out_master:
    clTipcMasterSegmentInitialize(gpClTipcNeighComps + CL_TIPC_NEIGHBORS_SEGMENT_SIZE, gClTipcNeighborSem);
out:
    return rc;
}


ClRcT clTipcNeighCompsFinalize(void)
{
    ClRcT rc = CL_IOC_RC(CL_ERR_NOT_INITIALIZED);

    if(!gClTipcNeighCompsInitialize)
    {
        goto out;
    }
    if(gpClTipcNeighComps != NULL)
    {
        clTipcMasterSegmentFinalize();
        rc = clOsalMsync(gpClTipcNeighComps, CL_TIPC_NEIGH_COMPS_SEGMENT_SIZE, MS_SYNC);
        CL_ASSERT(rc == CL_OK);

        rc = clOsalMunmap(gpClTipcNeighComps, CL_TIPC_NEIGH_COMPS_SEGMENT_SIZE);
        CL_ASSERT(rc == CL_OK);

        gpClTipcNeighComps = NULL;
        /*If created,delete*/
        if((gClTipcNeighCompsInitialize & 2))
        {
            clOsalShmUnlink(gpClTipcNeighCompsSegment);
            clOsalSemDelete(gClTipcNeighborSem);
        }
    }
    gClTipcNeighCompsInitialize = 0;
    
    rc = CL_OK;
out:
    return rc;
}



ClRcT clIocCompStatusGet(ClIocPhysicalAddressT compAddr, ClUint8T *pStatus)
{
    if(!gClTipcNeighCompsInitialize || !gpClTipcNeighComps)
        return CL_IOC_RC(CL_ERR_NOT_INITIALIZED);
    
    if(pStatus == NULL)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    if(compAddr.nodeAddress > CL_IOC_MAX_NODE_ADDRESS || compAddr.portId > CL_IOC_MAX_COMP_PORT) {
        *pStatus = CL_IOC_NODE_DOWN;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Invalid Address [node 0x%x : port 0x%x] passed.\n", compAddr.nodeAddress, compAddr.portId)); 
        return CL_IOC_RC(CL_ERR_NOT_EXIST);
    }

    clOsalSemLock(gClTipcNeighborSem);
    *pStatus = CL_IOC_NEIGH_COMPS_STATUS_GET(compAddr.nodeAddress, compAddr.portId);
    clOsalSemUnlock(gClTipcNeighborSem);

    return CL_OK;
}



ClRcT clIocCompStatusSet(ClIocPhysicalAddressT compAddr, ClUint32T status)
{
    if(!gpClTipcNeighComps)
    {
        return CL_IOC_RC(CL_ERR_NOT_INITIALIZED);
    }

    if(compAddr.nodeAddress > CL_IOC_MAX_NODE_ADDRESS ||  compAddr.portId > CL_IOC_MAX_COMP_PORT) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Invalid Address [node 0x%x : port 0x%x] passed.\n", compAddr.nodeAddress, compAddr.portId)); 
        return CL_IOC_RC(CL_ERR_NOT_EXIST);
    }

    clOsalSemLock(gClTipcNeighborSem);
    if(status == TIPC_PUBLISHED) {
        /*printf("Setting bit fiels for node %x and comp %x\n", compAddr.nodeAddress, compAddr.portId);*/
        CL_IOC_NEIGH_COMPS_STATUS_SET(compAddr.nodeAddress, compAddr.portId);
    } else if(status == TIPC_WITHDRAWN) {
        /*printf("Resetting bit fiels for node %x and comp %x\n", compAddr.nodeAddress, compAddr.portId); */
        CL_IOC_NEIGH_COMPS_STATUS_RESET(compAddr.nodeAddress, compAddr.portId);
    } 
    clOsalSemUnlock(gClTipcNeighborSem);

    return CL_OK;
}           


void clIocNodeCompsReset(ClIocNodeAddressT nodeAddr)
{
    if(!gpClTipcNeighComps)
    {
        return ;
    }

    if(nodeAddr > CL_IOC_MAX_NODE_ADDRESS)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Invalid node address [0x%x] passed.\n", nodeAddr)); 
        return;
    }

    clOsalSemLock(gClTipcNeighborSem);
    memset(gpClTipcNeighComps + (CL_TIPC_BYTES_FOR_COMPS_PER_NODE * nodeAddr), 0, CL_TIPC_BYTES_FOR_COMPS_PER_NODE);
    clOsalSemUnlock(gClTipcNeighborSem);
}


void clIocNodeCompsSet(ClIocNodeAddressT nodeAddr, ClUint8T *pBuff)
{
    if(!gpClTipcNeighComps)
    {
        return;
    }

    if(nodeAddr > CL_IOC_MAX_NODE_ADDRESS)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Invalid node address [0x%x] passed.\n", nodeAddr)); 
        return;
    }

    clOsalSemLock(gClTipcNeighborSem);
    memcpy(gpClTipcNeighComps + (CL_TIPC_BYTES_FOR_COMPS_PER_NODE * nodeAddr), pBuff, CL_TIPC_BYTES_FOR_COMPS_PER_NODE);
    clOsalSemUnlock(gClTipcNeighborSem);
}

                        
void clIocNodeCompsGet(ClIocNodeAddressT nodeAddr, ClUint8T *pBuff)
{
    if(!gpClTipcNeighComps)
    {
        return;
    }

    if(nodeAddr > CL_IOC_MAX_NODE_ADDRESS)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Invalid node address [0x%x] passed.\n", nodeAddr)); 
        return;
    }

    clOsalSemLock(gClTipcNeighborSem);
    memcpy(pBuff, gpClTipcNeighComps + (CL_TIPC_BYTES_FOR_COMPS_PER_NODE * nodeAddr), CL_TIPC_BYTES_FOR_COMPS_PER_NODE);
    clOsalSemUnlock(gClTipcNeighborSem);
}



ClRcT clIocRemoteNodeStatusGet(ClIocNodeAddressT nodeAddr, ClUint8T *pStatus)
{
    ClIocPhysicalAddressT compAddr = { nodeAddr, CL_IOC_TIPC_PORT};

    if(nodeAddr > CL_IOC_MAX_NODE_ADDRESS)
    {
        *pStatus = CL_IOC_NODE_DOWN;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Invalid node address [0x%x] passed.\n", nodeAddr)); 
        return CL_IOC_RC(CL_ERR_NOT_EXIST);
    }
    
    return clIocCompStatusGet(compAddr, pStatus);
}


ClRcT clTipcCheckAndGetPortId(ClIocPortT *portId)
{
    ClRcT rc = CL_OK;

    clOsalSemLock(gClTipcNeighborSem);
    if(*portId != 0)
    {
 
        if(CL_IOC_NEIGH_COMPS_STATUS_GET(gIocLocalBladeAddress, *portId) == CL_IOC_NODE_DOWN)
        {
            CL_IOC_NEIGH_COMPS_STATUS_SET(gIocLocalBladeAddress, *portId);
        } 
        else 
        {
            rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
        }
        goto out;
    }
    else
    {
        ClInt32T i;
        ClInt32T nextFreeBit;
        ClInt32T bitSpan = 0;
        ClUint8T *pMyComps;

        /* The bit-map for this node's components' status*/
        pMyComps = gpClTipcNeighComps + (gIocLocalBladeAddress * CL_TIPC_BYTES_FOR_COMPS_PER_NODE);

        /* The following number of bytes are already taken/reserved so skipping those bytes */
        i = CL_IOC_EPHEMERAL_PORTS_START;
        nextFreeBit = CL_TIPC_CTRL_SEGMENT(gpClTipcNeighComps)[0];

        if(nextFreeBit < i)
            nextFreeBit = i;

        bitSpan = CL_TIPC_BYTES_FOR_COMPS_PER_NODE << 3;
        clLogTrace("PORT", "GET", "Starting free port scan at [%#x], space [%#x]", nextFreeBit, bitSpan);
        for(i = nextFreeBit; i < bitSpan; ++i)
        {
            if(!(pMyComps[i>>3] & (1 << (i&7))))
            {
                *portId = i;
                nextFreeBit = i + 1;
                if(nextFreeBit >= bitSpan)
                    nextFreeBit = CL_IOC_EPHEMERAL_PORTS_START;
                CL_TIPC_CTRL_SEGMENT(gpClTipcNeighComps)[0] = nextFreeBit;
                CL_IOC_NEIGH_COMPS_STATUS_SET(gIocLocalBladeAddress, *portId);
                clLogTrace("PORT", "GET",
                           "Ephemeral Port [%#x] got assigned with next free port range starting at [%#x]",
                           i, nextFreeBit);
                goto out;
            }
        }

        if(nextFreeBit == CL_IOC_EPHEMERAL_PORTS_START)
        {
            rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
            goto out;
        }

        /*
         * Now scan the entire bit range as there could be holes in between.
         */
        for(i = CL_IOC_EPHEMERAL_PORTS_START; i < bitSpan; ++i)
        {
            if(!(pMyComps[i>>3] & (1 << (i&7))))
            {
                *portId = i;
                nextFreeBit = i + 1;
                if(nextFreeBit >= bitSpan)
                    nextFreeBit = CL_IOC_EPHEMERAL_PORTS_START;
                CL_TIPC_CTRL_SEGMENT(gpClTipcNeighComps)[0] = nextFreeBit;
                CL_IOC_NEIGH_COMPS_STATUS_SET(gIocLocalBladeAddress, *portId);
                clLogTrace("PORT", "GET", "Lower range ephemeral Port [%#x] got assigned with next free port range starting at [%#x]",
                           i, nextFreeBit);
                goto out;
            }
        }
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
    }

out:
    clOsalSemUnlock(gClTipcNeighborSem);
    return rc;
}


void clTipcPutPortId(ClIocPortT portId)
{
    clOsalSemLock(gClTipcNeighborSem);
    CL_IOC_NEIGH_COMPS_STATUS_RESET(gIocLocalBladeAddress, portId);
    clOsalSemUnlock(gClTipcNeighborSem);
}
