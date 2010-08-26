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

#ifndef __CL_TIPC_SETUP_H__
#define __CL_TIPC_SETUP_H__

#include <clIocConfig.h>


#define CL_TIPC_ALIGN(value, alignto) (((value) + (alignto)-1) & ~((alignto)-1))


#define CL_IOC_ALLOC_COMPS_PER_NODE         CL_TIPC_ALIGN(CL_IOC_MAX_COMPONENTS_PER_NODE, 8)
#define CL_TIPC_BYTES_FOR_COMPS_PER_NODE    (CL_IOC_ALLOC_COMPS_PER_NODE >> 3)
#define CL_TIPC_ALLOC_COMPS_IN_SYSTEM       (CL_IOC_MAX_NODES * CL_IOC_ALLOC_COMPS_PER_NODE)


#define CL_TIPC_BASE_TYPE         (0x100) /* Type codes lower than this are off use for ASP */

/* The main thread waittime is in seconds */
#define CL_TIPC_MAIN_THREAD_WAIT_TIME  (1000)
#define CL_TIPC_TOP_SRV_TIMEOUT   (1000)
#define CL_TIPC_TOP_SRV_READY_TIMEOUT (200)
#define CL_TIPC_SEGMENT_NAME_LENGTH  (50)


#define CL_IOC_BIT_RESET(addr, temp) ((addr)[(temp) >> 3] = (addr)[(temp) >> 3] & ~(1 << ((temp) & 7)))
#define CL_IOC_BIT_SET(addr, temp)   ((addr)[(temp) >> 3] = (addr)[(temp) >> 3] | (1 << ((temp) & 7)))
#define CL_IOC_BIT_GET(addr, temp)   (((addr)[(temp) >> 3] & (1 << ((temp) & 7)))? 1 : 0)


#define CL_IOC_COMM_PORT_MASK        (~0U >> CL_IOC_ADDRESS_TYPE_BITS)


#define CL_IOC_TIPC_HIGH_PRIORITY    TIPC_CRITICAL_IMPORTANCE
#define CL_IOC_TIPC_DEFAULT_PRIORITY TIPC_LOW_IMPORTANCE

#endif
