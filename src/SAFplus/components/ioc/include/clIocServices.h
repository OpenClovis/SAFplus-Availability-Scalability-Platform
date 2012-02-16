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
 * ModuleName  : ioc                                                           
 * File        : clIocServices.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * The reserved communication ports and other communication port related
 * information. 
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Reserved Communication Ports
 *  \ingroup ioc_apis
 */

/**
 ************************************
 *  \addtogroup ioc_apis
 *  \{
 */


#ifndef _CL_IOC_SERVICES_H_
# define _CL_IOC_SERVICES_H_

#include <clIocConfig.h>

# ifdef __cplusplus
extern "C"
{
# endif

/**
 * Start of valid communication port.
 */

/*
 * Reserved ports
 */

/**
 * Default communication port.
 */
# define CL_IOC_DEFAULT_COMMPORT                        0x0000


/**
 * Component Manager communication port.
 */
# define CL_IOC_CPM_PORT                                0x0001

/**
 * Event Manager communication port.
 */
# define CL_IOC_EVENT_PORT                              0x0002

/**
 * Trace Service communication port.
 */
# define CL_IOC_MSG_PORT                                0x0003

/**
 * Log Service communication port.
 */
# define CL_IOC_LOG_PORT                                0x0004

/**
 * Name Service communication port.
 */
# define CL_IOC_NAME_PORT                               0x0005

/**
 * Distributed shared memory service communication port.
 */
# define CL_IOC_DSHM_PORT                               0x0006

/**
 * Diagnostic Service communication port.
 */
# define CL_IOC_DIAG_PORT                               0x0007

/**
 * Fault service communication port.
 */
# define CL_IOC_FAULT_PORT                              0x0008

/**
 * Group Membersip Service communication port.
 */
# define CL_IOC_GMS_PORT                                0x0009

/**
 * Distributed lock service communication port.
 */
# define CL_IOC_DLOCK_PORT                              0x000a

/**
 * Transaction service communication port.
 */
# define CL_IOC_TXN_PORT                                0x000b

/**
 * Checkpointing service communication port.
 */
# define CL_IOC_CKPT_PORT                               0x000c

/**
 * COR communication port.
 */
# define CL_IOC_COR_PORT                                0x000d

/**
 * Chassis Manager communication port.
 */
# define CL_IOC_CM_PORT                                 0x000e

/**
 * HPI service communication port.
 */
# define CL_IOC_HPI_PORT                                0x000f

/**
 * SNMP communication port.
 */
# define CL_IOC_SNMP_PORT                               0x0010

/**
 * Alarm communication port.
 */
# define CL_IOC_ALARM_PORT                              0x0011

/**
 * Debug communication port.
 */
# define CL_IOC_DEBUG_PORT                              0x0012

/**
 * Upgrade Manager.
 */
# define CL_IOC_UM_PORT                                 0x0013

/**
 * Diagnostics Manager.
 */
# define CL_IOC_DM_PORT                                 0x0014

/**
 * Alarm application manager.
 */
# define CL_IOC_ALARM_APP_PORT                          0x0015


/**
 * Availability Management Service.
 */
# define CL_IOC_AMS_PORT                                0x0016

/**
 * Communication Module reserved port.
 */
# define CL_IOC_XPORT_PORT                              0x0017

/**
 * Communication Module reserved port.
 */
# define CL_IOC_GMS_UCAST_PORT                          0x0018

/**
 * Communication Module reserved port.
 */
# define CL_IOC_GMS_MCAST_PORT                          0x0019

/**
 * The ASP reserved ports end here.
 */
# define CL_IOC_ASP_PORTS_END                           (CL_IOC_RESERVED_PORTS - 1)

/**
 * The ASP applications should add their wellknow server ports here. The range
 * for this class of ports is CL_IOC_USER_APP_WELLKNOWN_PORT_START to
 * CL_IOC_USER_APP_WELLKNOWN_PORT_END, both inclusive.
 */
# define CL_IOC_USER_APP_WELLKNOWN_PORTS_START           (CL_IOC_RESERVED_PORTS + 0x00)
# define CL_IOC_USER_APP_WELLKNOWN_PORTS_END             (CL_IOC_RESERVED_PORTS + CL_IOC_USER_RESERVED_PORTS - 1)

/**
 * Client communication port range. Of these ports, IOC assigns the ports to
 * the application if it passes 0, in the communication port create API.
 */
# define CL_IOC_EPHEMERAL_PORTS_START                (CL_IOC_RESERVED_PORTS + CL_IOC_USER_RESERVED_PORTS + 0x00) 
# define CL_IOC_EPHEMERAL_PORTS_END                  (CL_IOC_MAX_COMP_PORT) 


# ifdef __cplusplus
}
# endif
#endif                          /* _CL_IOC_SERVICES_H_ */



/** \} */
