
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <netinet/in.h>

#include <clCommon.h>

#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clLogApi.hxx>
#include <clIocApi.h>
#include <clIocErrors.h>
#include <clIocUserApi.h>
#include <clIocNeighComps.h>
#include <clIocSetup.h>
#include <clIocMaster.h>
#include <clIocConfig.h>
#include <clIocIpi.h>
#include <clCommon.hxx>

#define CL_IOC_NEIGH_COMPS_SEGMENT_NAME   "/CL_IOC_NEIGHBOR_COMPONENTS"

#define CL_IOC_BYTES_FOR_COMPS_IN_SYSTEM  (CL_IOC_ALLOC_COMPS_IN_SYSTEM >> 3)
#define CL_IOC_NEIGHBORS_SEGMENT_SIZE     CL_IOC_ALIGN(CL_IOC_BYTES_FOR_COMPS_IN_SYSTEM, 8)

/*
 * Just store the next allocated bit.
 */
#define CL_IOC_CTRL_SEGMENT_SIZE CL_IOC_ALIGN(sizeof(ClUint32T), 8)
/* Includes both segments, neighbor-info and master-info. */
#define CL_IOC_MAIN_SEGMENT_SIZE (CL_IOC_NEIGHBORS_SEGMENT_SIZE + CL_IOC_MASTER_SEGMENT_SIZE)

#define CL_IOC_NODE_REP_PID_SEGMENT_SIZE    CL_IOC_ALIGN(sizeof(ClUint32T), 4)
#define CL_IOC_NODE_REP_PORT_SEGMENT_SIZE   CL_IOC_NODE_REP_PID_SEGMENT_SIZE
#define CL_IOC_NODE_REP_SEGMENT_SIZE   (CL_IOC_NODE_REP_PID_SEGMENT_SIZE + CL_IOC_NODE_REP_PORT_SEGMENT_SIZE)
#define CL_IOC_NEIGH_COMPS_SEGMENT_SIZE   (CL_IOC_MAIN_SEGMENT_SIZE + CL_IOC_CTRL_SEGMENT_SIZE + CL_IOC_NODE_REP_SEGMENT_SIZE )

#define CL_IOC_CTRL_SEGMENT(seg) ( (ClUint32T*)((ClUint8T*)(seg) + CL_IOC_MAIN_SEGMENT_SIZE) )
#define CL_IOC_NODE_REP_PID_SEGMENT(seg) ( (ClUint32T*)((ClUint8T*)(seg) + CL_IOC_MAIN_SEGMENT_SIZE + CL_IOC_CTRL_SEGMENT_SIZE))
#define CL_IOC_NODE_REP_PORT_SEGMENT(seg) ((ClUint32T*)((ClUint8T*)(seg) +CL_IOC_MAIN_SEGMENT_SIZE + CL_IOC_CTRL_SEGMENT_SIZE+ CL_IOC_NODE_REP_PID_SEGMENT_SIZE))

ClUint8T *gpClIocNeighComps;
static ClCharT gpClIocNeighCompsSegment[CL_IOC_SEGMENT_NAME_LENGTH + 1];
static ClFdT gClIocNeighCompsFd;
static ClUint32T gClIocNeighCompsInitialize;
static ClOsalSemIdT gClIocNeighborSem;
extern ClBoolT  gIsNodeRepresentative;

#define IOC_LOG_AREA_SEGMENT	"SEG"
#define IOC_LOG_AREA_STATUS	"STATUS"
#define IOC_LOG_AREA_COMP	"COMP"
#define IOC_LOG_CTX_CREATE	"CRE"
#define	IOC_LOG_CTX_OPEN	"OPE"
#define IOC_LOG_CTX_GET		"GET"
#define IOC_LOG_CTX_SET		"SET"
#define IOC_LOG_CTX_RESET	"RESET"

static ClRcT clIocElectNodeReprenstative()
{
    ClRcT rc = CL_OK;
    ClUint32T nodeRep_pid = 0;
    ClUint32T pid = getpid();

    clOsalSemLock(gClIocNeighborSem);

    nodeRep_pid = CL_IOC_NODE_REP_PID_SEGMENT(gpClIocNeighComps)[0];

    /* Algorithm for dynamic determination of NodeReprentative Process
       Read the NodeReprentative PID value from  the neighbor components shared memory segment and
       check whether the Node Reprentative Process exists or not. If the Node Representative process does not exist,
       writes its own PID into this field, delays for a second and then reads the value.
       If the value is its own PID, the process is the node representative.
       If the value has changed, restart the algorithm.
     */
    if(!SAFplus::clIsProcessAlive(nodeRep_pid))
    {
        while(1)
        {
            CL_IOC_NODE_REP_PID_SEGMENT(gpClIocNeighComps)[0] = pid;
            sleep(1);
            nodeRep_pid = CL_IOC_NODE_REP_PID_SEGMENT(gpClIocNeighComps)[0];
            if(nodeRep_pid == pid)
            {
                gIsNodeRepresentative = CL_TRUE;
                break;
            }
        }
    }
    else
    {
        logInfo(IOC_LOG_AREA_SEGMENT, CL_LOG_CONTEXT_UNSPECIFIED, " NodeRep Process with pid [%u] is alive",nodeRep_pid);
    }
    clOsalSemUnlock(gClIocNeighborSem);
    return rc;
}

ClRcT clIocNeighCompsSegmentCreate(void)
{
    ClRcT rc = CL_OK;
    ClFdT fd;

    rc = clOsalShmOpen(gpClIocNeighCompsSegment,O_RDWR|O_CREAT|O_EXCL,0666,&fd);
    if(rc != CL_OK)
    {
        /* Don't Log error in case of Already existed Shared Memory Segments */  
        if(CL_GET_ERROR_CODE(rc) != CL_ERR_ALREADY_EXIST) 
        {
            logError(IOC_LOG_AREA_SEGMENT,IOC_LOG_CTX_CREATE,"Error in osal shm open.rc=0x%x\n",rc);
        }
        return rc;
    }

    logTrace(IOC_LOG_AREA_SEGMENT,IOC_LOG_CTX_CREATE, "CL_IOC_NEIGH_COMPS_SEGMENT_SIZE = %lu", CL_IOC_NEIGH_COMPS_SEGMENT_SIZE);

    rc = clOsalFtruncate(fd, CL_IOC_NEIGH_COMPS_SEGMENT_SIZE);
    if(rc != CL_OK)
    {
        logError(IOC_LOG_AREA_SEGMENT,IOC_LOG_CTX_CREATE,"Error in ftruncate.rc=0x%x\n",rc);
        goto out_unlink;
    }
    
    rc = clOsalMmap(0, CL_IOC_NEIGH_COMPS_SEGMENT_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0, (ClPtrT *)&gpClIocNeighComps);
    if(rc != CL_OK)
    {
        logError(IOC_LOG_AREA_SEGMENT,IOC_LOG_CTX_CREATE,"Error in osal mmap.rc=0x%x\n",rc);
        goto out_unlink;
    }
    
    gClIocNeighCompsFd = fd;

    rc = clOsalSemCreate((ClUint8T*)gpClIocNeighCompsSegment, 1, &gClIocNeighborSem);
    if(rc != CL_OK)
    {
        /*Delete existing one and try again*/
        ClOsalSemIdT semId=0;
        if(clOsalSemIdGet((ClUint8T*)gpClIocNeighCompsSegment, &semId) != CL_OK)
        {
            logError(IOC_LOG_AREA_SEGMENT,IOC_LOG_CTX_CREATE,"Error in creating sem.rc=0x%x\n",rc);
            goto out_unlink;
        }
        if(clOsalSemDelete(semId) !=CL_OK)
        {
            logError(IOC_LOG_AREA_SEGMENT,IOC_LOG_CTX_CREATE,"Error in creating sem.rc=0x%x\n",rc);
            goto out_unlink;
        }
        rc = clOsalSemCreate((ClUint8T*)gpClIocNeighCompsSegment, 1, &gClIocNeighborSem);
    }

    return rc;


out_unlink:
    clOsalShmUnlink(gpClIocNeighCompsSegment);

    close((ClInt32T)fd);

out:
    return rc;
}

ClRcT clIocNeighCompsSegmentOpen(void)
{
    ClRcT rc = CL_OK;
    ClFdT fd;

    rc = clOsalShmOpen(gpClIocNeighCompsSegment,O_RDWR,0666,&fd);
    if(rc != CL_OK)
    {
        logWarning(IOC_LOG_AREA_SEGMENT,IOC_LOG_CTX_OPEN,"Error in shmopen.rc=0x%x\n",rc);
        goto out;
    }

    rc =clOsalMmap(0, CL_IOC_NEIGH_COMPS_SEGMENT_SIZE, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0,(ClPtrT*)&gpClIocNeighComps);
    if(rc != CL_OK)
    {
        logError(IOC_LOG_AREA_SEGMENT,IOC_LOG_CTX_OPEN,"Error in mmap.rc=0x%x\n",rc);
        close((ClInt32T)fd);
        goto out;
    }

    gClIocNeighCompsFd = fd;

    rc = clOsalSemIdGet((ClUint8T*)gpClIocNeighCompsSegment, &gClIocNeighborSem);
    if(rc != CL_OK)
    {
        logError(IOC_LOG_AREA_SEGMENT,IOC_LOG_CTX_OPEN,"Error getting semid.rc=0x%x\n",rc);
        close((ClInt32T)fd);
    }

out:
    return rc;
}

ClRcT clIocNeighCompsInitialize()
{
    ClRcT rc = CL_IOC_RC(CL_ERR_INITIALIZED);

    if(gClIocNeighCompsInitialize)
        return rc;

    sprintf(gpClIocNeighCompsSegment, "%s_%d",CL_IOC_NEIGH_COMPS_SEGMENT_NAME, gIocLocalBladeAddress); 

    rc = clIocNeighCompsSegmentCreate();
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST)
    {
        rc = clIocNeighCompsSegmentOpen();
    }
    if(rc != CL_OK)
    {
        logError(IOC_LOG_AREA_SEGMENT, CL_LOG_CONTEXT_UNSPECIFIED,"Error in segment initialize.rc=0x%x\n",rc);
        return rc;
    }
    if(rc == CL_OK) 
    {
        gClIocNeighCompsInitialize=1;
        CL_ASSERT(gpClIocNeighComps != NULL);
        clIocElectNodeReprenstative();
        logDebug(IOC_LOG_AREA_SEGMENT, CL_LOG_CONTEXT_UNSPECIFIED, "gIsNodeRepresentative value is [%s]",gIsNodeRepresentative?"TRUE":"FALSE");
        if(!gIsNodeRepresentative)
        {
            clIocMasterSegmentInitialize(gpClIocNeighComps + CL_IOC_NEIGHBORS_SEGMENT_SIZE, gClIocNeighborSem);
            return rc;
        }
    }

    rc = clOsalMsync(gpClIocNeighComps, CL_IOC_NEIGH_COMPS_SEGMENT_SIZE, MS_ASYNC);
    if(rc != CL_OK)
    {
        logError(IOC_LOG_AREA_SEGMENT,CL_LOG_CONTEXT_UNSPECIFIED,"Msync error.rc=0x%x\n",rc);
        CL_ASSERT(0);
    }
    gClIocNeighCompsInitialize = 3;
    return rc;
}


ClRcT clIocNeighCompsFinalize(void)
{
    ClRcT rc = CL_IOC_RC(CL_ERR_NOT_INITIALIZED);

    if(!gClIocNeighCompsInitialize)
    {
        goto out;
    }
    if(gpClIocNeighComps != NULL)
    {
        clIocMasterSegmentFinalize();
        rc = clOsalMsync(gpClIocNeighComps, CL_IOC_NEIGH_COMPS_SEGMENT_SIZE, MS_SYNC);
        CL_ASSERT(rc == CL_OK);

        rc = clOsalMunmap(gpClIocNeighComps, CL_IOC_NEIGH_COMPS_SEGMENT_SIZE);
        CL_ASSERT(rc == CL_OK);

        gpClIocNeighComps = NULL;
        /*If created,delete*/
        if((gClIocNeighCompsInitialize & 2))
        {
            clOsalShmUnlink(gpClIocNeighCompsSegment);
            clOsalSemDelete(gClIocNeighborSem);
        }
    }
    gClIocNeighCompsInitialize = 0;
    
    rc = CL_OK;
out:
    return rc;
}



ClRcT clIocCompStatusGet(ClIocPhysicalAddressT compAddr, ClUint8T *pStatus)
{
    if(!gClIocNeighCompsInitialize || !gpClIocNeighComps)
        return CL_IOC_RC(CL_ERR_NOT_INITIALIZED);
    
    if(pStatus == NULL)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    if(compAddr.nodeAddress > CL_IOC_MAX_NODE_ADDRESS || compAddr.portId > CL_IOC_MAX_COMP_PORT) {
        *pStatus = CL_IOC_NODE_DOWN;
        logError(IOC_LOG_AREA_STATUS,IOC_LOG_CTX_GET,"Error : Invalid Address [node 0x%x : port 0x%x] passed.\n", compAddr.nodeAddress, compAddr.portId); 
        return CL_IOC_RC(CL_ERR_NOT_EXIST);
    }

    clOsalSemLock(gClIocNeighborSem);
    *pStatus = CL_IOC_NEIGH_COMPS_STATUS_GET(compAddr.nodeAddress, compAddr.portId);
    clOsalSemUnlock(gClIocNeighborSem);

    return CL_OK;
}



ClRcT clIocCompStatusSet(ClIocPhysicalAddressT compAddr, ClUint32T status)
{
    if(!gpClIocNeighComps)
    {
        return CL_IOC_RC(CL_ERR_NOT_INITIALIZED);
    }

    if(compAddr.nodeAddress > CL_IOC_MAX_NODE_ADDRESS ||  compAddr.portId > CL_IOC_MAX_COMP_PORT) {
        logError(IOC_LOG_AREA_STATUS,IOC_LOG_CTX_SET,"Error : Invalid Address [node 0x%x : port 0x%x] passed.\n", compAddr.nodeAddress, compAddr.portId); 
        return CL_IOC_RC(CL_ERR_NOT_EXIST);
    }

    clOsalSemLock(gClIocNeighborSem);
    if(status == CL_IOC_NODE_UP) 
    {
        /*printf("Setting bit fiels for node %x and comp %x\n", compAddr.nodeAddress, compAddr.portId);*/
        CL_IOC_NEIGH_COMPS_STATUS_SET(compAddr.nodeAddress, compAddr.portId);
    } 
    else if(status == CL_IOC_NODE_DOWN)
    {
        /*printf("Resetting bit fiels for node %x and comp %x\n", compAddr.nodeAddress, compAddr.portId); */
        CL_IOC_NEIGH_COMPS_STATUS_RESET(compAddr.nodeAddress, compAddr.portId);
    } 
    clOsalSemUnlock(gClIocNeighborSem);

    return CL_OK;
}           


void clIocNodeCompsReset(ClIocNodeAddressT nodeAddr)
{
    if(!gpClIocNeighComps)
    {
        return ;
    }

    if(nodeAddr > CL_IOC_MAX_NODE_ADDRESS)
    {
        logError(IOC_LOG_AREA_STATUS,IOC_LOG_CTX_RESET,"Error : Invalid node address [0x%x] passed.\n", nodeAddr); 
        return;
    }

    clOsalSemLock(gClIocNeighborSem);
    memset(gpClIocNeighComps + (CL_IOC_BYTES_FOR_COMPS_PER_NODE * nodeAddr), 0, CL_IOC_BYTES_FOR_COMPS_PER_NODE);
    clOsalSemUnlock(gClIocNeighborSem);
}


void clIocNodeCompsSet(ClIocNodeAddressT nodeAddr, ClUint8T *pBuff)
{
    if(!gpClIocNeighComps)
    {
        return;
    }

    if(nodeAddr > CL_IOC_MAX_NODE_ADDRESS)
    {
        logError(IOC_LOG_AREA_COMP,IOC_LOG_CTX_SET,"Error : Invalid node address [0x%x] passed.\n", nodeAddr); 
        return;
    }

    clOsalSemLock(gClIocNeighborSem);
    memcpy(gpClIocNeighComps + (CL_IOC_BYTES_FOR_COMPS_PER_NODE * nodeAddr), pBuff, CL_IOC_BYTES_FOR_COMPS_PER_NODE);
    clOsalSemUnlock(gClIocNeighborSem);
}

                        
void clIocNodeCompsGet(ClIocNodeAddressT nodeAddr, ClUint8T *pBuff)
{
    if(!gpClIocNeighComps)
    {
        return;
    }

    if(nodeAddr > CL_IOC_MAX_NODE_ADDRESS)
    {
        logError(IOC_LOG_AREA_COMP,IOC_LOG_CTX_GET,"Error : Invalid node address [0x%x] passed.\n", nodeAddr); 
        return;
    }

    clOsalSemLock(gClIocNeighborSem);
    memcpy(pBuff, gpClIocNeighComps + (CL_IOC_BYTES_FOR_COMPS_PER_NODE * nodeAddr), CL_IOC_BYTES_FOR_COMPS_PER_NODE);
    clOsalSemUnlock(gClIocNeighborSem);
}


ClRcT clIocRemoteNodeStatusGet(ClIocNodeAddressT nodeAddr, ClUint8T *pStatus)
{
    ClIocPhysicalAddressT compAddr = { nodeAddr, CL_IOC_XPORT_PORT};
    if(nodeAddr > CL_IOC_MAX_NODE_ADDRESS)
    {
        *pStatus = CL_IOC_NODE_DOWN;
        logError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Error : Invalid node address [0x%x] passed.\n", nodeAddr); 
        return CL_IOC_RC(CL_ERR_NOT_EXIST);
    }
    
    return clIocCompStatusGet(compAddr, pStatus);
}


ClRcT clIocCheckAndGetPortId(ClIocPortT *portId)
{
    ClRcT rc = CL_OK;

    clOsalSemLock(gClIocNeighborSem);
    if(*portId != 0)
    {
 
        if(CL_IOC_NEIGH_COMPS_STATUS_GET(gIocLocalBladeAddress, *portId) == CL_IOC_NODE_DOWN)
        {
            CL_IOC_NEIGH_COMPS_STATUS_SET(gIocLocalBladeAddress, *portId);
        } 
        else 
        {
            /* Node Representative Process does not know which ports are up and running.
             * If the process is node representative process, it can create the communication port
               with desired port number.
             */
            if(!gIsNodeRepresentative)
            {
                rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
            }
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
        pMyComps = gpClIocNeighComps + (gIocLocalBladeAddress * CL_IOC_BYTES_FOR_COMPS_PER_NODE);

        /* The following number of bytes are already taken/reserved so skipping those bytes */
        i = CL_IOC_EPHEMERAL_PORTS_START;
        nextFreeBit = CL_IOC_CTRL_SEGMENT(gpClIocNeighComps)[0];

        if(nextFreeBit < i)
            nextFreeBit = i;

        bitSpan = CL_IOC_BYTES_FOR_COMPS_PER_NODE << 3;
        logTrace("PORT", "GET", "Starting free port scan at [%#x], space [%#x]", nextFreeBit, bitSpan);
        for(i = nextFreeBit; i < bitSpan; ++i)
        {
            if(!(pMyComps[i>>3] & (1 << (i&7))))
            {
                *portId = i;
                nextFreeBit = i + 1;
                if(nextFreeBit >= bitSpan)
                    nextFreeBit = CL_IOC_EPHEMERAL_PORTS_START;
                CL_IOC_CTRL_SEGMENT(gpClIocNeighComps)[0] = nextFreeBit;
                CL_IOC_NEIGH_COMPS_STATUS_SET(gIocLocalBladeAddress, *portId);
                logTrace("PORT", "GET",
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
                CL_IOC_CTRL_SEGMENT(gpClIocNeighComps)[0] = nextFreeBit;
                CL_IOC_NEIGH_COMPS_STATUS_SET(gIocLocalBladeAddress, *portId);
                logTrace("PORT", "GET", "Lower range ephemeral Port [%#x] got assigned with next free port range starting at [%#x]",
                           i, nextFreeBit);
                goto out;
            }
        }
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
    }

out:
    clOsalSemUnlock(gClIocNeighborSem);
    return rc;
}


void clIocPutPortId(ClIocPortT portId)
{
    clOsalSemLock(gClIocNeighborSem);
    CL_IOC_NEIGH_COMPS_STATUS_RESET(gIocLocalBladeAddress, portId);
    clOsalSemUnlock(gClIocNeighborSem);
}
