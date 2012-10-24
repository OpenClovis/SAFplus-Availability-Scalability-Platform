/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

#ifndef __CL_TIPC_MASTER_H__
#define __CL_TIPC_MASTER_H__

#include <clIocConfig.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_TIPC_MASTER_SEGMENT_SIZE       CL_TIPC_ALIGN((CL_IOC_MAX_COMPONENTS_PER_NODE * sizeof(ClIocNodeAddressT)), 8)

#define CL_IOC_TIPC_MASTER_TYPE(port)   ((CL_IOC_MASTER_ADDRESS_TYPE << CL_IOC_ADDRESS_TYPE_SHIFT_WORD) | ((port) & CL_IOC_COMM_PORT_MASK))


void clTipcMasterSegmentInitialize(void *pMasgerSegment, ClOsalSemIdT masterSem);
void clTipcMasterSegmentFinalize(void);
void clTipcMasterSegmentUpdate(ClIocPhysicalAddressT compAddr);
void clTipcMasterSegmentSet(ClIocPhysicalAddressT compAddr, ClIocNodeAddressT master);

#ifdef __cplusplus
}
#endif

#endif
