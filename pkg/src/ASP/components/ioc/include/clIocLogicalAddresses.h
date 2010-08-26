/*
 * Copyright (C) 2002-2008 by OpenClovis Inc. All  Rights Reserved.
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
 * Build: 3.1.0
 */
/*******************************************************************************
 * ModuleName  : ioc                                                           
 * File        : clIocLogicalAddresses.h
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
 *  \brief Header file for defining the logical addresses
 *  \ingroup ioc_apis
 */

/**
 ************************************
 *  \addtogroup ioc_apis
 *  \{
 */


#ifndef _CL_IOC_LOGICAL_ADDRESSES_H
# define _CL_IOC_LOGICAL_ADDRESSES_H

# ifdef __cplusplus
extern "C"
{
# endif


/**
 * Total number of Logical addresses in an ASP System.
 */
#define CL_IOC_TOTAL_LOGICAL_ADDRESSES                 (~0U)




/**
 * Total number of Logcaical Addresses reserved by ASP components.
 */
#define CL_IOC_RESERVED_LOGICAL_ADDRESSES              CL_IOC_RESERVED_PORTS

/**
 * Start of valid Logical Addresses.
 */
#define CL_IOC_LOGICAL_ADDRESS_START                   CL_IOC_LOGICAL_ADDRESS_FORM(0)

/**
 * Logical Address used by AMF
 */
# define CL_IOC_AMF_LOGICAL_ADDRESS                    (CL_IOC_LOGICAL_ADDRESS_START+CL_IOC_CPM_PORT)

/**
 * Logical Address used by Log Service
 */
# define CL_IOC_LOG_LOGICAL_ADDRESS                    (CL_IOC_LOGICAL_ADDRESS_START+CL_IOC_LOG_PORT)

/**
 * Logical Address used by Checkpoint service
 */
#define CL_IOC_CKPT_LOGICAL_ADDRESS                    (CL_IOC_LOGICAL_ADDRESS_START+CL_IOC_CKPT_PORT)

/**
 * End of Reserved Logical addresses
 */
#define  CL_IOC_RESERVED_LOGICAL_ADDRESS_END           (CL_IOC_LOGICAL_ADDRESS_START+CL_IOC_RESERVED_LOGICAL_ADDRESSES-1)





/**
 * Total number of Static Logical Addresses reserved for ASP Applications.
 */
#define CL_IOC_STATIC_LOGICAL_ADDRESSES                (1024)

/**
 * Start of Static Logical Addressess for ASP Applications.
 */
#define CL_IOC_STATIC_LOGICAL_ADDRESS_START            (CL_IOC_RESERVED_LOGICAL_ADDRESS_END+1)

/**
 * End of Static Logical Addressess for ASP Applications.
 */
#define CL_IOC_STATIC_LOGICAL_ADDRESS_END              (CL_IOC_RESERVED_LOGICAL_ADDRESS_END+CL_IOC_STATIC_LOGICAL_ADDRESSES)





/**
 * Start of Dynamic Logical addresses. This range of addresses is used only by Name Service
 * to allocate Logical Addresses to applications dynamically.
 */
#define CL_IOC_DYNAMIC_LOGICAL_ADDRESS_START           (CL_IOC_STATIC_LOGICAL_ADDRESS_END+1)

/**
 * End of Dynamic logical addresses.
 */
#define CL_IOC_DYNAMIC_LOGICAL_ADDRESS_END             (CL_IOC_LOGICAL_ADDRESS_START + CL_IOC_TOTAL_LOGICAL_ADDRESSES)




# ifdef __cplusplus
}
# endif

#endif                          /* _CL_IOC_LOGICAL_ADDRESSES_H */



/** \} */
