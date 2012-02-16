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
#ifndef __CL_IOC_MASTER_H__
#define __CL_IOC_MASTER_H__

#include <clIocConfig.h>

#define CL_IOC_MASTER_SEGMENT_SIZE       CL_IOC_ALIGN((CL_IOC_MAX_COMPONENTS_PER_NODE * sizeof(ClIocNodeAddressT)), 8)

#define CL_IOC_MASTER_TYPE(port)   ((CL_IOC_MASTER_ADDRESS_TYPE << CL_IOC_ADDRESS_TYPE_SHIFT_WORD) | ((port) & CL_IOC_COMM_PORT_MASK))


void clIocMasterSegmentInitialize(void *pMasterSegment, ClOsalSemIdT masterSem);
void clIocMasterSegmentFinalize(void);
void clIocMasterSegmentUpdate(ClIocPhysicalAddressT compAddr);
void clIocMasterSegmentSet(ClIocPhysicalAddressT compAddr, ClIocNodeAddressT master);

#endif
