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
 * ModuleName  : ioc                                                           
 * File        : clIocProtocols.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * Well known protocols are published here.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Ioc Protocols
 *  \ingroup ioc_apis
 */

/**
 ************************************
 *  \addtogroup ioc_apis
 *  \{
 */

#ifndef _CL_IOC_PROTOCOLS_H_
# define _CL_IOC_PROTOCOLS_H_


# ifdef __cplusplus
extern "C"
{
# endif


  /**
     * Reserved Protocol Type
     */
typedef enum
  {
/**
 * ARP protocol.
 */
    CL_IOC_PROTO_ARP   =                0x1,

/**
 * Flow control Protocol.
 */
    CL_IOC_PROTO_FLOWCONTROL   =        0x2,

/**
 * IOC Heartbeat protocol.
 */
    CL_IOC_PROTO_HB     =               0x3,

/**
 * IOC Control Protocol (for discovery and capability negotiation).
 */
    CL_IOC_PROTO_CTL      =             0x4,

/**
 * Transparency Layer protocol.
 */
    CL_IOC_PROTO_TL      =              0x5,

/**
 *  Messaging service protocol.
 */
    CL_IOC_PROTO_MSG    =               0x6,

/**
 * Group communication related message.
 */
    CL_IOC_PROTO_ICMP       =           0x7,

/**
 * IOC internal reserved protocols end.
 */
    CL_IOC_INTERNAL_PROTO_END  =        0xf,

    /*
     * various protocols supported by EO. 
     */

/**
 * RMD synchronous request.
 */
    CL_IOC_RMD_SYNC_REQUEST_PROTO   =   0x10,

/**
 * RMD synchronous reply.
 */
    CL_IOC_RMD_SYNC_REPLY_PROTO    =    0x11,

/**
 * RMD asynchronous request.
 */
    CL_IOC_RMD_ASYNC_REQUEST_PROTO  =   0x12,

/**
 * RMD asynchronous reply.
 */
    CL_IOC_RMD_ASYNC_REPLY_PROTO    =   0x13,

/**
 * Port close notification for CPM.
 */
    CL_IOC_PORT_NOTIFICATION_PROTO   =  0x14,

/**
 * Sys log protocol.
 */
    CL_IOC_SYSLOG_PROTO     =           0x15,

    CL_IOC_RMD_ACK_PROTO       =        0x16,

    CL_IOC_RMD_ORDERED_PROTO   =        0x17,

/**
 * Here the reserved protocols for ASP end.
 */
    CL_IOC_ASP_RESERVERD_PROTO_END =   0x7f,

    /*
     * User should use protocols ID from here onwards 
     */

/**
 * If the application wants to specify its own protocols it can start from here, For example \c CL_IOC_USER_PROTO_START+1.
 */
    CL_IOC_USER_PROTO_START         =   0x80,

/**
 * The application should specify its protocol number less than this value, if they plan to use some.
 */



    CL_IOC_PROTO_END            =       0xfe,

/**
 * Last protocol Id for EO.
 */

    CL_IOC_INVALID_PROTO         =      0xff,

    CL_IOC_NUM_PROTOS            =      0x100
  } ClIocProtocols;

# ifdef __cplusplus
}
# endif
#endif                          /* _CL_IOC_PROTOCOLS_H_ */

/** \} */
