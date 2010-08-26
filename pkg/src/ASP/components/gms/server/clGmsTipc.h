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

#include <clTipcSetup.h>

/*
 * Value of tipc unicast socket type. i.e. value of 'sockaddr.addr.name.name.type'
 * This is used by token socket in openais.
 */
#define CL_OPENAIS_TIPC_UCAST_TYPE (CL_TIPC_BASE_TYPE +  \
                                   CL_IOC_GMS_UCAST_PORT)

/*
 * Value of 'sockaddr.addr.nameseq.type' for multicast socket.
 */
#define CL_OPENAIS_TIPC_MCAST_TYPE (CL_TIPC_BASE_TYPE +  \
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


