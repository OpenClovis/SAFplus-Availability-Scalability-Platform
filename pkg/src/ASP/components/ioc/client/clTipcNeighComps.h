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
#ifndef __CL_TIPC_NEIGH_COMPS_H__ 
#define __CL_TIPC_NEIGH_COMPS_H__

#include <clCommon.h>
#include <clIocApi.h>

extern ClUint8T *gpClTipcNeighComps;

ClRcT clTipcNeighCompsInitialize(ClBoolT createFlag);
ClRcT clTipcNeighCompsFinalize(void);
/*ClRcT clIocCompStatusGet(ClIocPhysicalAddressT iocNodeAddr, ClUint8T *pStatus);*/
ClRcT clIocCompStatusSet(ClIocPhysicalAddressT iocNode, ClUint32T status);
void clIocNodeCompsGet(ClIocNodeAddressT node, ClUint8T *pBuff);
void clIocNodeCompsSet(ClIocNodeAddressT node, ClUint8T *pBuff);
void clIocNodeCompsReset(ClIocNodeAddressT node);

ClRcT clTipcCheckAndGetPortId(ClIocPortT *portId);
void clTipcPutPortId(ClIocPortT portId);

#define CL_IOC_NEIGH_COMPS_STATUS_SET(node, comp)   CL_IOC_BIT_SET(gpClTipcNeighComps, ((node) * (CL_IOC_ALLOC_COMPS_PER_NODE) + (comp)))
#define CL_IOC_NEIGH_COMPS_STATUS_RESET(node, comp) CL_IOC_BIT_RESET(gpClTipcNeighComps, ((node) * (CL_IOC_ALLOC_COMPS_PER_NODE) + (comp)))
#define CL_IOC_NEIGH_COMPS_STATUS_GET(node, comp)   CL_IOC_BIT_GET(gpClTipcNeighComps, ((node) * (CL_IOC_ALLOC_COMPS_PER_NODE) + (comp)))

#define CL_IOC_NEIGH_NODE_STATUS_GET(node) \
        (CL_IOC_NEIGH_COMPS_STATUS_GET(node, CL_IOC_CPM_PORT) && \
        CL_IOC_NEIGH_COMPS_STATUS_GET(node, CL_IOC_CKPT_PORT) && \
        CL_IOC_NEIGH_COMPS_STATUS_GET(node, CL_IOC_COR_PORT) && \
        CL_IOC_NEIGH_COMPS_STATUS_GET(node, CL_IOC_GMS_PORT) && \
        CL_IOC_NEIGH_COMPS_STATUS_GET(node, CL_IOC_LOG_PORT))

#endif
