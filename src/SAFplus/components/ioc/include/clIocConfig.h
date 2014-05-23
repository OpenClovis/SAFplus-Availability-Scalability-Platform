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
 * ModuleName : ioc
 * File       : clIocConfig.h
 ******************************************************************************/
/*******************************************************************************
 * Description :
 *
 * Ioc Configuration parameters file. 
 *
 *****************************************************************************/


/**
 *  \file
 *  \brief Header file of Ioc Data Structures and APIs
 *  \ingroup ioc_apis
 */

/**
 ************************************
 *  \addtogroup ioc_apis
 *  \{
 */



#ifndef __CL_IOC_CONFIG_H__
#define __CL_IOC_CONFIG_H__

/**
 * This value represents the total number of ASP nodes present in a System.
 */
#define CL_IOC_MAX_NODES                 (1024)
/**
 * This is the lowest ASP node address, which can be assigned to any ASP node.
 */
#define CL_IOC_MIN_NODE_ADDRESS          (1)
/**
 * This is the highest ASP node address, which can be assigned to any ASP node.
 */
#define CL_IOC_MAX_NODE_ADDRESS          (CL_IOC_MAX_NODES-1)



/**
 * This represents the maximum number of communication ports, which can be opened on a ASP node.
 */
#define CL_IOC_MAX_COMPONENTS_PER_NODE   (1024) 
/**
 * This is the lowest communication port address that can be present on a ASP node.
 */
#define CL_IOC_MIN_COMP_PORT             (1)
/**
 * This is the highest communication port address that can be present on a ASP node.
 */
#define CL_IOC_MAX_COMP_PORT             (CL_IOC_MAX_COMPONENTS_PER_NODE - 1)


/**
 * This value represents the maximum number of ASP communication ports that
 * can be present on a ASP node. No user application should choose a
 * communication port, which falls into this group. It might get used be ASP
 * in later releases.
 */
#define CL_IOC_RESERVED_PORTS            (64)
/**
 * This value represents the maximum number of USER communication ports that
 * can be present on a ASP node. If the user wants to assign a well know
 * address to his application then the communication port number has to be
 * from this group.
 */
#define CL_IOC_USER_RESERVED_PORTS       (64)
/**
 * This valude represents the maximum number of EPHIMERAL communication ports
 * that can be present on a ASP node. These communication ports cannot be
 * directly used by user application. These are in ASP control and will be
 * assigned to an application, which requests of it.
 */
#define CL_IOC_EPHEMERAL_PORTS           (CL_IOC_MAX_COMPONENTS_PER_NODE - CL_IOC_RESERVED_PORTS - CL_IOC_USER_RESERVED_PORTS)


/**
 * ASP provides a feature of sending any sized data to another ASP object. This
 * feature internally does the fragmentation and reassembly of the data. So
 * this timeout mentions the reassembly timeout, within wich the IOC should
 * reassemble the packet and give to the application.
 */
#define CL_IOC_REASSEMBLY_TIMEOUT        (5000)  /* milliseconds */

#define CL_IOC_RELIABLE_SEND_TIMEOUT        (3000)  /* milliseconds */



#endif

/**
 *  \}
 */
