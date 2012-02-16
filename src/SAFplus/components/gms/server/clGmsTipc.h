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
/*******************************************************************************
 * ModuleName  : gms                                                           
 * File        : clGms.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * Contains internal global data structures and function prototypes for the 
 * GMS component.
 *  
 *
 *****************************************************************************/

#ifndef _CL_GMS_TIPC_H_
#define _CL_GMS_TIPC_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clIocSetup.h>

/*
 * Value of tipc unicast socket type. i.e. value of 'sockaddr.addr.name.name.type'
 * This is used by token socket in openais.
 */
#define CL_OPENAIS_TIPC_UCAST_TYPE (CL_IOC_BASE_TYPE +  \
                                   CL_IOC_GMS_UCAST_PORT)

/*
 * Value of 'sockaddr.addr.nameseq.type' for multicast socket.
 */
#define CL_OPENAIS_TIPC_MCAST_TYPE (CL_IOC_BASE_TYPE +  \
                                    CL_IOC_GMS_MCAST_PORT)

/* 
 * Value of mcast_port in openais.conf. This is used as upper & lower value
 * while binding to multicast socket. Later, the same value is used to 
 * send multicast messages
 */
#define CL_OPENAIS_TIPC_MCAST_RANGE 1

#ifdef  __cplusplus
}
#endif

#endif  /* _CL_GMS_TIPC_H_ */


