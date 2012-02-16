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
#ifndef __CL_IOC_SETUP_H__
#define __CL_IOC_SETUP_H__

#include <clIocConfig.h>

#define CL_IOC_ALIGN(value, alignto) (((value) + (alignto)-1) & ~((alignto)-1))
#define CL_IOC_ALLOC_COMPS_PER_NODE         CL_IOC_ALIGN(CL_IOC_MAX_COMPONENTS_PER_NODE, 8)
#define CL_IOC_BYTES_FOR_COMPS_PER_NODE    (CL_IOC_ALLOC_COMPS_PER_NODE >> 3)
#define CL_IOC_ALLOC_COMPS_IN_SYSTEM       (CL_IOC_MAX_NODES * CL_IOC_ALLOC_COMPS_PER_NODE)

/* The main thread waittime is in seconds */
#define CL_IOC_MAIN_THREAD_WAIT_TIME  (1000)
#define CL_IOC_SEGMENT_NAME_LENGTH  (50)

#define CL_IOC_BIT_RESET(addr, temp) ((addr)[(temp) >> 3] = (addr)[(temp) >> 3] & ~(1 << ((temp) & 7)))
#define CL_IOC_BIT_SET(addr, temp)   ((addr)[(temp) >> 3] = (addr)[(temp) >> 3] | (1 << ((temp) & 7)))
#define CL_IOC_BIT_GET(addr, temp)   (((addr)[(temp) >> 3] & (1 << ((temp) & 7)))? 1 : 0)

#define CL_IOC_COMM_PORT_MASK        (~0U >> CL_IOC_ADDRESS_TYPE_BITS)

#define CL_IOC_BASE_TYPE         (0x100) /* Type codes lower than this are off use for ASP */

#endif
