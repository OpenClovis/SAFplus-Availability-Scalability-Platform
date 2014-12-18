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
#ifndef __CL_IOC_NEIGH_COMPS_H__ 
#define __CL_IOC_NEIGH_COMPS_H__

#include <clCommon.h>
#include <clIocApi.h>

extern ClUint8T *gpClIocNeighComps;

ClRcT clIocNeighCompsInitialize(void);
ClRcT clIocNeighCompsFinalize(void);

/*ClRcT clIocCompStatusGet(ClIocPhysicalAddressT iocNodeAddr, ClUint8T *pStatus);*/
ClRcT clIocCompStatusSet(ClIocPhysicalAddressT iocNode, ClUint32T status);
void clIocNodeCompsGet(ClIocNodeAddressT node, ClUint8T *pBuff);
void clIocNodeCompsSet(ClIocNodeAddressT node, ClUint8T *pBuff);
void clIocNodeCompsReset(ClIocNodeAddressT node);

ClRcT clIocCheckAndGetPortId(ClIocPortT *portId);
void clIocPutPortId(ClIocPortT portId);

#define CL_IOC_NEIGH_COMPS_STATUS_SET(node, comp)   CL_IOC_BIT_SET(gpClIocNeighComps, ((node) * (CL_IOC_ALLOC_COMPS_PER_NODE) + (comp)))
#define CL_IOC_NEIGH_COMPS_STATUS_RESET(node, comp) CL_IOC_BIT_RESET(gpClIocNeighComps, ((node) * (CL_IOC_ALLOC_COMPS_PER_NODE) + (comp)))
#define CL_IOC_NEIGH_COMPS_STATUS_GET(node, comp)   CL_IOC_BIT_GET(gpClIocNeighComps, ((node) * (CL_IOC_ALLOC_COMPS_PER_NODE) + (comp)))

#define CL_IOC_NEIGH_NODE_STATUS_GET(node) \
        (CL_IOC_NEIGH_COMPS_STATUS_GET(node, CL_IOC_CPM_PORT) && \
        CL_IOC_NEIGH_COMPS_STATUS_GET(node, CL_IOC_CKPT_PORT) && \
        CL_IOC_NEIGH_COMPS_STATUS_GET(node, CL_IOC_GMS_PORT) && \
        CL_IOC_NEIGH_COMPS_STATUS_GET(node, CL_IOC_LOG_PORT))

#endif
